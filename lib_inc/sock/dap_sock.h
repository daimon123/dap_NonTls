//
// Created by KimByoungGook on 2020-06-22.
//

#ifndef DAP_SOCK_H
#define DAP_SOCK_H

#include "secure/dap_SSLHelper.h"

#define SA struct sockaddr




void fsock_InitOpenssl();
SSL_CTX* fsock_CreateContext();
void fsock_ConfigureContext(SSL_CTX *ctx, char* CertFile, char* KeyFile);
int fsock_OpensslLibShowCerts(SSL* ssl);

int fsock_MakeSocket(int nSocketType, u_short usPort, int *nListener, char* cpResult);
int fsock_Socket(int family,int type,int protocol);
int fsock_Bind(int socket,const SA *address,socklen_t addrlen);
int fsock_Listen(int socket,int backlog);
int fsock_Accept(int socket);
int fsock_Recv(int socket,void *buffer,int len,int flag);
int fsock_SendSSL(int socket,void *buffer,int len, int *epipeError);
int fsock_Send(int socket,void *buffer,int len);
int fsock_Connect(int socket,const SA *serverAddress,socklen_t addrlen);
int fsock_Select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset, int thr_flag);
int fsock_Close(int socket);
int fsock_SetNonblock(int sfd);
unsigned int fsock_Ip2Ui(char* param_Ip);
char* fsock_Ui2Ip(unsigned int ipAsUInt);
int fsock_CompareIpRange(char *p_userIp, char *p_arrTokenIp);
void fsock_InetNtop(int af,const void *addrptr,char *strptr,size_t length);
int fsock_atoPort(char *cpService, char *cpProto);
void fsock_InetPton(int af,const char *strptr,void *addrptr);
int fsock_CidrToIpAndMask(const char *cidr, uint32_t *ip, uint32_t *mask);
int fsock_GetPeerAddr(int fd, char *raddr);
int fsock_getMacAddress(char* param_Nic, char* param_Mac);
void fsock_convrtMac(const char* param_Data, char* param_CovrtStr, int param_len);
int fsock_GetNic(char* param_NicName);
int fsock_GetIpAddress(char* param_Nic, char* param_Ipaddr);
int fsock_GetMacAddress(char *cpip, char *device, char *res);
int fsock_CheckWireless(const char* ifname, char* protocol);
int fsock_GetClientMacAddress(char *ip, char *device, char *res);

void fsock_CloseOpenssl(SSL *p_ssl);
void fsock_CleanupOpenssl();
int fsock_SslSocketSend(SSL *ssl, char * stData, int nLength);
int fsock_SslSocketRecv(SSL *ssl, char *stData, int nLength);
//int fsock_PreAuth(int csock, char* cpip, char* mType, int* mCode, int* mLeng, int retry_cnt, int retry_sleep);
//int	fsock_SendAck(SSL *ssl, char *p_msgType, int p_msgCode, int p_retryCnt, int retrySleep);
int	fsock_SendAck(int sock, char *p_msgType, int p_msgCode, int p_retryCnt, int retrySleep);
int	fsock_SendAckJson(int sock,
                         char *p_msgType,
                         int p_msgCode,
                         char *p_jsonData,
                         int p_jsonSize,
                         char* p_logip,
                         int retryAckCount,
                         int retryAcksleep
);
int fsock_PreAuth(SSL *ssl, int csock, char* cpip, char* mType, int* mCode, int* mLeng, int retry_cnt, int retry_sleep);

#endif //DAP_SOCK_H
