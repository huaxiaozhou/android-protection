#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>

#define LOG_TAG "DEBUG"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = -1;

EGLBoolean new_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    LOGD("New eglSwapBuffers\n");
    if (old_eglSwapBuffers == -1)
        LOGD("error\n");
    return old_eglSwapBuffers(dpy, surface);
}

void* get_module_base(pid_t pid, const char* module_name)
{
    FILE *fp;
    long addr = 0;
    char *pch;
    char filename[32];
    char line[1024];

    if (pid < 0) {
        /* self process */
        snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
    } else {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                pch = strtok( line, "-" );
                addr = strtoul( pch, NULL, 16 );

                if (addr == 0x8000)
                    addr = 0;

                break;
            }
        }

        fclose(fp) ;
    }

    return (void *)addr;
}

#define LIBSF_PATH  "/system/lib/libsurfaceflinger.so"
int hook_eglSwapBuffers()
{
    old_eglSwapBuffers = eglSwapBuffers;
    LOGD("Orig eglSwapBuffers = %p\n", old_eglSwapBuffers);
    void * base_addr = get_module_base(getpid(), LIBSF_PATH);
    LOGD("libsurfaceflinger.so address = %p\n", base_addr);

    int fd;
    fd = open(LIBSF_PATH, O_RDONLY);
    if (-1 == fd) {
        LOGD("error\n");
        return -1;
    }

    Elf32_Ehdr ehdr;
    read(fd, &ehdr, sizeof(Elf32_Ehdr));

    unsigned long shdr_addr = ehdr.e_shoff;
    int shnum = ehdr.e_shnum;
    int shent_size = ehdr.e_shentsize;
    unsigned long stridx = ehdr.e_shstrndx;

    Elf32_Shdr shdr;
    lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
    read(fd, &shdr, shent_size);

    char * string_table = (char *)malloc(shdr.sh_size);
    lseek(fd, shdr.sh_offset, SEEK_SET);
    read(fd, string_table, shdr.sh_size);
    lseek(fd, shdr_addr, SEEK_SET);

    int i;
    uint32_t out_addr = 0;
    uint32_t out_size = 0;
    uint32_t got_item = 0;
    int32_t got_found = 0;

    for (i = 0; i < shnum; i++) {
        read(fd, &shdr, shent_size);
        if (shdr.sh_type == SHT_PROGBITS) {
            int name_idx = shdr.sh_name;
            if (strcmp(&(string_table[name_idx]), ".got.plt") == 0
                    || strcmp(&(string_table[name_idx]), ".got") == 0) {
                out_addr = base_addr + shdr.sh_addr;
                out_size = shdr.sh_size;
                LOGD("out_addr = %lx, out_size = %lx\n", out_addr, out_size);

                for (i = 0; i < out_size; i += 4) {
                    got_item = *(uint32_t *)(out_addr + i);
                    if (got_item  == old_eglSwapBuffers) {
                        LOGD("Found eglSwapBuffers in got\n");
                        got_found = 1;

                        uint32_t page_size = getpagesize();
                        uint32_t entry_page_start = (out_addr + i) & (~(page_size - 1));
                        mprotect((uint32_t *)entry_page_start, page_size, PROT_READ | PROT_WRITE);
                        *(uint32_t *)(out_addr + i) = new_eglSwapBuffers;

                        break;
                    } else if (got_item == new_eglSwapBuffers) {
                        LOGD("Already hooked\n");
                        break;
                    }
                }
                if (got_found)
                    break;
            }
        }
    }

    free(string_table);
    close(fd);
}

int hook_entry(char * a){
    LOGD("Hook success\n");
    LOGD("Start hooking\n");
    hook_eglSwapBuffers();
    return 0;
}
