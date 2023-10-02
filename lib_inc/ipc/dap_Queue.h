//
// Created by KimByoungGook on 2020-06-12.
//

#ifndef DAP_QUEUE_H
#define DAP_QUEUE_H

#include <sys/un.h>

#include "ipc/dap_qdef.h"

/* UDS (FQ) */
/* ---------------------------------------------------------  */
int		g_Qsockfd[QCNT];
char 	g_QName  [QCNT][128];

int		g_nClilen[QCNT];

struct sockaddr_un g_stCli;
struct sockaddr_un g_stClientAddr[QCNT];
struct sockaddr_un g_stServerAddr[QCNT];
/* ---------------------------------------------------------  */


/* Struct Queue (SQ) */
/* ---------------------------------------------------------  */
int		g_nQnum;
int 	g_nRear;
int 	g_nFront;
int 	g_nWidth;
int 	g_nCrear;
int 	g_nCfront;
int		g_nCwidth;
char 	g_szBackpath[256];

_DAP_QUEUE_BUF* data;
/* ---------------------------------------------------------  */


/* dap_fQueue.c */
int  fipc_FQGetInit (int param_Idx, char *param_Qname                   );
int  fipc_FQGetData (int param_Idx, char* param_buff, int param_nBufflen);
int  fipc_FQPutInit (int param_Idx, char* param_Qname                   );
void fipc_FQPutReSet(int param_Idx, char* param_Qname                   );
int  fipc_FQPutData (int param_Idx, void* param_sbuff, int nBufflen     );
void fipc_FQClose   (int param_nIdx                                     );



/* ------------------------------------------------------------------- */
/* dap_sQueue.c */
int  fipc_SQInit     (int qsiz, char *fpath         );
int fipc_SQInitNotBackup( int qsiz );
int  fipc_SQPut      (_DAP_QUEUE_BUF *pData         );
int  fipc_SQLoadQ    (                              );
int  fipc_SQBreath   (                              );
int  fipc_SQIsFull   (                              );
int  fipc_SQIsEmpty  (                              );
int  fipc_SQIsAllRead(                              );
int  fipc_SQRemove   (                              );
void fipc_SQClear    (                              );
int  fipc_SQGetElCnt (                              );
int  fipc_SQSaveQ    (                              );

_DAP_QUEUE_BUF* fipc_SQGet (_DAP_QUEUE_BUF* pData   );
_DAP_QUEUE_BUF* fipc_SQRead(_DAP_QUEUE_BUF* pData   );
/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */
/* dap_linker.c */
void fipc_KeepSession(int nParam_CfgSession                         );
void fipc_PrmonLinker(char *pname, int frc, long* param_LastSendTime);
/* ------------------------------------------------------------------- */

#endif //DAP_QUEUE_H
