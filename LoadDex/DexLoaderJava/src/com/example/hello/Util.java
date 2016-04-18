package com.example.hello;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.security.Key;

import javax.crypto.Cipher;
import javax.crypto.CipherOutputStream;
import javax.crypto.spec.SecretKeySpec;

import android.content.Context;

public class Util {
    
	public static void decryptFile(Context paramContext, String destFilePath) throws Exception {
        InputStream in = paramContext.getAssets().open("mycode.dat");
        decryptFile(in, destFilePath);
    }

	/**
     * <p>
     * 文件解密
     * </p>
     *
     * @param key
     * @param sourceFilePath
     * @param destFilePath
     * @throws Exception
     */
    private static void decryptFile(InputStream in, String destFilePath) throws Exception {
        File destFile = new File(destFilePath);
        if (in != null) {
            if (!destFile.getParentFile().exists()) {
                destFile.getParentFile().mkdirs();
            }
            destFile.createNewFile();
            FileOutputStream out = new FileOutputStream(destFile);
            byte[] code = Base64Utils.decode("GiEhjghmZIO7RTWyycQ9PQ==");
            Key k = new SecretKeySpec(code, "AES");
            byte[] raw = k.getEncoded();
            SecretKeySpec secretKeySpec = new SecretKeySpec(raw, "AES");
            Cipher cipher = Cipher.getInstance("AES");
            cipher.init(Cipher.DECRYPT_MODE, secretKeySpec);
            CipherOutputStream cout = new CipherOutputStream(out, cipher);
            byte[] cache = new byte[1024];
            int nRead = 0;
            int index=0;
            while ((nRead = in.read(cache)) != -1) {
//            	System.out.println("nRead-->" + nRead);
                cout.write(cache, 0, nRead);
                cout.flush();
                index++;
            }
//            System.out.println("index-->" + index);
            cout.close();
            out.close();
            in.close();
        }
    }
    
}
