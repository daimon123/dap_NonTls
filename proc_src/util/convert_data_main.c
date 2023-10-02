
//#include <common.h>
#include <sys/wait.h>
#include <com/dap_com.h>
#include <string.h>
//#include <btree.h>

#include <secure/dap_secure.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>

char bufkey[1024];

/*
void normal_aes_test()
{
	char result[4024];
	char cpInBuf[1024];
	
	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	memset(result, 0x00, sizeof(result));

	printf("Data : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';
	
	aes_encrypt((char*)&cpInBuf, (char*)&result, sizeof(cpInBuf));
	printf("Convert=[%s]\n", result);
	int encrypt_size;
	char p_temp[1024];
	encrypt_size = (sizeof(cpInBuf) + AES_BLOCK_SIZE);
	memcpy(p_temp, result, encrypt_size);
	memset(result, 0x00, sizeof(result));
	aes_decrypt(p_temp, result, encrypt_size);
	printf("Decrypt=[%s]\n", result);
	printf("\n");
}
*/
/*
void encode_msggw()
{
	char result[4024];
	char cpInBuf[1024];
	
	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	memset(result, 0x00, sizeof(result));

	printf("Data : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';
	
	dataAESEncrypt((char*)&cpInBuf, strlen(cpInBuf), bufkey, (char*)&result);
	printf("Convert=[%s]\n", result);

	printf("\n");
}
*/
void encode_evp()
{
    int encrypto_size;
	char result[1024];
	char cpInBuf[1024];

	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	memset(result, 0x00, sizeof(result));

	printf("Data : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';

//	EncryptStr((char*)&cpInBuf, strlen(cpInBuf), result, &encrypto_size);
	fsec_EncryptStr((char*)&cpInBuf, strlen(cpInBuf), result, &encrypto_size);
	printf("Convert=[%s](%d)\n", result, encrypto_size);

	printf("\n");
}
/*
void decode_msggw()
{
	char result[4024];
	char cpInBuf[1024];
	
	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	memset(result, 0x00, sizeof(result));
	
	printf("Data : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';
	
	printf("bufkey=[%s]\n", bufkey);
	dataAESDecrypt((char*)&cpInBuf, bufkey, (char*)&result);
	printf("Convert=[%s]\n", result);

	printf("\n");
}
*/

void decode_evp()
{
	char result[4024];
	char cpInBuf[1024];
	int	 decrypto_size;
	
	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	memset(result, 0x00, sizeof(result));

	printf("Data : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';
	
//	DecryptStr((char*)&cpInBuf, (char*)&result, &decrypto_size);
	fsec_DecryptStr((char*)&cpInBuf, (char*)&result, &decrypto_size);
	printf("Convert=[%s](%d)\n", result,decrypto_size);

	printf("\n");
}


void pConfig()
{
	int		decrypto_size;
	char	cpUserName[64];
	char	cpPasswd[64];
	char	cpDB[64];
	char	cpBuf[64];
	char	tmpUser[256];
	char	tmpPwd[256];
	char	tmpDB[256];

	sprintf(cpBuf, "%s/config/dap.cfg", getenv("DAP_HOME"));
	fcom_SetIniName(cpBuf);

	memset(tmpUser, 0x00, sizeof(tmpUser));
	memset(tmpPwd, 0x00, sizeof(tmpPwd));
	memset(tmpDB, 0x00, sizeof(tmpDB));
	
//	GetProfile("MYSQL", "DB_ID", tmpUser,   "master");
	fcom_GetProfile("MYSQL", "DB_ID", tmpUser,   "master");
    fcom_GetProfile("MYSQL", "DB_PASSWORD", tmpPwd, "master");
    fcom_GetProfile("MYSQL", "DB_NAME", tmpDB, "master");

	printf("user=[%s]\n", tmpUser);	
	printf("password=[%s]\n", tmpPwd);	
	printf("dbname=[%s]\n", tmpDB);	
	memset(cpUserName, 0x00, sizeof(cpUserName));
	fsec_DecryptStr((char*)&tmpUser, (char*)&cpUserName, &decrypto_size);

	memset(cpPasswd, 0x00, sizeof(cpPasswd));
	fsec_DecryptStr((char*)&tmpPwd, (char*)&cpPasswd, &decrypto_size);
	
	memset(cpDB, 0x00, sizeof(cpDB));
	fsec_DecryptStr((char*)&tmpDB, (char*)&cpDB, &decrypto_size);
	
	printf("MySql : [mysql -u %s -p %s] pwd=[%s]\n", cpUserName, cpDB, cpPasswd);
	
}
/*
void psg()
{
	FILE *fp;
	char* posLastParam;
	char lastParam[500];
	char process[500];
	char decBuf[1024];
	
	char buff[1024];
	char cpInBuf[1024];
	
	char command[1024];
	int res=0;
	
	memset(cpInBuf, 0x00, sizeof(cpInBuf));
	
	printf("Process Name (default 'dap_') : ");
	fgets(cpInBuf, sizeof(cpInBuf), stdin);
	cpInBuf[strlen(cpInBuf)-1] = '\0';
	
	if(strlen(cpInBuf) == 0)
		strcpy(cpInBuf, "mms_");
	
	sprintf(command, "ps -ef | grep %s | grep -v grep", cpInBuf);
	
	fp = popen(command, "r");
	if (fp == NULL)
	{
		perror("erro : ");
		return;
	}
	
	while(fgets(buff, 1024, fp) != NULL)
	{
		res = 0;
		memset(lastParam, 0x00, sizeof(lastParam));
		memset(process, 0x00, sizeof(process));
		memset(decBuf, 0x00, sizeof(decBuf));
		
		posLastParam = strrchr(buff, ' ');
		
		strncpy(process, buff, posLastParam-buff);
		
		strcpy(lastParam, posLastParam+1);
		lastParam[strlen(lastParam) - 1] = 0x00;

		if(strncmp(lastParam, getenv("DAP_HOME"), strlen(getenv("DAP_HOME"))) != 0)
		{
			res = dataAESDecrypt(lastParam, bufkey, (char*)&decBuf);
		}

		printf("%s", process, res);

		if(res > 0)
			printf(" %s <-[%s]\n", decBuf, lastParam);
		else
			printf(" %s\n", lastParam);
	}
	
	pclose(fp);
}
*/

int	main(int argc, char* argv[])
{
	char cpInBuf[1024];
	char kind = 0;
	
	memset(bufkey, 0x00, 1024);
	memset(cpInBuf, 0x00, sizeof(cpInBuf));

	while(1)
	{
		printf("\n");
		printf("====================================\n");
		printf("| 1.Encode                         |\n");
        printf("| 8.Decode                         |\n");
        printf("| 9.Config                         |\n");
		printf("====================================\n");
		printf("command : ");
	
		fgets(cpInBuf, sizeof(cpInBuf), stdin);
		cpInBuf[strlen(cpInBuf)-1] = '\0';
	
		printf("\n");
		kind = cpInBuf[0];

		switch(kind)
		{
			case '1':
				//encode_msggw();
				encode_evp();
				break;
			case '8':
				//decode_msggw();
				decode_evp();
				break;
			case '9':
				pConfig();
				break;
			//case '4':
			//	psg();
			//	break;
			default:
				break;
		}
	}
	


	
	return 1;
}


