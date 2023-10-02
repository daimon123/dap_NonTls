//
// Created by KimByoungGook on 2021-11-25.
//

#ifndef _SYSLOG_MON_H
#define _SYSLOG_MON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/timeb.h>
#include <iconv.h>
#include <syslog.h>

#include "com/dap_com.h"
#include "com/dap_def.h"
#include "sock/dap_sock.h"

#define DF_CONFIG_FILE_NAME     "syslog.cfg"
#define DF_BUFFER_SIZE          2048

#define WRITE_LOG(MSG,...)         {                                              \
char buffer[1024 +1] = {0x00,};                                                   \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fsyslog_LogWrite( buffer, ##__VA_ARGS__); \
}

enum PROD_TYPE
{
    PRODUCT_DAP = 0,
    PRODUCT_FI,
    PRODUCT_NI,
};

enum PROTOCOL_TYPE
{
    PROTOCOL_UDP = 0,
    PROTOCOL_TCP,
    PROTOCOL_SYSLOG,
};





char    g_szProcessName[32];    // 프로세스명
char    g_szProcessHome[128];   // 프로세스 home 경로
char    g_szUnixSockPath[128];  // Unix Socket Path
char    g_szLogPath[256];       // Log Path
char    g_szCfgCharset[32];

char    g_szIface[32];
char    g_szMyIp[32];


char    g_cProdType;            // 제품 타입
char    g_cProtocolType;        // Protocol 타입
iconv_t g_iconv_t;

typedef struct
{
    // 서버 접속정보
    char szServerIp[16];
    char szServerPort[5];
    char szProtocol[16];
    char szTokenStr[8];

}_SYSLOG_CFG_INFO;
_SYSLOG_CFG_INFO g_stCfgInfo;

typedef struct
{
    char szFacility[2+1]; //Facility
    char szSeverityLevel[1+1]; //Severity Level
    char szClientIp[15 +1]; //Client IP
    char szProdType[3 +1]; //제품 구분
    char szEventLevel[8 +1]; //이벤트 레벨
    char szEventType[3 +1]; //이벤트 타입
    char szEvent[10 +1]; //이벤트
}_SYSLOG_COMM_INFO;

typedef struct
{
    int  nReadSize;
    int  nProtocolType;
    _SYSLOG_COMM_INFO stCommInfo;
}_THREAD_ARG;



/** UDS(Unix Domain Socket) 정보 **/
int g_UdsSockFd;
struct sockaddr_un g_stUdsSockAddr;


void fsyslog_PrintOut(void);
char* fsyslog_GetEventStr(char* param_Event);
void fsyslog_EventMsgOut(int param_EventType, char* param_EventMsg, int param_EventMsgSize, _THREAD_ARG* param_ThreadArg);
int fsyslog_LogInit(char* param_LogPath);
int fsyslog_GetLogTime(char *timebuf);
int fsyslog_GetSendTime(char *timebuf);
void fsyslog_LogWrite(const char *fmt, ...);
int fsyslog_UdpGetData(_THREAD_ARG* param_ThreadArg);
int fsyslog_ArgParse(int argc, char **argv);
pid_t fsyslog_SetDeamon(void);
void fsyslog_Init(void);
void fsyslog_HandleSignal(int sid);
int fsyslog_SignalInit(void);
int fsyslog_UnixSockInit(void);
void* fsyslog_SendThread(void* param_ThreadArg);
int fsyslog_GetNic(char* param_NicName);
void fsyslog_GetIp(char* param_InterfaceName, char* param_Ip);
int fsock_ConnectNonBlock(int param_nSocket, struct sockaddr_in* param_stptrServerAddress, socklen_t param_nAddrlen, int param_nTimesec);
#endif //_SYSLOG_MON_H
