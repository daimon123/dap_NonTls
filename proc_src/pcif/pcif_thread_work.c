//
// Created by KimByoungGook on 2020-10-13.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#include <string.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>


#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "secure/dap_secure.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "json/dap_json.h"

#include "pcif.h"
#include "json/dap_defstr.h"




int fpcif_ReloadConfig(void)
{
    struct stat     statBuf;

    if(g_pstNotiMaster[CONFIG_CHANGE].reload)
    {
        memset(pConfigInfo, 0x00, sizeof(dbConfig));
        if((g_stProcPcifInfo.nTotConfig = fpcif_LoadConfig(pConfigInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load_config ");
            return  -4;
        }
        stat(g_pstNotiMaster[CONFIG_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[CONFIG_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[CONFIG_CHANGE].reload        =   FALSE;

    }

    if(g_pstNotiMaster[RULE_CHANGE].reload)
    {
        memset(pRuleInfo, 0x00, sizeof(dbRule));
        if((g_stProcPcifInfo.nTotRule = fpcif_LoadRule(pRuleInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in load_rule config ");
            return  -4;
        }

        stat(g_pstNotiMaster[RULE_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[RULE_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[RULE_CHANGE].reload        =   FALSE;
    }

    if(g_pstNotiMaster[RULE_SCHEDULE_CHANGE].reload)
    {
        memset(pSchdInfo, 0x00, sizeof(dbSchd));
        if((g_stProcPcifInfo.nTotSchd = fpcif_LoadScheduleSt(pSchdInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in load_schedule_st config ");
            return  -4;
        }

        stat(g_pstNotiMaster[RULE_SCHEDULE_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[RULE_SCHEDULE_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[RULE_SCHEDULE_CHANGE].reload        =   FALSE;
    }

    if(g_pstNotiMaster[RULE_DETECT_CHANGE].reload)
    {
        if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
        {
            memset(pDetectInfo, 0x00, sizeof(detectInfo));
            if((g_stProcPcifInfo.nTotDetect = fpcif_LoadDetectSt(pDetectInfo)) < 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in load_detect_st config ");
                return  -4;
            }
        }
        stat(g_pstNotiMaster[RULE_DETECT_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[RULE_DETECT_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[RULE_DETECT_CHANGE].reload        =   FALSE;
    }

    if(g_pstNotiMaster[AGENT_UPGRADE_CHANGE].reload)
    {
        memset(pUgdInfo, 0x00, sizeof(dbUgd));
        /* 0x00 : ???? * 0x01 : ?б??? * 0x02 : ??????? */
        UpgradestFlag = 0x02;
        if((g_stProcPcifInfo.nTotUpgrade = fpcif_LoadUpgradeSt(pUgdInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load_upgrade_st config ");
            UpgradestFlag = 0x00;
            return  -4;
        }

        UpgradestFlag = 0x01; /* ????? */

        stat(g_pstNotiMaster[AGENT_UPGRADE_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[AGENT_UPGRADE_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[AGENT_UPGRADE_CHANGE].reload        =   FALSE;
    }

    if(g_pstNotiMaster[GW_CHANGE].reload)
    {
        memset(pGwInfo, 0x00, sizeof(dbGw));
        if((g_stProcPcifInfo.nTotGw = fpcif_LoadGwSt(pGwInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load_gw_st config ");
            return  -4;
        }
        stat(g_pstNotiMaster[GW_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[GW_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[GW_CHANGE].reload        =   FALSE;
    }

    if(g_pstNotiMaster[CP_CHANGE].reload)
    {
        memset(pManagerInfo, 0x00, sizeof(dbManager));
        if((g_stProcPcifInfo.nTotManager = fpcif_LoadManager(pManagerInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load manager ");
            return  -4;
        }

        memset(pUserInfo, 0x00, sizeof(dbUser));
        if((g_stProcPcifInfo.nTotUser = fpcif_LoadUser(pUserInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load user ");
            return  -4;
        }

        memset(pGroupLink, 0x00, sizeof(dbGroupLink));
        if((g_stProcPcifInfo.nTotGroupLink = fpcif_LoadGroupLink(pGroupLink)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load group link ");
            return  -4;
        }

        stat(g_pstNotiMaster[CP_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[CP_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[CP_CHANGE].reload        =   FALSE;
    }

    return TRUE;
}
void fpcif_CleanupSocket(int socketFd) {
    char buffer[1024] = {0x00,};
    ssize_t bytesRead;

    do {
        bytesRead = recv(socketFd, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            break;
        }
        // 데이터를 처리하거나 무시
    } while (bytesRead > 0);
}
void* fpcif_PcifThread(void *p_conn)
{
    int rxt = 0;
    int sslRet = 0;
    int nManagerLoop = 0;
    int MsgCode = 0;
    int leng = 0;
    int pidNum;
    int sslFd = 0;
    int ReturnStep = 0;

    char type[10 + 1];
    char strFQIp[15 + 1];
    char strFQNum[10 + 1];
    char mnqPath[128 + 1];
    char fileName[128 + 1];
    char local_err_msg[128 +1] = {0x00,};
    char delMnqFilePath[256 + 1];
    char szlocalClientIP[16 + 1];
    char szEncHeaderbuffer[32] = {0x00,};
    char* szEncBuffer = NULL;
    char* szDecryptBuffer = NULL;
    DIR *pMngDir = NULL;
    struct dirent *pDirent = NULL;
    struct proxy_conn *conn = NULL;

    int thrNum = 0;
    int local_socket = 0;
    int nByteAlreadyRead = 0;

    SSL_CTX *local_ctx = NULL;
    SSL *local_socket_ssl = NULL;


    /***************************************
    # Local Init.
    ****************************************/
    conn = (struct proxy_conn *) p_conn;

    memset(szlocalClientIP, 0x00, sizeof(szlocalClientIP));
    memset(type,            0x00, sizeof(type));
    memset(strFQIp,         0x00, sizeof(strFQIp));
    memset(strFQNum,        0x00, sizeof(strFQNum));
    memset(mnqPath,         0x00, sizeof(mnqPath));
    memset(fileName,        0x00, sizeof(fileName));
    memset(delMnqFilePath,  0x00, sizeof(delMnqFilePath));

    thrNum = conn->cli_thread;
    local_socket = g_stThread_arg[thrNum].cli_sock;
    snprintf(szlocalClientIP, sizeof(szlocalClientIP), "%s", g_stThread_arg[thrNum].cli_ip);
    pidNum = getpid(); //PID

    g_stThread_arg[thrNum].threadStepStatus = 1;

    CRhead	RHead;
    memset((void *)&RHead, 0x00, sizeof(RHead));
    char szHeader[32] = {0x00,};
    int decrypto_size = 0;

    WRITE_INFO(CATEGORY_INFO, "Thread[%d:%d] ip(%s)sock(%d) ", pidNum, thrNum, szlocalClientIP, local_socket);

    memset(type, 0x00, sizeof(type));
    g_stThread_arg[thrNum].threadStepStatus = 5;

    fcom_SleepWait(5);

    nByteAlreadyRead = 0;
    while(sizeof(szEncHeaderbuffer) != nByteAlreadyRead) {
        if ( nByteAlreadyRead > sizeof(szEncHeaderbuffer) )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", nByteAlreadyRead);
            break;
        }
        rxt = fsock_Recv(local_socket, (char *)&szEncHeaderbuffer[nByteAlreadyRead], sizeof(szEncHeaderbuffer) - nByteAlreadyRead,0);
        if ( rxt <= 0){
            break;
        }
        nByteAlreadyRead += rxt;
    }
    if ( rxt <= 0 || rxt > sizeof(szEncHeaderbuffer) ){
        WRITE_CRITICAL(CATEGORY_DEBUG,"recv Failed rxt %d %d",rxt,errno);
        szEncBuffer = NULL;
        szDecryptBuffer = NULL;
        goto __FINISH__;
    }
    ret = fsec_Decrypt(szEncHeaderbuffer, rxt, ENC_AESKEY, ENC_IV, (unsigned char*)szHeader, &decrypto_size);
    if (ret != 1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Decrypt Failed %s %d", szlocalClientIP, local_socket);
        szEncBuffer = NULL;
        szDecryptBuffer = NULL;
        goto __FINISH__;
    }
    memcpy(&RHead, szHeader, sizeof(RHead));

    memcpy(type, RHead.msgtype, sizeof(RHead.msgtype));
    if ( strncmp(type, DAP_MANAGE, 10) != 0 && strncmp(type, DAP_AGENT, 10) != 0) {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Header Msg type %s", type);
        szEncBuffer = NULL;
        szDecryptBuffer = NULL;
        goto __FINISH__;
    }

    MsgCode = ntohs(RHead.msgcode);
    if( MsgCode <= 0) {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Header Msg Code Value <= 0 %d", MsgCode );
        szEncBuffer = NULL;
        szDecryptBuffer = NULL;
        goto __FINISH__;
    }
    leng = ntohl(RHead.msgleng);
    if( leng <= 0) {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Header Msg Length <= 0  %d", leng);
        szEncBuffer = NULL;
        szDecryptBuffer = NULL;
        goto __FINISH__;
    }
    WRITE_INFO(CATEGORY_DEBUG,"fsock_PreAuth MsgCode : [%d] leng : [%d] type : [%s] ",  MsgCode, leng, type);

    szEncBuffer = NULL;
    if(fcom_malloc((void**)&szEncBuffer, leng) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc szEncBuffer Failed " );
        goto __FINISH__;
    }
    nByteAlreadyRead = 0;
    while(leng != nByteAlreadyRead) {
        if ( nByteAlreadyRead > leng )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", nByteAlreadyRead);
            break;
        }
        rxt = fsock_Recv(local_socket, (char *)&szEncBuffer[nByteAlreadyRead], leng - nByteAlreadyRead,0);
        if ( rxt <= 0){
            break;
        }
        nByteAlreadyRead += rxt;
    }
    if ( rxt <= 0){
        WRITE_CRITICAL(CATEGORY_DEBUG,"recv Failed rxt %d %d",rxt,errno);
        goto __FINISH__;
    }

    szDecryptBuffer = NULL;
    if(fcom_malloc((void**)&szDecryptBuffer, leng*2) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc szDecryptBuffer Failed " );
        goto __FINISH__;
    }

    decrypto_size = 0;
    if (!fsec_Decrypt(szEncBuffer, leng, ENC_AESKEY, ENC_IV, (unsigned char*)szDecryptBuffer, &decrypto_size))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Decrypt Failed ");
        goto __FINISH__;
    }

    g_stThread_arg[thrNum].threadStepStatus = 7;

    // Get manager fq_num
    if ( !strncmp(type, DAP_MANAGE, 10) )
    {
        memset(mnqPath, 0x00, sizeof(mnqPath));
        snprintf(mnqPath, sizeof(mnqPath),
                "%s/MNGQ/", g_stServerInfo.stDapQueueInfo.szDAPQueueHome );
        if (access(mnqPath, W_OK) != 0)
        {
            rxt = fcom_MkPath(mnqPath, 0755);
            if (rxt < 0)
            {
                WRITE_CRITICAL_IP(szlocalClientIP, "Fail in make path(%s) ", mnqPath);
            }
            else
            {
                chmod(mnqPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH);
                WRITE_INFO_IP(szlocalClientIP, "Succeed in make path(%s) ", mnqPath);
            }
        }
        if (!(pMngDir = opendir(mnqPath)))
        {
            WRITE_CRITICAL_IP(szlocalClientIP, "Fail in manager queue dir ");
        }
        else
        {
            // init mngFQNum
            for (nManagerLoop = 0; nManagerLoop < MAX_MANAGER_COUNT; nManagerLoop++)
            {
                mngFQNum[nManagerLoop] = -1;
            }
            // put mngFQNum
            while ((pDirent = readdir(pMngDir)) != NULL)
            {
                if (strlen(pDirent->d_name) < 10)
                    continue;
                if (fcom_TokenCnt(pDirent->d_name, "_") == 0)
                    continue;

                // delete already fq file for ip
                memset(fileName, 0x00, sizeof(fileName));
                memset(strFQIp, 0x00, sizeof(strFQIp));
                snprintf(fileName, sizeof(fileName), "%s", pDirent->d_name);
                fcom_StrTokenR(fileName, "_", 1, strFQIp);
                if (!strncmp(strFQIp, szlocalClientIP, strlen(strFQIp)))
                {
                    memset(delMnqFilePath, 0x00, sizeof(delMnqFilePath));
                    snprintf(delMnqFilePath, sizeof(delMnqFilePath), "%s%s", mnqPath, pDirent->d_name);
                    if (remove(delMnqFilePath))
                    {
                        WRITE_CRITICAL_IP(szlocalClientIP, "Fail in delete init queue file(%s)error(%s) ",
                                          delMnqFilePath, strerror(errno));
                    }
                    else // success
                    {
                        WRITE_INFO_IP(szlocalClientIP, "Succeed in delete init queue file(%s) ",
                                      delMnqFilePath);
                        continue;
                    }
                }

                // get fq num
                memset(fileName, 0x00, sizeof(fileName));
                snprintf(fileName, sizeof(fileName), "%s", pDirent->d_name);
                memset(strFQNum, 0x00, sizeof(strFQNum));

                fcom_StrTokenR(fileName, "_", 0, strFQNum);

                if (!isdigit(strFQNum[0]))
                {
                    continue;
                }
                else
                {
                    fqIdx = atoi(strFQNum);
                    mngFQNum[fqIdx] = fqIdx;
                }
            }
            if ( pMngDir != NULL)
            {
                closedir(pMngDir);
            }
        }

        //Get mngFQNum 0~5,11,12,13까지 사용중이므로 20부터시작
        for (nManagerLoop = START_MANAGER_FQ; nManagerLoop < MAX_MANAGER_COUNT; nManagerLoop++)
        {
            if (mngFQNum[nManagerLoop] == -1)
            {
                fqIdx = nManagerLoop;
                break;
            }
        }

        WRITE_DEBUG_IP(szlocalClientIP, "Get manager fq_num(%d) ", fqIdx);
        g_stThread_arg[thrNum].threadStepStatus = 99;
        fpcif_ManagerWork(thrNum,  local_socket, szlocalClientIP, type, MsgCode, decrypto_size, fqIdx, szDecryptBuffer);

    }
    else
    {
        g_stThread_arg[thrNum].threadStepStatus = 8;
        fpcif_AgentRecv(thrNum,  local_socket, szlocalClientIP, type, MsgCode, decrypto_size, szDecryptBuffer);
    }

__FINISH__:
    g_stThread_arg[thrNum].threadStepStatus = 41;

    if( local_socket_ssl != NULL)
    {
        sslFd = SSL_get_fd(local_socket_ssl);
    }
    g_stThread_arg[thrNum].threadStepStatus = 44;
    if( local_socket_ssl != NULL)
    {
        int err = 0;
        err = SSL_get_error(local_socket_ssl, sslRet);
        if ( err == SSL_ERROR_ZERO_RETURN && ReturnStep > 1) {
            SSL_shutdown(local_socket_ssl);
        }
        SSL_free(local_socket_ssl);
        local_socket_ssl = NULL;
    }

    fsock_CleanupOpenssl();

    if( local_socket > 0)
    {
        fpcif_CleanupSocket(local_socket);
        shutdown(local_socket, SHUT_RDWR);
        close(local_socket);
        local_socket = 0;
    }



    g_stThread_arg[thrNum].threadStepStatus = 47;

    pthread_mutex_lock( &g_pthread_Mutex );
    thrFlag[thrNum] = 'N';
    sleep(0);
    pthread_mutex_unlock( &g_pthread_Mutex );

    WRITE_INFO(CATEGORY_DEBUG,"Cleint (%s)Thread(%d) thread_cleanup Is End..! ",
               szlocalClientIP,
               thrNum);

    fcom_MallocFree((void**)&szEncBuffer);
    fcom_MallocFree((void**)&szDecryptBuffer);
    pthread_exit(NULL);

}
void fpcif_OpensslLibLoadCertificates(SSL_CTX* ctx)
{
    if (SSL_CTX_use_certificate_file(ctx, g_stProcPcifInfo.certFullPath, SSL_FILETYPE_PEM) <= 0)
    {
        perror("SSL_CTX_use_certificate_file :");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cert SSL_CTX_use_certificate_file Fail (%d) (%s) ",
                       errno,
                       strerror(errno));
        ERR_print_errors_fp(stderr);
        return;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, g_stProcPcifInfo.keyFullPath, SSL_FILETYPE_PEM) <= 0)
    {
        perror("SSL_CTX_use_PrivateKey_file :");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cert SSL_CTX_use_PrivateKey_file Fail (%d) (%s) ",
                       errno,
                       strerror(errno));
        ERR_print_errors_fp(stderr);
        return;
    }


    if (!SSL_CTX_check_private_key(ctx))
    {
        perror("SSL_CTX_use_certificate_file :");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cert SSL_CTX_use_certificate_file Fail (%d) (%s) ",
                       errno,
                       strerror(errno));
        return;
    }
}

SSL_CTX* fpcif_CertInit(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx = NULL;

    // SSL 버전3 프로토콜 사용
//    method = SSLv23_server_method();
    method = TLSv1_2_server_method();

    // SSL 컨텍스트 생성
    ctx = SSL_CTX_new(method);
    if (ctx == NULL)
    {
        perror("Unable to create SSL context");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cert SSL CTX New Fail (%d) (%s) ",
                       errno,
                       strerror(errno));

        ERR_print_errors_fp(stderr);
        return NULL;
    }

    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

    if( SSL_CTX_set_ecdh_auto(ctx, 1) != 1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"SSL_CTX_set_ecdh_auto Failed ");
    }

    return ctx;
}

int	fpcif_AgentRecv(
        int     thrNum,
        int		sock,
        char*	cpip,
        char*	msgType,
        int		msgCode,
        int		msgLeng,
        char*   Buffer
)
{
    int		rxt = 0;
    int		jsonSize = 0;
    int		buffLen = 0;
    int		idxB = -1;
    int     idxDupl  = -1;
    int		idxU = -1;
    int		us_db_router = 0;
    int		prefix = 0;
    int		managerFq = 0;
    int		findCnt = 0;
    int		serverId = 0;
    int     local_loop_count = 0;
    int     decrypto_size = 0;
    char	realip[15 +1] = {0x00,};
    char    realMac[17 +1] = {0x00,};
    char	managerIp[15 +1] = {0x00,};
    char	managerId[30 +1] = {0x00,};
    char	filePath[30 +1] = {0x00,};
    char	beginDate[10 +1] = {0x00,};
    char	endDate[10 +1] = {0x00,};
    char	category[128 +1] = {0x00,};
    char	log_home[128 +1] = {0x00,};
    char	tar_home[256 +1] = {0x00,};
    char	strCommand[64 +1] = {0x00,};
    char	compDate1[8 +1] = {0x00,};
    char	compDate2[8 +1] = {0x00,};
    char	fqPath[128 +1] = {0x00,};
    char    *EncBuffer = NULL; //aes 256 복호화 버퍼
    char    *DecyptBuffer = NULL; //aes 256 복호화 버퍼
    char	*tmpJson = NULL;
    char	*resData = NULL;
    char	*resData2 = NULL;
    char	*findIp = NULL;
    char	*findStr = NULL;
    char    *TempPtr = NULL;

    json_t			        *jsonRoot;
    json_t			        *jsonRootLogList; // LogList
    json_error_t	        error;
    DETECT_ITEM		        DetectItem;

    _DAP_AGENTLOG_INFO      AgentLogInfo;
    _DAP_AGENT_INFO 		AgentInfo;
    _DAP_AGENT_INFO 		CopyAgentInfo;
    _DAP_DETECT 			Detect;
    _DAP_DETECT_DATA 		DetectData;
    _DAP_QUEUE_BUF 		    QBuf;

    struct _STLOGTAIL TInfo;

    cpRule			cpRuleInfo;
    cpCycle			cpCycleInfo;
    cpFlag          cpFlagInfo;

    memset( &cpRuleInfo, 0x00, sizeof(cpRuleInfo) );
    memset( &cpCycleInfo, 0x00, sizeof(cpCycleInfo) );
    memset( &cpFlagInfo,  0x00, sizeof(cpFlagInfo) );

    for (;;)
    {
        if(g_ForkHangFlag != 0x00 || g_ChildExitFlag != 0x00)
        {
            break;
        }

        WRITE_INFO_IP(cpip,	"Receive ip(%s)pid(%d): type(%s)code(%d)leng(%d)",
                      cpip,getpid(),msgType,msgCode,msgLeng);

        if(fcom_malloc((void**)&tmpJson, sizeof(char)*((msgLeng)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        memcpy(tmpJson, Buffer, msgLeng);

        g_stThread_arg[thrNum].threadStepStatus = 10;

        WRITE_INFO_JSON(cpip, "Recv json(%s)", tmpJson);

        g_stThread_arg[thrNum].threadStepStatus = 13;
        jsonRoot = json_loads(tmpJson, 0, &error);
        if(!jsonRoot)
        {
            fcom_MallocFree((void**)&tmpJson);
            WRITE_INFO_IP(cpip,	"Fail in json parse line(%d)(%s)cpip(%s)",
                          error.line, error.text, cpip);
            if(	strncmp(msgType, DAP_AGENT, 10) == 0)
            {
                rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_FAIL, g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
            }
            else if(strncmp(msgType, DAP_MANAGE, 10) != 0)
            {
                rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL, g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
            }
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                  strerror(errno), cpip);
                WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in send socket, errno(%s)cpip(%s)",
                               strerror(errno),cpip);
                break;
            }
            break;
        }
        fcom_MallocFree((void**)&tmpJson);

//        print_json(jsonRoot);

        g_stThread_arg[thrNum].threadStepStatus = 14;
        g_stThread_arg[thrNum].threadMsgCode = msgCode;
        switch(msgCode)
        {
            case DATACODE_REQUEST_TEST: // 0
            {
                WRITE_INFO_IP(cpip, "[REQ] DATACODE_REQUEST_TEST");
                break;
            }
            case DATACODE_AGENTLOG_DATA: //6
            {
                WRITE_INFO_IP(cpip, "[REQ] DATACODE_AGENTLOG_DATA");
                memset(&AgentLogInfo, 0x00, sizeof(AgentLogInfo));
                fpcif_ParseRequestAgentLog(cpip, jsonRoot, &AgentLogInfo);

                /** DBLOG send **/
                if ( fcom_AgentDbLogWrite(  AgentLogInfo.hb_sq,
                                            AgentLogInfo.user_key,
                                            AgentLogInfo.user_ip,
                                            AgentLogInfo.process,
                                            AgentLogInfo.log_date,
                                            AgentLogInfo.log_level,
                                            AgentLogInfo.log_msg) < 0)
                {
                    /** Send ACk Fail **/
                    WRITE_CRITICAL_IP(cpip,	"Fail in fcom_AgentDbLogWrite, cpip(%s)code(%d)",
                                      cpip,msgCode);
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fcom_AgentDbLogWrite, errno(%s)cpip(%s)",
                                   strerror(errno),cpip);

                    rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s) cpip(%s)",
                                          strerror(errno),cpip);
                        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in send socket, errno(%s) cpip(%s)",
                                       strerror(errno),
                                       cpip);
                        return (-1);
                    }
                    break;
                }

                /** Send ACk Success **/
                rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_SUCCESS,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,"Fail in send socket, errno(%s)cpip(%s) ", strerror(errno), cpip);
                }
                break;
            }
            case EXTERNAL_REQUEST_CFG: // 4
            case DATACODE_REQUEST_CFG: // 1
            {
                if(msgCode == DATACODE_REQUEST_CFG)
                {
                    WRITE_INFO_IP(cpip, "[REQ] DATACODE_REQUEST_CFG");

                    memset(&AgentInfo, 0x00, sizeof(AgentInfo));
                    g_stThread_arg[thrNum].threadStepStatus = 15;
                    rxt = fpcif_ParseRequestCfg(sock, cpip, jsonRoot, &AgentInfo, thrNum);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in parse request cfg, cpip(%s)code(%d)",
                                          cpip,msgCode);
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in parse request cfg, cpip(%s)code(%d)",
                                       strerror(errno),cpip);
                        rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_FAIL,
                                            g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                              strerror(errno),cpip);
                            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in send socket, errno(%s) cpip(%s)",
                                           strerror(errno),
                                           cpip);
                            return (-1);
                        }
                    }
                }
                else if(msgCode == EXTERNAL_REQUEST_CFG)
                {
                    WRITE_INFO_IP(cpip, "[REQ] EXTERNAL_REQUEST_CFG");
                    memset(&AgentInfo, 0x00, sizeof(AgentInfo));
                    rxt = fpcif_ParseExternalRequestCfg(sock, cpip, jsonRoot, &AgentInfo);
                    if( rxt < 0 )
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in parse request cfg, cpip(%s)code(%d)",
                                          cpip,msgCode);
                        rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_FAIL,g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,"Fail in send socket, errno(%s)cpip(%s) ",
                                              strerror(errno),cpip);
                            return (-1);
                        }
                    }
                }

                memset(&CopyAgentInfo, 0x00, sizeof(_DAP_AGENT_INFO));
                memcpy(&CopyAgentInfo, &AgentInfo,sizeof(_DAP_AGENT_INFO));

                g_stThread_arg[thrNum].threadStepStatus = 21;

                if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
                {
                    while(1)
                    {
                        if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                        {
                            idxDupl = fpcif_GetIdxByBaseKey(AgentInfo.user_key, 0);
                            pthread_rwlock_unlock(&mutexpolicy);
                            break;
                        }
                        else
                        {
                            fcom_SleepWait(5);
                            local_loop_count++;
                        }

                        if( local_loop_count >= 10 )
                        {
                            WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                            break;
                        }
                    }
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",4 );
                    json_decref(jsonRoot);
                    return (-1);
                }

                if(idxDupl != -1)
                {
                    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
                    {
                        while(1)
                        {
                            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                            {
                                fpcif_ParseDuplicateCheck(&AgentInfo, idxDupl);

                                pthread_rwlock_unlock(&mutexpolicy);
                                break;
                            }
                            else
                            {
                                fcom_SleepWait(5);
                                local_loop_count++;
                            }

                            if( local_loop_count >= 10 )
                            {
                                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                                break;
                            }
                        }
                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",2 );
                        json_decref(jsonRoot);
                        return (-1);
                    }
                }

                g_stThread_arg[thrNum].threadStepStatus = 22;

                // hw_base_tb 구조체 직접 핸들링
                // dbif에서는 touch 안함
                while(1)
                {
                    if(LoadConfigFlag == 0x01) //Load Complete State
                    {
                        if(!fpcif_SetAgentByBbaseKey(&CopyAgentInfo))
                        {
                            WRITE_CRITICAL_IP(cpip,"Not found base st, key(%s) ",AgentInfo.user_key );
                            break;
                        }
                        else break;  /* Key 찾아도 break */
                    }
                    else
                    {
                        local_loop_count++;
                        if(local_loop_count > 10) //2초
                        {
                            if(!fpcif_SetAgentByBbaseKey(&CopyAgentInfo))
                            {
                                WRITE_CRITICAL_IP(cpip,"Not found base st, Not Yet Load key(%s) ",
                                                  AgentInfo.user_key );
                                break;
                            }
                            else break; /* Key 찾아도 break */
                        }
                    }
                    fcom_SleepWait(5);
                }
                g_stThread_arg[thrNum].threadStepStatus = 23;

                //for test
                //print_all_agent_info(&AgentInfo);
                //strcpy(AgentInfo.user_ip, cpip);20181128부터 Agent에서보내주는ip사용

                //HB_ACCESS_TIME 때문에 무조건 던지자
                memset(&QBuf, 0x00, sizeof(QBuf));
                QBuf.packtype = msgCode;
                memcpy((void *)&QBuf.buf, (void *)&AgentInfo, sizeof(AgentInfo));

                g_stThread_arg[thrNum].threadStepStatus = 24;

                if(fpcif_ChkLoadFlag() == 0)
                {
                    idxU = fpcif_SetLogLevel(AgentInfo.user_ip);
                    if(idxU > -1)
                        us_db_router = (pUserInfo[idxU].us_db_router-'0')%48;
                }
                else
                {
                    WRITE_WARNING_IP(cpip,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True ");
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True ");
                    json_decref(jsonRoot);
                    return (-1);
                }

                g_stThread_arg[thrNum].threadStepStatus = 25;
                if(us_db_router > 0 && us_db_router <= g_stProcPcifInfo.cfgQCount)
                {
                    /** 이전 DBIF UDP 전송로직 **/
#if 0
                    rxt = fipc_FQPutData(us_db_router, (char *)&QBuf, sizeof(QBuf));
                    prefix = us_db_router;
#endif
                    rxt = fpcif_SendToFw( QBuf.packtype, AgentInfo.user_key, (char *)&QBuf,
                                          sizeof(AgentInfo) + sizeof(QBuf.packtype) );
                    if ( rxt < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"FW Send To Failed ");
                    }
                }
                else
                {
#if 0
                    modVal = getpid() % g_stProcPcifInfo.cfgQCount;

                    rxt = fipc_FQPutData(modVal+1, (char *)&QBuf, sizeof(QBuf));
                    prefix = modVal+1;
#endif
                    rxt = fpcif_SendToFw( QBuf.packtype, AgentInfo.user_key, (char *)&QBuf,
                                          sizeof(AgentInfo) + sizeof(QBuf.packtype) );
                    if ( rxt < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"FW Send To Failed ");
                    }
                }

                g_stThread_arg[thrNum].threadStepStatus = 26;
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"[FQPUT POLICY] (%d) Fail in send, ip(%s)code(%d)",
                                      prefix,cpip,msgCode);
                }
                else
                {
                    WRITE_INFO_IP(cpip, "[FQPUT POLICY] (%d) Succeed in send, ip(%s)code(%d)",
                                  prefix,cpip,msgCode);
                }
                break;
            }
            case DATACODE_SERVICE_STATUS: //5
            {
                int  local_nOffset  = 0;
                int  local_nSendLen = 0;
                char local_szStatus[1 +1] = {0x00,};
                char local_szHbSq[20 +1]  = {0x00,};

                WRITE_INFO_IP(cpip, "[REQ] DATACODE_SERVICE_STATUS ");
                memset( &AgentInfo, 0x00, sizeof(AgentInfo) );
                rxt = fpcif_ParseServiceStatus(sock,
                                               cpip,
                                               jsonRoot,
                                               &AgentInfo,
                                               local_szStatus,
                                               thrNum);

                rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_SUCCESS, g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,"Fail in send socket, errno(%s)cpip(%s) ",
                                      strerror(errno),cpip);
                    WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in send socket, errno(%s) cpip(%s)",
                                   strerror(errno),
                                   cpip);
                }
                break;
            }

            case SERVER_CMD_DATA_REFRESH: // 10
            {
                WRITE_INFO_IP(cpip, "[REQ] SERVER_CMD_DATA_REFRESH");
                break;
            }
            case SERVER_CMD_ADDRESS_LIST: // 11
            case SERVER_CMD_MATCH_MAC: // 12
            {
                if(msgCode == SERVER_CMD_ADDRESS_LIST)
                {
                    WRITE_INFO_IP(cpip, "[REQ] SERVER_CMD_ADDRESS_LIST");
                }
                else if(msgCode == SERVER_CMD_MATCH_MAC)
                {
                    WRITE_INFO_IP(cpip, "[REQ] SERVER_CMD_MATCH_MAC");
                }
                rxt = fpcif_ParseJsonCmd(cpip, sock, msgCode);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse json cmd, cpip(%s)code(%d)",
                                      cpip,msgCode);
                    return (-1);
                }
                break;
            }
            case AGENT_URGENT_PATCH:
            {
                g_stThread_arg[thrNum].threadStepStatus = 27;
                rxt = fpcif_ParseUrgentReq(sock, cpip, jsonRoot, &AgentInfo);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse Urgent Patch  cmd, cpip(%s)code(%d)", cpip,msgCode);
                }
                g_stThread_arg[thrNum].threadStepStatus = 28;
                break;
            }
            case DATACODE_DETECT_DATA: // 2
            case EXTERNAL_DETECT_DATA: // 3
            case DATACODE_ADDRESS_LIST: // 21
            {
                if(msgCode == DATACODE_DETECT_DATA)
                {
                    WRITE_INFO_IP(cpip, "[REQ] DATACODE_DETECT_DATA");
                }
                else if(msgCode == EXTERNAL_DETECT_DATA)
                {
                    WRITE_INFO_IP(cpip, "[REQ] EXTERNAL_DETECT_DATA");
                }
                else if(msgCode == DATACODE_ADDRESS_LIST)
                {
                    WRITE_INFO_IP(cpip, "[REQ] DATALOADER_ADDRESS_LIST");
                }

                memset(&Detect,     0x00, sizeof(Detect));
                memset(&DetectData, 0x00, sizeof(DetectData));
                g_stThread_arg[thrNum].threadStepStatus = 29;
                fjson_ParseJsonFt(jsonRoot, &Detect, &DetectData, cpip);
                g_stThread_arg[thrNum].threadStepStatus = 30;

                if ( strstr(Detect.change_item, STR_OPERATING_SYSTEM) != NULL ||
                     strstr(Detect.change_item, STR_WIN_DRV) != NULL ||
                     strstr(Detect.change_item, STR_NET_SCAN) != NULL) {
                    WRITE_DEBUG(CATEGORY_DEBUG,"[%s] Data Except [%s] ", cpip, Detect.change_item);

                    WRITE_INFO_IP(cpip,	"[FQPUT] (%d) Succeed in send, ip(%s)code(%d)item(%s)",
                                  prefix, cpip, msgCode, Detect.change_item);
                    fsock_SendAck(sock, msgType, DATACODE_RTN_SUCCESS,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    break;
                }

                //for test
                //print_all_detect(&Detect);
                buffLen = 0;

                if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
                {
                    // found realip
                    while(1)
                    {
                        if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                        {
                            if(msgCode == EXTERNAL_DETECT_DATA)
                                idxB = fpcif_GetIdxByBaseKey(Detect.AgentInfo.user_key, 1);
                            else
                                idxB = fpcif_GetIdxByBaseKey(Detect.AgentInfo.user_key, 0);

                            pthread_rwlock_unlock(&mutexpolicy);
                            break;
                        }
                        else
                        {
                            fcom_SleepWait(5);
                            local_loop_count++;
                        }
                        if( local_loop_count >=  10 )
                        {
                            WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                            break;
                        }
                    }
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",4 );
                    return (-1);
                }

                g_stThread_arg[thrNum].threadStepStatus = 31;

                if (idxB > -1)
                {
                    WRITE_INFO_IP(cpip, "Found realip idx(%d), key(%s)", idxB, Detect.AgentInfo.user_key);
                    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
                    {
                        while(1)
                        {
                            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                            {
                                if (strlen(pBaseInfo[idxB].hb_access_ip) > 0)
                                {
                                    if (g_stProcPcifInfo.cfgUseDebugRealIp) // debugging
                                    {
                                        WRITE_INFO_IP(cpip, "Convert debugging ip(%s->%s)",
                                                      cpip,pBaseInfo[idxB].hb_access_ip);
                                        memset(cpip, 0x00, 15+1);
                                        snprintf(cpip, 15+1, "%s", pBaseInfo[idxB].hb_access_ip);
                                    }
                                    memset(realip, 0x00, sizeof(realip));
                                    memset(realMac, 0x00, sizeof(realMac));

                                    snprintf(realip, sizeof(realip), "%s",   pBaseInfo[idxB].hb_access_ip);
                                    snprintf(realMac, sizeof(realMac), "%s", pBaseInfo[idxB].hb_access_mac);
                                }
                                else
                                {
                                    WRITE_INFO_IP(cpip, "Invalid realip(%s), key(%s)",
                                                  pBaseInfo[idxB].hb_access_ip,Detect.AgentInfo.user_key);
                                    memset(realip, 0x00,  sizeof(realip));
                                    memset(realMac, 0x00, sizeof(realMac));

                                    snprintf(realip, sizeof(realip), "%s", cpip);
                                    snprintf(realMac, sizeof(realMac), "%s", realMac);
                                }
                                pthread_rwlock_unlock(&mutexpolicy);
                                break;
                            }
                            else
                            {
                                fcom_SleepWait(5);
                                local_loop_count++;
                            }
                            if( local_loop_count >=  10 )
                            {
                                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                                break;
                            }
                        }
                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d)",4 );
                        return (-1);
                    }
                }
                else
                {
                    WRITE_INFO_IP(cpip, "Not found realip, key(%s)", Detect.AgentInfo.user_key);

                    memset(realip, 0x00, 15+1);
                    memset(realMac, 0x00, sizeof(realMac));

                    snprintf(realip, sizeof(realip), "%s", cpip);
                }

                g_stThread_arg[thrNum].threadStepStatus = 32;
                // set AgentInfo.user_ip
                snprintf(Detect.AgentInfo.user_ip, sizeof(Detect.AgentInfo.user_ip), "%s", realip);

                // set AgentInfo.MAC
                snprintf(Detect.AgentInfo.user_mac, sizeof(Detect.AgentInfo.user_mac), "%s", realMac);

                // get rule by cpip
                fpcif_GetRuleSt(realip, cpip, Detect.AgentInfo.user_sno, &cpRuleInfo, &cpCycleInfo, &cpFlagInfo, 0);
                g_stThread_arg[thrNum].threadStepStatus = 33;

                int 	firstItem               = 1;
                char	msgLen[5 +1]            = {0x00,};
                char	tmpArrTokenKey[256 +1]  = {0x00,};
                char    *arrTokenKey = NULL;
                char	*tokenKey = NULL;

                memset(tmpArrTokenKey, 0x00, sizeof(tmpArrTokenKey));
                strcpy(tmpArrTokenKey, "");

                if(fcom_malloc((void**)&arrTokenKey, sizeof(char)*(strlen(Detect.change_item)+1)) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }

                snprintf(arrTokenKey, sizeof(char) * strlen(Detect.change_item)+1,"%s", Detect.change_item);
                tokenKey = strtok_r(arrTokenKey, ",",&TempPtr);

                WRITE_INFO_IP(cpip, "- TokenKey(%s)(%s)",tokenKey, arrTokenKey);

                memset(&QBuf, 0x00, sizeof(QBuf));

                //1. QBuf.packtype
                QBuf.packtype = msgCode;

                //2. QBuf.buf = sizeof(int)
                buffLen = sizeof(msgLen);
                memcpy((void *)&QBuf.buf[buffLen], (void *)&Detect, sizeof(Detect));  //???? copy?????
                buffLen += sizeof(Detect);
                g_stThread_arg[thrNum].threadStepStatus = 34;
                if (idxB > -1)
                {
                    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
                    {
                        while(1)
                        {
                            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                            {
                                idxU = fpcif_SetLogLevel(pBaseInfo[idxB].hb_access_ip);
                                pthread_rwlock_unlock(&mutexpolicy);
                                break;
                            }
                            else
                            {
                                fcom_SleepWait(5);
                                local_loop_count++;
                            }
                            if( local_loop_count >= 10 )
                            {
                                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid() );
                                break;
                            }
                        }

                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ", 4 );
                        break;
                    }

                }
                g_stThread_arg[thrNum].threadStepStatus = 35;

                while(tokenKey != NULL)
                {
                    DetectItem = fjson_GetDetectItem(tokenKey);
                    // 0:pass 1:drop 2:info 3:nouse 4:warning 5:critical 6:block
                    if( fpcif_ChkAllowRule(cpip, DetectItem, &DetectData, &cpRuleInfo) == 0 )
                    {
                        tokenKey = strtok_r(NULL, ",",&TempPtr);
                        continue;
                    }

                    /* function ?и? */
                    fpcif_AgentRcvTask(DetectItem, &buffLen, &firstItem, tmpArrTokenKey,cpip, &QBuf, &DetectData);

                    tokenKey = strtok_r(NULL, ",",&TempPtr);
                    WRITE_DEBUG(CATEGORY_DEBUG, "DetectItem TokenKey (%s)", tokenKey)
                }//while

                fcom_MallocFree((void**)&arrTokenKey);
                g_stThread_arg[thrNum].threadStepStatus = 36;

                // in case all data have drop-rule
                if( buffLen <= ( sizeof(Detect) + sizeof(msgLen) ) )
                {
                    WRITE_INFO_IP(cpip,	"[FQPUT] Cancel, maybe because of drop-rule, buffLen(%d)",
                                  buffLen);
                    // send ack
                    fsock_SendAck(sock, msgType, DATACODE_RTN_SUCCESS,
                                  g_stProcPcifInfo.cfgRetryAckCount,
                                  g_stProcPcifInfo.retryAckSleep);
                    return (-1);
                }

                memset(Detect.change_item, 0x00, sizeof(Detect.change_item));
                snprintf(Detect.change_item, sizeof(Detect.change_item) -1, "%s", tmpArrTokenKey);
                memcpy((void *)&QBuf.buf[sizeof(msgLen)], (void *)&Detect, sizeof(Detect));

                memset(msgLen, 0x00, sizeof(msgLen));
                snprintf(msgLen, sizeof(msgLen), "%d", buffLen);
                memcpy((void *)&QBuf.buf, &msgLen, sizeof(msgLen));

                if(idxU > -1)
                    us_db_router = (pUserInfo[idxU].us_db_router-'0')%48;

                g_stThread_arg[thrNum].threadStepStatus = 37;
                if(us_db_router > 0 && us_db_router <= g_stProcPcifInfo.cfgQCount)
                {
#if 0
                    rxt = fipc_FQPutData(us_db_router, (char *)&QBuf, sizeof(QBuf));
                    prefix = us_db_router;
#endif
                    if ( fpcif_FileMutexLock(&mutexFile) == 0)
                    {
                        /** File Write **/
                        fpcif_WriteDataFile(g_nPcifForkIdx, &QBuf, sizeof(_DAP_QUEUE_BUF), g_stProcPcifInfo.Fp );
                        pthread_mutex_unlock(&mutexFile);
                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Detect Data Mutex Failed ");
                        rxt = (-1);
                    }
                }
                else
                {
#if 0
                    modVal = getpid() % g_stProcPcifInfo.cfgQCount;

                    rxt = fipc_FQPutData(modVal+1, (char *)&QBuf, sizeof(QBuf));
                    prefix = modVal+1;
#endif
                    if ( fpcif_FileMutexLock(&mutexFile) == 0)
                    {
                        /** File Write **/
                        fpcif_WriteDataFile(g_nPcifForkIdx, &QBuf, sizeof(_DAP_QUEUE_BUF), g_stProcPcifInfo.Fp );
                        pthread_mutex_unlock(&mutexFile);
                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Detect Data Mutex Failed ");
                        rxt = (-1);
                    }
                }

                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"[FQPUT] (%d) Fail in send, ip(%s)code(%d)size(%s)item(%s)",
                                      prefix, cpip, msgCode, msgLen, Detect.change_item);
                    rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_FAIL,
                                  g_stProcPcifInfo.cfgRetryAckCount,
                                  g_stProcPcifInfo.retryAckSleep);
                    if ( rxt < 0 )
                        return (-1);
                }
                else
                {
                    WRITE_INFO_IP(cpip,	"[FQPUT] (%d) Succeed in send, ip(%s)code(%d)size(%s)item(%s)",
                                  prefix, cpip, msgCode, msgLen, Detect.change_item);
                    rxt = fsock_SendAck(sock, msgType, DATACODE_RTN_SUCCESS,
                                  g_stProcPcifInfo.cfgRetryAckCount,
                                  g_stProcPcifInfo.retryAckSleep);
                    if ( rxt < 0 )
                        return (-1);
                }

                g_stThread_arg[thrNum].threadStepStatus = 38;
                break;
            }

            case DATACODE_MATCH_MAC: // 22
            {
                WRITE_INFO_IP(cpip, "[REQ] DATACODE_MATCH_MAC");
                rxt = fpcif_ParseMatchMac(cpip, sock, jsonRoot);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse match mac, cpip(%s)code(%d)",
                                      cpip, msgCode);
                }
                break;
            }
            case MANAGE_FORWARD_SERVER_LOGTAIL: //57
            {
                WRITE_INFO_IP(cpip, "[REQ] MANAGE_FORWARD_SERVER_LOGTAIL");

                memset(managerIp,	0x00, sizeof(managerIp));
                rxt = fpcif_ParseForwardServerLogtail(cpip, jsonRoot, managerIp, &managerFq, &resData);
                if(rxt < 0)
                {
                    resData = NULL;
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse server tail, manager(%s)code(%d)",
                                      managerIp, msgCode);
                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);

                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                          strerror(errno),managerIp);
                    }
                }
                else
                {
                    memset(&TInfo, 0x00, sizeof(struct _STLOGTAIL));

                    sprintf(TInfo.FlagFilePath,"%s/%s/%s_%d",
                            getenv("DAP_HOME"),
                            "config/.TAIL",
                            "DISTAGENT",
                            g_stServerInfo.stDapComnInfo.nCfgMgwId);

                    if (access(TInfo.FlagFilePath, W_OK) != 0)  // If not exist
                    {
                        rxt = fcom_MkPath(TInfo.FlagFilePath, 0755);
                        if (rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip, "Fail in make path(%s)", TInfo.FlagFilePath );
                        }
                        else
                        {
                            chmod(tar_home, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                            WRITE_INFO_IP(cpip, "Succeed in make path(%s) ", TInfo.FlagFilePath );
                        }
                    }

                    if(fcom_fileCheckStatus(TInfo.FlagFilePath) == 0)
                    {
                        unlink(TInfo.FlagFilePath);
                    }

                    TInfo.cpip = cpip;
                    TInfo.ssl = NULL;
                    TInfo.sock = sock;
                    sprintf(TInfo.FilePath,"%s",resData);

                    fpcif_ReqForwardLogTail(&TInfo);
                }
                break;
            }

            case MANAGE_FORWARD_SERVER_LOGLIST: //55
            case MANAGE_FORWARD_SERVER_LOGFILE: //56
            {
                memset(managerId,	0x00, sizeof(managerId));
                memset(beginDate,	0x00, sizeof(beginDate));
                memset(endDate, 	0x00, sizeof(endDate));
                memset(category, 	0x00, sizeof(category));

                if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_FORWARD_SERVER_LOGLIST");

                    rxt = fpcif_ParseServerLoglist(	cpip,
                                                       jsonRoot,
                                                       managerId,
                                                       beginDate,
                                                       endDate,
                                                       category,
                                                       &resData);
                }
                else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_FORWARD_SERVER_LOGFILE");
                    rxt = fpcif_ParseServerLogfile(	cpip,
                                                       jsonRoot,
                                                       managerId,
                                                       beginDate,
                                                       endDate,
                                                       category,
                                                       &resData,
                                                       &resData2);
                }

                if(rxt < 0)
                {
                    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                    {
                        resData = NULL;
                    }
                    else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                    {
                        resData = NULL;
                        resData2 = NULL;
                    }
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse forward server log, manager(%s)code(%d)",
                                      managerId,msgCode);
                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                          strerror(errno),managerId);
                    }
                }
                else
                {
                    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                    {
                        // get find ip
                        jsonSize = strlen(resData)+1;
                        if(fcom_malloc((void**)&findIp, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }

                        strcpy(findIp, resData);
                        resData = NULL;
                        WRITE_INFO_IP(cpip, "- find_ip(%s)", findIp);
                    }
                    else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                    {
                        // get find ip
                        jsonSize = strlen(resData)+1;
                        if(fcom_malloc((void**)&findIp, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }

                        strcpy(findIp, resData);
                        resData = NULL;
                        // get find str
                        jsonSize = strlen(resData2)+1;
                        if(fcom_malloc((void**)&findStr, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            break;
                        }

                        strcpy(findStr, resData2);
                        resData2 = NULL;

                        WRITE_INFO_IP(cpip, "- find_ip(%s)find_str(%s)", findIp,findStr);
                    }

                    memset(log_home, 0x00, sizeof(log_home));
                    sprintf(log_home, "%s/log", g_stServerInfo.stDapComnInfo.szDapHome);
                    memset(tar_home, 0x00, sizeof(tar_home));
                    sprintf(tar_home, "%s/.TMPLOG/%d/%s/", g_stServerInfo.stDapComnInfo.szDapHome,
                            g_stServerInfo.stDapComnInfo.nCfgMgwId,
                            managerId);
                    if (access(tar_home, W_OK) != 0) // If not exist
                    {
                        rxt = fcom_MkPath(tar_home, 0755);
                        if (rxt < 0)
                        {
                            if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                            {
                                fcom_MallocFree((void**)&findIp);
                            }
                            else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                            {
                                fcom_MallocFree((void**)&findIp);
                                fcom_MallocFree((void**)&findStr);
                            }
                            WRITE_CRITICAL_IP(cpip, "Fail in make path(%s)", tar_home);
                            break;
                        }
                        else
                        {
                            chmod(tar_home, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                            WRITE_INFO_IP(cpip, "Succeed in make path(%s)", tar_home);
                        }
                    }
                    else // 있으면지우고
                    {
                        if(strstr(tar_home, ".TMPLOG") != NULL)
                        {
                            memset	(strCommand, 0x00, sizeof(strCommand));
                            sprintf	(strCommand, "rm -rf %s", tar_home);
                            system(strCommand);
                            WRITE_INFO_IP(cpip, "Del(%s)", strCommand);
                        }
                        // 다시만들자
                        rxt = fcom_MkPath(tar_home, 0755);
                        if (rxt < 0)
                        {
                            if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                            {
                                fcom_MallocFree((void**)&findIp);
                            }
                            else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                            {
                                fcom_MallocFree((void**)&findIp);
                                fcom_MallocFree((void**)&findStr);
                            }
                            WRITE_CRITICAL_IP(cpip, "Fail in make path(%s)", tar_home);
                            break;
                        }
                        else
                        {
                            chmod(tar_home, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                            WRITE_INFO_IP(cpip, "Succeed in make path(%s)", tar_home);
                        }
                    }

                    memset		(compDate1,	0x00, sizeof(compDate1));
                    memset		(compDate2,	0x00, sizeof(compDate2));
                    fcom_ReplaceAll(beginDate, "-", "", compDate1);
                    fcom_ReplaceAll(endDate, "-", "", compDate2);

                    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
                    {
                        fpcif_ReqServerLogList(&jsonRootLogList,log_home,category,compDate1,compDate2,findIp,&findCnt,cpip);
                    }
                    else if( msgCode == MANAGE_FORWARD_SERVER_LOGFILE )
                    {
                        fpcif_ReqServerLogFile(log_home,tar_home, category,compDate1,compDate2,findIp,"",&findCnt,cpip);

                        fcom_MallocFree((void**)&findIp);
                        fcom_MallocFree((void**)&findStr);
                    }

                    rxt = fpcif_ProcForwardServerLog(msgCode,  sock, cpip, compDate1, compDate2, tar_home, findCnt,jsonRootLogList);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send the json to forward, manager(%s)code(%d)",
                                          managerId,msgCode);
                    }
                    else
                    {
                        // 종료rsp
                        rxt = fsock_SendAck(sock, msgType, MANAGE_RSP_TERM,
                                            g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                              strerror(errno),cpip);
                        }
                    }
                    // 지우자
                    if(strstr(tar_home, ".TMPLOG") != NULL)
                    {
                        memset	(strCommand, 0x00, sizeof(strCommand));
                        sprintf	(strCommand, "rm -rf %s", tar_home);
                        system(strCommand);
                        WRITE_INFO_IP(cpip, "Del(%s)", strCommand);
                    }
                }
                break;
            }

            case MANAGE_COLLECT_SERVER_LOGTAIL: //58
            {
                WRITE_INFO_IP(cpip, "[REQ] MANAGE_COLLECT_SERVER_LOGTAIL");

                memset(managerIp, 0x00, sizeof(managerIp));
                memset(filePath, 0x00, sizeof(filePath));
                rxt = fpcif_ParseCollectServerTail(cpip, jsonRoot, &serverId, managerIp, &managerFq, filePath, &resData);
                if(rxt < 0)
                {
                    resData = NULL;
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse collect server tail, manager(%s)code(%d)",
                                      managerIp,msgCode);
                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                          strerror(errno),managerIp);
                    }
                }
                else
                {
                    _DAP_EventParam EventParam;

                    memset(&EventParam, 0x00, sizeof(EventParam));
                    strcpy(EventParam.user_key, filePath);
                    EventParam.ev_type = MANAGE_RSP_LOGTAIL;
                    EventParam.ev_level = serverId;
                    snprintf( EventParam.ev_context, sizeof(EventParam.ev_context), "%s", resData);
                    resData = NULL;

                    memset  (fqPath, 0x00, sizeof(fqPath));
                    sprintf (fqPath, "%s/.DAPQ/MNGQ/%d_%s",
                             g_stServerInfo.stDapComnInfo.szDapHome,
                             managerFq,
                             managerIp);
                    if (fipc_FQPutInit(managerFq, fqPath) < 0)
                    {
                        WRITE_INFO_IP(cpip, "Fail in fqueue init, path(%s)", fqPath);
                        break;
                    }
                    rxt = fipc_FQPutData(managerFq, (char *)&EventParam, sizeof(EventParam));
                    if( rxt < 0 )
                    {
                        WRITE_INFO_IP(cpip, "Fail in put, rxt(%d)", rxt);
                        fipc_FQClose(managerFq);
                        break;
                    }

                    WRITE_INFO_IP(cpip, "Succeed in put, rxt(%d)fq(%d)", rxt,managerFq);

                    fipc_FQClose(managerFq);
                }
                break;
            }

            case MANAGE_FORWARD_SERVER_TERM: //59
            {
                WRITE_INFO_IP(cpip, "[REQ] MANAGE_FORWARD_LOGTAIL_TERM");

                sprintf(TInfo.FlagFilePath,"%s/%s/%s_%d",
                        getenv("DAP_HOME"),
                        "config/.TAIL",
                        "DISTAGENT",
                        g_stServerInfo.stDapComnInfo.nCfgMgwId);

                fcom_MakeFlagFile(TInfo.FlagFilePath);

                WRITE_INFO_IP(cpip,"[REQ] MANAGE_FORWARD_LOGTAIL_TERM Make Path : [%s] ",TInfo.FlagFilePath );

                memset(managerIp,	0x00, sizeof(managerIp));
                rxt = fpcif_ParseForwardServerTerm(cpip, jsonRoot, managerIp, &resData);

                if(rxt < 0)
                {
                    resData = NULL;
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse server tail, manager(%s)code(%d)",
                                      managerIp, msgCode);

                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                          strerror(errno), managerIp);
                    }
                }
                else
                {
//                    fpcif_KillTail(cpip, managerIp, resData);
                    resData = NULL;
                }
                break;
            }

            case MANAGE_FORWARD_SHUTDOWN: //65
            {
                WRITE_INFO_IP(cpip, "[REQ] MANAGE_FORWARD_SHUTDOWN");

                memset(managerId,	0x00, sizeof(managerId));
                rxt = fpcif_ParseReqShutdown(cpip, jsonRoot, managerId);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse forward shutdown, cpip(%s)code(%d)",
                                      cpip,msgCode);
                }
                else
                {
                    int shutdownType = rxt;

                    rxt = fpcif_SendNotiByFq("", shutdownType, cpip); // 0:종료, 1:리부팅
                    if( rxt > -1 )
                    {
                        WRITE_INFO_IP(cpip, "End in about 10 seconds");
                        sleep(10); // 10?? ?? ????
                        memset	(strCommand, 0x00, sizeof(strCommand));
                        if( shutdownType == 1 )
                        {
                            sprintf	(strCommand, "%s/%s",
                                        g_stServerInfo.stDapComnInfo.szDapHome,
                                        "bin/dap_server.sh restart");
                            WRITE_INFO_IP(cpip, "Restart(%s)", strCommand);
                            system(strCommand);

                        }
                        else
                        {
                            sprintf	(strCommand, "%s/%s",
                                        g_stServerInfo.stDapComnInfo.szDapHome,
                                        "bin/dap_server.sh stop");
                            WRITE_INFO_IP(cpip, "Shutdown(%s)", strCommand);
                            system(strCommand);
                        }
                        WRITE_INFO_IP(cpip, "- cmd(%s)", strCommand);
                    }
                }
                break;
            }
            default:
            {
                WRITE_INFO_IP(cpip, "[REQ] Unknown code(%d)", msgCode);
                break;
            }

        } //switch
        g_stThread_arg[thrNum].threadStepStatus = 39;
        json_decref(jsonRoot);
        break;
    } //for
    g_stThread_arg[thrNum].threadStepStatus = 40;
    return 0;
}
int	fpcif_MngRecv(
        int		sock,
        char*	cpip,
        int		fqSelf)
{
    int		rxt = 0;
    int		loop = 0;
    int		msgLeng = 0;
    int		msgCode = 0;
    int		convMsgCode = 0;
    int		jsonSize = 0;
    int		findCnt = 0;
    int		shutdownType = -1;
    int		tokenCnt = 0;
    int     nByteAlreadyRead = 0;
    char	command[128 +1] = {0x00,};
    char	managerId[30 +1] = {0x00,};
    char	msgType[10 +1] = {0x00,};
    char	agentIp[15 +1] = {0x00,};
    char	beginDate[10 +1] = {0x00,};
    char	endDate[10 +1] = {0x00,};
    char	category[128 +1] = {0x00,};
    char	log_home[128 +1] = {0x00,};
    char	tar_home[128 +1] = {0x00,};
    char	strCommand[128 +1] = {0x00,};
    char	compDate1[8 +1] = {0x00,};
    char	compDate2[8 +1] = {0x00,};
    char	distServerIp[100 +1] = {0x00,};
    char    szEncHeaderBuffer[32] = {0x00,};
    char	*tokenIp = NULL;
    char	*tmpJson = NULL;
    char	*resData = NULL;
    char	*jsonData = NULL;
    char	*resData2 = NULL;
    char	*findIp = NULL;
    char	*findStr = NULL;
    char    *TempPtr = NULL;
    char    *szEncBuffer = NULL;
    char    *szDecryptBuffer = NULL;


    CRhead			SHead;
    CRhead			RHead;
    json_t			*jsonRoot;
    json_t			*jsonRootLogList;
    json_error_t	error;

    struct _STLOGTAIL stTail;
    struct _DistTailInfo stDistTailInfo;

    _DAP_QUEUE_INFO* pstQueueInfo;

    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    //main에 checkNotiFile()을 수행하기위해 정상일 경우 break, 에러일경우 -1리턴
    for (;;)
    {
        int nReadLen = 0;
        int decrypto_size = 0;
        char szHeader[32] = {0x00,};
        rxt = fsock_Recv(sock, (char *)szEncHeaderBuffer, sizeof(szEncHeaderBuffer),0);
        if ( rxt <= 0) {
            WRITE_CRITICAL(CATEGORY_DEBUG, "recv Failed rxt %d %d", rxt, errno);
            return -1;
        }
        if (!fsec_Decrypt(szEncHeaderBuffer, rxt, ENC_AESKEY, ENC_IV, (unsigned char*)szHeader, &decrypto_size))
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Decrypt Failed ");
            return -1;
        }
        memcpy(&RHead, szHeader, sizeof(RHead));
        memset(msgType, 0x00, sizeof(msgType));
        strcpy(msgType, RHead.msgtype);
        msgLeng = ntohl(RHead.msgleng);
        msgCode = ntohs(RHead.msgcode);
        WRITE_INFO_IP(cpip, "Receive ip(%s)pid(%d): type(%s)code(%d)leng(%d)",
                      cpip,getpid(),msgType,msgCode,msgLeng);

        if (msgLeng > 0) {
            if(fcom_malloc((void**)&szEncBuffer, msgLeng) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return -1;
            }

            nByteAlreadyRead = 0;
            while(msgLeng != nByteAlreadyRead) {
                if ( nByteAlreadyRead > msgLeng )
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", nByteAlreadyRead);
                    break;
                }
                rxt = fsock_Recv(sock, (char *)&szEncBuffer[nByteAlreadyRead], msgLeng - nByteAlreadyRead,0);
                if ( rxt <= 0){
                    break;
                }
                nByteAlreadyRead += rxt;
            }
            if ( rxt <= 0){
                WRITE_CRITICAL(CATEGORY_DEBUG,"recv Failed rxt %d %d",rxt,errno);
                return -1;
            }

            if(fcom_malloc((void**)&szDecryptBuffer, msgLeng) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return -1;
            }

            if (!fsec_Decrypt(szEncBuffer, msgLeng, ENC_AESKEY, ENC_IV, (unsigned char*)szDecryptBuffer, &decrypto_size))
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Decrypt Failed ");
                return -1;
            }
        }


        if(strncmp(msgType, DAP_MANAGE, 10) != 0)
        {
            memset(&SHead, 0x00, sizeof(SHead));
            strcpy(SHead.msgtype, msgType);
            SHead.msgleng = 0;
            SHead.msgcode = MANAGE_RTN_FAIL;
            rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);

            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(cpip,	"Fail in send socket, error(%s)cpip(%s)",
                                  strerror(errno),cpip);
                return -1;
            }
            WRITE_CRITICAL_IP(cpip,	"Fail in recv, type(%s)cpip(%s)",
                              msgType,cpip);
            return -1;
        }

        if(msgCode == MANAGE_PING)
        {
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_PING");
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_PING,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(cpip,	"Fail in send ack(%s)",
                                  DAP_MANAGE);
                return -1;
            }
            break;
        }

        if(msgCode == MANAGE_RTN_SUCCESS || msgCode == MANAGE_RTN_FAIL)
        {
            WRITE_INFO_IP(cpip, "Receive return code(%d)", msgCode);
            break;
        }

        if(fcom_malloc((void**)&tmpJson, sizeof(char)*(msgLeng+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        memcpy(tmpJson, szDecryptBuffer,msgLeng);

        WRITE_INFO_JSON(cpip, "Recv json(%s)", tmpJson);
        jsonRoot = json_loads(tmpJson, 0, &error);
        if(!jsonRoot)
        {
            fcom_MallocFree((void**)&tmpJson);
            WRITE_INFO_IP(cpip, "Fail in json parse line(%d)(%s)cpip(%s)",
                          error.line,error.text,cpip);
            rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                  strerror(errno),cpip);
            }
            return -2;
        }
        fcom_MallocFree((void**)&tmpJson);
        fcom_MallocFree((void**)&szEncBuffer);
        fcom_MallocFree((void**)&szDecryptBuffer);

        //print_json(jsonRoot, 0);

        switch(msgCode)
        {
            case MANAGE_REQ_ACCOUNT: //51
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_ACCOUNT");
                break;
            case MANAGE_REQ_AGENT_LOGLIST:
            case MANAGE_REQ_AGENT_LOGFILE:
            case MANAGE_REQ_WIN_LOGFILE:
                if( msgCode == MANAGE_REQ_AGENT_LOGLIST )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_AGENT_LOGLIST");
                }
                else if( msgCode == MANAGE_REQ_AGENT_LOGFILE )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_AGENT_LOGFILE");
                }
                else
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_WIN_LOGFILE");
                }


                memset(agentIp, 0x00, sizeof(agentIp));
                rxt = fpcif_ParseReqAgentLog(cpip, jsonRoot, agentIp, &resData);
                if(rxt < 0)
                {
                    fcom_MallocFree((void**)&resData);
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse agent log, code(%d)",
                                      msgCode);
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket to manager(%s)errno(%s)",
                                          cpip,strerror(errno));
                    }
                }
                else
                {
                    jsonSize = rxt + 1;
                    if(fcom_malloc((void**)&jsonData, sizeof(char)*jsonSize) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                        return (-1);
                    }

                    strcpy(jsonData, resData);
                    fcom_MallocFree((void**)&resData);

                    rxt = fpcif_ProcReqAgentLog(sock, agentIp, cpip, DAP_AGENT, msgCode, jsonData, jsonSize);

                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,"Fail in send the json to agent, agent(%s)code(%d)",
                                          agentIp,
                                          msgCode);
                        rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                            g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,"Fail in send socket, errno(%s)manager(%s)",
                                              strerror(errno),cpip);
                            break;
                        }
                    }
                    fcom_MallocFree((void**)&jsonData);
                }
                break;
            case MANAGE_REQ_AGENT_LOGTAIL: //71
            case MANAGE_REQ_SERVER_LOGTAIL: //72
            case MANAGE_REQ_LOGTAIL_TERM: //73
                if( msgCode == MANAGE_REQ_AGENT_LOGTAIL )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_AGENT_LOGTAIL");
                }
                if( msgCode == MANAGE_REQ_SERVER_LOGTAIL )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_SERVER_LOGTAIL");
                }
                else
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_LOGTAIL_TERM");
                }

                memset(managerId,	0x00, sizeof(managerId));
                rxt = fpcif_ParseReqServerTail(cpip, jsonRoot, managerId, &resData);
                if(rxt < 0)
                {
                    resData = NULL;
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse server tail, manager(%s)code(%d)",
                                      cpip,msgCode);
                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                          strerror(errno),cpip);
                    }
                }
                else
                {
                    int local_nIdx = 0;
                    pthread_mutex_lock( &g_pthread_Mutex );
                    local_nIdx = fpcif_ThreadGetIdx(g_stProcPcifInfo.cfgThreadCnt);
                    pthread_mutex_unlock( &g_pthread_Mutex );
                    if ( local_nIdx < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,"Fail in Thread Get Idx Failed");
                        return (-1);
                    }
                    sprintf(stTail.FlagFilePath,"%s/%s/%s_%d_%d",
                            getenv("DAP_HOME"),
                            "config/.TAIL","REQ",
                            getpid(),local_nIdx);

                    if(msgCode != MANAGE_REQ_LOGTAIL_TERM)
                    {
                        if (access(stTail.FlagFilePath, W_OK) != 0)  // If not exist
                        {
                            rxt = fcom_MkPath(stTail.FlagFilePath, 0755);
                            if (rxt < 0)
                            {
                                WRITE_CRITICAL_IP(cpip,"Fail in make path(%s)",stTail.FlagFilePath );
                            }
                            else
                            {
                                chmod(stTail.FlagFilePath, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                                WRITE_CRITICAL_IP(cpip,"Succeed in make path(%s)",stTail.FlagFilePath );
                            }
                        }
                        WRITE_INFO_IP(cpip,"[REQ] MANAGE_REQ_LOGTAIL_TERM Make File (%s) ",stTail.FlagFilePath );
                        if(fcom_fileCheckStatus(stTail.FlagFilePath) == 0)
                        {
                            WRITE_INFO_IP(cpip,"[REQ] MANAGE_REQ_LOGTAIL Is Running (%s)",stTail.FlagFilePath);
                            break;
                        }
                        sprintf(stDistTailInfo.FlagFilePath,"%s",stTail.FlagFilePath);
                    }

                    // send ack success (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_SUCCESS,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        resData = NULL;
                        WRITE_CRITICAL_IP(cpip,	"[ACK] Fail in send ack to manager(%s)code(%d)errno(%s)",
                                          cpip,MANAGE_RTN_SUCCESS,strerror(errno));
                        break;
                    }

                    if( msgCode == MANAGE_REQ_LOGTAIL_TERM )
                    {
                        fpcif_DistKillTail(cpip, cpip, resData);
                        resData = NULL;

                        fcom_MakeFlagFile(stTail.FlagFilePath);
                        WRITE_DEBUG_IP(cpip, "[REQ] MANAGE_REQ_LOGTAIL_TERM Make File (%s) ",  stTail.FlagFilePath);
                        resData = NULL;
                    }
                    else
                    {
                        char szCopyFilePath[256+1];
                        char UnpackName[256+1];
                        char Temp[2+1];

                        char* szPtrTmp;

                        memset(UnpackName, 0x00, sizeof(UnpackName));
                        memset(szCopyFilePath, 0x00, sizeof(szCopyFilePath));
                        memset(Temp, 0x00, sizeof(Temp));

//                        stTail.ssl = ssl;
                        stTail.sock = sock;
                        sprintf(stTail.FilePath, "%s", resData);
                        stTail.cpip = cpip;
                        sprintf(UnpackName,"%s",stTail.FilePath);
                        sprintf(szCopyFilePath,"%s",stTail.FilePath);
                        szPtrTmp = strtok_r(szCopyFilePath,"/",&TempPtr);
                        sprintf(Temp,"%s",szPtrTmp);
                        stTail.mgwid = atoi(Temp);
                        if(fcom_ThreadCreate(&pthreadtail,fpcif_ReqServerLogTail,(void*)&stTail,g_stProcPcifInfo.nThreadStackSize) != 0)
                        {
                            WRITE_CRITICAL_IP(cpip,"Fail in create thread");
                            return (-1);
                        }

                        // Deliver distribution server
                        fcom_GetProfile("PCIF", "DISTRIBUTION_SERVER_IP", distServerIp, "");
                        if( strlen(distServerIp) > 6 && stTail.mgwid != g_stServerInfo.stDapComnInfo.nCfgMgwId)
                        {
                            json_t*	root;

                            root = json_pack("{s:s, s:i, s:s}",	"manager_ip",cpip,
                                             "manager_fq",fqSelf,
                                             "file_name",resData);
                            resData = NULL;
                            jsonData = json_dumps(root, JSON_INDENT(0));
                            if( jsonData == NULL )
                            {
                                json_decref(root);
                                WRITE_CRITICAL_IP(cpip,"Json pack error" );
                                break;
                            }
                            json_decref(root);
                            jsonSize = strlen(jsonData);
                            WRITE_INFO_JSON(cpip,"Dump json(%s)", jsonData);
                            convMsgCode = MANAGE_FORWARD_SERVER_LOGTAIL;
                            tokenCnt = fcom_TokenCnt(distServerIp, ",");
                            if(tokenCnt > 0)
                            {
                                loop = 0;
                                tokenIp = strtok_r(distServerIp, ",",&TempPtr);
                                while(tokenIp != NULL)
                                {
                                    fpcif_DistServerTail(cpip, tokenIp, convMsgCode, jsonData, jsonSize,&stDistTailInfo.sslcon);
                                    loop++;
                                    tokenIp = strtok_r(NULL, ",",&TempPtr);
                                }
                            }
                            else
                            {
                                fpcif_DistServerTail(cpip, distServerIp, convMsgCode, jsonData, jsonSize,&stDistTailInfo.sslcon);
                            }

                            // Manager Socket, SSL 복사.
//                            stDistTailInfo.p_manager_ssl = ssl;
                            stDistTailInfo.manager_sock = sock;
                            stDistTailInfo.cpip = cpip;

                            if(fcom_ThreadCreate(&pthreadDisttail, fpcif_DistServerLogTail, (void*)&stDistTailInfo,g_stProcPcifInfo.nThreadStackSize) != 0)
                            {
                                WRITE_CRITICAL_IP(cpip,"Fail in create thread" );
                            }
                            fcom_MallocFree((void**)&jsonData);
                        }
                    }
                }
                break;
            case MANAGE_REQ_SERVER_LOGLIST: //75
            case MANAGE_REQ_SERVER_LOGFILE: //78
                memset(managerId,	0x00, sizeof(managerId));
                memset(beginDate,	0x00, sizeof(beginDate));
                memset(endDate, 	0x00, sizeof(endDate));
                memset(category, 	0x00, sizeof(category));

                if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_SERVER_LOGLIST");
                    rxt = fpcif_ParseServerLoglist(cpip,
                                                   jsonRoot,
                                                   managerId,
                                                   beginDate,
                                                   endDate,
                                                   category,
                                                   &resData);
                }
                else
                {
                    WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_SERVER_LOGFILE");
                    rxt = fpcif_ParseServerLogfile(	cpip,
                                                       jsonRoot,
                                                       managerId,
                                                       beginDate,
                                                       endDate,
                                                       category,
                                                       &resData,
                                                       &resData2);
                }

                if(rxt < 0)
                {
                    if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                    {
                        resData = NULL;
                    }
                    else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                    {
                        resData = NULL;
                        resData2 = NULL;
                    }
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse server log, manager(%s)code(%d)",
                                      cpip,msgCode);
                    // send ack fail (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, error(%s)manager(%s)",
                                          strerror(errno),cpip);
                    }
                }
                else
                {
                    // send ack success (server -> manager)
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_SUCCESS,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                        {
                            resData = NULL;
                        }
                        else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                        {
                            resData = NULL;
                            resData2 = NULL;
                        }
                        WRITE_CRITICAL_IP(cpip,	"[ACK] Fail in send ack to manager(%s)code(%d)errno(%s)",
                                          cpip,MANAGE_RTN_SUCCESS,strerror(errno));
                        break;
                    }

                    if( msgCode == MANAGE_REQ_SERVER_LOGLIST)
                    {
                        // get find ip
                        jsonSize = strlen(resData)+1;
                        if(fcom_malloc((void**)&findIp, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }
                        strcpy(findIp, resData);
                        resData = NULL;
                        WRITE_INFO_IP(cpip, "- find_ip(%s)", findIp);
                    }
                    else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                    {
                        // get find ip
                        jsonSize = strlen(resData)+1;
                        if(fcom_malloc((void**)&findIp, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }

                        strcpy(findIp, resData);
                        resData = NULL;
                        // get find str
                        jsonSize = strlen(resData2)+1;
                        if(fcom_malloc((void**)&findStr, sizeof(char)*jsonSize) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }

                        strcpy(findStr, resData2);
                        resData2 = NULL;
                        WRITE_INFO_IP(cpip, "- find_ip(%s)find_str(%s)", findIp,findStr);
                    }

                    memset(log_home, 0x00, sizeof(log_home));
                    sprintf(log_home, "%s/log", g_stServerInfo.stDapComnInfo.szDapHome);
                    memset(tar_home, 0x00, sizeof(tar_home));
                    sprintf(tar_home, "%s/.TMPLOG/%d/%s/",
                            g_stServerInfo.stDapComnInfo.szDapHome,
                            g_stServerInfo.stDapComnInfo.nCfgMgwId,
                            managerId);
                    if (access(tar_home, W_OK) != 0) // If not exist
                    {
                        rxt = fcom_MkPath(tar_home, 0755);
                        if (rxt < 0)
                        {
                            if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                            {
                                fcom_MallocFree((void**)&findIp);
                            }
                            else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                            {
                                fcom_MallocFree((void**)&findIp);
                                fcom_MallocFree((void**)&findStr);
                            }
                            WRITE_CRITICAL_IP(cpip, "Fail in make path(%s)", tar_home);
                            break;
                        }
                        else
                        {
                            chmod(tar_home, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                            WRITE_INFO_IP(cpip, "Succeed in make path(%s)", tar_home);
                        }
                    }

                    else // If exist
                    {
                        // Delete
                        if(strstr(tar_home, ".TMPLOG") != NULL)
                        {
                            memset	(strCommand, 0x00, sizeof(strCommand));
                            sprintf	(strCommand, "rm -rf %s", tar_home);
                            system(strCommand);
                            WRITE_INFO_IP(cpip, "Del(%s)", strCommand);
                        }
                        // Make again
                        rxt = fcom_MkPath(tar_home, 0755);
                        if (rxt < 0)
                        {
                            if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                            {
                                fcom_MallocFree((void**)&findIp);
                            }
                            else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                            {
                                fcom_MallocFree((void**)&findIp);
                                fcom_MallocFree((void**)&findStr);
                            }
                            WRITE_CRITICAL_IP(cpip, "Fail in make path(%s)", tar_home);
                            break;
                        }
                        else
                        {
                            chmod(tar_home, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                            WRITE_INFO_IP(cpip, "Succeed in make path(%s)", tar_home);
                        }
                    }

                    memset (compDate1,	0x00, sizeof(compDate1));
                    memset (compDate2,	0x00, sizeof(compDate2));
                    fcom_ReplaceAll(beginDate, "-", "", compDate1);
                    fcom_ReplaceAll(endDate, "-", "", compDate2);

                    if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                    {
                        fpcif_ReqServerLogList(&jsonRootLogList,log_home,category,compDate1,compDate2,findIp,&findCnt,cpip);
                    }
                    else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                    {
                        fpcif_ReqServerLogFile(log_home,tar_home, category,compDate1,compDate2,findIp,"",&findCnt,cpip);
                    }

                    // Deliver distribution server
                    fcom_GetProfile("PCIF", "DISTRIBUTION_SERVER_IP", distServerIp, "");
                    if( strlen(distServerIp) > 6 )
                    {
                        if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                        {
                            jsonSize = fpcif_MakeJsonForward( cpip,
                                                              msgCode,
                                                              &convMsgCode,
                                                              managerId,
                                                              beginDate,
                                                              endDate,
                                                              category,
                                                              findIp,
                                                              "",
                                                              &resData );
                            fcom_MallocFree((void**)&findIp);
                        }
                        else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                        {
                            jsonSize = fpcif_MakeJsonForward(	cpip,
                                                                 msgCode,
                                                                 &convMsgCode,
                                                                 managerId,
                                                                 beginDate,
                                                                 endDate,
                                                                 category,
                                                                 findIp,
                                                                 findStr,
                                                                 &resData );

                            fcom_MallocFree((void**)&findIp);
                            fcom_MallocFree((void**)&findStr);
                        }

                        if(fcom_malloc((void**)&jsonData, sizeof(char)*(jsonSize+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            return (-1);
                        }

                        strcpy(jsonData, resData);
                        fcom_MallocFree((void**)&resData);

                        tokenCnt = fcom_TokenCnt(distServerIp, ",");
                        if(tokenCnt > 0)
                        {
                            loop = 0;
                            tokenIp = strtok_r(distServerIp, ",",&TempPtr);
                            while(tokenIp != NULL)
                            {
                                fpcif_DistServerLog(sock, cpip, tokenIp, convMsgCode, jsonData, jsonSize);
                                loop++;
                                tokenIp = strtok_r(NULL, ",", &TempPtr);
                            }
                        }
                        else
                        {
                            fpcif_DistServerLog(sock, cpip, distServerIp, convMsgCode, jsonData, jsonSize);

                        }

                        fcom_MallocFree((void**)&jsonData);
                    }
                    else
                    {
                        if( msgCode == MANAGE_REQ_SERVER_LOGLIST )
                        {
                            fcom_MallocFree((void**)&findIp);
                        }
                        else if( msgCode == MANAGE_REQ_SERVER_LOGFILE )
                        {
                            fcom_MallocFree((void**)&findIp);
                            fcom_MallocFree((void**)&findStr);
                        }
                    }

                    rxt = fpcif_ProcReqServerLog( msgCode,
                                                  sock,
                                                  cpip,
                                                  compDate1,
                                                  compDate2,
                                                  tar_home,
                                                  findCnt,
                                                  jsonRootLogList);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send the json to manager, cpip(%s)code(%d)",
                                          cpip,msgCode);
                        rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                            g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                              strerror(errno),cpip);
                        }
                    }
                    else
                    {
                        // 종료rsp
                        rxt = fsock_SendAck(sock, msgType, MANAGE_RSP_TERM,
                                            g_stProcPcifInfo.cfgRetryAckCount,
                                            g_stProcPcifInfo.retryAckSleep);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                              strerror(errno),cpip);
                        }
                    }
                    // 지우자
                    if(strstr(tar_home, ".TMPLOG") != NULL)
                    {
                        memset	(strCommand, 0x00, sizeof(strCommand));
                        sprintf	(strCommand, "rm -rf %s", tar_home);
                        system(strCommand);
                        WRITE_INFO_IP(cpip, "Del(%s)", strCommand);
                    }
                }
                break;

            case MANAGE_NOTIFY_POLICY: //60
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_NOTIFY_POLICY");
                break;
            case MANAGE_NOTIFY_REPORT: //61
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_NOTIFY_REPORT");
                _DAP_REPORT_INFO 	ReportInfo;
                memset(&ReportInfo, 0x00, sizeof(ReportInfo));
                rxt = fpcif_ParseNotifyReport(cpip, jsonRoot, &ReportInfo);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse notify report, cpip(%s)code(%d)",
                                      cpip,msgCode);
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }
                }
                else
                {
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_SUCCESS,
                                        g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }
                    rxt = fipc_FQPutData(REPORT_QUEUE, (char *)&ReportInfo, sizeof(ReportInfo));
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip, "[FQPUT] Fail in send report");
                    }
                    else
                    {
                        WRITE_INFO_IP(cpip, "[FQPUT] Succeed in send report");
                    }

                    // polling report folder and send file to manager
                    if (rxt > 0 && (ReportInfo.view_type == 1 || ReportInfo.view_type == 2))
                    {
                        rxt = fpcif_ProcReqServerReport( sock,
                                                         cpip,
                                                         ReportInfo.manager_id,
                                                         ReportInfo.begin_date,
                                                         ReportInfo.end_date);
                        if(rxt < 0)
                        {
                            WRITE_CRITICAL_IP(cpip,	"Fail in send the json to manager, ip(%s)code(%d)",
                                              cpip,msgCode);
                            rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL,
                                                g_stProcPcifInfo.cfgRetryAckCount,
                                                g_stProcPcifInfo.retryAckSleep);
                            if(rxt < 0)
                            {
                                WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                                  strerror(errno),cpip);
                            }
                        }
                        else
                        {
                            // 종료rsp
                            rxt = fsock_SendAck(sock, msgType, MANAGE_RSP_TERM ,
                                                g_stProcPcifInfo.cfgRetryAckCount,
                                                g_stProcPcifInfo.retryAckSleep);
                            if(rxt < 0)
                            {
                                WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)manager(%s)",
                                                  strerror(errno),cpip);
                            }
                        }
                    } // if (rxt > 0 && (ReportInfo.view_type == 1 || ReportInfo.view_type == 2))
                }

                break;
            case MANAGE_NOTIFY_PREFER: //62
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_NOTIFY_PREFER");
                break;
            case MANAGE_NOTIFY_ACCOUNT_PREFER: //63
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_NOTIFY_ACCOUNT_PREFER");
                break;
            case MANAGE_REQ_SHUTDOWN: //67
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_SHUTDOWN");

                memset(managerId,	0x00, sizeof(managerId));
                rxt = fpcif_ParseReqShutdown(cpip, jsonRoot, managerId);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse req shutdown, cpip(%s)code(%d)",
                                      cpip,msgCode);
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL, g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }
                }
                else
                {
                    shutdownType = rxt;
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_SUCCESS, g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }

                    // Deliver distribution server
                    fcom_GetProfile("PCIF","DISTRIBUTION_SERVER_IP",distServerIp,"");

                    if( strlen(distServerIp) > 6 )
                    {
                        json_t*	root;
                        root = json_pack("{s:s, s:i}",	"manager_id",managerId,
                                         "type",shutdownType);
                        jsonData = json_dumps(root, JSON_INDENT(0));
                        if( jsonData == NULL )
                        {
                            json_decref(root);
                            WRITE_CRITICAL_IP(cpip, "Json pack error");
                            break;
                        }
                        json_decref(root);

                        jsonSize = strlen(jsonData);

                        WRITE_INFO_JSON(cpip, "Dump json(%s)", jsonData);

                        convMsgCode = MANAGE_FORWARD_SHUTDOWN;

                        tokenCnt = fcom_TokenCnt(distServerIp, ",");
                        if(tokenCnt > 0)
                        {
                            loop = 0;
                            tokenIp = strtok_r(distServerIp, ",",&TempPtr);
                            while(tokenIp != NULL)
                            {
                                fpcif_DistServerTail(cpip, tokenIp, convMsgCode, jsonData, jsonSize,NULL);
                                loop++;
                                tokenIp = strtok_r(NULL, ",", &TempPtr);
                            }
                        }
                        else
                        {
                            fpcif_DistServerTail(cpip, distServerIp, convMsgCode, jsonData, jsonSize,NULL);
                        }

                        fcom_MallocFree((void**)&jsonData);
                    }

                    rxt = fpcif_SendNotiByFq(cpip, shutdownType, cpip); // 0:종료, 1:리부팅
                    if( rxt > -1 )
                    {
                        WRITE_INFO_IP(cpip, "End in about 10 seconds");
                        sleep(10);// 10초 후 종료
                        memset	(command, 0x00, sizeof(command));
                        if( shutdownType == 1 )
                        {
                            sprintf	(command,
                                        "%s/%s",
                                        pstQueueInfo->szDAPQueueHome,
                                        "bin/dap_server.sh restart");
                            WRITE_INFO_IP(cpip, "Restart(%s)", command);
                            system(command);

                        }
                        else
                        {
                            sprintf	(command, "%s/%s", pstQueueInfo->szDAPQueueHome,"bin/dap_server.sh stop");
                            WRITE_INFO_IP(cpip, "Shutdown(%s)", command);
                            system(command);
                        }
                        WRITE_INFO_IP(cpip, "- cmd(%s)", command);
                    }
                }

                break;
            case MANAGE_REQ_UPLOAD_FILE: //90
            WRITE_INFO_IP(cpip, "[REQ] MANAGE_REQ_UPLOAD_FILE");

                memset(managerId,	0x00, sizeof(managerId));
                rxt = fpcif_ParseReqUploadFile(cpip, sock, jsonRoot, managerId);
                if(rxt < 0)
                {
                    WRITE_CRITICAL_IP(cpip,	"Fail in parse req upload file, cpip(%s)code(%d)",
                                      cpip,msgCode);
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_FAIL, g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }
                }
                else
                {
                    rxt = fsock_SendAck(sock, msgType, MANAGE_RTN_SUCCESS ,g_stProcPcifInfo.cfgRetryAckCount,
                                        g_stProcPcifInfo.retryAckSleep);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL_IP(cpip,	"Fail in send socket, errno(%s)cpip(%s)",
                                          strerror(errno),cpip);
                    }
                }
                break;
            default:
            WRITE_INFO_IP(cpip, "[REQ] Unknown code(%d)", msgCode);
                break;
        } //switch
        json_decref(jsonRoot);
        break;

    } //for

    return 0;
}

int fpcif_AgentRcvTask(int DetectType,
                    int* buffLen,
                    int* firstItem,
                    char* tmpArrTokenKey,
                    char* cpip,
                    _DAP_QUEUE_BUF* QBuf,
                    _DAP_DETECT_DATA* DetectData)
{

    switch(DetectType)
    {
        case MAIN_BOARD:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->MainBoard,
                       sizeof(DetectData->MainBoard) );
            *buffLen += sizeof(DetectData->MainBoard);
            WRITE_INFO_IP(cpip, "- MAIN_BOARD: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "main_board");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",main_board");
            }
            break;
        case SYSTEM:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->System,
                       sizeof(DetectData->System) );
            *buffLen += sizeof(DetectData->System);
            WRITE_INFO_IP(cpip, "- SYSTEM: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "system");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",system");
            }
            break;
        case CONNECT_EXT_SVR:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->ConnectExt,
                       sizeof(DetectData->ConnectExt) );
            *buffLen += sizeof(DetectData->ConnectExt);
            WRITE_INFO_IP(cpip, "- CONNECT_EXT_SVR: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "connect_ext_svr");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",connect_ext_svr");
            }
            break;
        case OPERATING_SYSTEM:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->OperatingSystem,
                       sizeof(DetectData->OperatingSystem) );
            *buffLen += sizeof(DetectData->OperatingSystem);
            WRITE_INFO_IP(cpip, "- OPERATING_SYSTEM: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "operating_system");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",operating_system");
            }
            break;
        case CPU:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Cpu,
                       sizeof(DetectData->Cpu) );
            *buffLen += sizeof(DetectData->Cpu);
            WRITE_INFO_IP(cpip, "- CPU: buffLen(%d)", *buffLen);
            //print_all_cpu(&DetectData->Cpu);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "cpu");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",cpu");
            }
            break;
        case NET_ADAPTER:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->NetAdapter,
                       sizeof(DetectData->NetAdapter) );
            *buffLen += sizeof(DetectData->NetAdapter);
            WRITE_INFO_IP(cpip, "- NET_ADAPTER: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "net_adapter");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",net_adapter");
            }
            break;
        case WIFI:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Wifi,
                       sizeof(DetectData->Wifi) );
            *buffLen += sizeof(DetectData->Wifi);
            WRITE_INFO_IP(cpip, "- WIFI: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "wifi");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",wifi");
            }
            break;
        case BLUETOOTH:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Bluetooth,
                       sizeof(DetectData->Bluetooth) );
            *buffLen += sizeof(DetectData->Bluetooth);
            WRITE_INFO_IP(cpip, "- BLUETOOTH: buffLen(%d)", *buffLen);
            if(	*firstItem == 1 )
            {
                strcat(tmpArrTokenKey, "bluetooth");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",bluetooth");
            }
            break;
        case NET_CONNECTION:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->NetConnection,
                       sizeof(DetectData->NetConnection) );
            *buffLen += sizeof(DetectData->NetConnection);
            WRITE_INFO_IP(cpip, "- NET_CONNECTION: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "net_connection");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",net_connection");
            }
            break;
        case DISK:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Disk,
                       sizeof(DetectData->Disk) );
            *buffLen += sizeof(DetectData->Disk);
            WRITE_INFO_IP(cpip, "- DISK: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "disk");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",disk");
            }
            break;
        case NET_DRIVE:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->NetDrive,
                       sizeof(DetectData->NetDrive) );
            *buffLen += sizeof(DetectData->NetDrive);
            WRITE_INFO_IP(cpip, "- NET_DRIVE: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "net_drive");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",net_drive");
            }
            break;
        case OS_ACCOUNT:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->OSAccount,
                       sizeof(DetectData->OSAccount) );
            *buffLen += sizeof(DetectData->OSAccount);
            WRITE_INFO_IP(cpip, "- OS_ACCOUNT: buffLen(%d)", *buffLen);
            //print_all_os_account(&DetectData->OSAccount);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "os_account");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",os_account");
            }
            break;
        case SHARE_FOLDER:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->ShareFolder,
                       sizeof(DetectData->ShareFolder) );
            *buffLen += sizeof(DetectData->ShareFolder);
            WRITE_INFO_IP(cpip, "- SHARE_FOLDER: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "share_folder");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",share_folder");
            }
            break;
        case INFRARED_DEVICE:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->InfraredDevice,
                       sizeof(DetectData->InfraredDevice) );
            *buffLen += sizeof(DetectData->InfraredDevice);
            WRITE_INFO_IP(cpip, "- INFRARED_DEVICE: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "infrared_device");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",infrared_device");
            }
            break;
        case PROCESS:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Process,
                       sizeof(DetectData->Process) );
            *buffLen += sizeof(DetectData->Process);
            WRITE_INFO_IP(cpip, "- PROCESS: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "process");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",process");
            }
            break;
        case ROUTER:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->Router,
                       sizeof(DetectData->Router) );
            *buffLen += sizeof(DetectData->Router);
            WRITE_INFO_IP(cpip, "- ROUTER: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "router");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",router");
            }
            break;
        case NET_PRINTER:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->NetPrinter,
                       sizeof(DetectData->NetPrinter) );
            *buffLen += sizeof(DetectData->NetPrinter);
            WRITE_INFO_IP(cpip, "- NET_PRINTER: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "net_printer");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",net_printer");
            }
            break;
        case NET_SCAN:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->NetScan,
                       sizeof(DetectData->NetScan) );
            *buffLen += sizeof(DetectData->NetScan);
            WRITE_INFO_IP(cpip, "- NET_SCAN: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "net_scan");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",net_scan");
            }
            break;

        case SSO_CERT: //알람정보로 DB에 기록하지 않고 이벤트만 발생
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->SsoCert,
                       sizeof(DetectData->SsoCert) );
            *buffLen += sizeof(DetectData->SsoCert);
            WRITE_INFO_IP(cpip, "- SSO_CERT: buffLen(%d)", buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "sso_cert");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",sso_cert");
            }
            break;
        case WIN_DRV: //수집정보로 DB에만 기록함
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->WinDrv,
                       sizeof(DetectData->WinDrv) );
            *buffLen += sizeof(DetectData->WinDrv);
            WRITE_INFO_IP(cpip, "- WIN_DRV: buffLen(%d)", buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "win_drv");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",win_drv");
            }
            break;
        case RDP_SESSION:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->RdpSession,
                       sizeof(DetectData->RdpSession) );
            *buffLen += sizeof(DetectData->RdpSession);
            WRITE_INFO_IP(cpip, "- RDP_SESSION: buffLen(%d)", *buffLen);
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "rdp_session");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",rdp_session");
            }
            break;
        case CPU_USAGE:
            memcpy(	(void *)&QBuf->buf[*buffLen],
                       (void *)&DetectData->CpuUsage,
                       sizeof(DetectData->CpuUsage) );
            *buffLen += sizeof(DetectData->CpuUsage);
            WRITE_INFO_IP(cpip, "- CPU_USAGE: buffLen(%d) ",*buffLen );
            if(*firstItem == 1)
            {
                strcat(tmpArrTokenKey, "cpu_usage");
                *firstItem = 0;
            }
            else
            {
                strcat(tmpArrTokenKey, ",cpu_usage");
            }
            break;

        default:
        WRITE_INFO_IP(cpip,	"Unknown detect item(%d)",
                      DetectType);
            break;
    }//switch

    return RET_SUCC;
}

int fpcif_ManagerWork(
        int     thrNum,
        int     sock,
        char*   cpip,
        char*   msgType,
        int     msgCode,
        int     msgLeng,
        int     fqNum,
        char*   Buffer
)
{
    int		rxt;
    int		resp;
    int		LastAction;
    int		EventAction;
    int		jsonSize;
    int		retryCnt;
    int		bEventFlag = 0;
    int		idxM;
    int		dbMnEventNoti;
    int     local_loop_count = 0;
    char	*resJson = NULL;

    char	strNoti[10];
    char	realip[15+1];
    char	loginid[32+1];
    char	mnq_path[128];
    struct 	pollfd       fds;
    struct 	stat     statBuf;

    CRhead      SHead;
    _DAP_COMN_INFO* pstComnInfo;
    _DAP_QUEUE_INFO* pstQueueInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    _DAP_EventParam EventParam;

    WRITE_INFO_IP(cpip, "Work manager pid(%d)ppid(%d)timeout(%d)fqnum(%d) ",
                  getpid(),getppid(),g_stProcPcifInfo.time_out,fqNum);

    LastAction = time(NULL);
    EventAction = time(NULL);

    memset(realip, 0x00, sizeof(realip));
    memset(loginid, 0x00, sizeof(loginid));

    // MANAGER 로그인 처리 , Manager 버전체크 및 SEND ACK 처리.
    rxt = fpcif_MngLoginRecv(sock, cpip, msgType, msgCode, msgLeng, fqNum, realip, loginid,Buffer);
    if(rxt < 0)
    {
        return -1;
    }

    idxM = fpcif_GetIdxByManagerIpIdSt(realip, loginid);
    WRITE_INFO_IP(cpip, "Get idxM(%d) by realip(%s)loginid(%s) ", idxM,realip,loginid);

    // realip는 idxM만 가져오는데 사용한다.
    // cfgUseDebugRealIp는agent와 달리 manager에서는사용하지 않는다.
    // manager는 NAT 타고 오지 않을뿐더러,
    // agent의 단방향 전달과는 달리 상호 양방향 패킷교환이 많아서
    // 만약, 필요하다면 logip로 로그찍을때만 사용하도록 해야한다.

    dbMnEventNoti = (pManagerInfo[idxM].mn_event_noti-'0')%48;
    memset(mnq_path, 0x00, sizeof(mnq_path));
    sprintf(mnq_path,"%s/MNGQ/%d_%s", pstQueueInfo->szDAPQueueHome,
            fqNum,
            cpip);
    WRITE_INFO_IP(cpip, "FQGetInit fqNum(%d)mnq_path(%s) ", fqNum,mnq_path);
    // Receive 용 Manager Queue Init
    if (fipc_FQGetInit(fqNum, mnq_path) < 0)
    {
        WRITE_CRITICAL_IP(cpip, "Fail in fqueue init, path(%s)",
                          mnq_path);
        fpcif_HandleSignal(15);
    }

    gFQNum = fqNum; //signal 만났을때, close하기 위해 글로벌변수에 대입

    while(1)
    {
        if(pthread_mutex_trylock(&mutexsum) == 0)
        {
            // update sock ip
            fpcif_UpdateSockIpManager(pManagerInfo[idxM].mn_id, cpip);
            pthread_mutex_unlock(&mutexsum);

            break;
        }
        else
        {
            fcom_SleepWait(5);
            local_loop_count++;
        }
        if( local_loop_count >=  20 )
        {
            WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );
            break;
        }
    }


    while(1)
    {
        fds.fd = sock;
        fds.events = POLLIN;

        resp = poll(&fds, 1, 10);
        if(g_ForkHangFlag != 0x00 || g_ChildExitFlag != 0x00)
        {
            break;
        }

        if(resp == 0)
        {
            if(time(NULL)-LastAction > g_stProcPcifInfo.time_out)
            {
                WRITE_INFO_IP(cpip, "Timeout client, sock(%d)(%d) ", sock,g_stProcPcifInfo.time_out);
                WRITE_INFO_IP(cpip, "Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);

                while(1)
                {
                    if(pthread_mutex_trylock(&mutexsum) == 0)
                    {
                        fpcif_UpdateTerminateManager(pManagerInfo[idxM].mn_id);
                        pthread_mutex_unlock(&mutexsum);

                        break;
                    }
                    else
                    {
                        fcom_SleepWait(5);
                        local_loop_count++;
                    }
                    if( local_loop_count >=  20 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );   break; }
                }
                if (!fpcif_SetTerminateByManager_st(pManagerInfo[idxM].mn_id))
                {
                    WRITE_WARNING_IP(cpip, "Not found manager st, so try update touch, idx(%d)(%s) ",
                                     idxM,pManagerInfo[idxM].mn_id);


                    local_loop_count = 0;
                    while(1)
                    {
                        if(pthread_mutex_trylock(&mutexsum) == 0)
                        {
                            fpcif_UpdateConfigChange(pstComnInfo->szDebugName, "cp_change");
                            pthread_mutex_unlock(&mutexsum);
                            break;
                        }
                        else
                        {
                            fcom_SleepWait(5);
                            local_loop_count++;
                        }
                        if( local_loop_count >=  10 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );  break; }
                    }
                }

                if(fqNum >= START_MANAGER_FQ)
                {
                    fipc_FQClose(fqNum);
                }
                if(strlen(mnq_path) > 5)
                {
                    WRITE_INFO_IP(cpip, "Try unlink queue file(%s) ", mnq_path);
                    if( unlink(mnq_path) )
                    {
                        WRITE_MAJOR_IP(cpip, "Fail in unlink queue file(%s)error(%s) ",
                                       mnq_path,strerror(errno));
                        usleep(5000);
                        if( remove(mnq_path) )
                        {
                            WRITE_CRITICAL_IP(cpip, "Fail in remove queue file(%s)error(%s) ",
                                              mnq_path,strerror(errno));
                        }
                    }
                }
                fpcif_KillTail    (cpip, cpip, "all");
                fpcif_DistKillTail(cpip, cpip, "all");
                WRITE_INFO_IP(cpip, "- Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);
                return -3;
            }
        }
        else if(resp > 0)
        {
            // 로그인 처리 이후 매니저 요청 처리
            rxt = fpcif_MngRecv( sock, cpip, fqNum);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(cpip, "Fail in receive client, sock(%d) ",
                                  sock);

                WRITE_CRITICAL_IP(cpip, "- Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);
                while(1)
                {
                    if(pthread_mutex_trylock(&mutexsum) == 0)
                    {
                        fpcif_UpdateTerminateManager(pManagerInfo[idxM].mn_id);
                        pthread_mutex_unlock(&mutexsum);

                        break;
                    }
                    else
                    {
                        fcom_SleepWait(5);
                        local_loop_count++;
                    }
                    if( local_loop_count >=  10 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );  break; }
                }
                if (!fpcif_SetTerminateByManager_st(pManagerInfo[idxM].mn_id))
                {
                    WRITE_INFO_IP(cpip, "Not found manager st, so try update touch, idx(%d)(%s) ",
                                  idxM,pManagerInfo[idxM].mn_id);
                    while(1)
                    {
                        if(pthread_mutex_trylock(&mutexsum) == 0)
                        {
                            fpcif_UpdateConfigChange(pstComnInfo->szDebugName, "cp_change");
                            pthread_mutex_unlock(&mutexsum);
                            break;
                        }
                        else
                        {
                            fcom_SleepWait(5);
                            local_loop_count++;
                        }
                        if( local_loop_count >=  10 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );  break; }

                    }
                }

                if(fqNum > START_MANAGER_FQ)
                    fipc_FQClose(fqNum);
                if(strlen(mnq_path) > 5)
                {
                    WRITE_INFO_IP(cpip, "Try unlink queue file(%s) ", mnq_path);
                    if( unlink(mnq_path) )
                    {
                        WRITE_CRITICAL_IP(cpip, "Fail in unlink queue file(%s)error(%s) ",
                                          mnq_path,strerror(errno));
                        usleep(5000);
                        if( remove(mnq_path) )
                        {
                            WRITE_CRITICAL_IP(cpip, "Fail in remove queue file(%s)error(%s) ",
                                              mnq_path,strerror(errno));
                        }
                    }
                }
                fpcif_KillTail(cpip, cpip, "all");
                fpcif_DistKillTail(cpip, cpip, "all");
                WRITE_INFO_IP(cpip, "- Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);

                return -2;
            }
            LastAction = time(NULL);
        }
        else
        {
            WRITE_WARNING_IP(cpip, "Fail in poll exit ");
            WRITE_WARNING_IP(cpip, "- Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);

            while(1)
            {
                if(pthread_mutex_trylock(&mutexsum) == 0)
                {
                    fpcif_UpdateTerminateManager(pManagerInfo[idxM].mn_id);
                    pthread_mutex_unlock(&mutexsum);
                    break;
                }
                else
                {
                    local_loop_count++;
                }
                if( local_loop_count >=  10 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );  break; }
            }
            if (!fpcif_SetTerminateByManager_st(pManagerInfo[idxM].mn_id))
            {
                WRITE_INFO_IP(cpip, "Not found manager st, so try update touch, idx(%d)(%s) ",
                              idxM,pManagerInfo[idxM].mn_id);
                while(1)
                {
                    if(pthread_mutex_trylock(&mutexsum) == 0)
                    {
                        fpcif_UpdateConfigChange(g_stServerInfo.stDapComnInfo.szDebugName, "cp_change");
                        pthread_mutex_unlock(&mutexsum);
                        break;
                    }
                    else
                    {
                        fcom_SleepWait(5);
                        local_loop_count++;
                    }
                    if( local_loop_count >=  10 ) { WRITE_DEBUG_IP(cpip,"PID[%d] Mutex loop break ",getpid() );  break; }
                }
            }

            if(fqNum > START_MANAGER_FQ)
                fipc_FQClose(fqNum);
            if(strlen(mnq_path) > 5)
            {
                WRITE_INFO_IP(cpip, "Try unlink queue file(%s) ", mnq_path);
                if( unlink(mnq_path) )
                {
                    WRITE_CRITICAL_IP(cpip, "Fail in unlink queue file(%s)error(%s) ",
                                      mnq_path,strerror(errno));
                    usleep(5000);
                    if( remove(mnq_path) )
                    {
                        WRITE_CRITICAL_IP(cpip, "Fail in remove queue file(%s)error(%s) ",
                                          mnq_path,strerror(errno));
                    }
                }
            }
            fpcif_KillTail(cpip, cpip, "all");
            fpcif_DistKillTail(cpip, cpip, "all");
            WRITE_INFO_IP(cpip, "Terminate, idx(%d)(%s) ", idxM,pManagerInfo[idxM].mn_id);
            return -3;
        }

        if( dbMnEventNoti != 1 )  // send noti to manager
        {
            if(time(NULL)-EventAction > g_stProcPcifInfo.cfgCheckEventInterval)
            {
                bEventFlag = 1;
            }
            if(bEventFlag)
            {
                bEventFlag = 0;
                if(resp == 0 && g_pstNotiEvent[EVENT_CHANGE].reload)
                {

                    char szEncHeaderBuffer[32] = {0x00,};
                    memset(&SHead, 0x00, sizeof(SHead));
                    strcpy(SHead.msgtype, DAP_MANAGE);
                    SHead.msgcode  = htons(MANAGE_RSP_EVENT);
                    SHead.msgleng = htonl(2);

                    int encrypto_size = 0;

                    encrypto_size = fsec_Encrypt((char*)&SHead, sizeof(SHead), ENC_AESKEY, ENC_IV, szEncHeaderBuffer);

                    rxt = fsock_Send(sock, (char *)szEncHeaderBuffer, encrypto_size);
                    if(rxt <= 0)
                    {
                        WRITE_WARNING_IP(cpip, "[NOTI] Retry, because fail in send, "
                                               "errno(%d)code(%d) ",
                                         errno,MANAGE_RSP_EVENT);
                        retryCnt = 0;
                        while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
                        {
                            fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
                            rxt = fsock_Send(sock, (char *)szEncHeaderBuffer, encrypto_size);
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
                        if(retryCnt == g_stProcPcifInfo.cfgRetryAckCount)
                        {
                            WRITE_CRITICAL_IP(cpip, "[NOTI] Fail in send, "
                                                    "retry(%d)errno(%d)code(%d) ",
                                              retryCnt,errno,MANAGE_RSP_EVENT);
                            return -1;
                        }
                    }

                    WRITE_INFO_IP(cpip, "[NOTI] Succeed in send, code(%d) ",
                                  MANAGE_RSP_EVENT);

                    stat(g_pstNotiEvent[EVENT_CHANGE].szNotiFileName, &statBuf);
                    g_pstNotiEvent[EVENT_CHANGE].lastModify    =   statBuf.st_mtime;
                    g_pstNotiEvent[EVENT_CHANGE].reload        =   FALSE;
                }

                fpcif_CheckEventNotiFile();
                EventAction = time(NULL);
            }//bEventFlag
        } // if( dbMnEventnoti != 1 )

        memset(&EventParam, 0x00, sizeof(EventParam));
        rxt = fipc_FQGetData(fqNum, (char *)&EventParam, sizeof(EventParam));

        if(rxt == 0)
            continue;
        if(rxt < 0)
        {
            WRITE_INFO_IP(cpip, "Fail in receive, rxt(%d)fq(%d) ", rxt, fqNum);
            sleep(g_stProcPcifInfo.cfgCheckEventInterval);
            continue;
        }
        else
        {
            if( sock > 1 && (EventParam.ev_type == MANAGE_FQ_KILL
                             || EventParam.ev_type == MANAGE_NOTI_SHUTDOWN
                             || EventParam.ev_type == MANAGE_NOTI_RESTART) )
            {

                memset(&SHead, 0x00, sizeof(SHead));
                memset(strNoti, 0x00, sizeof(strNoti));

                strcpy(SHead.msgtype, DAP_MANAGE);
                if( EventParam.ev_type == MANAGE_FQ_KILL )
                {
                    WRITE_INFO_IP(cpip, "MANAGE_FQ_KILL ");
                    SHead.msgcode  = htons(MANAGE_FORCED_KILL);
                    strcpy(strNoti, "KILL");
                }
                else
                {
                    SHead.msgcode  = htons(EventParam.ev_type);
                    if( EventParam.ev_type == MANAGE_NOTI_SHUTDOWN )
                    {
                        WRITE_INFO_IP(cpip, "MANAGE_NOTI_SHUTDOWN ");
                        strcpy(strNoti, "SHUTDOWN");
                    }
                    else
                    {
                        WRITE_INFO_IP(cpip, "MANAGE_NOTI_RESTART ");
                        strcpy(strNoti, "RESTART");
                    }
                }
                SHead.msgleng = htonl(2);
                rxt = fsock_Send(sock, (char *)&SHead, sizeof(CRhead));
                if(rxt <= 0)
                {
                    WRITE_WARNING_IP(cpip, "[%s] Retry, because fail in send, "
                                           "errno(%d)code(%d)size(%d) ",
                                     strNoti,
                                     errno,
                                     MANAGE_FORCED_KILL,
                                     sizeof(CRhead));
                    retryCnt = 0;
                    while(retryCnt < g_stProcPcifInfo.cfgRetryAckCount)
                    {
                        fcom_Msleep(g_stProcPcifInfo.retryAckSleep);
                        rxt = fsock_Send(sock, (char *)&SHead, sizeof(CRhead));
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
                        WRITE_CRITICAL_IP(cpip, "[%s] Fail in send, "
                                                "retry(%d)errno(%d)code(%d)size(%d) ",
                                          strNoti,retryCnt,errno,MANAGE_FORCED_KILL,
                                          sizeof(CRhead));
                        return -1;
                    }
                }
                WRITE_INFO_IP(cpip, "[%s] Succeed in send ", strNoti);
            }
            else if( sock > 1 && EventParam.ev_type == MANAGE_RSP_LOGTAIL ) // Log tail
            {
                /* 여기서는 로그 찍으면 안됨 tail 시 중복현상이 일어남
                WRITE_INFO_IP(cpip, "MANAGE_RSP_LOGTAIL\n");
                WRITE_INFO_IP(cpip, "- server_id : '%d'\n", EventParam.ev_level);
                WRITE_INFO_IP(cpip, "- file_name : '%s'\n", EventParam.user_key);
                WRITE_INFO_IP(cpip, "- message   : '%s'\n", EventParam.ev_context);
                */

                jsonSize = fpcif_MakeJsonTail(    EventParam.ev_level,
                                                  EventParam.user_key,
                                                  EventParam.ev_context,
                                                  &resJson );

                if( jsonSize < 0 )
                {
                    continue;
                }

                WRITE_DEBUG_IP(cpip,"Send Data : [%s] ",resJson);
                rxt = fpcif_SendJsonToManager(sock, EventParam.ev_type, resJson, jsonSize);
                if( rxt < 0 )
                {
                    fcom_MallocFree((void**)&resJson);
                    continue;
                }
                fcom_MallocFree((void**)&resJson);
            }
            else
            {
                // Send event to manager
                WRITE_INFO_IP(cpip, "MANAGE_RSP_EVENT");
                WRITE_INFO_IP(cpip, "- user_key   : '%s' ", EventParam.user_key);
                WRITE_INFO_IP(cpip, "- user_ip    : '%s' ", EventParam.user_ip);
                WRITE_INFO_IP(cpip, "- detect_time: '%s' ", EventParam.detect_time);
                WRITE_INFO_IP(cpip, "- ev_type    : %d ", EventParam.ev_type);
                WRITE_INFO_IP(cpip, "- ev_level   : %d ", EventParam.ev_level);
                WRITE_INFO_IP(cpip, "- ev_context : '%s' ", EventParam.ev_context);
                WRITE_INFO_IP(cpip, "- prefix     : '%s' ", EventParam.prefix);
                WRITE_INFO_IP(cpip, "- postfix    : '%s' ", EventParam.postfix);

                if( sock > 1 && dbMnEventNoti == 1)
                {
                    jsonSize = fpcif_MakeJsonEvent(cpip, &EventParam, &resJson);
                    if( jsonSize < 0 ) continue;

                    rxt = fpcif_SendJsonToManager(sock, MANAGE_RSP_EVENT, resJson, jsonSize);
                    if( rxt < 0 )
                    {
                        fcom_MallocFree((void**)&resJson);
                        continue;
                    }
                    fcom_MallocFree((void**)&resJson);
                }
            }
        }
    }//while

    return 1;
}
void* fpcif_ThreadLoop(void* arg)
{
    int		bFirstReload = 1;
    long 	child_job_time = 0;
    time_t  chkTime = 0;
    time_t 	chkMysqlPingTime = 0;
    time_t 	chkThreadHangTime = 0;
    time_t  chkMyPpidTime     = 0;
    int     nCurrMin = 0;
    int     nLoop = 0;
    int     local_loop_count = 0;
    int     local_exit_flag = 0;
    int     local_nRet  = 0;

    struct stat     statBuf;

    memset(&statBuf, 0x00, sizeof(struct stat));

    while(1)
    {
        if(g_ForkHangFlag != 0x00 || g_ChildExitFlag != 0x00)
        {
            break;
        }

        child_job_time = time(NULL);
        nCurrMin = fcom_GetSysMinute();

        if(g_stProcPcifInfo.cfgThreadHangCheck[0] == 'Y' && difftime(child_job_time, chkThreadHangTime) >= 300)
        {
            chkThreadHangTime = time(NULL);

            pthread_mutex_lock( &g_pthread_Mutex );
            local_nRet = fpcif_ThreadHangCheck();
            pthread_mutex_unlock( &g_pthread_Mutex );
            if(local_nRet == 1)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Soon Exited, Because Thraed Is Hang Status .. " );

                if(g_ForkHangFlag == 0x03)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Finished Wait...");
                    while(1)
                    {
                        local_exit_flag = 0;
                        child_job_time = time(NULL);
                        for(nLoop = 0; nLoop < g_stProcPcifInfo.cfgThreadCnt; nLoop++)
                        {
                            if(thrFlag[nLoop] == 'Y' && difftime(child_job_time, thrTimer[nLoop]) < 300 )
                            {
                                local_exit_flag = 0;
                                break;
                            }
                            if(thrFlag[nLoop] == 'Y' && difftime(child_job_time, thrTimer[nLoop]) >= 300 )
                            {
                                local_exit_flag = 1;
                            }
                        }
                        local_loop_count++;
                        if(local_exit_flag == 1 || local_loop_count > 60)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Finished Success cnt(%d) flag(%d) ...",
                                           local_loop_count,
                                           local_exit_flag);
                            exit(0);

                        }
                        sleep(1);
                    }
                }
            }
            else
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"Thread Status Is Not Hang Status!" );
            }
        }

        if( difftime(child_job_time, chkMysqlPingTime) > 600 )
        {
            if (mysql_ping(g_stMyCon->connect) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Mysql ReConnect(mysql_ping) Is Fail code(%d) msg(%s)",
                               mysql_errno(g_stMyCon->connect),
                               mysql_error(g_stMyCon->connect));
            }
            else
            {
                WRITE_DEBUG(CATEGORY_DB,"Mysql ReConnect(mysql_ping) Is Success " );
            }
            chkMysqlPingTime = time(NULL);
        }

        if( difftime(child_job_time, chkMyPpidTime) > 60 )
        {
            int myPpid = getppid();
            if ( myPpid == 1 ){
                WRITE_DEBUG(CATEGORY_DB,"Orpan Process Status Exited ");
                exit(0);
            }
            chkMyPpidTime = time(NULL);
        }

        if(bFirstReload || difftime(child_job_time, chkTime) >= g_stProcPcifInfo.cfgCheckNotiInterval)
        {
            fpcif_CheckNotiFile();
            chkTime = time(NULL);

            memset(&statBuf, 0x00, sizeof(struct stat));

            if(g_pstNotiMaster[BASE_CHANGE].reload)
            {
                if(fpcif_LoadBase() < 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load base ");
                }
                stat(g_pstNotiMaster[BASE_CHANGE].szNotiFileName, &statBuf);
                g_pstNotiMaster[BASE_CHANGE].lastModify    =   statBuf.st_mtime;
                g_pstNotiMaster[BASE_CHANGE].reload        =   FALSE;
            }

            local_loop_count = 0;
            while(1)
            {
                if(pthread_mutex_trylock(&mutexsum) == 0)
                {
                    LoadConfigFlag = 0x02;
                    fpcif_ReloadConfig();
                    LoadConfigFlag = 0x01; /* Load ??? */
                    pthread_mutex_unlock(&mutexsum);

                    break;

                }
                else
                {
                    fcom_SleepWait(5);
                    local_loop_count++;
                }
                if( local_loop_count >=  10 )
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                    LoadConfigFlag=0x00;
                    break;
                }
            }

            fipc_KeepSession(g_stProcPcifInfo.cfgKeepSession);
            bFirstReload = 0;
            //print_base();
        }

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcPcifInfo.nConfigLastModify) == 1)
                fpcif_ReloadCfgFile();
        }

        fcom_Msleep(900); //0.9초
    }

    pthread_exit(NULL);
}

int	fpcif_ThreadGetIdx( int param_nThreadCnt )
{
    int local_nLoop = 0;

    for(local_nLoop = 0; local_nLoop < param_nThreadCnt; local_nLoop++)
    {
        if (thrFlag[local_nLoop] == 'N')
        {
            // 찾았다면, 현재 값을 전달한다.
            return local_nLoop;
        }

    }

    return (-1);

}
void fpcif_PrintThreadStatus(int nCfgThreadCnt)
{
    int nThreadCnt = 0;
    int nUseCnt = 0;
    int nUnUseCnt = 0;


    for(nThreadCnt = 0; nThreadCnt < nCfgThreadCnt; nThreadCnt++)
    {
        if(thrFlag[nThreadCnt] == 'Y')
        {
            nUseCnt++;
        }
        else
        {
            nUnUseCnt++;
        }
    }
    if(nUseCnt > 0)
    {
        WRITE_INFO(CATEGORY_DEBUG,"PID %d MAX Thread (%d) Use Thread (%d) UnUse Thread ( %d) ",
                    getpid(),
                    g_stProcPcifInfo.cfgThreadCnt,
                    nUseCnt,
                    nUnUseCnt);
        // pcif가 cfg설정 thread개수의 반이상 쓰고있으면
//        if ( nUseCnt > g_stProcPcifInfo.cfgThreadCnt/2)
//        {
//            WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
//                                 "Pcif Max Thread (%d) Use Thread (%d)", g_stProcPcifInfo.cfgThreadCnt, nUseCnt);
//        }
    }

}

//void fpcif_ThreadHangCheck(void)
//{
//    int sslFd = 0;
//    int nLoop = 0;
//    time_t CurrentTime =  time(NULL);
//
//    for(nLoop = 0; nLoop < g_stProcPcifInfo.cfgThreadCnt; nLoop++)
//    {
//        if( (thrFlag[nLoop] == 'Y') && (difftime(CurrentTime,thrTimer[nLoop]) >= 300) )
//        {
//            /* 행 체크 DAP_MANAGER 세션 예외 */
//            if(g_stThread_arg[nLoop].threadStepStatus == 99)
//            {
//                continue;
//            }
//
//            WRITE_CRITICAL(CATEGORY_DEBUG,"Thread idx (%d) Is Hang Client Info : (%s) Msgcode : (%d) Step : (%d)",
//                           nLoop,
//                           g_stThread_arg[nLoop].cli_ip,
//                           g_stThread_arg[nLoop].threadMsgCode,
//                           g_stThread_arg[nLoop].threadStepStatus);
//
//            // Pthread Kill
//            pthread_cancel(g_stThread_arg[nLoop].pthread_t);
//
//            //Hang 걸린 Thread의 자원 정리
//            if( g_stThread_arg[nLoop].cli_ssl != NULL)
//            {
//                sslFd = SSL_get_fd(g_stThread_arg[nLoop].cli_ssl);
//            }
//
//            if( g_stThread_arg[nLoop].cli_ssl != NULL)
//            {
//                SSL_shutdown(g_stThread_arg[nLoop].cli_ssl);
//                SSL_free(g_stThread_arg[nLoop].cli_ssl);
//                g_stThread_arg[nLoop].cli_ssl = NULL;
//            }
//
//            if( g_stThread_arg[nLoop].cli_sock > 0)
//            {
//                close(g_stThread_arg[nLoop].cli_sock);
//                g_stThread_arg[nLoop].cli_sock = 0;
//            }
//
//            if ( sslFd > 0)
//            {
//                close(sslFd);
//            }
//
//            if ( g_stThread_arg[nLoop].cli_ctx != NULL)
//            {
//                SSL_CTX_free( g_stThread_arg[nLoop].cli_ctx );
//                g_stThread_arg[nLoop].cli_ctx = NULL;
//            }
//
//            thrFlag[nLoop] = 'N';
//        }
//    }
//
//    return;
//}
int fpcif_ThreadHangCheck(void)
{
    int nLoop = 0;
    time_t CurrentTime =  time(NULL);

    for(nLoop = 0; nLoop < g_stProcPcifInfo.cfgThreadCnt; nLoop++)
    {
        if( (thrFlag[nLoop] == 'Y') && (difftime(CurrentTime,thrTimer[nLoop]) >= 300) )
        {
            /* 행 체크 DAP_MANAGER 세션 예외 */
            if(g_stThread_arg[nLoop].threadStepStatus == 99)
            {
                return 0;
            }

            WRITE_CRITICAL(CATEGORY_DEBUG,"Thread idx (%d) Is Hang Client Info : (%s) Msgcode : (%d) Step : (%d)",
                           nLoop,
                           g_stThread_arg[nLoop].cli_ip,
                           g_stThread_arg[nLoop].threadMsgCode,
                           g_stThread_arg[nLoop].threadStepStatus);

            g_ForkHangFlag = 0x03;
            return 1;
        }
    }

    return 0;
}


int	fpcif_PthreadMemoryReadLockCheck( int pre_sec )
{
    int		local_count = 0;

    while( 1 )
    {
        if( WrMutexFlag == 0x05 )
        {
            return 0;
        }

        if( local_count > pre_sec * 500 ){ break; }
        local_count++;

        fcom_SleepWait(4); // 0.01
    }
    return -1;
}

void fpcif_PthreadMemoryWriteLockBegin(void)
{
    WrMutexFlag = 0x03;
}
void fpcif_PthreadMemoryWriteLockEnd(void)
{
    WrMutexFlag = 0x05;
}


