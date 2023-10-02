//
// Created by KimByoungGook on 2020-07-02.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <openssl/pem.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <sys/poll.h>

#include "com/dap_com.h"
#include "com/dap_req.h"
#include "sock/dap_sock.h"
#include "ipc/dap_Queue.h"

#include "proxy.h"
#include "dap_version.h"


int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fproxy_ProxyInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fproxy_MainTask();

    return RET_SUCC;

}

int fproxy_MainTask()
{
    char 	*proxyAddress,*serverAddress;
    struct sockaddr_in proxyMaster;

    struct sigaction saDeadChild;

    int     bufsize = 212992;
    int		reuseC = 1;
    int		cpId = 0;
    int     loop = 0;
    char    caFullPath[286 +1]          = {0x00,};
    char    certFullPath[286+1]         = {0x00,};
    char    keyFullPath[286+1]          = {0x00,};
    char    local_szPidPath[127 +1]     = {0x00,};
    char*   local_ptrProcessName        = NULL;

    memset((void*)&proxyMaster,0,sizeof(proxyMaster)); //clear proxyMaster address

    fsock_InitOpenssl();

    g_stProcProxyInfo.ctx = fsock_CreateContext();

    snprintf(caFullPath,    sizeof(caFullPath),     "%s/%s", g_stProcProxyInfo.certPath, g_stProcProxyInfo.caFile);
    snprintf(certFullPath,  sizeof(certFullPath),   "%s/%s", g_stProcProxyInfo.certPath, g_stProcProxyInfo.certFile);
    snprintf(keyFullPath,   sizeof(keyFullPath),    "%s/%s", g_stProcProxyInfo.certPath, g_stProcProxyInfo.keyFile);

    fsock_ConfigureContext(g_stProcProxyInfo.ctx, certFullPath, keyFullPath);

    proxyMaster.sin_family      =   AF_INET;
    proxyMaster.sin_addr.s_addr =   htonl(INADDR_ANY);
    proxyMaster.sin_port        =   htons(atoi(g_stProcProxyInfo.cfgListenPort));
    proxyAddress                =   fcom_StringAllocExing(STRING_LEN);

    fsock_InetNtop(AF_INET,&proxyMaster.sin_addr,proxyAddress,STRING_LEN);

    //setting print
    WRITE_INFO(CATEGORY_INFO, "-----------------------------");
    WRITE_INFO(CATEGORY_INFO, "- PROXY parameter:");
    WRITE_INFO(CATEGORY_INFO, "- \tIP Address: %s",proxyAddress);
    WRITE_INFO(CATEGORY_INFO, "- \tListening port: %d",ntohs(proxyMaster.sin_port));
    WRITE_INFO(CATEGORY_INFO, "-----------------------------");

    free(proxyAddress); //after this point it SHOULD BE useless

    /**SERVER SETTINGS**/
    memset((void*)&server,0,sizeof(server)); //clear server address

    server.sin_family   =   AF_INET;
    fsock_InetPton(AF_INET,g_stProcProxyInfo.cfgDbIp,&(server.sin_addr));
    server.sin_port     =   htons(g_stProcProxyInfo.cfgDbPort);
    serverAddress       =   fcom_StringAllocExing(STRING_LEN);
    fsock_InetNtop(AF_INET,&server.sin_addr,serverAddress,STRING_LEN); //just to verify

    //setting print
    WRITE_INFO(CATEGORY_INFO, "-----------------------------");
    WRITE_INFO(CATEGORY_INFO, "- SERVER parameter:");
    WRITE_INFO(CATEGORY_INFO, "- \tIP Address: %s",serverAddress);
    WRITE_INFO(CATEGORY_INFO, "- \tListening port: %d",ntohs(server.sin_port));
    WRITE_INFO(CATEGORY_INFO, "-----------------------------");

    free(serverAddress); //after this point it SHOULD BE useless

    /**SOCKET CREATION**/
    g_listenSock = fsock_Socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(g_listenSock < 0)
    {
        exit((-1));
    }

    /**SETSOCKOPT SO_REUSEADDR**/
    if(setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, &reuseC,sizeof(reuseC)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option. ");
        return((-1));
    }

    if(setsockopt(g_listenSock,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        return((-1));
    }
    if(setsockopt(g_listenSock,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option. ");
        return((-1));
    }

    /**BINDING**/
    if(fsock_Bind(g_listenSock,(struct sockaddr*)&proxyMaster,sizeof(proxyMaster)) != 0)
    {
        exit((-1));
    }

    /**LISTEN**/
    if(fsock_Listen(g_listenSock,BACKLOG_QUEUE) < 0)
    {
        exit((-1));
    }

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PROXY);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    /**SETTING HANDLER FOR SIGCHLD**/
    saDeadChild.sa_handler = fproxy_SigchldHandler; // reap all dead processes
    sigemptyset(&saDeadChild.sa_mask);
    saDeadChild.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saDeadChild, NULL)==-1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit((-1));
    }

    /** SETTING HANDLER FOR SIGINT (ONLY FATHER) **/
    saSignInt.sa_handler = fproxy_SigHandler;
    sigemptyset(&saSignInt.sa_mask);
    saSignInt.sa_flags = 0;
    if(sigaction(SIGINT|SIGTERM|SIGSEGV, &saSignInt, NULL)==-1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit((-1));
    }

    /**SETTING HANDLER FOR SIGPIPE**/
    saSignPipe.sa_handler=SIG_IGN;
    sigemptyset(&saSignPipe.sa_mask);
    saSignPipe.sa_flags = SA_RESTART;
    if (sigaction(SIGPIPE, &saSignPipe, NULL) == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit((-1));
    }

    g_ParentPid = getpid();

    if(fcom_malloc((void**)&g_ChildPid,sizeof(int) * g_stProcProxyInfo.cfgForkCnt ) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    g_ForkExit = SUCCESS;

    WRITE_INFO(CATEGORY_INFO,"cfgForkCnt(%d)", g_stProcProxyInfo.cfgForkCnt);
    for(loop=0; loop < g_stProcProxyInfo.cfgForkCnt; loop++)
    {
        if(g_numChildren > g_stProcProxyInfo.cfgForkCnt )
        {
            WRITE_INFO(CATEGORY_DEBUG,"Max Child Process : [%d]", g_numChildren);
            break;
        }

        if((cpId = fork()) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fork ");
            continue;
        }
        else if(cpId > 0) // parent process 부모를 죽일필요있을 때만
        {
            g_numChildren++;
            g_ChildPid[loop] = cpId;
            continue;
        }
        else if(cpId == 0) // child process
        {
            fproxy_ForkWork();
            WRITE_DEBUG(CATEGORY_DEBUG,"Exit ");
            exit(0); //child이므로 걍 종료

            break;
        }
    }

    fstSendLink();
    ERR_free_strings();

    return RET_SUCC;

}


int fproxy_ProxyInit(void)
{
    char                local_szTmp[255 +1]         = {0x00,};
    char                local_szDefaultIP[15 +1]    = {0x00,};
    _DAP_COMN_INFO*     pstComnInfo = NULL;
    _DAP_QUEUE_INFO*    pstQueueInfo = NULL;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    fcom_GetProfile("PROXY","CERT_PATH",g_stProcProxyInfo.certPath,"/home/intent/dap/config/cert");
    fcom_GetProfile("PROXY","CA_FILE",g_stProcProxyInfo.caFile,"root.crt");
    fcom_GetProfile("PROXY","CERT_FILE",g_stProcProxyInfo.keyFile,"broker.pem");
    fcom_GetProfile("PROXY","KEY_FILE",g_stProcProxyInfo.certFile,"broker.pem");

    fcom_GetProfile("PROXY","LISTEN_PORT",g_stProcProxyInfo.cfgListenPort,"50207");
    fcom_GetProfile("PROXY","TARGET_IP",g_stProcProxyInfo.cfgDbIp,"127.0.0.1");

    fsock_GetNic(local_szTmp);
    fsock_GetIpAddress(local_szTmp, local_szDefaultIP);
    fcom_GetProfile("COMMON", "SERVER_IP", g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIP );

    g_stProcProxyInfo.cfgThreadCnt      = fcom_GetProfileInt("PROXY","THREAD_COUNT",5);
    g_stProcProxyInfo.cfgForkCnt        = fcom_GetProfileInt("PROXY","FORK_COUNT", 1);
    g_stProcProxyInfo.cfgDbPort         = fcom_GetProfileInt("PROXY","TARGET_PORT",3306);
    g_stProcProxyInfo.cfgPrmonInterval  = fcom_GetProfileInt("PRMON","INTERVAL",600);

    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ", pstComnInfo->szDapHome);

    /* 프로세스 종료할 경우에 자원해제중에 Fork 방지 플래그.*/
    g_signalFlag = FALSE;

//    /* Master Process PID 저장 */
//    sprintf(g_stServerInfo.stDapComnInfo.szPidPath,"%s%s",g_stServerInfo.stDapComnInfo.szDapHome,"/bin/var");
//    if(access(g_stServerInfo.stDapComnInfo.szPidPath, W_OK) != 0 )
//    {
//        if (mkdir(g_stServerInfo.stDapComnInfo.szPidPath,S_IRWXU|S_IRWXG|S_IRWXO) != 0)
//        {
//            printf("Fail in Pid File directory(%s)|%s\n",
//                            g_stServerInfo.stDapComnInfo.szPidPath,
//                           __func__ );
//            return 0;
//        }
//    }

    if (access(pstQueueInfo->szDAPQueueHome,R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",
                   pstQueueInfo->szDAPQueueHome,__func__);
        }
    }

    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
             "%s/PRMONQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n",
               pstQueueInfo->szPrmonQueueHome,__func__);
    }


    snprintf(pstQueueInfo->szDblogQueueHome,
            sizeof(pstQueueInfo->szDblogQueueHome),
            "%s/DBLOGQ",
            pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n",
               pstQueueInfo->szDblogQueueHome,__func__);
    }

    memset(local_szTmp, 0x00, sizeof(local_szTmp));
    snprintf(local_szTmp, sizeof(local_szTmp), "%s/%s/%s",pstComnInfo->szDapHome,"log","stdout.log");

    /* 표준출력(1) 표준오류(2) 리다이렉션 처리 */
    fcom_InitStdLog(local_szTmp);

    printf("Succeed in init, pid(%d) |%s\n", getpid(),__func__);
    return TRUE;


}


int fstSendLink()
{
    int nCurrMin = 0;

    while(1)
    {
        g_stProcProxyInfo.ing_job_time = time(NULL);

        if((g_stProcProxyInfo.ing_job_time % g_stProcProxyInfo.cfgPrmonInterval) == 0) //foreach 1min
        {
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcProxyInfo.last_send_time);
        }

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcProxyInfo.nConfigLastModify) == 1)
            {
                fproxy_ReloadCfgFile();
            }

        }
        sleep(1);
    }

    return 0;
}

int fstPreAuthSSL(SSL *ssl)
{
    int 	rxt = 0;
    int		msgLeng = 0;
    int		msgCode = 0;
    char	msgType[10 +1] = {0x00,};
    CRhead RHead;

    memset((void *)&RHead, 0x00, sizeof(RHead));

#ifdef 	USE_OPENSSL
    rxt = SSL_read(ssl,(char *)&RHead,sizeof(RHead));
#else
    rxt = fsock_Recv(csock,(char *)&RHead,sizeof(RHead),0);
#endif
    if (rxt == 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Connection closed by client, errno(%d)pid(%d)",
                       errno,getpid());
        return 0;
    }
    else if(rxt < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket receive, errno(%d) pid(%d)",
                       errno,getpid());
        return -1;
    }
    else if(rxt != sizeof(RHead))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match header size, errno(%d) pid(%d)",
                       errno,getpid());
        return -2;
    }

    memset(msgType, 0x00, sizeof(msgType));
    strcpy(msgType, RHead.msgtype);
    msgLeng = ntohl(RHead.msgleng);
    msgCode = ntohs(RHead.msgcode);

    if(memcmp(RHead.msgtype, MSG_CP_TYPE, 9) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match msg type, errno(%d) pid(%d)",
                       errno,getpid());
        return -3;
    }

    if(msgCode != 110)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match msg code, errno(%d) pid(%d)",
                       errno,getpid());
        return -4;
    }

    return msgLeng;
}


int fstPreAuth(int sock)
{
    int 	rxt = 0;
    int		msgLeng = 0;
    int		msgCode = 0;
    char	msgType[10 +1] = {0x00,};
    CRhead RHead;

    memset((void *)&RHead, 0x00, sizeof(RHead));

    rxt = fsock_Recv(sock,(char *)&RHead,sizeof(RHead),0);
    if (rxt == 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Connection closed by client, errno(%d)pid(%d)",
                errno,getpid());
        return 0;
    }
    else if(rxt < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket receive, errno(%d) pid(%d)",
                errno,getpid());
        return -1;
    }
    else if(rxt != sizeof(RHead))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match header size, errno(%d) pid(%d)",
                errno,getpid());
        return -2;
    }

    memset(msgType, 0x00, sizeof(msgType));
    strcpy(msgType, RHead.msgtype);
    msgLeng = ntohl(RHead.msgleng);
    msgCode = ntohs(RHead.msgcode);

    if(memcmp(RHead.msgtype, MSG_CP_TYPE, 9) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match msg type, errno(%d) pid(%d)",
                errno,getpid());
        return -3;
    }

    if(msgCode != 110)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Not match msg code, errno(%d) pid(%d)",
                errno,getpid());
        return -4;
    }

    return msgLeng;
}


void fproxy_ReloadCfgFile( )
{
    g_stProcProxyInfo.cfgPrmonInterval             = fcom_GetProfileInt("PRMON","INTERVAL", 600);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcProxyInfo.cfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);
}