//
// Created by KimByoungGook on 2020-06-12.
//

#ifndef DAP_NEW_DAP_SECURE_H
#define DAP_NEW_DAP_SECURE_H


#define _countof(_Array)     sizeof(_Array) / sizeof(_Array[0])



#define DAP_KEY "INTENT00"
/* dap_aes.c */
/* ================================================================ */

char* fsec_KeyEncrypt(char* data, char* result);
char* fsec_DeriveKeyCrypt(char* hBaseData, char* derivedKey);
int fsec_DataAESDecrypt(char* data, char* key, char* result);
int fsec_PKCS5_UnPadding(char* src, int size, int block_size, char** result);
/* ================================================================ */
void fsec_HandleErrors(void);
int fsec_Base64Encode(unsigned char *data, int inSize, char** result);
int fsec_Encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);
int fsec_Decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext, int *pDecrypt_size);
int fsec_DecryptStr(char *szData, char *szDecryptData, int *pnDecryptDataSize);
int fsec_Base64Decode(unsigned char *data, char **result);
int fsec_EncryptStr(char *szData, int nDataSize, char *szCryptData, int *pnCryptDataSize);

/* ------------------------------------------------------------------- */


#endif //DAP_NEW_DAP_SECURE_H
