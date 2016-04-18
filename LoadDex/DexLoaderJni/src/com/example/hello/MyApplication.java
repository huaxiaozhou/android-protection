package com.example.hello;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Map;

import android.app.Application;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.Configuration;

import com.example.hello.vo.RefInvoke;

public class MyApplication extends Application {
	private long start;
	private long end;
	
	static{
		System.loadLibrary("load");
	}

	@Override
	protected void attachBaseContext(Context base) {
		super.attachBaseContext(base);
		Object currentActivityThread = RefInvoke.invokeStaticMethod(
				"android.app.ActivityThread", "currentActivityThread",
				new Class[] {}, new Object[] {});
		String packageName = getPackageName();
		Map mPackages = (Map) RefInvoke.getFieldOjbect(
				"android.app.ActivityThread", currentActivityThread,
				"mPackages");
		WeakReference wr = (WeakReference) mPackages.get(packageName);

		run(this, base, "/data/data/"+ base.getPackageName() + "/lib/", base.getClassLoader().getParent(),wr.get());
	}
	
	private native String run(Application wrapper, Context context, String libPath,ClassLoader classLoader, Object obj);
	
 
	@Override
	public void onCreate() {
		// 如果源应用配置有Appliction对象，则替换为源应用Applicaiton，以便不影响源程序逻辑。  
		String appClassName = "com.ncf.firstp2p.MobileApplication";
		start = System.currentTimeMillis();
		/**
         * 调用静态方法android.app.ActivityThread.currentActivityThread
         * 获取当前activity所在的线程对象
         */
        Object currentActivityThread = RefInvoke.invokeStaticMethod(  
                "android.app.ActivityThread", "currentActivityThread",  
                new Class[] {}, new Object[] {});  
        /**
         * 获取currentActivityThread中的mBoundApplication属性对象，该对象是一个
         *  AppBindData类对象，该类是ActivityThread的一个内部类
         */
        Object mBoundApplication = RefInvoke.getFieldOjbect(  
                "android.app.ActivityThread", currentActivityThread,  
                "mBoundApplication");  
        /**
         * 获取mBoundApplication中的info属性，info 是 LoadedApk类对象
         */
        Object loadedApkInfo = RefInvoke.getFieldOjbect(  
                "android.app.ActivityThread$AppBindData",  
                mBoundApplication, "info");  
        /**
         * loadedApkInfo对象的mApplication属性置为null
         */
        RefInvoke.setFieldOjbect("android.app.LoadedApk", "mApplication",  
                loadedApkInfo, null);  
        /**
         * 获取currentActivityThread对象中的mInitialApplication属性
         * 这货是个正牌的 Application
         */
        Object oldApplication = RefInvoke.getFieldOjbect(  
                "android.app.ActivityThread", currentActivityThread,  
                "mInitialApplication");  
        /**
         * 获取currentActivityThread对象中的mAllApplications属性
         * 这货是 装Application的列表
         */
        ArrayList<Application> mAllApplications = (ArrayList<Application>) RefInvoke  
                .getFieldOjbect("android.app.ActivityThread",  
                        currentActivityThread, "mAllApplications");  
      //列表对象终于可以直接调用了 remove调了之前获取的application 抹去记录的样子
        mAllApplications.remove(oldApplication);  
        /**
         * 获取前面得到LoadedApk对象中的mApplicationInfo属性，是个ApplicationInfo对象
         */
        ApplicationInfo appinfo_In_LoadedApk = (ApplicationInfo) RefInvoke  
                .getFieldOjbect("android.app.LoadedApk", loadedApkInfo,  
                        "mApplicationInfo");  
        /**
         * 获取前面得到AppBindData对象中的appInfo属性，也是个ApplicationInfo对象
         */
        ApplicationInfo appinfo_In_AppBindData = (ApplicationInfo) RefInvoke  
                .getFieldOjbect("android.app.ActivityThread$AppBindData",  
                        mBoundApplication, "appInfo");  
      //把这两个对象的className属性设置为从meta-data中获取的被加密apk的application路径
        appinfo_In_LoadedApk.className = appClassName;  
        appinfo_In_AppBindData.className = appClassName;  
        /**
         * 调用LoadedApk中的makeApplication 方法 造一个application
         * 前面改过路径了 
         */
        Application app = (Application) RefInvoke.invokeMethod(  
                "android.app.LoadedApk", "makeApplication", loadedApkInfo,  
                new Class[] { boolean.class, Instrumentation.class },  
                new Object[] { false, null });  
        RefInvoke.setFieldOjbect("android.app.ActivityThread",  
                "mInitialApplication", currentActivityThread, app);  

        end = System.currentTimeMillis();
        System.out.println("替换进程spent time is " + (end - start) + "ms");
//        Map mProviderMap = (Map) RefInvoke.getFieldOjbect(  
//                "android.app.ActivityThread", currentActivityThread,  
//                "mProviderMap");  
//        Iterator it = mProviderMap.values().iterator();  
//        while (it.hasNext()) {  
//            Object providerClientRecord = it.next();  
//            Object localProvider = RefInvoke.getFieldOjbect(  
//                    "android.app.ActivityThread$ProviderClientRecord",  
//                    providerClientRecord, "mLocalProvider");  
//            RefInvoke.setFieldOjbect("android.content.ContentProvider",  
//                    "mContext", localProvider, app);  
//        }  
        if(null == app){
//            Log.e(TAG, "application get is null !");
        }else{
        	start = System.currentTimeMillis();
            app.onCreate();
        }
        end = System.currentTimeMillis();
        System.out.println("启动新进程spent time is " + (end - start) + "ms");
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
	}

	@Override
	public void onLowMemory() {
		super.onLowMemory();
	}

}
