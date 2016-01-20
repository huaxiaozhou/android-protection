#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/ptrace.h>
#include <asm/user.h>
#include <asm/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>


#define ENABLE_DEBUG        1

#define PTRACE_PEEKTEXT     1
#define PTRACE_POKETEXT     4
#define PTRACE_ATTACH       16
#define PTRACE_CONT         7
#define PTRACE_DETACH       17
#define PTRACE_SYSCALL      24
#define CPSR_T_MASK         (1u << 5)

#define  MAX_PATH           0x100

//本地ShellCode的指令或者数据的内存地址到远程目标进程的内存地址的重定位映射
#define REMOTE_ADDR(addr, local_base, remote_base) ((uint32_t)(addr) + (uint32_t)(remote_base) - (uint32_t)(local_base))

//系统调用函数mmap所在的模块
const char *libc_path = "/system/lib/libc.so";

//系统调用函数dlopn、dlsym、dlclose所在的模块
const char *linker_path = "/system/bin/linker";

//显示调试的信息
#if ENABLE_DEBUG
    #define DEBUG_PRINT(format,args...) \
        LOGD(format, ##args)
#else
    #define DEBUG_PRINT(format,args...)
#endif


//#########################################################################################################

//查找要注入的目标进程的PID
//process_name为要查找的进程名字
int find_pid_of(const char *process_name)
{
    int id;
    DIR* dir;
    FILE *fp;

    //保存进程的PID
    pid_t pid = -1;

    //保存进程的名称
    char filename[32];

    //保存运行进程的命令行
    char cmdline[256];

    struct dirent * entry;

    //进程的名字不能为NULL
    if (process_name == NULL)
        return -1;

    //打开文件目录"/proc"
    dir = opendir("/proc");

    //文件目录"/proc"的句柄不能为NULL
    if (dir == NULL)
        return -1;

    /*
     * 函数struct dirent* readdir(DIR* dir_handle);  //读取目录（循环遍历）
     * struct dirent
     * {
     *  long d_ino;                          //inode number 索引节点号
     *  off_t d_off;                         //offset to this dirent 在目录文件中的偏移
     *  unsigned short d_reclen;             //length of this d_name 文件名长
     *  unsigned char d_type;                //the type of d_name 文件类型
     *  char d_name [NAME_MAX+1];            //file name (null-terminated) 文件名，最长255字符
     * }
     */

    //循环读取文件目录"/proc"里的文件
    while((entry = readdir(dir)) != NULL)
    {
        //将文件名字符串转整型得到进程的PID
        id = atoi(entry->d_name);
        if (id != 0)
        {
            //格式化字符串得到"/proc/pid/cmdline"
            sprintf(filename, "/proc/%d/cmdline", id);

            //打开文件"/proc/pid/cmdline"
            fp = fopen(filename, "r");
            if (fp)
            {
                //读取运行进程的命令行中的arg[0]即进程的名称
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);

                //判断获取到进程的名字是否与要查找的目标进程名字process_name相等
                if (strcmp(process_name, cmdline) == 0)
                {
                    //保存目标进程的PID
                    pid = id;

                    break;
                }
            }
        }
    }

    closedir(dir);

    //返回查找到目标进程的PID
    return pid;
}


//#########################################################################################################

/*
 * 对远程目标进程进行LibInject和函数的Hook
 * library_path------------------自定义的Hook函数所在的模块（libHook.so库）的路径
 * function_name-----------------Hook函数在libHook.so库中名称Hook_Api
 * param-------------------------Hook函数调用所需要的参数
 * param_size--------------------Hook函数调用所需要的参数的大小
 */
int inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, void *param, size_t param_size)
{
    int ret = -1;
    void *mmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr;
    void *local_handle, *remote_handle, *dlhandle;
    uint8_t *map_base;
    uint8_t *dlopen_param1_ptr, *dlsym_param2_ptr, *saved_r0_pc_ptr, *inject_param_ptr, *remote_code_ptr, *local_code_ptr;

    struct pt_regs regs, original_regs;

    //导出全局变量
    extern uint32_t _dlopen_addr_s, _dlopen_param1_s, _dlopen_param2_s, _dlsym_addr_s, \
            _dlsym_param2_s, _dlclose_addr_s, _inject_start_s, _inject_end_s, _inject_function_param_s, \
            _saved_cpsr_s, _saved_r0_pc_s;

    uint32_t code_length;
    long parameters[10];

    DEBUG_PRINT("[+] Injecting process: %d\n", target_pid);

    //附加远程目标进程
    if (ptrace_attach(target_pid) == -1)
        return EXIT_SUCCESS;

    //获取附加远程目标进程此时寄存器的状态值
    if (ptrace_getregs(target_pid, ®s) == -1)
        goto exit;

    //保存获取到的附加远程目标进程的寄存器的状态值
    memcpy(&original_regs, ®s, sizeof(regs));

    //获取附加远程目标进程"/system/lib/libc.so"模块中函数mmap的调用地址
    mmap_addr = get_remote_addr(target_pid, libc_path, (void *)mmap);    //"/system/lib/libc.so"

    DEBUG_PRINT("[+] Remote mmap address: %x\n", mmap_addr);

    //格式化函数mmap的调用参数
    parameters[0] = 0;                                      // addr
    parameters[1] = 0x4000;                                 // size 申请内存空间的大小
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;     // prot 可读可写可执行
    parameters[3] =  MAP_ANONYMOUS | MAP_PRIVATE;           // flags
    parameters[4] = 0;                                      // fd
    parameters[5] = 0;                                      // offset

    DEBUG_PRINT("[+] Calling mmap in target process.\n");

    //在附加远程目标进程中调用函数mmmap申请内存空间
    if (ptrace_call(target_pid, (uint32_t)mmap_addr, parameters, 6, ®s) == -1)
        goto exit;

    //读取附加远程目标进程中此时寄存器的状态值，获取函数mmap调用返回的申请内存空间的地址
    if (ptrace_getregs(target_pid, ®s ) == -1)
        goto exit;

    DEBUG_PRINT("[+] Target process returned from mmap, return value=%x, pc=%x \n", regs.ARM_r0, regs.ARM_pc );

    //保存在附加远程目标进程中申请到内存空间的地址map_base = r0
    map_base = (uint8_t *)regs.ARM_r0;

    //获取附加远程目标进程中函数dlopen的调用地址
    dlopen_addr = get_remote_addr(target_pid, linker_path, (void *)dlopen);   //"/system/bin/linker"

    //获取附加远程目标进程中函数dlsym的调用地址
    dlsym_addr = get_remote_addr(target_pid, linker_path, (void *)dlsym);     //"/system/bin/linker"

    //获取附加远程目标进程中函数dlclose的调用地址
    dlclose_addr = get_remote_addr(target_pid, linker_path, (void *)dlclose); //"/system/bin/linker"

    DEBUG_PRINT("[+] Get imports: dlopen: %x, dlsym: %x, dlclose: %x\n", dlopen_addr, dlsym_addr, dlclose_addr );

    //附加远程目标进程注入代码ShellCode的起始地址，并预留0x3C00的内存空间空间
    remote_code_ptr = map_base + 0x3C00;

    //注入ShellCode的本地起始地址
    local_code_ptr = (uint8_t *)&_inject_start_s;

    //保存函数dlopen的调用地址到全局变量_dlopen_addr_s中
    _dlopen_addr_s = (uint32_t)dlopen_addr;

    //保存函数dlsym的调用地址到全局变量_dlsym_addr_s中
    _dlsym_addr_s = (uint32_t)dlsym_addr;

    //保存函数dlclose的调用地址到全局变量_dlclose_addr_s中
    _dlclose_addr_s = (uint32_t)dlclose_addr;

    DEBUG_PRINT("[+] Inject code start: %x, end: %x\n", local_code_ptr, &_inject_end_s);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //获取注入ShellCode代码指令的长度
    code_length = (uint32_t)&_inject_end_s - (uint32_t)&_inject_start_s;

    //本地为函数dlopen的第1个参数pathname变量申请内存空间
    //void * dlopen(const char* pathname, int mode);
    dlopen_param1_ptr = local_code_ptr + code_length + 0x20;

    //本地为函数dlsym的第2个参数symbol变量申请内存空间
    //void*dlsym(void* handle, constchar* symbol);
    dlsym_param2_ptr = dlopen_param1_ptr + MAX_PATH;

    //本地为附加远程目标进程的寄存器状态值r0-r15(pc)的保存申请内存空间
    saved_r0_pc_ptr = dlsym_param2_ptr + MAX_PATH;

    //本地为附加远程目标进程的Hook函数的参数inject_param_ptr申请内存空间
    inject_param_ptr = saved_r0_pc_ptr + MAX_PATH;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //拷贝函数dlopen的第1个参数到本地内存空间dlopen_param1_ptr中
    //函数dlopen的第1个参数也就是附加远程目标中要调用的Hook函数所在的模块
    strcpy(dlopen_param1_ptr, library_path);

    //获取函数dlopen的第1个参数从本地内存地址到附加远程目标进程内存映射的重定位地址
    _dlopen_param1_s = REMOTE_ADDR(dlopen_param1_ptr, local_code_ptr, remote_code_ptr);
    DEBUG_PRINT("[+] _dlopen_param1_s: %x\n", _dlopen_param1_s);

    //拷贝函数dlsym的第2个参数到本地内存空间dlsym_param2_ptr中
    //函数dlsym的第2个参数也就是附加远程目标中要调用的Hook函数的名称
    strcpy(dlsym_param2_ptr, function_name);

    //获取函数dlsym的第2个参数从本地内存地址到附加远程目标进程内存映射的重定位地址
    _dlsym_param2_s = REMOTE_ADDR(dlsym_param2_ptr, local_code_ptr, remote_code_ptr);
    DEBUG_PRINT("[+] _dlsym_param2_s: %x\n", _dlsym_param2_s);

    //保存附加远程目标进程的cpsr寄存器的值（cpsr寄存器在ARM的模式切换的时候会使用）
    _saved_cpsr_s = original_regs.ARM_cpsr;

    //保存附加远程目标进程的寄存器r0-r15(pc)的状态值
    memcpy(saved_r0_pc_ptr, &(original_regs.ARM_r0), 16*4); // r0 ~ r15

    //获取附加远程目标进程的寄存器r0-r15(pc)的状态值从本地内存保存地址到附加远程目标进程内存映射的重定位地址
    _saved_r0_pc_s = REMOTE_ADDR( saved_r0_pc_ptr, local_code_ptr, remote_code_ptr);
    DEBUG_PRINT("[+] _saved_r0_pc_s: %x\n", _saved_r0_pc_s);

    //拷贝附加远程目标进程的Hook函数的参数到本地内存空间inject_param_ptr中
    memcpy(inject_param_ptr, param, param_size);

    //获取附加远程目标进程的Hook函数的参数从本地内存地址到附加远程目标进程内存映射的重定位地址
    _inject_function_param_s = REMOTE_ADDR(inject_param_ptr, local_code_ptr, remote_code_ptr);
    DEBUG_PRINT("[+] _inject_function_param_s: %x\n", _inject_function_param_s);

    //显示附加远程目标进程的ShellCode注入的内存地址
    DEBUG_PRINT("[+] Remote shellcode address: %x\n", remote_code_ptr);

    //向附加远程目标进程的内存空间中写入0x400大小的本地ShellCode指令代码
    ptrace_writedata(target_pid, remote_code_ptr, local_code_ptr, 0x400);

    //拷贝附加远程目标进程被附加时寄存器的状态值到临时变量regs中
    memcpy(®s, &original_regs, sizeof(regs));

    //修改附加远程目标进程的sp寄存器的值为ShellCode的注入地址
    regs.ARM_sp = (long)remote_code_ptr;

    //修改附加远程目标进程的pc寄存器的值为ShellCode的注入地址
    regs.ARM_pc = (long)remote_code_ptr;

    //设置附加远程目标进程的寄存器的状态值即让附加远程目标进程执行注入的ShellCode指令代码
    ptrace_setregs(target_pid, ®s);

    //结束目标进程的附加
    ptrace_detach(target_pid);

    //进程注入成功
    ret = 0;

exit:
    return ret;
}


//读取被附加调试目标进程内存中的数据
//读取的数据保存在buf缓冲区中
int ptrace_readdata(pid_t pid,  uint8_t *src, uint8_t *buf, size_t size)
{
    uint32_t i, j, remain;
    uint8_t *laddr;

    //联合体
    union u{
        long val;
        char chars[sizeof(long)];
    } d;

    //4字节的整数倍
    j = size / 4;

    //剩余的字节数
    remain = size % 4;

    //src为要读取数据的目标进程的内存地址
    //buf保存读取到目标进程中的数据
    laddr = buf;

    //在目标进程中读取4字节的整数倍的数据
    for (i = 0; i < j; i++)
    {
         //在目标进程中读取数据
         d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);

         //拷贝读取的数据到临时缓冲区中
         memcpy(laddr, d.chars, 4);

         src += 4;
         laddr += 4;
    }

    //在目标进程中读取剩余的数据
    if (remain > 0)
    {
        //在目标进程中读取数据
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);

        //拷贝读取的数据到临时缓冲区中
        memcpy(laddr, d.chars, remain);
    }

    return 0;

}


//向附加调试的目标进程内存中写入数据
int ptrace_writedata(pid_t pid, uint8_t *dest, uint8_t *data, size_t size)
{
    uint32_t i, j, remain;
    uint8_t *laddr;

    //联合体
    union u {
        long val;
        char chars[sizeof(long)];
    } d;

    //4字节整数倍
    j = size / 4;

    //剩余的字节数
    remain = size % 4;

    //data中存放的是要写入目标进程的数据
    laddr = data;

    //向目标进程中写入4字节的整数倍的数据
    for (i = 0; i < j; i ++)
    {
        memcpy(d.chars, laddr, 4);

        //向目标中写入1个字的数据
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);

        dest  += 4;
        laddr += 4;
    }

    //向目标进程中写入剩余的数据
    if (remain > 0)
    {
        //d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);   //原来的代码中有，感觉是多余的

        for ( i = 0; i < remain; i++)
        {
            d.chars[i] = *laddr++;
        }

        //向目标进程中写入剩余的数据
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);

    }

    return 0;
}


//向附加调试的目标进程内存中写入字符串数据
int ptrace_writestring(pid_t pid, uint8_t *dest, char *str)
{
    //调用函数向附加目标进程内存中写入数据
    return ptrace_writedata(pid, dest, str, strlen(str)+1);
}


/*
 * 在其他进程（远程目标进程）中调用系统函数mmap申请内存空间
 * void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
 * params是已经格式化的mmap函数的参数，num_params是mmap函数的参数的个数
 * regs是远程目标进程的寄存器的数据，addr为远程目标进程中函数mmap的调用地址
 */
int ptrace_call(pid_t pid, uint32_t addr, long *params, uint32_t num_params, struct pt_regs* regs)
{
    uint32_t i;

    /*
    struct user_regs_struct
    {
      long int ebx;
      long int ecx;
      long int edx;
      long int esi;
      long int edi;
      long int ebp;
      long int eax;
      long int xds;
      long int xes;
      long int xfs;
      long int xgs;
      long int orig_eax;
      long int eip;
      long int xcs;
      long int eflags;
      long int esp;
      long int xss;
    };
    */

    //ARM中函数mmap的前4个参数通过r0-r3来传入
    for (i = 0; i < num_params && i < 4; i ++)
    {
        regs->uregs[i] = params[i];
    }

    //ARM中函数mmap的剩余2个参数通过栈来传入
    if (i < num_params)
    {
        //在目标进程的ARM栈中为剩余的2个参数申请内存空间
        regs->ARM_sp -= (num_params - i)*sizeof(long);

        //向目标进程的ARM栈中写入剩余的2个参数的数据
        ptrace_writedata(pid, (void *)regs->ARM_sp, (uint8_t *)¶ms[i], (num_params - i)*sizeof(long));
    }

    //设置远程目标进程的的PC寄存器的值（修改目标进程的执行）
    regs->ARM_pc = addr;  //addr为远程目标进程中函数mmap的调用地址

    //根据远程目标进程的运行模式，设置目标进程的CPSR寄存器的值
    if (regs->ARM_pc & 1)
    {
        //thumb模式
        regs->ARM_pc &= (~1u);
        regs->ARM_cpsr |= CPSR_T_MASK;
    }
    else
    {
        //arm模式
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    }

    //设置远程目标进程的LR寄存器的值为0，触发地址0异常回到当前进程中
    regs->ARM_lr = 0;

    //设置远程目标进程各寄存器的值然后在远程目标进程中调用mmap函数申请内存空间
    if (ptrace_setregs(pid, reg) == -1
        || ptrace_continue(pid) == -1)
    {
        return -1;
    }

    //等待在远程目标进程中申请内存空间操作的完成
    //申请到的内存空间的地址保存在返回值寄存器r0中
    waitpid(pid, NULL, WUNTRACED);

    return 0;
}


//获取被附加调试进程的寄存器的值
int ptrace_getregs( pid_t pid, struct pt_regs* regs )
{
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0)
    {
        perror( "ptrace_getregs: Can not get register values");

        return -1;
    }

    return 0;
}


//设置被附加调试进程的寄存器的值
int ptrace_setregs(pid_t pid, struct pt_regs* regs)
{
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0)
    {
        perror("ptrace_setregs: Can not set register values");

        return -1;
    }

    return 0;
}


//附加的目标进程继续执行
int ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0)
    {
        perror("ptrace_cont");

        return -1;
    }

    return 0;
}


//附加目标进程
int ptrace_attach(pid_t pid)
{
    //附加目标进程
    if (ptrace(PTRACE_ATTACH, pid, NULL, 0 ) < 0)
    {
        perror("ptrace_attach");

        return -1;
    }

    //等待目标进程附加完成
    waitpid(pid, NULL, WUNTRACED);

    //DEBUG_PRINT("attached\n");

    //目标进程继续执行，让目标进程在下次进/出系统调用时被调试
    if (ptrace(PTRACE_SYSCALL, pid, NULL, 0) < 0)
    {
        perror("ptrace_syscall");

        return -1;
    }

    //等待目标进程的此设置的完成
    waitpid(pid, NULL, WUNTRACED);

    return 0;
}


//结束目标进程的附加
int ptrace_detach(pid_t pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0)
        {
            perror("ptrace_detach");

            return -1;
        }

        return 0;
}


//获取进程加载模块的基址
void* get_module_base(pid_t pid, const char* module_name)
{
    FILE *fp;
    long addr = 0;
    char *pch;

    //保存模块的名称
    char filename[32];

    //保存读取的信息
    char line[1024];

    if (pid < 0)
    {
        //获取当前进程的模块的基址
        snprintf(filename, sizeof(filename), "/proc/self/maps", pid);
    }
    else
    {
        //获取其他进程的模块的基址
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    //打开"/proc/pid/maps"文件
    fp = fopen(filename, "r");

    if (fp != NULL)
    {
        //循环读取"/proc/pid/maps"文件的信息，每次一行
        while (fgets(line, sizeof(line), fp))
        {
            //判断读取的信息line中是否包含要查找的模块名称
            if (strstr(line, module_name))
            {
                //以"-"为标记拆分字符串
                pch = strtok(line, "-");

                //字符串转无符号长整型的模块基址
                addr = strtoul(pch, NULL, 16 );

                //排除特殊情况
                if (addr == 0x8000)
                    addr = 0;

                break;
            }
        }

            fclose( fp );
    }

    //返回获取到的模块的基址
    return (void *)addr;
}

//获取其他进程的某加载模块中某系统函数的调用地址
/*
 * Once we know the base address of a given library both in our process and in the target process,
 * what we can do to resolve the remote function address is:
 *      REMOTE_ADDRESS = LOCAL_ADDRESS + (REMOTE_BASE - LOCAL_BASE)
 *
 */
void* get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr)
{
    void* local_handle, *remote_handle;

    //获取某系统模块在当前进程中的加载基址
    local_handle = get_module_base(-1, module_name);

    //获取其他进程（目标进程）中某系统模块的加载基址
    remote_handle = get_module_base(target_pid, module_name);

    DEBUG_PRINT("[+] get_remote_addr: local[%x], remote[%x]\n", local_handle, remote_handle);

    //REMOTE_ADDRESS = LOCAL_ADDRESS + (REMOTE_BASE - LOCAL_BASE)
    //获取其他进程（目标进程）某系统模块中某系统函数的调用地址并返回
    return (void *)((uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle);
}



//******************************************************************************************************

//main函数
int main(int argc, char** argv)
{

    //要注入的进程的PID
    pid_t target_pid;

    //查找要注入的目标进程"/system/bin/servicemanager"的PID
    target_pid = find_pid_of("/system/bin/surfaceflinger");

    //对目标进程servicemanager进行LibInject和函数的Hook
    //"/data/local/tmp/libhookdll.so"为要注入到目标进程中的so库
    //"hook_entry"为注入要调用的so库中的函数
    inject_remote_process(target_pid, "/data/local/tmp/libhookdll.so", "hook_entry", "I'm parameter!", strlen("I'm parameter!"));

}
