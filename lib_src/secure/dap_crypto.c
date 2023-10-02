//
// Created by KimByoungGook on 2020-06-12.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "secure/dap_secure.h"
#include "com/dap_com.h"


static unsigned char ENC_AESKEY[] =
        {
                0x31, 0xf7, 0xe5, 0xab, 0x43, 0xd2,
                0x43, 0x9f, 0x87, 0xf1, 0x13, 0x6f,
                0x14, 0xcd, 0x96, 0x5f,	0xff, 0x95,
                0x10, 0x7c, 0x6b, 0xc8, 0x4d, 0x8c,
                0x93, 0xbb, 0x0e, 0x88, 0xd8, 0xe8,
                0xe9, 0xf5
        };

static unsigned char ENC_IV[] =
        {
                0x32, 0xf8, 0xe6, 0xac, 0x44,
                0xd3, 0x44, 0xa0, 0x88, 0xf2,
                0x14, 0x70, 0x15, 0xce, 0x07, 0x60,
                0x11, 0x45, 0x35, 0xce, 0x17, 0x50,
                0x12, 0x46, 0x36, 0xc6, 0x18, 0x51,
                0x13, 0x42, 0x31, 0xc6
        };


static char base64_table[] =
        { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
          'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
          'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
          'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
        };
static char base64_pad = '=';

static const short base64_reverse_table[256] = {
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
        -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
        -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};


// OpenSSL 초기화 함수
static void fstInitOpenSSL() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

// OpenSSL 정리 함수
static void fstCleanupOpenSSL() {
    ERR_free_strings();
    EVP_cleanup();
}

int fsec_Base64Encode(unsigned char *data, int inSize, char** result)
{
    unsigned char *current = data;
    int i = 0;
    int resSize = (((inSize + 3 - inSize % 3) * 4 / 3 + 1) * sizeof(char));
    // or  size = ((length + 2) / 3) * 4 * sizeof(char)
    unsigned char *tmpResult = (unsigned char *)malloc(resSize + 1);
    memset(tmpResult, 0x00, resSize + 1);

    while (inSize > 2) { /* keep going until we have less than 24 bits */
        tmpResult[i++] = base64_table[current[0] >> 2];
        tmpResult[i++] = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
        tmpResult[i++] = base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
        tmpResult[i++] = base64_table[current[2] & 0x3f];

        current += 3;
        inSize -= 3; /* we just handle 3 octets of data */
    }

    /* now deal with the tail end of things */
    if (inSize != 0) {
        tmpResult[i++] = base64_table[current[0] >> 2];
        if (inSize > 1) {
            tmpResult[i++] = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            tmpResult[i++] = base64_table[(current[1] & 0x0f) << 2];
            tmpResult[i++] = base64_pad;
        }
        else {
            tmpResult[i++] = base64_table[(current[0] & 0x03) << 4];
            tmpResult[i++] = base64_pad;
            tmpResult[i++] = base64_pad;
        }
    }

    tmpResult[i] = '\0';

    *result = tmpResult;

    return i;
}




void fsec_HandleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int fsec_Encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx = NULL;

    int len = 0;
    int ciphertext_len = 0;

    fstInitOpenSSL();

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        unsigned long errCode = 0;
        char errStr[256]  = {0x00,};
        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Encrypto Failed (%d)(%s)", errCode,errStr);

        fstCleanupOpenSSL(); // OpenSSL 정리
        return 0;
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        unsigned long errCode = 0;
        char errStr[256]  = {0x00,};
        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Encrypto Failed (%d)(%s)", errCode,errStr);

        fstCleanupOpenSSL(); // OpenSSL 정리
        return 0;
    }

    /* Provide the message to be encrypted, and obtain the encrypted output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        unsigned long errCode = 0;
        char errStr[256]  = {0x00,};
        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Encrypto Failed (%d)(%s)", errCode,errStr);

        fstCleanupOpenSSL(); // OpenSSL 정리
        return 0;
    }
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    * this stage.
    */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        unsigned long errCode = 0;
        char errStr[256]  = {0x00,};
        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Encrypto Failed (%d)(%s)", errCode,errStr);

        fstCleanupOpenSSL(); // OpenSSL 정리
        return 0;
    }
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    fstCleanupOpenSSL(); // OpenSSL 정리

    return ciphertext_len;
}


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext, int *pDecrypt_size)
{
    EVP_CIPHER_CTX *ctx = NULL;

    int len = 0;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        ERR_print_errors_fp(stderr);

        return 0;
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        ERR_print_errors_fp(stderr);

        EVP_CIPHER_CTX_free(ctx);

        return 0;
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_DecryptUpdate can be called multiple times if necessary
    */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        ERR_print_errors_fp(stderr);

        EVP_CIPHER_CTX_free(ctx);

        return 0;
    }

    *pDecrypt_size = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        ERR_print_errors_fp(stderr);

        EVP_CIPHER_CTX_free(ctx);

        return 0;
    }

    *pDecrypt_size += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return 1;
}


int fsec_Decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext, int *pDecrypt_size)
{
    EVP_CIPHER_CTX *ctx = NULL;

    int len = 0;

    fstInitOpenSSL();

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        unsigned long errCode = 0;
        char errStr[256]  = {0x00,};
        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Decyprto Failed (%d)(%s)", errCode,errStr);

        fstCleanupOpenSSL(); // OpenSSL 정리
        return 0;
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    * IV size for *most* modes is the same as the block size. For AES this
    * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        unsigned long errCode;
        char errStr[256];

        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Decyprto Failed (%d)(%s)", errCode,errStr);

        EVP_CIPHER_CTX_free(ctx);
        fstCleanupOpenSSL(); // OpenSSL 정리

        return 0;
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_DecryptUpdate can be called multiple times if necessary
    */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        unsigned long errCode;
        char errStr[256];

        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DEBUG,"Decyprto Failed (%d)(%s)", errCode,errStr);

        EVP_CIPHER_CTX_free(ctx);
        fstCleanupOpenSSL(); // OpenSSL 정리

        return 0;
    }

    *pDecrypt_size = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        unsigned long errCode;
        char errStr[256];

        errCode = ERR_get_error(); // 에러 코드 가져오기
        ERR_error_string_n(errCode, errStr, sizeof(errStr)); // 에러 문자열 가져오기
        WRITE_CRITICAL(CATEGORY_DB,"Decyprto Failed (%d)(%s)", errCode,errStr);

        EVP_CIPHER_CTX_free(ctx);
        fstCleanupOpenSSL(); // OpenSSL 정리

        return 0;
    }

    *pDecrypt_size += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    fstCleanupOpenSSL(); // OpenSSL 정리

    return 1;
}

int fsec_DecryptStr(char *szData, char *szDecryptData, int *pnDecryptDataSize)
{
    int nDecryptDataSize = 0;
    int nDataSize = strlen(szData);
    int nIndex = 0, nValue = 0;
    int nLen = nDataSize >> 1;
    unsigned char *pCiphertext = NULL;
    unsigned char *byBuff = NULL;


    if (NULL == szData)
    {
        return 0;
    }
    if (nDataSize <= 0)
    {
        return 0;
    }

    pCiphertext = (unsigned char *)malloc(nLen + 1);
    memset(pCiphertext, 0x00, nLen + 1);
    byBuff = (unsigned char*)malloc(nLen + 1);
    for (nIndex = 0; nIndex < nLen; ++nIndex)
    {
        sscanf(&szData[nIndex << 1], "%02X", &nValue);
        byBuff[nIndex] = (unsigned char)nValue;
    }

    // Decrypt
    if (!fsec_Decrypt(byBuff, nLen, ENC_AESKEY, ENC_IV, (unsigned char*)pCiphertext, &nDecryptDataSize))
    {
        free(pCiphertext);
        pCiphertext = NULL;
        free(byBuff);
        byBuff = NULL;
        return 0;
    }

    memcpy(szDecryptData, (unsigned char*)pCiphertext, nDecryptDataSize);

    free(pCiphertext);
    pCiphertext = NULL;

    free(byBuff);
    byBuff = NULL;

    *pnDecryptDataSize = nDecryptDataSize;

    return 1;
}
int fsec_Base64Decode(unsigned char *data, char **result)
{
    const unsigned char *current = data;
    int ch, i = 0, j = 0, k;
    int strict = 0;
    int length = (int)strlen(data);
    /* this sucks for threaded environments */
    unsigned char *tmpResult;

    tmpResult = (unsigned char *)malloc(length);

    /* run through the whole string, converting as we go */
    while ((ch = *current++) != '\0' && length-- > 0)
    {
        if (ch == base64_pad)
        {
            if (*current != '=' && ((i % 4) == 1 || (strict && length > 0)))
            {
                if ((i % 4) != 1)
                {
                    while (isspace(*(++current)))
                    {
                        continue;
                    }
                    if (*current == '\0') {
                        continue;
                    }
                }
                free(tmpResult);
                return -1;
            }
            continue;
        }

        ch = base64_reverse_table[ch];
        if ((!strict && ch < 0) || ch == -1) /* a space or some other separator character, we simply skip over */
        {
            continue;
        }
        else if (ch == -2)
        {
            free(tmpResult);
            return -1;
        }

        switch(i % 4)
        {
            case 0:
                tmpResult[j] = ch << 2;
                break;
            case 1:
                tmpResult[j++] |= ch >> 4;
                tmpResult[j] = (ch & 0x0f) << 4;
                break;
            case 2:
                tmpResult[j++] |= ch >>2;
                tmpResult[j] = (ch & 0x03) << 6;
                break;
            case 3:
                tmpResult[j++] |= ch;
                break;
        }
        i++;
    }

    k = j;
    /* mop things up if we ended on a boundary */
    if (ch == base64_pad)
    {
        switch(i % 4)
        {
            case 1:
                free(tmpResult);
                return -1;
            case 2:
                k++;
            case 3:
                tmpResult[k] = 0;
        }
    }

    tmpResult[j] = '\0';

    *result = tmpResult;

    return j;
}


int fsec_EncryptStr(char *szData, int nDataSize, char *szCryptData, int *pnCryptDataSize)
{
    if (NULL == szData || nDataSize <= 0)
    {
        return 0;
    }

    int nStrSize = strlen(szData);

    int nCryptDataSize = 0;

    unsigned char *pCiphertext = NULL;
    pCiphertext = (unsigned char *)malloc(nDataSize + _countof(ENC_IV));

    memset(pCiphertext, 0x00, nDataSize + _countof(ENC_IV));

    //BYTE *byBuffData = NULL;
    unsigned char *byBuffData = NULL;
    int nBufferLength = (nStrSize + 1) * 2;
    if (nBufferLength < sizeof(ENC_IV))
        nBufferLength = sizeof(ENC_IV) + 1;
    //byBuffData = (BYTE*)malloc(nBufferLength);
    byBuffData = (unsigned char*)malloc(nBufferLength);

    // Encrypt
    nCryptDataSize = fsec_Encrypt((unsigned char*)szData, nDataSize, ENC_AESKEY, ENC_IV, pCiphertext);

    memcpy(byBuffData, pCiphertext, nCryptDataSize);

    free(pCiphertext);
    pCiphertext = NULL;

    *pnCryptDataSize = nCryptDataSize;

    char szTemp[3] = "";

    int nIndex = 0;
    for (nIndex = 0; nIndex < nCryptDataSize; ++nIndex)
    {
        sprintf(szTemp, "%02X", byBuffData[nIndex]);
        strcat(szCryptData, szTemp);
    }

    free(byBuffData);
    byBuffData = NULL;

    return 1;
}