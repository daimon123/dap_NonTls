//
// Created by KimByoungGook on 2020-07-02.
//

#ifndef PROXY_H
#define PROXY_H

#define MAX_THREAD 255
//#define BUFFER_LEN 8192


#define BUFFER_LEN 10240
//#define BUFFER_LEN 51200


#define STRING_LEN 50
#define CONTINUE 0
#define BACKLOG_QUEUE 10000
#define MSG_DB_TYPE "BROKER_DB"
#define MSG_CP_TYPE "BROKER_CP"
#define MAX_RETRY_CNT	3
#define	USE_OPENSSL

struct sockaddr_in server;
struct sigaction saSignInt,saSignPipe;
int     thrFlag[MAX_THREAD];



/**CONSTANTS DEFINE**/
#define FAILURE 1
#define SUCCESS 0

pthread_mutex_t mutexsum;
pthread_mutex_t g_pthread_Mutex;
pthread_t pthreadrecv[MAX_THREAD];

/* 프로세스 종료할 경우에 자원해제중에 Fork 방지 플래그.*/
int       g_signalFlag;
typedef struct
{
    int 	cfgThreadCnt;
    int 	cfgForkCnt;
    int 	cfgDbPort;
    int     cfgPrmonInterval;
    int     ing_job_time;

    time_t  nConfigLastModify;
    long    last_send_time;
    char    certPath[256];
    char    caFile[30];
    char    certFile[30];
    char    keyFile[30];
    char 	cfgListenPort[10];
    char 	cfgDbIp[15+1];

    SSL_CTX *ctx;

}_DAP_PROC_PROXY_INFO;

typedef struct
{
    SSL *sslClient;
    int  thrNum;
    int  clientSock;
    int  ServerSock;
    char ClientIp[15+1];
}_DAP_SOCK_PROXY_INFO;

pthread_t pthreadrecv[MAX_THREAD];

/* ------------------------------------------------------------------- */
///* Thread Exit Flag */
//char*    g_threadExit;

/* Child Process Exit Flag */
int      g_ForkExit;
/* Child Process Pid 저장 */
int*     g_ChildPid;


/* ------------------------------------------------------------------- */

_DAP_PROC_PROXY_INFO    g_stProcProxyInfo;
int fproxy_MainTask();
int fproxy_ProxyInit();
void fproxy_SigchldHandler(int signo);
void fproxy_SigHandler(int s);
//static void fstReloadCfgFile( );
void fproxy_ReloadCfgFile( );
int fstSendLink();
void* fstThreadRecv(void *thrid);
int fstPreAuthSSL(SSL *ssl);
int fstPreAuth(int sock);
int fproxy_ForkWork();
void* fproxy_ThreadWorkd(void* Param_pstProcSockInfo);
void* fproxy_ThreadWorkd_NonEnc(void* Param_pstProcSockInfo);
void* fproxy_ThreadWorkd_SSL(void* Param_pstProcSockInfo);

int     g_ParentPid;
int		g_listenSock;
int   	g_numChildren;

#endif //PROXY_H
