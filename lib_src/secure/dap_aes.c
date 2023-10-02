//
// Created by KimByoungGook on 2020-06-15.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/des.h>
#include "secure/dap_secure.h"


char* fsec_DeriveKeyCrypt(char* hBaseData, char* derivedKey)
{
    int i = 0;
    int keyLength = strlen(hBaseData);
    int requiredLength = 24;

    SHA_CTX sha;
    SHA_CTX sha2;

    char buff1[64];
    char buff2[64];

    char result1[200];
    char result2[200];

    if(keyLength >= requiredLength)
    {
        for(i = 0; i < requiredLength; i++)
        {
            derivedKey[i] = hBaseData[i];
        }
        return derivedKey;
    }

    memset(buff1, 0x36, sizeof(buff1));
    memset(buff2, 0x5C, sizeof(buff2));

    for(i = 0; i < keyLength; i++)
    {
        buff1[i] ^= hBaseData[i];
        buff2[i] ^= hBaseData[i];
    }

    memset(result1, 0x00, sizeof(result1));
    SHA1_Init(&sha);
    SHA1_Update(&sha, (void*)&buff1, 64);
    SHA1_Final((char*)&result1 ,&sha);

    memset(result2, 0x00, sizeof(result2));
    SHA1_Init(&sha2);
    SHA1_Update(&sha2, &buff2, 64);
    SHA1_Final((char*)&result2 ,&sha2);

    for(i = 0; i < requiredLength; i++)
    {
        if(i < (int)strlen(result1))
        {
            derivedKey[i] = result1[i];
        }
        else
        {
            derivedKey[i] = result2[i - strlen(result1)];
        }
    }

    return derivedKey;
}


char* fsec_KeyEncrypt(char* data, char* result)
{
    SHA_CTX sha;
    char shaResult[100];

    memset(shaResult, 0x00, sizeof(shaResult));

    SHA1_Init(&sha);
    SHA1_Update(&sha, data, strlen(data));
    SHA1_Final((char*)&shaResult ,&sha);

    fsec_DeriveKeyCrypt((char*)&shaResult, result);

    return NULL;
}

int fsec_PKCS5_UnPadding(char* src, int size, int block_size, char** result)
{
    int unpad = 0;

    if(size < block_size)
    {
        *result = NULL;
        return 0;
    }

    unpad = src[size-1];

    if((unpad < 1) || (unpad > block_size))
    {
        *result = NULL;
        return 0;
    }
    else
    {
        *result = (char*)malloc(size-unpad);
        memcpy(*result, src, size-unpad);
        return size-unpad;
    }

    return 0;
}


int fsec_DataAESDecrypt(char* data, char* key, char* result)
{
    AES_KEY wctx;
    char deriveKey[24];

    char* decData = NULL;
    char* desResult = NULL;
    char* tmpRes = NULL;
    char iv_tmp[16];
    int bin_size;

    int resSize = 0;


    if(strlen(data) == 0)
    {
        result[0] = 0x00;
        return 0;
    }
    memset(iv_tmp, 0x00, 32);

    memset(deriveKey, 0x00, sizeof(deriveKey));
    fsec_KeyEncrypt(key, (char*)&deriveKey);

    bin_size = fsec_Base64Decode(data, &decData);
    if(bin_size < 1)
    {
        result[0] = 0x00;
        return 0;
    }

    AES_set_decrypt_key((char*)&deriveKey, 128, &wctx);

    desResult = (char*)malloc(bin_size);
    memset(desResult, 0x00, bin_size);
    AES_cbc_encrypt(decData, desResult, bin_size, &wctx,  (char*)&iv_tmp,  AES_DECRYPT);

//    resSize = PKCS5_UnPadding((char*)desResult, bin_size, 16, &tmpRes);
    resSize = fsec_PKCS5_UnPadding((char*)desResult, bin_size, 16, &tmpRes);
    strncpy(result, tmpRes, resSize);

    free(desResult);
    free(decData);
    free(tmpRes);

    return resSize;
}