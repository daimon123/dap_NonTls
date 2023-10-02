//
// Created by KimByoungGook on 2020-06-12.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "ipc/dap_Queue.h"
#include "com/dap_com.h"


static int fipc_SQMake();

/*    ------------------------------------------             */
int fipc_SQInit(int qsiz, char *fpath)
{
    int rxt;

    g_nRear = 0;
    g_nFront = 0;
    g_nWidth = 0;


    g_nCrear = 0;
    g_nCfront = 0;
    g_nCwidth = 0;

    strcpy(g_szBackpath, fpath);
    g_nQnum = qsiz+1;

    if (data != NULL)
    {
        free(data);
        data = NULL;
    }

    rxt = fipc_SQMake();

    if (rxt < 0)
        return -1;
    else
        return 0;
}

int fipc_SQInitNotBackup( int qsiz )
{
    int rxt;

    g_nRear = 0;
    g_nFront = 0;
    g_nWidth = 0;


    g_nCrear = 0;
    g_nCfront = 0;
    g_nCwidth = 0;

    g_nQnum = qsiz+1;

    if (data != NULL)
    {
        free(data);
        data = NULL;
    }

    rxt = fipc_SQMake();

    if (rxt < 0)
        return -1;
    else
        return 0;
}

static int fipc_SQMake()
{
    if (data != NULL)
        free(data);
    data = (_DAP_QUEUE_BUF *)calloc(g_nQnum, sizeof(_DAP_QUEUE_BUF));
    if (data == NULL)
        return -1;
    else
        return 0;
}

void fipc_SQCrReset()
{
    g_nCrear = g_nRear;
    g_nCfront = g_nFront;
    g_nCwidth = g_nWidth;
}

int fipc_SQSaveQ()
{
    FILE *pFile;
    _DAP_QUEUE_BUF tbuf;
    size_t bsize;

    fipc_SQCrReset();

    if (strlen(g_szBackpath) < 1)
        return -1;
    if (data == NULL)
        return -2;

    pFile = fopen(g_szBackpath, "wb");
    if (pFile == NULL)
        return -3;

    bsize = sizeof(tbuf);

    for (;!fipc_SQIsAllRead();)
    {
        fipc_SQRead(&tbuf);
        fwrite((const void *)&tbuf, 1, bsize, pFile);
    }

    fclose(pFile);

    return 0;
}


int fipc_SQPut(_DAP_QUEUE_BUF *pData)
{
    if (data == NULL)
        return -2;

    if (!fipc_SQIsFull())
    {
        memcpy((void *)&data[g_nRear], (void *)pData, sizeof(_DAP_QUEUE_BUF));
        g_nRear++;
        g_nRear = g_nRear % g_nQnum;
        g_nWidth++;
        return 0;
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_IPC, "SQ is Full! ");
        return -1;
    }
}


int fipc_SQLoadQ()
{
    FILE *pFile;
    _DAP_QUEUE_BUF tbuf;

    size_t rslt;
    size_t bsize;

    if (strlen(g_szBackpath) < 1)
        return -1;

    pFile = fopen(g_szBackpath, "rb");
    if (pFile == NULL)
        return -2;

    bsize = sizeof(tbuf);

    while(1)
    {
        rslt = fread((void *)&tbuf, 1, bsize,pFile);
        if (ferror(pFile))
        {
            fclose(pFile);
            return -3;
        }
        if (rslt != bsize)
            break;
        fipc_SQPut(&tbuf);
    }

    fclose(pFile);

    return 0;
}

void fipc_SQClear()
{
    g_nRear 	= 0;
    g_nFront	= 0;
    g_nWidth	= 0;

    g_nCrear	= 0;
    g_nCfront	= 0;
    g_nCwidth	= 0;

    memset(g_szBackpath, 0x00, sizeof(g_szBackpath));

    fcom_MallocFree((void**)&data);
}

int fipc_SQRemove()
{
    char cmdBuff[256];

    if (strlen(g_szBackpath) < 1)
        return -1;

    memset(cmdBuff, 0x00, sizeof(cmdBuff));
    sprintf(cmdBuff, "cat /dev/null > %s", g_szBackpath);
    if(system(cmdBuff) < 0)
    {
        return -1;
    }

    return 0;
}

_DAP_QUEUE_BUF* fipc_SQGet(_DAP_QUEUE_BUF *pData)
{
    int tmp = g_nFront;

    if (data == NULL)
        return NULL;

    if (!fipc_SQIsEmpty())
    {
        g_nFront++;
        g_nFront = g_nFront % g_nQnum;
        memcpy( (void *)pData, (void *)&data[tmp], sizeof(_DAP_QUEUE_BUF) );
        g_nWidth--;
        return pData;
    }
    else
    {
        return NULL;
    }
}

_DAP_QUEUE_BUF* fipc_SQRead (_DAP_QUEUE_BUF* pData)
{
    int tmp = g_nCfront;

    if (data == NULL) return NULL;

    if (!fipc_SQIsAllRead())
    {
        g_nCfront++;
        g_nCfront = g_nCfront % g_nQnum;

        memcpy( (void *)pData, (void *)&data[tmp], sizeof(_DAP_QUEUE_BUF) );
        g_nCwidth--;
        return pData;
    }
    else
    {
//		printf("SQueue is Empty!\n");
        return NULL;
    }
}

int fipc_SQBreath()
{
    return g_nWidth;
}

int fipc_SQIsFull()
{
    return (g_nFront == (g_nRear+1)%g_nQnum);
}

int fipc_SQIsEmpty()
{
    return (g_nFront == g_nRear);
}

int fipc_SQIsAllRead()
{
    return (g_nCfront == g_nCrear);
}

int fipc_SQGetElCnt()
{
    return fipc_SQBreath();
}