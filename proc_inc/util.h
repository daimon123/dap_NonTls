//
// Created by KimByoungGook on 2020-10-22.
//


/* ------------------------------------------------------------------- */
/* dap_connect_util */
int fUtil_ParseCmdOption(int argc, char **argv);
void fUtil_DisplayVersion(void);
int fUtil_Init(void);
int fUtil_TcpConnectionTest(char *addr, int port, int SslFlag, int* RefSocket);
int fUtil_SslConnectionTest(char *addr, int port);
int fUtil_HwBaseTcpCnnectionTest(void);
int fUtil_HwBaseInactiveTcpCnnectionTest(void);
int fUtil_HwBaseSslCnnectionTest(void);
int fUtil_HwBaseSslInActiveConectionTest(void);
void 	func_DUMP_BIN_DATA_VIEW( char *pre_data_buff , int pre_data_buff_size );

/* Agent Client IP */
char g_szClientIp[32 +1];
char szCertPath[256 +1];
char szCaFile  [30  +1];
char szCertFile[30  +1];
char szKeyFile [30  +1];
char g_szErrMsg[64 +1];


int  g_nClientPort;
int  g_nTimeOutUsec;
int  g_nTimeOutsec;


typedef	struct _CRhead
{
    char	msgtype[10];
    short	msgcode;
    int  	msgleng;
}CRhead;


char	*data;
CRhead	SHead;
/* ------------------------------------------------------------------- */


/* ------------------------------------------------------------------- */
/* dap_stress_util */
int g_nForkCnt;
int g_nThreadCnt;
int g_nCurrentThreadCnt;
int* g_ptrForkPidArr;
char g_ThreadStopFlag;
char g_SendFlag;
char g_NIC[32+1];
char g_MAC[64+1];
char g_MyIp[32+1];
char g_ConnSvrIp[32+1];
int  g_ConnSvrPort;
pthread_mutex_t ThreadCntMutex;
/* ------------------------------------------------------------------- */


#define		DAP_AGENT		"INTENT-DAP"
