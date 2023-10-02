//
// Created by KimByoungGook on 2021-03-31.
//


#include <stdio.h>
#include <string.h>
#include <mutils/mcrypt.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

#define         IVKEY   "_FILE_INSPECTOR_"
int func_Decrypt(char* param_InputBuffer);
int func_Encrypt(char* param_InputBuffer);

int encrypt(void* buffer, int buffer_len, /* Because the plaintext could include null bytes*/
            char* IV, char* key, int key_len)
{
    MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
    int blocksize = mcrypt_enc_get_block_size(td);
    if (buffer_len % blocksize != 0)
    {
        return 1;
    }

    mcrypt_generic_init(td, key, key_len, IV);
    mcrypt_generic(td, buffer, buffer_len);
    mcrypt_generic_deinit(td);
    mcrypt_module_close(td);

    return 0;
}

int decrypt(void* buffer, int buffer_len, char* IV, char* key, int key_len)
{
    MCRYPT td = mcrypt_module_open("rijndael-128", NULL, "cbc", NULL);
    int blocksize = mcrypt_enc_get_block_size(td);
    if (buffer_len % blocksize != 0)
    {
        return 1;
    }

    mcrypt_generic_init(td, key, key_len, IV);
    mdecrypt_generic(td, buffer, buffer_len);
    mcrypt_generic_deinit(td);
    mcrypt_module_close(td);

    return 0;
}

int func_Decrypt(char* param_InputBuffer)
{
    unsigned char tr[16];
    unsigned char buffer[32];
    unsigned char result[32];
    char buff[32];
    int af[16];
    char Pre_Opt[256+1];

    int keysize = 16;
    int result_len = 16;
    int length = 0, j = 0;

    memset( tr , 0x00 , sizeof( tr ) );
    memset( buffer , 0x00 , sizeof( buffer ) );
    memset( result , 0x00 , sizeof( result ) );

    memset( buff , 0x00 , sizeof( buff ) );

    memset( af , 0x00 , sizeof( af ) );
    strcpy( Pre_Opt , param_InputBuffer);

    for(length = 0; length <= 32; length++)
    {
        if(length > 0 && (length % 2) == 0)
        {
            strncpy(buffer, Pre_Opt, strlen(Pre_Opt));
            strncpy(buff, &buffer[length - 2], 2);
            af[j] = strtol( buff , NULL , 16 );

            tr[j] = (unsigned char) af[j];
            j++;
        }
    }

    for(length = 0; length < 16; length++)
    {
        result[length] = tr[length];
    }

    decrypt(result, result_len, IVKEY, IVKEY, keysize);
    printf("Decrypt : [%s] \n",result);
}

int func_Encrypt(char* param_InputBuffer)
{
    unsigned char tr[16] = {0x00,};
    unsigned char buffer[32] = {0x00,};
    unsigned char	buffer_db[32] = { };
    char buff[32];
    int af[16];
    char Pre_Opt[256+1] = {0x00,};
    unsigned char	result[32] = {0x00, };
    unsigned char	result_buff[32] = {0x00, };

    int i = 0;
    int keysize = 16;
    int result_len = 16;
    int length = 0, j = 0;

    memset( tr , 0x00 , sizeof( tr ) );
    memset( buffer , 0x00 , sizeof( buffer ) );
    memset( result , 0x00 , sizeof( result ) );
    memset( buff , 0x00 , sizeof( buff ) );
    memset( af , 0x00 , sizeof( af ) );

    strncpy( buffer_db , param_InputBuffer, strlen(param_InputBuffer));

    encrypt(buffer_db, 16, IVKEY, IVKEY,  keysize);

    for(i = 0; i < 16; i++)
    {
        sprintf(result_buff, "%02X", buffer_db[i]);
        strcat(result, result_buff);
    }

    printf("Encrypt : [%s] \n",result);
}




int main(void)
{

    char cpInBuf[1024];
    char kind = 0;

    memset(cpInBuf, 0x00, sizeof(cpInBuf));

    while(1)
    {
        printf("\n");
        printf("====================================\n");
        printf("| 1.Encode                         |\n");
        printf("| 2.Decode                         |\n");
        printf("====================================\n");
        printf("command : ");

        fgets(cpInBuf, sizeof(cpInBuf), stdin);
        cpInBuf[strlen(cpInBuf) - 1] = '\0';

        printf("\n");
        kind = cpInBuf[0];

        switch(kind)
        {
            case '1':
            {
                memset(cpInBuf, 0x00, sizeof(cpInBuf));

                printf("Data : ");
                fgets(cpInBuf, sizeof(cpInBuf), stdin);
                cpInBuf[strlen(cpInBuf)-1] = '\0';

                func_Encrypt(cpInBuf);

                break;
            }

            case '2':
            {
                memset(cpInBuf, 0x00, sizeof(cpInBuf));

                printf("Data : ");
                fgets(cpInBuf, sizeof(cpInBuf), stdin);
                cpInBuf[strlen(cpInBuf)-1] = '\0';

                func_Decrypt(cpInBuf);
                break;
            }

        }
    }

}

