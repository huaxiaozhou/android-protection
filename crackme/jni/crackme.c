#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/types.h>
#include <elf.h>
#include <sys/mman.h>
#include<pthread.h>
#include <sys/stat.h>
#define LOG_TAG "System.out.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

jvalue JNU_CallMethodByName(JNIEnv *env, jboolean *hasException, jobject obj,
		const char *name, const char *descriptor, ...) {
	va_list args;
	jclass clazz;
	jmethodID mid;
	jvalue result;
	if ((*env)->EnsureLocalCapacity(env, 2) == JNI_OK) {
		clazz = (*env)->GetObjectClass(env, obj);
		mid = (*env)->GetMethodID(env, clazz, name, descriptor);
		if (mid) {
			const char *p = descriptor;
			/* skip over argument types to find out the
			  return type */
			while (*p != ')')
				p++;
			/* skip ')' */
			p++;
			va_start(args, descriptor);
			switch (*p) {
			case 'V':
				(*env)->CallVoidMethodV(env, obj, mid, args);
				break;
			case '[':
			case 'L':
				result.l = (*env)->CallObjectMethodV(env, obj, mid, args);
				break;
			case 'Z':
				result.z = (*env)->CallBooleanMethodV(env, obj, mid, args);
				break;
			case 'B':
				result.b = (*env)->CallByteMethodV(env, obj, mid, args);
				break;
			case 'C':
				result.c = (*env)->CallCharMethodV(env, obj, mid, args);
				break;
			case 'S':
				result.s = (*env)->CallShortMethodV(env, obj, mid, args);
				break;
			case 'I':
				result.i = (*env)->CallIntMethodV(env, obj, mid, args);
				break;
			case 'J':
				result.j = (*env)->CallLongMethodV(env, obj, mid, args);
				break;
			case 'F':
				result.f = (*env)->CallFloatMethodV(env, obj, mid, args);
				break;
			case 'D':
				result.d = (*env)->CallDoubleMethodV(env, obj, mid, args);
				break;
			default:
				(*env)->FatalError(env, "illegaldescriptor");
			}
			va_end(args);
		}
		(*env)->DeleteLocalRef(env, clazz);
	}
	if (hasException) {
		*hasException = (*env)->ExceptionCheck(env);
	}
	return result;
}

jstring getString(JNIEnv* env,  jobject thiz){
	// 获得 Context 类
		jboolean hasException;
		int legitimate = 0;
		//获取包名
		jstring jstr_packageName = (jstring) JNU_CallMethodByName(env,
				&hasException, thiz, "getPackageName", "()Ljava/lang/String;").l;

		if ((*env)->ExceptionCheck(env) || jstr_packageName == NULL) {
	//		LOGI("can't get jstr of getPackageName");
	//		return -1;
			return (*env)->NewStringUTF(env,"");
		}
		//获取包名的字符串
		const char* loc_str_app_packageName = (*env)->GetStringUTFChars(env,
				jstr_packageName, NULL);
		if (loc_str_app_packageName == NULL) {
	//		LOGI("can't get packagename from jstring");
	//		return -2;
			return (*env)->NewStringUTF(env,"");
		}
		//当前应用包名与合法包名对比
		if (strcmp(loc_str_app_packageName, "com.example.nocrack") != 0) {
			LOGI("this app is illegal");
	//		return -3;

			return (*env)->NewStringUTF(env,"this app is illegal");
		}
		else{
			LOGI("running successfully");
			return (*env)->NewStringUTF(env,"running successfully");
		}

		//释放loc_str_app_packageName
		(*env)->ReleaseStringUTFChars(env, jstr_packageName,
				loc_str_app_packageName);

	return (*env)->NewStringUTF(env, "Native method return!");
};


JNIEXPORT jstring JNICALL
Java_com_example_crackme_MainActivity_getString( JNIEnv* env,
                                                  jobject thiz )
{
    return getString(env, thiz);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	LOGE("JNI_OnLoad start");

	return JNI_VERSION_1_4;
}
