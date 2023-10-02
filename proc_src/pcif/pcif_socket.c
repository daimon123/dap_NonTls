//
// Created by KimByoungGook on 2020-06-30.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>

#include "com/dap_com.h"
#include "sock/dap_sock.h"
#include "com/dap_req.h"
#include "json/jansson.h"
#include "json/jansson_config.h"
#include "secure/dap_SSLHelper.h"
#include "secure/dap_secure.h"
#include "ipc/dap_Queue.h"
#include "pcif.h"


int	fpcif_MngLoginRecv(
        int		sock,
        char*	cpip,
        char*	msgType,
        int		msgCode,
        int		msgLeng,
        int     fqnum,
        char*	p_realip,
        char*	p_loginid,
        char*   Buffer
)
{
    int				rxt = 0;
    char			*tmpJson = NULL;

    json_t			*jsonRoot;
    json_error_t	error;
    _DAP_MANAGER_INFO ManagerInfo;

    WRITE_DEBUG_IP(cpip, "Receive ip(%s)pid(%d): type(%s)code(%d)leng(%d) ",
              cpip, getpid(), msgType, msgCode, msgLeng );

    if(fcom_malloc((void**)&tmpJson, sizeof(char)*(msgLeng+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    memcpy(tmpJson, Buffer, msgLeng);

    WRITE_DEBUG_JSON(cpip, "Recv json(%s)", tmpJson);
    jsonRoot = json_loads(tmpJson, 0, &error);
    if(!jsonRoot)
    {
        fcom_MallocFree((void**)&tmpJson);
        WRITE_DEBUG_IP(cpip, "Fail in json parse line(%d)(%s)cpip(%s)",
                  error.line,error.text,cpip);
        rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL, g_stProcPcifInfo.cfgRetryAckCount, g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
           WRITE_CRITICAL_IP(cpip, "Fail in send socket, errno(%s)cpip(%s)",
                      strerror(errno),cpip);
        }
        return -1;
    }

    fcom_MallocFree((void**)&tmpJson);

    switch(msgCode)
    {
        case MANAGE_REQ_LOGIN: //50
        {
            WRITE_DEBUG_IP(cpip, "[REQ] MANAGE_REQ_LOGIN ");
            memset(&ManagerInfo, 0x00, sizeof(ManagerInfo));
            rxt = fpcif_ParseReqLogin(sock, jsonRoot, &ManagerInfo, cpip, msgCode, fqnum);
            if(rxt <= 0) //실패 or 유저없음
            {
                json_decref(jsonRoot);
                return -1;
            }
            break;
        }

        case MANAGE_FORCED_LOGIN: //52
        {
            WRITE_DEBUG_IP(cpip, "[REQ] MANAGE_FORCED_LOGIN ");
            //1. kill PID
            memset(&ManagerInfo, 0x00, sizeof(ManagerInfo));
            rxt = fpcif_ParseReqLogin(sock, jsonRoot, &ManagerInfo, cpip, msgCode, fqnum);
            if(rxt <= 0) //실패 or 유저없음
            {
                json_decref(jsonRoot);
                return -1;
            }
            break;
        }

        default:
            WRITE_DEBUG_IP(cpip, "[REQ] Unknown code(%d)", msgCode);
            return -1;
            break;
    } //switch

    json_decref(jsonRoot);

    // get realip
    strcpy(p_realip, ManagerInfo.manager_ip);
    // get id
    strcpy(p_loginid, ManagerInfo.manager_id);

    return 0;
}

int fpcif_SendJsonToManager(int sock, int code, char *jsonData, int jsonSize)
{
    int		rxt = 0;
    int		encrypto_size = 0;
    int		retryCnt = 0;
    char	*data = NULL;
    char	*EncBuffer = NULL;
    CRhead	SHead;

    memset(&SHead, 0x00, sizeof(SHead));
    strcpy(SHead.msgtype, DAP_MANAGE);
    SHead.msgcode  = htons(code);
    SHead.msgleng = htonl(jsonSize + 2);


    if(fcom_malloc((void**)&data, sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    if(fcom_malloc((void**)&EncBuffer, 4096 + sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    memcpy(data, &SHead, sizeof(CRhead));
    memcpy(&data[sizeof(CRhead)], (char *)jsonData, jsonSize);

    encrypto_size = fsec_Encrypt((char*)data, sizeof(CRhead)+jsonSize, ENC_AESKEY, ENC_IV, EncBuffer);
    rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
    if(rxt <= 0)
    {
        retryCnt = 0;
        while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
        {
            fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
            rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
                continue;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == g_stProcPcifInfo.cfgRetryAckCount - 1)
        {
            fcom_MallocFree((void**)&data);
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    }
    fcom_MallocFree((void**)&data);
    fcom_MallocFree((void**)&EncBuffer);
    return 0;
}

int fpcif_SendJsonToAgent(int sock, char *type, int code, char *jsonData, int jsonSize)
{
    int		rxt = 0;
    int		retryCnt = 0;
    int		encrypto_size = 0;
    char	*data = NULL;
    char	*EncBuffer = NULL;
    CRhead	SHead;

    memset(&SHead,   0x00, sizeof(SHead));
    strcpy(SHead.msgtype, type);
    if		( code == MANAGE_REQ_AGENT_LOGLIST )
        code = SERVER_CMD_LOG_LIST;
    else if	( code == MANAGE_REQ_AGENT_LOGFILE )
        code = SERVER_CMD_LOG_FILE;
    else if	( code == MANAGE_REQ_WIN_LOGFILE )
        code = SERVER_CMD_WIN_LOGFILE;

    SHead.msgcode  = htons(code);
    SHead.msgleng = htonl(jsonSize + 2); //include msgCode size(2)

    if(fcom_malloc((void**)&data, sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    memcpy(data, &SHead, sizeof(CRhead));
    memcpy(&data[sizeof(CRhead)], (char *)jsonData, jsonSize);

    if(fcom_malloc((void**)&EncBuffer, 4096+sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    encrypto_size = fsec_Encrypt((char*)data, sizeof(CRhead)+jsonSize, ENC_AESKEY, ENC_IV, EncBuffer);
    rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);

//    rxt = fsock_SslSocketSend(ssl, data, sizeof(CRhead)+jsonSize);
//    rxt = fsock_Send(sock, data, sizeof(CRhead)+jsonSize);
    if(rxt <= 0)
    {
        retryCnt = 0;
        while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
        {
            fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
//            rxt = fsock_SslSocketSend(sock, data, sizeof(CRhead) + jsonSize);
            rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == g_stProcPcifInfo.cfgRetryAckCount)
        {
            fcom_MallocFree((void**)&data);
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    }
    fcom_MallocFree((void**)&data);
    fcom_MallocFree((void**)&EncBuffer);


    return 0;
}

int fpcif_SendJsonToServer(int sock, int code, char *jsonData, int jsonSize)
{
    int     rxt = 0;
    int     encrypto_size = 0;
    int     retryCnt = 0;
    char    *data = NULL;
    char    *EncBuffer = NULL;
    CRhead  SHead;

    memset(&SHead,   0x00, sizeof(SHead));
    strcpy(SHead.msgtype, DAP_AGENT); // Agent에서 보내는것처럼

    SHead.msgcode  = htons(code);
    SHead.msgleng = htonl(jsonSize + 2); //include msgCode size(2)

    if(fcom_malloc((void**)&data, sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    memcpy(data, &SHead, sizeof(CRhead));
    memcpy(&data[sizeof(CRhead)], (char *)jsonData, jsonSize);

    if(fcom_malloc((void**)&EncBuffer, 4096+sizeof(CRhead)+jsonSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    encrypto_size = fsec_Encrypt((char*)data, sizeof(CRhead)+jsonSize, ENC_AESKEY, ENC_IV, EncBuffer);
    rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
//    rxt = fsock_Send(sock, data, sizeof(CRhead)+jsonSize);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,  "[REQ] Retry, because fail in send, errno(%d)code(%d)",
                errno,code);
        retryCnt = 0;
        while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
        {
            fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
//            rxt = fsock_Send(sock, data, sizeof(CRhead) + jsonSize);
            rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }

        if(retryCnt == g_stProcPcifInfo.cfgRetryAckCount)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,  "[REQ] Fail in send, retry(%d)errno(%d)code(%d)",
                    retryCnt,errno,code);

            fcom_MallocFree((void**)&data);
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    };


    fcom_MallocFree((void**)&data);
    fcom_MallocFree((void**)&EncBuffer);

    return 0;
}

int fpcif_SendNotiByFq(char *cpip, int fqn, char *logip)
{
    int		rxt;
    int		fqIdx;
    char	mnqPath[128];
    char	mnqCpPath[128];
    char	fileName[30];
    char	strFQNum[10];
    char	strFlag[10];
    char	strFQIp[15+1];

    struct  dirent  *pDirent;
    DIR     *pMngDir;

    _DAP_EventParam EventParam;

    memset(mnqPath, 0x00, sizeof(mnqPath));
    sprintf(mnqPath, "%s/MNGQ", g_stServerInfo.stDapQueueInfo.szDAPQueueHome);
    if(!(pMngDir = opendir(mnqPath)))
    {
       WRITE_CRITICAL_IP(logip, "Fail in manager queue dir");
        return -1;
    }
    else
    {
        memset(&EventParam, 0x00, sizeof(EventParam));
        while((pDirent = readdir(pMngDir)) != NULL)
        {
            if(strlen(pDirent->d_name) < 10)
                continue;
            if(fcom_TokenCnt(pDirent->d_name, "_") == 0)
                continue;

            memset(fileName, 0x00, sizeof(fileName));
            strcpy(fileName, pDirent->d_name);
            memset(strFQNum, 0x00, sizeof(strFQNum));
            fcom_GetFQNum(fileName, "_", strFQNum);

            if(!isdigit(strFQNum[0]))
                continue;
            else
                fqIdx = atoi(strFQNum);

            memset(mnqCpPath, 0x00, sizeof(mnqCpPath));
            sprintf(mnqCpPath, "%s/%s", mnqPath,fileName);

            WRITE_INFO_IP( logip, "- fqn(%d)fqIdx(%d)mnqCpPath(%s)", fqn, fqIdx, mnqCpPath);

            if( fqn >= START_MANAGER_FQ && fqIdx == fqn )
            {
                if (fipc_FQPutInit(fqIdx, mnqCpPath) < 0)
                {
                   WRITE_CRITICAL_IP(logip, "Fail in fqueue init, path(%s)",
                              mnqPath);
                    rxt = -1;
                }
                else
                {
                    EventParam.ev_type = MANAGE_FQ_KILL;
                    rxt = fipc_FQPutData(fqIdx, (char *)&EventParam, sizeof(EventParam));
                    fipc_FQClose(fqIdx);
                }
                if (rxt < 0)
                {
                    WRITE_CRITICAL_IP(logip, "Fail in send kill, fq(%d)",
                                        fqIdx);
                }
                else
                {
                    WRITE_DEBUG_IP(logip, "Success in send kill, fq(%d)",
                                        fqIdx);
                }
                break;
            }
            else // send noti to all manage
            {
                memset(strFQIp, 0x00, sizeof(strFQIp));

                fcom_StrTokenR(fileName, "_", 1, strFQIp);
                // 자신은 뺀다. 자신한테는 FQ send가 안감
                if( strlen(cpip) > 0 && !strcmp(cpip, strFQIp) )
                {
                    WRITE_DEBUG_IP(logip, "Except for self, cpip(%s)fqip(%s)fqidx(%d)",
                              cpip,strFQIp,fqIdx);
                    continue;
                }

                if (fipc_FQPutInit(fqIdx, mnqCpPath) < 0)
                {
                   WRITE_CRITICAL_IP(logip, "Fail in fqueue init, path(%s)",
                              mnqPath);
                    rxt = -1;
                }
                else
                {
                    memset(strFlag, 0x00, sizeof(strFlag));
                    if( fqn == 0 ) // shutdown
                    {
                        EventParam.ev_type = MANAGE_NOTI_SHUTDOWN;
                        strcpy(strFlag, "shutdown");
                    }
                    else  // restart
                    {
                        EventParam.ev_type = MANAGE_NOTI_RESTART;
                        strcpy(strFlag, "restart");
                    }
                    rxt = fipc_FQPutData(fqIdx, (char *)&EventParam, sizeof(EventParam));
                    fipc_FQClose(fqIdx);
                }
                if (rxt < 0)
                {
                    WRITE_CRITICAL_IP(logip, "FQ put failed in %s, fq(%d)",
                                    strFlag,fqIdx);
                }
                else
                {
                    WRITE_DEBUG_IP(logip, "FQ put succeed in %s, fq(%d)",
                                    strFlag,fqIdx);
                }
            }
        } // while
        closedir(pMngDir);
    }

    return 0;
}
int fpcif_SendToBinary(int sock, char *binData, int binSize)
{
    int		rxt;
    int		encrypto_size;
    int		retryCnt;
    char*   EncBuffer = NULL;


    if(fcom_malloc((void**)&EncBuffer, 4096) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    encrypto_size = fsec_Encrypt((char*)binData, binSize, ENC_AESKEY, ENC_IV, EncBuffer);
    rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);

//    rxt = fsock_Send(sock, binData, binSize);
    if(rxt <= 0)
    {

        retryCnt = 0;
        while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
        {
            fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
//            rxt = fsock_Send(sock, binData, binSize);
            rxt = fsock_Send(sock, (char *)EncBuffer, encrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
                continue;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == g_stProcPcifInfo.cfgRetryAckCount - 1)
        {
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    };

    fcom_MallocFree((void**)&EncBuffer);
    return 0;
}

int	fpcif_ProcReqAgentLog(
        int		sockMng,
        char*	agentIp,
        char*	mngIp,
        char*	p_msgType,
        int     p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize)
{

    int				rxt = 0;
    int				msgLeng = 0;
    int				msgCode = 0;
    int				agentPort = 0;
    int				jsonSize = 0;
    int				fileSize = 0;
    int				distWaitTime = 0;
    int             agentEmPort = 0;
    char			msgType[10 +1] = {0x00,};
    char			caFullPath[296 +1] = {0x00,};
    char			certFullPath[286 +1] = {0x00,};
    char			keyFullPath[286 +1] = {0x00,};
    char			fileName[50 +1] = {0x00,};
    char			*tmpJson = NULL;
    const char		*tmpFileName = NULL;
    int sockAgent = 0;

    struct	pollfd fds;
    CRhead	RHead;
    SSL_Connection *sslcon = NULL;

    rxt = fpcif_GetIdxByConfigSt("AGENT_LISTEN_PORT");
    if(rxt < 0)
    {
        WRITE_CRITICAL_IP(mngIp, "Not found config value");
        agentPort = 50204;
    }
    else
    {
        agentPort = atoi(pConfigInfo[rxt].cfvalue);
    }

    rxt = fpcif_GetIdxByConfigSt("AGENT_EM_LISTEN_PORT");
    if(rxt < 0)
    {
        WRITE_CRITICAL_IP(mngIp,"Not found config value" );
        agentEmPort = 50119;
    }
    else
    {
        agentEmPort = atoi(pConfigInfo[rxt].cfvalue);
    }

    memset(caFullPath, 0x00, sizeof(caFullPath));
    memset(certFullPath, 0x00, sizeof(certFullPath));
    memset(keyFullPath, 0x00, sizeof(keyFullPath));
    sprintf(caFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.caFile);
    sprintf(certFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.certFile);
    sprintf(keyFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.keyFile);

    WRITE_INFO_IP( mngIp, "- agentIp(%s)",  agentIp);
    WRITE_INFO_IP( mngIp, "- agentPort(%d)",  agentPort);
    WRITE_INFO_IP( mngIp, "- caFullPath(%s)",  caFullPath);
    WRITE_INFO_IP( mngIp, "- certFullPath(%s))", certFullPath);
    WRITE_INFO_IP( mngIp, "- keyFullPath(%s)", keyFullPath);

//    // 1. connect (server -> agent)
//    sslcon = ssl_non_connect(agentIp, agentPort, certFullPath, keyFullPath);
//    if(sslcon == NULL)
//    {
//       WRITE_CRITICAL_IP(mngIp, "Fail in ssl connect to agent(%s)", agentIp);
//       WRITE_CRITICAL_IP(mngIp, "Fail in ssl connect to agent(%s) Retry Connect Port(%d)",
//                  agentIp,
//                  agentEmPort);
//
//        /* AGENT_LISTEN_PORT로 연결 실패시 Em Port로 재처리. */
//        sslcon = ssl_non_connect(agentIp, agentEmPort, certFullPath, keyFullPath);
//        if(sslcon == NULL)
//        {
//            WRITE_CRITICAL_IP(mngIp,"Fail in ssl connect retry to agent(%s)", agentIp );
//            return -1;
//        }
//        else
//        {
//            WRITE_DEBUG_IP(mngIp,"EMG Agent Connect Success " );
//        }
//    }
//    else
//    {
//        WRITE_DEBUG_IP(mngIp,"Agent Connect Success " );
//    }


    struct sockaddr_in server;
    server.sin_family = PF_INET;
    server.sin_port = htons(agentPort);
    fsock_InetPton(PF_INET, agentIp, &(server.sin_addr));

    sockAgent = fsock_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if( fsock_Connect(sockAgent, ( struct sockaddr*)&server, sizeof(server) ) !=0 )
    {
        WRITE_CRITICAL_IP(mngIp, "Fail in ssl connect to agent(%s) Retry Connect Port(%d)",
                          agentIp,
                          agentEmPort);
        server.sin_family = PF_INET;
        server.sin_port = htons(agentEmPort);
        fsock_InetPton(PF_INET, agentIp, &(server.sin_addr));

        /* AGENT_LISTEN_PORT로 연결 실패시 Em Port로 재처리. */
        if (fsock_Connect(sockAgent,( struct sockaddr*)&server, sizeof(server) ) != 0) {
            WRITE_CRITICAL_IP(mngIp,"Fail in ssl connect retry to agent(%s)", agentIp );
            return -1;
        }
        else
        {
            WRITE_DEBUG_IP(mngIp,"EMG Agent Connect Success " );
        }
    }


    //show_certs_info(sslcon->ssl);

    // 2. send req json (server -> agent)
    rxt = fpcif_SendJsonToAgent(sockAgent, p_msgType, p_msgCode, p_jsonData, p_jsonSize);
    if( rxt < 0 )
    {
//        ssl_disconnect(sslcon);
        fsock_Close(sockAgent);
        return -1;
    }
    else
    {
        WRITE_DEBUG_IP(mngIp, "[REQ] Succeed in send json to agent(%s)code(%d)size(%d)",
                  agentIp,p_msgCode,p_jsonSize);
    }

    // 3. send ack success (server -> manager)
    rxt = fsock_SendAck(sockMng, DAP_MANAGE, MANAGE_RTN_SUCCESS,
                        g_stProcPcifInfo.cfgRetryAckCount,
                        g_stProcPcifInfo.retryAckSleep);
    if(rxt < 0)
    {
       WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to manager(%s)errno(%s)",
                  mngIp,strerror(errno));
    }

    // 4. recv result (agent -> server)
    fds.fd		= sslcon->sock;
    fds.events	= POLLIN;

    distWaitTime = fcom_GetProfileInt("PCIF","DISTRIBUTION_WAIT_TIME",10);
    rxt = poll(&fds, 1, (distWaitTime*1000));
    if(rxt == 0)
    {
       WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in receive poll from agent(%s)errno(%d)wait(%d)",
                  agentIp,errno,distWaitTime);
//        ssl_disconnect(sslcon);
        fsock_Close(sockAgent);
        return -1;
    }
    else if(rxt > 0)
    {
        memset(&RHead, 0x00, sizeof(RHead));
//        rxt = fsock_SslSocketRecv(sslcon->ssl, (char *)&RHead, sizeof(RHead));
        rxt = fsock_Recv(sockAgent, (char *)&RHead, sizeof(RHead),0);
        if(rxt <=  0)
        {
           WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in recv header from agent(%s)errno(%d)",
                      agentIp,errno);
//            ssl_disconnect(sslcon);
            fsock_Close(sockAgent);
            return -2;
        }
    }
    else
    {
       WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in poll, errno(%d)", errno);
//        ssl_disconnect(sslcon);
        fsock_Close(sockAgent);
        return -1;
    }

    memset(msgType, 0x00, sizeof(msgType));
    strcpy(msgType, RHead.msgtype);
    msgLeng = ntohl(RHead.msgleng);
    msgCode = ntohs(RHead.msgcode);

    if(	strncmp(msgType, DAP_AGENT, 10) != 0)
    {
        WRITE_CRITICAL_IP(mngIp, "[RCV] Unknown receive, type(%d)", msgType);
        return -1;
    }

    if(msgCode == DATACODE_RTN_SUCCESS || msgCode == DATACODE_RTN_FAIL)
    {
        WRITE_DEBUG_IP(mngIp, "[RCV] Receive return code(%d)", msgCode);
        ssl_disconnect(sslcon);
        return 0;
    }

    if(fcom_malloc((void**)&tmpJson, sizeof(char)*((msgLeng-2)+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    rxt = fsock_SslSocketRecv(sslcon->ssl, tmpJson, msgLeng-2);
    if(rxt <= 0)
    {
        fcom_MallocFree((void**)&tmpJson);
       WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in recv json from agent(%s)errno(%d)",
                  agentIp,errno);
        ssl_disconnect(sslcon);
        return -1;
    };

    if(rxt != msgLeng-2)
    {
        fcom_MallocFree((void**)&tmpJson);
        WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in rxt(%d)<>len(%d)", rxt,msgLeng);
        rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
           WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                      agentIp,strerror(errno));
        }
        return -1;
    }

    jsonSize = strlen(tmpJson);
    WRITE_DEBUG_JSON(mngIp, "Recv json(%s)", tmpJson);

    WRITE_DEBUG_IP(mngIp, "[RCV] Succeed in recv json from agent(%s)size(%d)",
              agentIp,jsonSize);

    // 5. send result json (server -> manager)
    if( msgCode == DATACODE_LOG_LIST ) 		msgCode = MANAGE_RSP_LOGLIST;
    else if( msgCode == DATACODE_LOG_FILE ) msgCode = MANAGE_RSP_LOGFILE;
    else if( msgCode == DATACODE_WIN_LOGFILE ) msgCode = MANAGE_RSP_LOGFILE;

    rxt = fpcif_SendJsonToManager(sockMng, msgCode, tmpJson, jsonSize);
    if( rxt < 0 )
    {
        // 5-1. send ack fail (server -> agent)
        rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
           WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                      agentIp,strerror(errno));
        }
    }
    else
    {
        WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                  mngIp,msgCode,jsonSize);

    }

    // 6. recv file binary (agent -> server)
    if( msgCode == MANAGE_RSP_LOGFILE )
    {
        json_error_t	error;
        json_t *jsonRoot = json_loads(tmpJson, 0, &error);
        if(!jsonRoot)
        {
            WRITE_DEBUG_IP(mngIp, "[ACK] Fail in json parse line(%d)(%s)agent(%s)",
                      error.line,error.text,agentIp);
            // 6-1. send ack fail (server -> agent)
            rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
               WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                          agentIp,strerror(errno));
            }
            fcom_MallocFree((void**)&tmpJson);
            ssl_disconnect(sslcon);
            return -1;
        }
        fcom_MallocFree((void**)&tmpJson);

        json_unpack(jsonRoot, "{s:s, s:i}", "file_name", &tmpFileName,"file_size", &fileSize);

        memset(fileName, 0x00, sizeof(fileName));
        if(tmpFileName != NULL)
        {
            strcpy(fileName, tmpFileName);
            tmpFileName = NULL;
        }
        json_decref(jsonRoot);

        WRITE_DEBUG_IP(mngIp, "Unpack file_name: '%s'", fileName);
        WRITE_DEBUG_IP(mngIp, "Unpack file_size: %d", fileSize);

        if( strlen(fileName) > 0 && fileSize > 0 )
        {
            char *tmpBin = NULL;
            if(fcom_malloc((void**)&tmpBin, sizeof(char)*fileSize) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            // 6-2. recv binary (agent -> server)
            rxt = fsock_SslSocketRecv(sslcon->ssl, tmpBin, fileSize);
            if(rxt <= 0)
            {
                fcom_MallocFree((void**)&tmpBin);
               WRITE_CRITICAL_IP(mngIp, "[RCV] Fail in recv binary from agent(%s)errno(%d)",
                          agentIp,errno);
                ssl_disconnect(sslcon);
                return -1;
            }
            WRITE_DEBUG_IP(mngIp, "[RCV] Succeed in recv binary from agent(%s)size(%d)",
                      agentIp,fileSize);

            // 6-3. send binary (server -> manager)
            rxt = fpcif_SendToBinary(sockMng, tmpBin, fileSize);
            if( rxt < 0 )
            {
                // 6-4. send ack fail (server -> agent);
                rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                        g_stProcPcifInfo.cfgRetryAckCount,
                        g_stProcPcifInfo.retryAckSleep);
                if(rxt < 0)
                {
                   WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                              agentIp,strerror(errno));
                }
            }
            else
            {
                WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send binary to manager(%s)code(%d)size(%d)",
                          mngIp,MANAGE_RSP_LOGLIST,fileSize);
            }
            fcom_MallocFree((void**)&tmpBin);

        }
        else if( strlen(fileName) > 0 && fileSize <= 0 )
        {
           WRITE_CRITICAL_IP(mngIp, "Invalied file, name(%s)size(%d)agent(%s)",
                            fileName,fileSize,agentIp);
            // 6-2. send ack fail (server -> agent);
            rxt = fsock_SendAck(sockMng, msgType, DATACODE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
               WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                          agentIp,strerror(errno));
            }
            ssl_disconnect(sslcon);
            return -1;
        }
        else
        {
           WRITE_CRITICAL_IP(mngIp, "Not found file, name(%s)size(%d)agent(%s)",
                            fileName,fileSize,agentIp);
        }
    } // if( msgCode == MANAGE_RSP_LOGFILE )
    else
    {
        fcom_MallocFree((void**)&tmpJson);
    }

    // 7. send ack success (server -> agent)
    rxt = fsock_SendAck(sockMng, msgType, DATACODE_RTN_SUCCESS,
                        g_stProcPcifInfo.cfgRetryAckCount,
                        g_stProcPcifInfo.retryAckSleep);
    if(rxt < 0)
    {
       WRITE_CRITICAL_IP(mngIp, "[ACK] Fail in send ack to agent(%s)errno(%s)",
                  agentIp,strerror(errno));
    }

    ssl_disconnect(sslcon);

    return 0;
}

int	fpcif_ProcReqServerLog(
        int 	msgCode,
        int		sockMng,
        char*	mngIp,
        char*	sDate,
        char*	eDate,
        char*	tarHome,
        int		findCnt,
        json_t* ProcessListroot
)
{
    int				rxt;

    int				jsonSize = 0;
    int				fileSize = 0;
    char			fileName[30];
    char			command[256];
    char			strMgwId[10];
    char			*str = NULL;

    struct	stat 	buf;
    json_t			*root = NULL;


    memset(&buf, 0x00, sizeof(struct stat));


    if(strstr(tarHome, ".TMPLOG") == NULL)
    {
       WRITE_CRITICAL_IP(mngIp, "Tar home error, (%s)", tarHome);
        return -1;
    }

    memset(strMgwId, 0x00, sizeof(strMgwId));
    sprintf(strMgwId, "%d", g_stServerInfo.stDapComnInfo.nCfgMgwId);

    if(msgCode == MANAGE_REQ_SERVER_LOGLIST)
    {
        str = json_dumps(ProcessListroot, JSON_INDENT(0));
    }

    if(msgCode == MANAGE_REQ_SERVER_LOGFILE)
    {
        if( findCnt > 0 )
        {
            // Compress
            chdir(tarHome);
            WRITE_DEBUG_IP(mngIp, "Compression Dir (%s)", tarHome);
            memset	(fileName, 	0x00, sizeof(fileName));
            memset	(command, 	0x00, sizeof(command));

            /* 2020.07.31 tar.gz -> zip으로 압축파일 형식 변경. */
            /* 리눅스 서버내에 zip 커맨드 존재하여야 함.          */

            sprintf     (fileName, "%s-%s_log_%s.zip", sDate,eDate,strMgwId);
            sprintf 	(command, "zip %s *", fileName);
            system	(command);

            if(fpcif_CheckCommandZip(mngIp) != 0)
            {
                sprintf	(fileName, "%s_%s-%s_log.tar.gz", strMgwId,sDate,eDate);
                sprintf	(command, "tar cvfz %s *", fileName);
                system	(command);

            }
            else
            {
                sprintf(fileName, "%s-%s_log_%s.zip", sDate,eDate,strMgwId);
                sprintf(command, "zip %s *", fileName);
                system(command);
            }

            WRITE_DEBUG_IP(mngIp, "Compression zip(%s)", command);
            if( stat(fileName, &buf) < 0 )
            {
               WRITE_CRITICAL_IP(mngIp, "Stat error, file(%s)",
                          fileName);
                return -1;
            }
            fileSize = buf.st_size;
            chdir("..");

            root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",fileName,"file_size",fileSize);
        }
        else
        {
            memset(fileName, 0x00, sizeof(fileName));
            strcpy(fileName, "");
            root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",fileName,"file_size",fileSize);
        }
        str = json_dumps(root, JSON_INDENT(0));
    }

    if( str == NULL )
    {
        json_decref(root);
       WRITE_CRITICAL_IP(mngIp, "Json pack error");
        return -1;
    }


    WRITE_DEBUG_JSON(mngIp, "Dump json(%s)", str);
    jsonSize = strlen(str);

    if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
    {
        json_decref(ProcessListroot);
        // Send result json (server -> manager)
        rxt = fpcif_SendJsonToManager(sockMng, MANAGE_RSP_LOGLIST, str, jsonSize);
        if( rxt < 0 )
        {
           WRITE_CRITICAL_IP(mngIp, "[RSP] Fail in send json to manager(%s)errno(%s)",
                      mngIp,strerror(errno));
            fcom_MallocFree((void**)&str);
            return -1;
        }
        WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                  mngIp,MANAGE_RSP_LOGLIST,jsonSize);
    }
    else // MANAGE_REQ_SERVER_LOGFILE
    {
        json_decref(root);
        // Send result json (server -> manager)
        rxt = fpcif_SendJsonToManager(sockMng, MANAGE_RSP_LOGFILE, str, jsonSize);
        if( rxt < 0 )
        {
           WRITE_CRITICAL_IP(mngIp, "[RSP] Fail in send json to manager(%s)errno(%s)",
                      mngIp,strerror(errno));
            fcom_MallocFree((void**)&str);
            return -1;
        }

        WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                  mngIp,MANAGE_RSP_LOGFILE,jsonSize);

        if( strlen(fileName) > 0 && fileSize > 0 )
        {
            // Send file binary (server -> manager)
            int		binFile_len = 0;
            char	gzipFilePath[256 +1] = {0x00,};

            char* tmpBin = NULL;
            if(fcom_malloc((void**)&tmpBin,sizeof(char)*fileSize) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            memset	(gzipFilePath, 0x00, sizeof(gzipFilePath));
            sprintf	(gzipFilePath, "%s%s", tarHome,fileName);

            WRITE_INFO_IP( mngIp, "- gzipFilePath(%s)", gzipFilePath);
            binFile_len = fpcif_GetFileToBin(gzipFilePath, &tmpBin);
            if( binFile_len != fileSize )
            {
               WRITE_CRITICAL_IP(mngIp, "[RSP] Invalid file size(%d<>%d) manager(%s)errno(%s)",
                          binFile_len,fileSize,mngIp,strerror(errno));

                fcom_MallocFree((void**)&str);
                fcom_MallocFree((void**)&tmpBin);
                return -1;
            }

            rxt = fpcif_SendToBinary(sockMng, tmpBin, binFile_len);
            if( rxt < 0 )
            {
                WRITE_DEBUG_IP(mngIp, "[RSP] Fail in send binary to manager(%s)code(%d)size(%d)",
                          mngIp,MANAGE_RSP_LOGLIST,fileSize);
            }
            else
            {
                WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send binary to manager(%s)code(%d)size(%d)",
                          mngIp,MANAGE_RSP_LOGLIST,fileSize);
            }

            fcom_MallocFree((void**)&tmpBin);
        }
    }

    fcom_MallocFree((void**)&str);

    return 0;
}

int	fpcif_ProcForwardServerLog(
        int 	msgCode,
        int		sock,
        char*	cpip,
        char*	sDate,
        char*	eDate,
        char*	tarHome,
        int		findCnt,
        json_t* ProcessListroot
)
{
    int				rxt = 0;
    int				jsonSize = 0;
    int				fileSize = 0;
    char			fileName[30 +1] = {0x00,};
    char			command[256 +1] = {0x00,};
    char			strMgwId[10 +1] = {0x00,};
    char			*str = NULL;
    char			*tmpBin = NULL;
    struct	stat 	buf;
    json_t			*root = NULL;


    memset(&buf, 0x00, sizeof(struct stat));


    if(strstr(tarHome, ".TMPLOG") == NULL)
    {
        WRITE_CRITICAL_IP(cpip,"Tar home error, (%s)",tarHome );
        return -1;
    }

    memset(strMgwId, 0x00, sizeof(strMgwId));
    sprintf(strMgwId, "%d", g_stServerInfo.stDapComnInfo.nCfgMgwId);


    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
    {
        // Make json
        str = json_dumps(ProcessListroot, JSON_INDENT(0));
    }
    else // MANAGE_FORWARD_SERVER_LOGFILE
    {
        if( findCnt > 0 )
        {
            // Compress
            chdir(tarHome);
            memset	(fileName, 	0x00, sizeof(fileName));
            memset	(command, 	0x00, sizeof(command));

            if(fpcif_CheckCommandZip(cpip) != 0)
            {
                sprintf	(fileName, "%s_%s-%s_log.tar.gz", strMgwId,sDate,eDate);
                sprintf	(command, "tar cvfz %s *", fileName);
                system	(command);
            }
            else
            {
                sprintf(fileName, "%s-%s_log_%s.zip", sDate,eDate,strMgwId);
                sprintf(command, "zip %s *", fileName);
                system(command);
            }
            WRITE_DEBUG_IP(cpip, "Compression zip(%s)", command);
            if( stat(fileName, &buf) < 0 )
            {
                WRITE_CRITICAL_IP(cpip,"Stat error, file(%s)",fileName );
                return -1;
            }

            fileSize = buf.st_size;
            chdir("..");

            root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",fileName,"file_size",fileSize);
        }
        else
        {
            memset(fileName, 0x00, sizeof(fileName));
            strcpy(fileName, "");
            root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",fileName,"file_size",fileSize);
        }
        str = json_dumps(root, JSON_INDENT(0));
    }

    if( str == NULL )
    {
        json_decref(root);
        WRITE_CRITICAL_IP(cpip,"Json pack error" );
        return -1;
    }

    WRITE_INFO_JSON(cpip,"Dump json(%s) ",str );
    jsonSize = strlen(str);

    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
    {
        json_decref(ProcessListroot);
    }
    else
    {
        json_decref(root);
    }

    // Send result json (server -> 1st server)
    rxt = fpcif_SendJsonToServer(sock, msgCode, str, jsonSize);
    if( rxt < 0 )
    {
        fcom_MallocFree((void**)&str);
        WRITE_CRITICAL_IP(cpip,"[RSP] Fail in send json to server(%s)errno(%s)",cpip,strerror(errno));
        return -1;
    }
    fcom_MallocFree((void**)&str);

    WRITE_INFO_IP(cpip,"[RSP] Succeed in send json to server(%s)code(%d)size(%d)",cpip,msgCode,jsonSize);

    if( strlen(fileName) > 0 && fileSize > 0 )
    {
        // Send file binary (server -> 1st server)
        int		binFile_len = 0;
        char	gzipFilePath[256 +1] = {0x00,};

        if(fcom_malloc((void**)&tmpBin, sizeof(char)*fileSize) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        memset	(gzipFilePath, 0x00, sizeof(gzipFilePath));
        sprintf	(gzipFilePath, "%s%s", tarHome,fileName);
        WRITE_DEBUG_IP(cpip,"- gzipFilePath(%s)",gzipFilePath );

        binFile_len = fpcif_GetFileToBin(gzipFilePath, &tmpBin);
        if( binFile_len != fileSize )
        {
            WRITE_CRITICAL_IP(cpip,"[RSP] Invalid file size(%d<>%d) server(%s)errno(%s)",binFile_len,fileSize,cpip,strerror(errno));
            fcom_MallocFree((void**)&tmpBin);
            return -1;
        }

        rxt = fpcif_SendToBinary(sock, tmpBin, fileSize);
        if( rxt < 0 )
        {
            WRITE_CRITICAL_IP(cpip,"[RSP] Fail in send binary to server(%s)code(%d)size(%d)",cpip,msgCode,fileSize);
        }
        else
        {
            WRITE_DEBUG_IP(cpip,"[RSP] Succeed in send binary to server(%s)code(%d)size(%d)",cpip,msgCode,fileSize);
        }
        fcom_MallocFree((void**)&tmpBin);
    }

    return 0;
}


/** FW 프로세스에 DB처리를 위한 UDP 데이터 전송 함수 **/
int fpcif_SendToFw(int   param_PackType,
                   char* param_UserKey,
                   char* param_Buffer,
                   int   param_BufferSize)
{
    int     local_nRet      = 0;

    if( strlen(param_UserKey) < 20)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid User Key (%s)", param_UserKey);
        return -1;
    }

    local_nRet = fpcif_UdpSendToFw( param_PackType, param_UserKey, (char*)param_Buffer, param_BufferSize );
    if ( local_nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_IPC,"fpcif_UdpSendToFw Failed ");
        return (-1);
    }

    return 0;

}

int fpcif_UdpSendToFw(int param_PackType, char* param_UserKey, char* param_Buffer, int param_BufferSize)
{
    int nRet = 0;

    switch(param_PackType)
    {
        /** 정책 **/
        case DATACODE_REQUEST_TEST:
        case EXTERNAL_REQUEST_CFG:
        case DATACODE_REQUEST_CFG:
        {
            nRet = fipc_FQPutData(FW_POLICY, (char *)param_Buffer, param_BufferSize);
            if( nRet <= 0)
            {
                WRITE_CRITICAL(CATEGORY_IPC,"FW_DT_E_QUEUE Put Data Failed ret(%d) errno(%d) msg(%s)",nRet, errno, strerror(errno));
                return (-1);
            }
            break;
        }
        /** 서비스 Status 정책 **/
        case DATACODE_SERVICE_STATUS:
        {
            nRet = fipc_FQPutData(FW_SERVICE, (char *)param_Buffer, param_BufferSize);
            if( nRet <= 0)
            {
                WRITE_CRITICAL(CATEGORY_IPC,"FW_DT_E_QUEUE Put Data Failed ret(%d) errno(%d) msg(%s)",nRet, errno, strerror(errno));
                return (-1);
            }

            break;
        }

        /** 검출 데이터 **/
/*        case DATACODE_DETECT_DATA:
        case EXTERNAL_DETECT_DATA:
        case DATACODE_ADDRESS_LIST:
        {
            snprintf(local_szUserKey, sizeof(local_szUserKey), "%s", &param_UserKey[18]);
            local_nIdx = atoi( local_szUserKey );

            local_nModValue = local_nIdx % g_stProcPcifInfo.cfgQCount;
            nRet = fipc_FQPutData(local_nStartQueueIdx + local_nModValue, (char *)param_Buffer, param_BufferSize);
            if( nRet < 0)
            {
                WRITE_CRITICAL(CATEGORY_IPC,"FW_DT_E_QUEUE Put Data Failed ");
                return (-1);
            }

            break;
        }*/
    }

    return 0;

}
