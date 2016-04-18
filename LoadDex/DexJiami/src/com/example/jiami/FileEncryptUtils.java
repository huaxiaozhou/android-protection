package com.example.jiami;


import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;

public class FileEncryptUtils {
	static void encryptFile(String key, String sourceFilePath, String destFilePath) throws Exception {
//        String sourceFilePath = "e:/firstp2p.jar";
//        String destFilePath = "e:/firstp2p.dat";
        AESUtils.encryptFile(key, sourceFilePath, destFilePath);
    }
     
    static void decryptFile(String key, String sourceFilePath, String destFilePath) throws Exception {
//        String sourceFilePath = "e:/firstp2p.dat";
//        String destFilePath = "e:/firstp2p_unaes.jar";
        AESUtils.decryptFile(key, sourceFilePath, destFilePath);
    }
	
	
	/**
     * 初始化 AES Cipher
     * @param sKey
     * @param cipherMode
     * @return
     */
//	public static Cipher initAESCipher(String sKey, int cipherMode) {
//		// 创建Key gen
//		KeyGenerator keyGenerator = null;
//		Cipher cipher = null;
//		SecretKeySpec key = null;
//		try {
//			keyGenerator = KeyGenerator.getInstance("AES");
//			keyGenerator.init(128, new SecureRandom(sKey.getBytes()));
//			SecretKey secretKey = keyGenerator.generateKey();
//			byte[] codeFormat = secretKey.getEncoded();
//			switch(cipherMode){
//			case Cipher.ENCRYPT_MODE:
//				key = new SecretKeySpec(Base64.getEncoder().encode(codeFormat), "AES");
//				break;
//			case Cipher.DECRYPT_MODE:
//				key = new SecretKeySpec(Base64.getDecoder().decode(codeFormat), "AES");
//				break;
//			}
//			cipher = Cipher.getInstance("AES");
//			// 初始化
//			cipher.init(cipherMode, key);
//		} catch (NoSuchAlgorithmException e) {
//			e.printStackTrace(); // To change body of catch statement use File |
//									// Settings | File Templates.
//		} catch (NoSuchPaddingException e) {
//			e.printStackTrace(); // To change body of catch statement use File |
//									// Settings | File Templates.
//		} catch (InvalidKeyException e) {
//			e.printStackTrace(); // To change body of catch statement use File |
//									// Settings | File Templates.
//		}
//		return cipher;
//	}
	
	/**
     * <p>
     * 文件转换为二进制数组
     * </p>
     *
     * @param filePath 文件路径
     * @return
     * @throws Exception
     */
	public static byte[] fileToByte(String filePath) throws Exception {
		byte[] data = new byte[0];
		File file = new File(filePath);
		if (file.exists()) {
			FileInputStream in = new FileInputStream(file);
			ByteArrayOutputStream out = new ByteArrayOutputStream(2048);
			byte[] cache = new byte[1024];
			int nRead = 0;
			while ((nRead = in.read(cache)) != -1) {
				out.write(cache, 0, nRead);
				out.flush();
			}
			out.close();
			in.close();
			data = out.toByteArray();
		}
		return data;
	}
	
	/**
     * <p>
     * 将文件编码为BASE64字符串
     * </p>
     * <p>
     * 大文件慎用，可能会导致内存溢出
     * </p>
     *
     * @param filePath 文件绝对路径
     * @return
     * @throws Exception
     */
//    public static byte[] encodeFile(String filePath) throws Exception {
//        byte[] bytes = fileToByte(filePath);
//        return encode(bytes);
//    }
    
    /**
     * <p>
     * BASE64字符串解码为二进制数据
     * </p>
     *
     * @param base64
     * @return
     * @throws Exception
     */
//    public static byte[] decode(byte[] base64) throws Exception {
//        return Base64.getDecoder().decode(base64);
//    }
     
    /**
     * <p>
     * 二进制数据编码为BASE64字符串
     * </p>
     *
     * @param bytes
     * @return
     * @throws Exception
     */
//    public static byte[] encode(byte[] bytes) throws Exception {
//        return Base64.getEncoder().encode(bytes);
//    }
}
