#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <qdef.h>
#include <rqueue.h>

int mkey;
int msgid;


int RQInit(int key)
{
	mkey = key;
	msgid = msgget(key,IPC_CREAT|0666);
	if (msgid < 0) return -1;
}

int RQClear()
{
	if (msgctl(msgid, IPC_RMID, 0) == -1) return -1;
	return 0;
}

int RQPut(_QueueBuf *pBuf, int type)
{
	int rxt;
	int trycnt;
	struct msgbuf mbuf;

	trycnt = 0;

	mbuf.mtype = type;

	memcpy((void *)&mbuf+4, (void *)pBuf, sizeof(_QueueBuf));

resendQ:
	rxt = msgsnd(msgid, (void *)&mbuf, sizeof(struct msgbuf), IPC_NOWAIT);
	if (rxt < 0) 
	{
		switch(errno)
		{
		case EIDRM:    // message removed
			RQInit(mkey);		
			if (trycnt > 3) return -2;
			else 
			{
				trycnt++;
				goto resendQ;	
			}

			break;	

		case EINTR:    // message queue full!
			if (trycnt > 2) return -3;
			else 
			{
				trycnt++;
				sleep(1);	
				goto resendQ;
			}

			break;

		default:
			break;
		}

		return -1;
	}

	return 0;
}

int RQGet(_QueueBuf *pBuf, int type)
{
	int rxt;
	int trycnt;

	struct msgbuf mbuf;
	
	trycnt = 0;
	
rerecvQ:
	rxt = msgrcv(msgid, (void *)&mbuf, sizeof(mbuf), type, IPC_NOWAIT);
	if (rxt < 0)
	{
		switch(errno)
		{
		case EIDRM:    // message removed
			RQInit(mkey);		
			if (trycnt > 3) return -2;
			else 
			{
				trycnt++;
				goto rerecvQ;	
			}

			break;	

		default:
			break;
		}

		return -1;
	}

	memcpy((void *)pBuf, (void *)&mbuf+4, sizeof(_QueueBuf));	

	return 0;
}



