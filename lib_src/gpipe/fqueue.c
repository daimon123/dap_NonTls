// unix domain socket
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

#define QCNT	100

int		qsockfd[QCNT];
char 	q_name[QCNT][128];

int		clilen[QCNT];

struct 	sockaddr_un clientaddr[QCNT], serveraddr[QCNT], cli;


/***************************/
/****   Receive Part    ****/
/***************************/
int FQGetInit(int idx, char *qname)
{
	strcpy(q_name[idx], qname);

	qsockfd[idx] = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (qsockfd < 0) return -1;
	
	unlink(q_name[idx]);

	bzero(&serveraddr[idx], sizeof(serveraddr[idx])); 
	serveraddr[idx].sun_family = AF_UNIX; 
	strcpy(serveraddr[idx].sun_path, q_name[idx]); 

	if (bind(qsockfd[idx], (struct sockaddr *)&serveraddr[idx], sizeof(serveraddr[idx])) < 0) 
	{ 
		perror("bind error : "); 
		return -1;
	} 

	clilen[idx] = sizeof(clientaddr[idx]); 

	return 0;
}

int FQGetData(int idx, char *rbuff, int bufflen)
{
	int rxt;
	struct pollfd fds;

	fds.fd = qsockfd[idx];
	fds.events = POLLIN;

	rxt = poll(&fds, 1, 2000);
	if (rxt == 0) return 0;
	else if (rxt < 0) return rxt;

	rxt = recvfrom(qsockfd[idx], (void *)rbuff, bufflen, 0, (struct sockaddr *)&clientaddr[idx], (socklen_t *)&clilen[idx]); 

	
	return rxt;
}

/***************************/
/****   Receive Part    ****/
/***************************/



/***************************/
/****     Send Part     ****/
/***************************/
int FQPutInit(int idx, char *qname)
{
	strcpy(q_name[idx], qname);

	qsockfd[idx] = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (qsockfd[idx] < 0) return -1;
/* snd,rcv 버퍼사이즈 수정(사용안함)
    if( optval < 204800 ) {
        optval = 204800;
        setsockopt(qsockfd[idx], SOL_SOCKET, SO_SNDBUF, (char *)&optval, sizeof(optval));
        setsockopt(qsockfd[idx], SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof(optval));
    }
*/	
	bzero(&serveraddr[idx], sizeof(serveraddr[idx])); 
	serveraddr[idx].sun_family = AF_UNIX; 
	strcpy(serveraddr[idx].sun_path, q_name[idx]); 
	clilen[idx] = sizeof(clientaddr[idx]); 

	return 0;
}

void FQPutReSet(int idx, char *qname)
{
	strcpy(q_name[idx], qname);
	strcpy(serveraddr[idx].sun_path, q_name[idx]); 
}

int FQPutData(int idx, char *sbuff, int bufflen)
{
	int rxt;

	rxt = sendto(qsockfd[idx], (void *)sbuff, bufflen, 0, (struct sockaddr *)&serveraddr[idx], clilen[idx]);
	return rxt;
}

void FQClose(int idx)
{
	if (qsockfd[idx] > 0) close(qsockfd[idx]);
}



