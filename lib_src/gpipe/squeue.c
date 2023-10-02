/*************************************************
Linear Queue  
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <qdef.h>
#include <squeue.h>

_QueueBuf *data = NULL;

static int SQMake()
{
	if (data != NULL) free(data);
	data = (_QueueBuf *)calloc(qnum, sizeof(_QueueBuf));		
	if (data == NULL) return -1;
	else return 0;
}

static void SQCrReset()
{
    crear  = rear; 
	cfront = front;
    cwidth = width;
}

int SQInit(int qsiz, char *fpath)
{
	int rxt; 

    rear 	= 0; 
	front	= 0;
    width	= 0;

    crear	= 0; 
	cfront	= 0;
    cwidth	= 0;

	strcpy(backpath, fpath);
	qnum = qsiz+1;
	
	if (data != NULL)	
	{
		free(data);
		data = NULL;
	}

	rxt = SQMake();

	if (rxt < 0) return -1;
	else return 0;
}

int SQPut(_QueueBuf *pData)
{
	if (data == NULL) return -2;

    if (!SQIsFull()) {
	    memcpy((void *)&data[rear], (void *)pData, sizeof(_QueueBuf));
	    rear = ++rear % qnum;
	    width++;
		return 0;
    }
    else 
	{
		printf("SQ is Full!\n");
		return -1;	
	}
}

_QueueBuf *SQGet(_QueueBuf *pData)
{
	int tmp = front;

	if (data == NULL) return NULL;
	
	if (!SQIsEmpty())
	{
        front = ++front % qnum; 
		memcpy( (void *)pData, (void *)&data[tmp], sizeof(_QueueBuf) );
	    width--;
		return pData;
    }
    else 
	{ 
//		printf("SQueue is Empty!\n");
		return NULL;
    }
}

_QueueBuf *SQRead (_QueueBuf *pData)
{
    int tmp = cfront;

	if (data == NULL) return NULL;

    if (!SQIsAllRead()) 
	{
        cfront = ++cfront % qnum; 

		memcpy( (void *)pData, (void *)&data[tmp], sizeof(_QueueBuf) );
		
	    cwidth--;
		return pData;
    }
    else 
	{ 
//		printf("SQueue is Empty!\n");
		return NULL;
    }
}

_QueueBuf *SQReadR(_QueueBuf *pData)
{
	if (data == NULL) return NULL;
	if (SQIsEmpty()) return NULL;

	memcpy((void *)pData, (void *)&data[abs(rear-1)], sizeof(_QueueBuf));
	return pData;

}

_QueueBuf *SQReadF(_QueueBuf *pData)
{
	if (data == NULL) return NULL;
	if (SQIsEmpty()) return NULL;

	memcpy((void *)pData, (void *)&data[front], sizeof(_QueueBuf));
	return pData;
}

int SQLoadQ()
{
	FILE *pFile;
	_QueueBuf tbuf;
	
	size_t rslt;
	size_t bsize;

	if (strlen(backpath) < 1) return -1;

	pFile = fopen(backpath, "rb");
	if (pFile == NULL) return -2;
	
	bsize = sizeof(tbuf);

	while(1)
	{
		rslt = fread((void *)&tbuf, 1, bsize,pFile);
		//printf("Read Bytes -[%d]\n", rslt);
		if (ferror(pFile))
		{
			fclose(pFile);
			return -3;
		}

		if (rslt != bsize) break;
		
		SQPut(&tbuf);
	}

	fclose(pFile);

	return 0;
}

int SQSaveQ()
{
	FILE *pFile;
	_QueueBuf tbuf;
	size_t bsize;

	SQCrReset();

	if (strlen(backpath) < 1) return -1;
	if (data == NULL) return -2;

	pFile = fopen(backpath, "wb");
	//pFile = fopen(backpath, "ab"); //SQLoad 후 SQRemove하므로 불필요
	if (pFile == NULL) return -3;

	bsize = sizeof(tbuf);
	
	for (;!SQIsAllRead();)
	{
		SQRead(&tbuf);
		fwrite((const void *)&tbuf, 1, bsize, pFile);
	}

	fclose(pFile);
	
	return 0;
}

int SQRemove()
{
	char cmdBuff[256];

	if (strlen(backpath) < 1) return -1;
/*
	if(remove(backpath) == -1) {
		printf("Can't delete -[%s]\n", backpath);
		return -1;
	} 
*/
	memset(cmdBuff, 0x00, sizeof(cmdBuff));
	sprintf(cmdBuff, "cat /dev/null > %s", backpath);
	if(system(cmdBuff) < 0) {
		return -1;
	}

	return 0;
}

void SQClear()
{
	int rxt; 

    rear 	= 0; 
	front	= 0;
    width	= 0;

    crear	= 0; 
	cfront	= 0;
    cwidth	= 0;

	memset(backpath, 0x00, sizeof(backpath));
	
	if (data != NULL)	
	{
		free(data);
		data = NULL;
	}
}

int SQBreath()
{
	return width;
}

int SQIsFull()
{
    return (front == (rear+1)%qnum);	
}

int SQIsEmpty()
{
    return (front == rear);
}

int SQIsAllRead()
{
	return (cfront == crear);
}

void SQShowElement()
{
	_QueueBuf tbuf;
  
	SQCrReset();

    for (;!SQIsAllRead();) 
	{
		printf("%s\n", (SQRead(&tbuf))->buf);
		LogRet("%s\n", (SQRead(&tbuf))->buf);
	}	
}

int SQGetElCnt()
{
	return SQBreath();
}

void SQResetQSiz(int qsiz)
{
	if (data != NULL) free(data);
	SQInit(qsiz, backpath);
}
/*
int SQDel()
{
	FILE *pFile;

	SQCrReset();

	if (strlen(backpath) < 1) return -1;

	pFile = fopen(backpath, "wb");
	if (pFile == NULL) return -3;

	fclose(pFile);
	
	return 0;
}
*/

