#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string.h>
#include "load.h"
#include <android/log.h>
#include <malloc.h>
#define LOG_TAG "System.out.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)




/**
 * 工具方法
 * 返回值 char* 这个代表char数组的首地址
 *  Jstring2CStr 把java中的jstring的类型转化成一个c语言中的char 字符串
 */
char* Jstring2CStr(JNIEnv* env, jstring jstr) {
	char* rtn = NULL;
	jclass clsstring = (*env)->FindClass(env, "java/lang/String");
	jstring strencode = (*env)->NewStringUTF(env, "utf-8");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes",
			"(Ljava/lang/String;)[B");
	jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid,
			strencode);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0) {
		rtn = (char*) malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);
	return rtn;
}

jobject rr(JNIEnv* env){
	char* rrr = "GiEhjghmZIO7RTWyycQ9PQ==";

	jstring psd = (*env)->NewStringUTF(env, rrr);
	jclass stringClass = (*env)->FindClass(env, "java/lang/String");
	jmethodID getBytes_method = (*env)->GetMethodID(env, stringClass, "getBytes", "()[B");
	jbyteArray bytes = (jbyteArray)(*env)->CallObjectMethod(env, psd, getBytes_method);
	jclass base64Class = (*env)->FindClass(env, "android/util/Base64");
	jmethodID decode_method = (*env)->GetStaticMethodID(env, base64Class, "decode", "([BI)[B");
	jfieldID fid_no_padding = (*env)->GetStaticFieldID(env, base64Class, "NO_PADDING", "I");
	int i_no_padding = (*env)->GetStaticIntField(env, base64Class, fid_no_padding);
	jbyteArray resultArray = (jbyteArray)(*env)->CallStaticObjectMethod(env, base64Class,decode_method, bytes, i_no_padding);

	jclass secretKeySpecClass = (*env)->FindClass(env, "javax/crypto/spec/SecretKeySpec");
	jmethodID initSecretKeySpecMethod =(*env)->GetMethodID(env, secretKeySpecClass, "<init>","([BLjava/lang/String;)V");
	jstring str_aes = (*env)->NewStringUTF(env, "AES");
	jobject key =(*env)->NewObject(env, secretKeySpecClass,initSecretKeySpecMethod, resultArray, str_aes);
	jclass keyClass = (*env)->FindClass(env, "java/security/Key");
	jmethodID getEncoded_method = (*env)->GetMethodID(env, keyClass, "getEncoded", "()[B");
	jbyteArray raw = (jbyteArray)(*env)->CallObjectMethod(env, key, getEncoded_method);
	jobject secretKeySpec =(*env)->NewObject(env, secretKeySpecClass,initSecretKeySpecMethod, raw, str_aes);

	jclass cipherClass = (*env)->FindClass(env, "javax/crypto/Cipher");
	jmethodID getInstance_method = (*env)->GetStaticMethodID(env, cipherClass, "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Cipher;");
	jobject cipher = (*env)->CallStaticObjectMethod(env, cipherClass,getInstance_method, str_aes);
	jmethodID initCipher_method = (*env)->GetMethodID(env, cipherClass, "init", "(ILjava/security/Key;)V");
	jfieldID fid_cipher = (*env)->GetStaticFieldID(env, cipherClass, "DECRYPT_MODE", "I");
	int i_cipher = (*env)->GetStaticIntField(env, cipherClass, fid_cipher);
	(*env)->CallVoidMethod (env, cipher,initCipher_method, i_cipher, secretKeySpec);
	return cipher;
}


jstring rrr(JNIEnv* env, jobject application, jobject context,jstring libPath, jobject classLoader, jobject obj){
//	LOGI("%s:", "rrr start");
	jclass contextWrapperClass = (*env)->FindClass(env, "android/content/ContextWrapper");
	jmethodID getDir_method = (*env)->GetMethodID(env, contextWrapperClass, "getDir", "(Ljava/lang/String;I)Ljava/io/File;");
	jstring str_path = (*env)->NewStringUTF(env, "cache");
	jclass contextClass = (*env)->FindClass(env, "android/content/Context");
	jfieldID fid = (*env)->GetStaticFieldID(env, contextClass, "MODE_PRIVATE", "I");
	int i = (*env)->GetStaticIntField(env, contextClass, fid);
	jobject file = (*env)->CallObjectMethod(env, application, getDir_method, str_path, i);

	jclass fileClass = (*env)->FindClass(env, "java/io/File");
	jmethodID getAbsolutePath_method = (*env)->GetMethodID(env, fileClass, "getAbsolutePath", "()Ljava/lang/String;");
	jstring path = (jstring)(*env)->CallObjectMethod(env, file, getAbsolutePath_method);
	//	jclass MobileApplicationClass = (*env)->FindClass(env, "com/ncf/firstp2p/MobileApplication");
		//	jmethodID decryptFile_method = (*env)->GetMethodID(env, MobileApplicationClass, "decryptFile", "(Landroid/content/Context;Ljava/lang/String;)V");
	jclass StringBufferClass = (*env)->FindClass(env, "java/lang/StringBuffer");
	jmethodID initStringBufferMethod =(*env)->GetMethodID(env, StringBufferClass, "<init>","()V");
	jobject stringBuffer = (*env)->NewObject(env, StringBufferClass,initStringBufferMethod);
	jmethodID appendMethod =(*env)->GetMethodID(env, StringBufferClass, "append","(Ljava/lang/String;)Ljava/lang/StringBuffer;");
	(*env)->CallObjectMethod(env, stringBuffer, appendMethod, path);
	jstring str_jar = (*env)->NewStringUTF(env, "/mycode.jar");
	(*env)->CallObjectMethod(env, stringBuffer, appendMethod, str_jar);
	jmethodID toStringMethod =(*env)->GetMethodID(env, StringBufferClass, "toString","()Ljava/lang/String;");
	jstring fianlpath = (jstring)(*env)->CallObjectMethod(env, stringBuffer, toStringMethod);
//	(*env)->CallVoidMethod(env, application, decryptFile_method, context, fianlpath);
//	jstring psd = (*env)->NewStringUTF(env, rrr);
	jobject cipher = rr(env);
	//打印路径
				//	char* jstr = Jstring2CStr(env, fianlpath);
				//		LOGI("%s", jstr);
			//jiemi
			jclass destFileClass = (*env)->FindClass(env, "java/io/File");
			jmethodID initDestFileMethod =(*env)->GetMethodID(env, destFileClass, "<init>","(Ljava/lang/String;)V");
			jobject destFile =(*env)->NewObject(env, destFileClass,initDestFileMethod, fianlpath);
			jmethodID createNewFileMethod =(*env)->GetMethodID(env, destFileClass, "createNewFile","()Z");
			jboolean result = (*env)->CallBooleanMethod (env, destFile,createNewFileMethod);
//						LOGI("-------------------createNewFile result is %d -------------------", result);
	//输出流
			jclass fileOutputStreamClass = (*env)->FindClass(env, "java/io/FileOutputStream");
			jmethodID initFileOutputStreamClassMethod =(*env)->GetMethodID(env, fileOutputStreamClass, "<init>","(Ljava/io/File;)V");
			jobject out =(*env)->NewObject(env, fileOutputStreamClass,initFileOutputStreamClassMethod, destFile);
			jclass CipherOutputStreamClass = (*env)->FindClass(env, "javax/crypto/CipherOutputStream");
			jmethodID initCipherOutputStreamClassMethod =(*env)->GetMethodID(env, CipherOutputStreamClass, "<init>",
					"(Ljava/io/OutputStream;Ljavax/crypto/Cipher;)V");
			jobject cout =(*env)->NewObject(env, CipherOutputStreamClass,initCipherOutputStreamClassMethod, out, cipher);
			jclass AssetManagerClass = (*env)->FindClass(env, "android/content/res/AssetManager");
			jmethodID open_method = (*env)->GetMethodID(env, AssetManagerClass, "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
			jclass ContextClass = (*env)->FindClass(env, "android/content/Context");
			jmethodID getAssets_method = (*env)->GetMethodID(env, ContextClass, "getAssets", "()Landroid/content/res/AssetManager;");
			jobject assetManager = (*env)->CallObjectMethod(env, context,getAssets_method);
			jstring str_dat = (*env)->NewStringUTF(env, "mycode.dat");
			jobject in = (*env)->CallObjectMethod(env, assetManager,open_method, str_dat);
			if (in != NULL) {
	//			LOGI("%d", 1);
				jbyteArray  cache = (*env)->NewByteArray(env,1024);//建立jbarray数组
				int nRead = 0;
				int index=0;
				jclass InputStreamClass = (*env)->FindClass(env, "java/io/InputStream");
				jmethodID read_method = (*env)->GetMethodID(env, InputStreamClass, "read", "([B)I");
				jmethodID write_method = (*env)->GetMethodID(env, CipherOutputStreamClass, "write", "([BII)V");
				jmethodID flush_method = (*env)->GetMethodID(env, CipherOutputStreamClass, "flush", "()V");
				jmethodID cout_close_method = (*env)->GetMethodID(env, CipherOutputStreamClass, "close", "()V");
				jmethodID out_close_method = (*env)->GetMethodID(env, fileOutputStreamClass, "close", "()V");
				jmethodID in_close_method = (*env)->GetMethodID(env, InputStreamClass, "close", "()V");
//				nRead = (*env)->CallIntMethod (env, in,read_method, cache);
				while((nRead = (*env)->CallIntMethod (env, in,read_method, cache)) != -1){
//					LOGI("%d", nRead);
					(*env)->CallVoidMethod (env, cout,write_method, cache, 0, nRead);
					(*env)->CallVoidMethod (env, cout,flush_method);
//					nRead = (*env)->CallIntMethod (env, in,read_method, cache);
					index++;
				}
//						LOGI("%d", index);
	//			LOGI("%d", 2);
				(*env)->CallVoidMethod (env, cout,cout_close_method);
	//			LOGI("%d", 3);
				(*env)->CallVoidMethod (env, out,out_close_method);
	//			LOGI("%d", 4);
				(*env)->CallVoidMethod (env, in,in_close_method);
	//			LOGI("%d", 5);
			}

			jclass dexLoaderClass = (*env)->FindClass(env, "dalvik/system/DexClassLoader");
			jmethodID initDexLoaderMethod =(*env)->GetMethodID(env, dexLoaderClass, "<init>","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
			jobject dexLoader =(*env)->NewObject(env, dexLoaderClass,initDexLoaderMethod, fianlpath, path, libPath, classLoader);


			//delete
			jclass StringBufferClass1 = (*env)->FindClass(env, "java/lang/StringBuffer");
			jmethodID initStringBufferMethod1 =(*env)->GetMethodID(env, StringBufferClass1, "<init>","()V");
			jobject stringBuffer1 = (*env)->NewObject(env, StringBufferClass1,initStringBufferMethod1);
			jmethodID appendMethod1 =(*env)->GetMethodID(env, StringBufferClass1, "append","(Ljava/lang/String;)Ljava/lang/StringBuffer;");
			(*env)->CallObjectMethod(env, stringBuffer1, appendMethod1, path);
			jstring str_dex = (*env)->NewStringUTF(env, "/mycode.dex");
			(*env)->CallObjectMethod(env, stringBuffer1, appendMethod1, str_dex);
			jmethodID toStringMethod1 =(*env)->GetMethodID(env, StringBufferClass1, "toString","()Ljava/lang/String;");
			jstring dexPath = (jstring)(*env)->CallObjectMethod(env, stringBuffer1, toStringMethod1);

			jclass StringBufferClass2 = (*env)->FindClass(env, "java/lang/StringBuffer");
			jmethodID initStringBufferMethod2 =(*env)->GetMethodID(env, StringBufferClass2, "<init>","()V");
			jobject stringBuffer2 = (*env)->NewObject(env, StringBufferClass2,initStringBufferMethod2);
			jmethodID appendMethod2 =(*env)->GetMethodID(env, StringBufferClass2, "append","(Ljava/lang/String;)Ljava/lang/StringBuffer;");
			(*env)->CallObjectMethod(env, stringBuffer2, appendMethod2, path);
			jstring str_jar1 = (*env)->NewStringUTF(env, "/mycode.jar");
			(*env)->CallObjectMethod(env, stringBuffer2, appendMethod2, str_jar1);
			jmethodID toStringMethod2 =(*env)->GetMethodID(env, StringBufferClass2, "toString","()Ljava/lang/String;");
			jstring jarPath = (jstring)(*env)->CallObjectMethod(env, stringBuffer2, toStringMethod2);

//			LOGI("%s", "--stringbuffer success--");
			jclass deleteFileClass = (*env)->FindClass(env, "java/io/File");
			jmethodID exists_method = (*env)->GetMethodID(env, deleteFileClass, "exists", "()Z");
			jmethodID delete_method = (*env)->GetMethodID(env, deleteFileClass, "delete", "()Z");
		//	jboolean isExists = (*env)->CallBooleanMethod (env, destFile,exists_method);
			jmethodID initDeleteFileClassMethod =(*env)->GetMethodID(env, deleteFileClass, "<init>","(Ljava/lang/String;)V");

			jobject file1 =(*env)->NewObject(env, deleteFileClass,initDeleteFileClassMethod, dexPath);
			if((*env)->CallBooleanMethod (env, file1,exists_method)){
				(*env)->CallBooleanMethod (env, file1,delete_method);
			}
			jobject file2 =(*env)->NewObject(env, deleteFileClass,initDeleteFileClassMethod, jarPath);
			if((*env)->CallBooleanMethod (env, file2,exists_method)){
				(*env)->CallBooleanMethod (env, file2,delete_method);
			}
//			LOGI("%s", "--delete success--");
			jclass refInvokeClass = (*env)->FindClass(env, "com/example/hello/vo/RefInvoke");
			const char* func = "setFieldOjbect";
			jmethodID inject_method = (*env)->GetStaticMethodID(env, refInvokeClass, "setFieldOjbect", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V");
			jstring para1 = (*env)->NewStringUTF(env, "android.app.LoadedApk");
			jstring para2 = (*env)->NewStringUTF(env, "mClassLoader");
			(*env)->CallStaticVoidMethod(env, refInvokeClass,inject_method, para1, para2, obj, dexLoader);



	return (*env)->NewStringUTF(env, "");
//	return (*env)->NewStringUTF(env, "Native method return!");
};

JNIEXPORT jstring JNICALL Java_com_example_hello_MyApplication_run(JNIEnv *env,jclass myclass, jobject application,
		 jobject context,jstring libPath, jobject classLoader, jobject obj) {

    return rrr(env, application, context, libPath, classLoader, obj);
}

