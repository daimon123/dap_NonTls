//
// Created by KimByoungGook on 2020-06-10.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "com/dap_com.h"

char 	cfgname[256];


void fcom_SetIniName(char* name)
{
    strcpy(cfgname, name);

}
char* fcom_makeToken(char* cpApp, char* cpRxt)

{
    int	loop;
    int in = 0;
    int	leng;


    leng = strlen(cpApp);

    for(loop=0; loop < leng; loop ++)
    {
        if(cpApp[loop] == '#')
        {
            cpRxt[0] = '\0';
            return &cpApp[leng];
        };

        if((cpApp[loop] == ' ') || (cpApp[loop] == '\t') || (cpApp[loop] == '=') ||  (cpApp[loop] == '\n') )
            continue;

        for(; loop < leng; loop ++)
        {
            if((cpApp[loop] == ' ') || (cpApp[loop] == '\t') || (cpApp[loop] == '=') ||  (cpApp[loop] == '\n') )
            {
                cpRxt[in] ='\0';
                return &cpApp[loop];
            };
            cpRxt[in++] = cpApp[loop];
        };
    };
    cpRxt[in] ='\0';
    return &cpApp[loop];
}

int	fcom_ReadProfile(char* cpApp, char* cpKey,char* cpRxt)
{
    char cpBuf[MAX_BUF] = {0x00,};
    char cpTmp[MAX_BUF] = {0x00,};
    char cpAppBuf[MAX_BUF] = {0x00,};
    char *cpPtr = NULL;
    FILE *fp = NULL;

    sprintf(cpAppBuf,"[%s]", cpApp);
    fp = fopen(cfgname, "r");


    if(fp == NULL)
    {
        return -1;
    }

    if( (strlen(cpApp) < 1) || (strlen(cpKey) < 1) )
    {
        if(fp != NULL)
            fclose(fp);
        return -2;
    }


    for(;;)
    {
        fgets(cpBuf,MAX_BUF, fp);
        if(feof(fp) )
        {
            fclose(fp);
            return -3;
        };

        cpPtr = fcom_makeToken(cpBuf, cpTmp);
        if(strcmp(cpTmp, cpAppBuf) == 0)
            break;
    }

    for(;;)
    {
        fgets(cpBuf,MAX_BUF, fp);
        if(feof(fp) )
            break;

        cpPtr = fcom_makeToken(cpBuf, cpTmp);

        if( (cpTmp[0] == '[') && (cpTmp[strlen(cpTmp) -1] == ']') )
        {
            if(fp != NULL)
                fclose(fp);
            return -2;
        }


        if(strcmp(cpTmp, cpKey) == 0)
        {
            cpPtr = fcom_makeToken(cpPtr, cpRxt);
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return -4;
}

int fcom_GetProfileInt(char* part, char* spart, int dval)
{

    int		rxt;
    char	buf[100];

    rxt = fcom_ReadProfile(part, spart,buf);

    if(rxt < 0)
    {
        return dval;
    }

    return (int) fcom_Str2Dec(buf);
}

char* fcom_GetProfile(char* part, char* spart, char* rval, char* dval )
{
    int     rxt;

    rxt = fcom_ReadProfile(part, spart,rval);
    if(rxt < 0)
    {
        strcpy(rval, dval);
        return dval;
    }

    return rval;
}


long fcom_Str2Dec(char *cpHex)
{
    char cpBuf[100];
    int nLen,nLoop;
    long lnResult=0,lnMultiple=1;

    strcpy(cpBuf,cpHex);
    if(strncmp(cpBuf,"0x",2)==0)
        strcpy(cpBuf,cpBuf+2);
    else
        return atol(cpBuf);

    nLen = strlen(cpBuf);
    for(nLoop=nLen-1;nLoop>=0;nLoop--)
    {
        if (cpBuf[nLoop]>='0'&&cpBuf[nLoop]<='9')
            lnResult += ((cpBuf[nLoop]-'0')*lnMultiple);
        else
        if (cpBuf[nLoop]>='A'&&cpBuf[nLoop]<='F')
            lnResult += ((cpBuf[nLoop]-'A'+10)*lnMultiple);
        else
        if (cpBuf[nLoop]>='a'&&cpBuf[nLoop]<='f')
            lnResult += ((cpBuf[nLoop]-'A'+10)*lnMultiple);

        lnMultiple *=16;
    }
    return lnResult;
}
