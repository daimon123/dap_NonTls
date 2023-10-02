//
// Created by KimByoungGook on 2020-06-22.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#include "com/dap_com.h"
#include "secure/dap_SSLHelper.h"
#include "json/jansson.h"
#include "com/dap_req.h"
#include "sock/dap_sock.h"
#include "vwlog.h"


static void fstHandleSignal(int sid);
static int fstTailUtilN(char* svrIp, char* mngIp, int mngFq, char* filename );
static int fstSendTailData(char* p_svrIp, char* p_mngIp, int p_mngFq, char* p_fName, char* p_buf);
static int fstSendJsonToServer(SSL *ssl, int code, char *jsonData, int jsonSize);
static int fstSigHandler(void);
static void fstReloadCfgFile( );

int main(int argc, char **argv)
{
    struct 	sigaction act;
    _DAP_COMN_INFO* pstComnInfo = NULL;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    if (argc < 3)
    {
        printf("Process Exited argc : %d\n",argc);
        return 0;
    }

    fcom_ArgParse(argv);

    snprintf(g_stProcVwlogInfo.szServerIp, sizeof(g_stProcVwlogInfo.szServerIp),
             "%s",argv[1]);
    snprintf(g_stProcVwlogInfo.szManagerIp, sizeof(g_stProcVwlogInfo.szManagerIp),
             "%s",argv[2]);
    g_stProcVwlogInfo.nManagerFq = atoi(argv[3]);
    snprintf(g_stProcVwlogInfo.szFilePath, sizeof(g_stProcVwlogInfo.szFilePath),"%s",argv[4]);

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")      );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    fcom_SetIniName(pstComnInfo->szComConfigFile  );

    fcom_GetProfile("SSL","CERT_PATH",g_stServerInfo.stDapCertInfo.szCertPath,"/home/intent/dap/config/cert");
    fcom_GetProfile("SSL","CA_FILE",g_stServerInfo.stDapCertInfo.szCaFile,"root.crt");
    fcom_GetProfile("SSL","CERT_FILE",g_stServerInfo.stDapCertInfo.szCertFile,"server.crt");
    fcom_GetProfile("SSL","KEY_FILE",g_stServerInfo.stDapCertInfo.szKeyFile,"server.key");

    pstComnInfo->nCfgMgwId = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    g_stProcVwlogInfo.nCfgRetryAckCount = fcom_GetProfileInt("PCIF","ACK_RETRY_LIMIT_COUNT",3);
    g_stProcVwlogInfo.nCfgRetryAckSleep = fcom_GetProfileInt("PCIF","ACK_RETRY_WAIT_TIME", 100);
    g_stProcVwlogInfo.nCfgMaxSendPerSecond = fcom_GetProfileInt("VWLOG","MAX_SEND_PER_SECOND", 10);
    g_stProcVwlogInfo.nCfgSendDelayTime = fcom_GetProfileInt("VWLOG","SEND_DELAY_TIME", 1);

    fstSigHandler();

    fcom_LogInit(g_stProcVwlogInfo.szManagerIp);

    fstTailUtilN(g_stProcVwlogInfo.szServerIp,
                 g_stProcVwlogInfo.szManagerIp,
                 g_stProcVwlogInfo.nManagerFq,
                 g_stProcVwlogInfo.szFilePath);

    WRITE_CRITICAL(CATEGORY_DEBUG, "Try Process termination!");

    act.sa_handler = fstAlarmInterrupt;
    sigfillset(&(act.sa_mask));
    sigaction(SIGALRM, &act, NULL);
    alarm(3);

    alarm(0);

    WRITE_INFO(CATEGORY_DEBUG,"Process exit(0)!");

    return RET_SUCC;
}



int fstSendTailData(char* p_svrIp, char* p_mngIp, int p_mngFq, char* p_fName, char* p_buf)
{
    int     rxt;
    int     listenPort = 0;
    int     jsonSize = 0;
    char    caFullPath[296+1];
    char    certFullPath[286+1];
    char    keyFullPath[286+1];


    memset(caFullPath, 0x00, sizeof(caFullPath));
    memset(certFullPath, 0x00, sizeof(certFullPath));
    memset(keyFullPath, 0x00, sizeof(keyFullPath));

    snprintf(caFullPath, sizeof(caFullPath), "%s/%s",
            g_stServerInfo.stDapCertInfo.szCertPath,
            g_stServerInfo.stDapCertInfo.szCaFile);

    snprintf(certFullPath, sizeof(certFullPath), "%s/%s",
             g_stServerInfo.stDapCertInfo.szCertPath,
             g_stServerInfo.stDapCertInfo.szCertFile);

    snprintf(keyFullPath, sizeof(keyFullPath), "%s/%s",
             g_stServerInfo.stDapCertInfo.szCertPath,
             g_stServerInfo.stDapCertInfo.szKeyFile);

    listenPort = fcom_GetProfileInt( "PCIF", "LISTEN_PORT", 50203);


    // 1. connect (server -> forward server)
    SSL_Connection *sslcon = ssl_connect(   p_svrIp,
                                            listenPort,
                                            certFullPath,
                                            keyFullPath);
    if(sslcon == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in ssl connect to server(%s)",p_svrIp );
        return -1;
    }

    //show_certs_info(sslcon->ssl);
    // 2. make json
    json_t* root = json_pack("{s:i, s:s, s:i, s:s, s:s}", "server_id",
                             g_stServerInfo.stDapComnInfo.nCfgMgwId,
                             "manager_ip",p_mngIp,
                             "manager_fq",p_mngFq,
                             "file_name",p_fName,
                             "tail_data",p_buf);
    char* jsonData = json_dumps(root, JSON_INDENT(0));
    if( jsonData == NULL )
    {
        json_decref(root);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Json pack error" );
        return -1;
    }
    json_decref(root);
    jsonSize = strlen(jsonData);

    WRITE_INFO_JSON(p_mngIp,"jsonData(%s)",jsonData );
    // 3. send req json (server -> forward server)
    rxt = fstSendJsonToServer(sslcon->ssl, MANAGE_COLLECT_SERVER_LOGTAIL, jsonData, jsonSize);
    if( rxt < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"[REQ] Fail in send json to server(%s)code(%d)size(%d)",
                       p_svrIp,
                       MANAGE_COLLECT_SERVER_LOGTAIL,
                       jsonSize);

        fcom_MallocFree((void**)&jsonData);
        ssl_disconnect(sslcon);
        return -1;
    }

    fcom_MallocFree((void**)&jsonData);

    WRITE_INFO(CATEGORY_INFO,"[REQ] Succeed in send json to server(%s)code(%d)size(%d)",
                p_svrIp,
                MANAGE_COLLECT_SERVER_LOGTAIL,
                jsonSize);

    ssl_disconnect(sslcon);

    return 0;
}

int fstTailUtilN(char* svrIp, char* mngIp, int mngFq, char* filename )
{
    int		rxt;
    int 	fd;

    int 	result;
    int 	sendCnt = 0;
    int     nCurrMin;
    char 	buf[BUFSIZE];
    char* 	ret;
    time_t 	curTm, oldTm;
    struct 	stat fbuf;
    FILE*	fp;
    fd_set 	reads;

    oldTm = time(&oldTm);

    WRITE_INFO(CATEGORY_INFO,"fPath(%s) ",filename);

    if( (fp = fopen(filename, "r")) == NULL )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File open error, file(%s) ",filename );
        return -1;
    }

    if( stat(filename, &fbuf) < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File stat error, file(%s) ",filename );
        return -1;
    }

    fd = fileno(fp);

    FD_ZERO(&reads);
    FD_SET(fd, &reads);

    fseek(fp, 0, SEEK_END); // 파일의 맨 끝으로 커서 이동
    while(1)
    {
        nCurrMin = fcom_GetSysMinute();

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcVwlogInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }

        /* timeout 설정을 하지 않음 */
        result = select(fd+1, &reads, NULL, NULL, NULL);
        if( result )
        {
            if(FD_ISSET(fd, &reads))
            {
                if( stat(filename, &fbuf) < 0 )
                {
                    fclose(fp);
                    WRITE_CRITICAL(CATEGORY_DEBUG,"File stat error, file(%s)",filename );
                    return -1;
                }

                ret = fgets(buf, BUFSIZE, fp);
                if (ret != NULL)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"Send Tail Buffer : [%s] ",buf);

                    rxt = fstSendTailData(svrIp, mngIp, mngFq, filename, buf);
                    if( rxt < 0 )
                        break;
                    else
                        sendCnt++;

                    curTm = time(&curTm);
                    if( curTm-oldTm > 1 || sendCnt >= g_stProcVwlogInfo.nCfgMaxSendPerSecond )
                    {
                        oldTm = curTm;
                        sendCnt = 0;
                        fcom_Msleep(g_stProcVwlogInfo.nCfgSendDelayTime);
                    }
                }
            }
        }
        else
        {
            ret = NULL;
            fclose(fp);
            WRITE_INFO(CATEGORY_DEBUG,"File select, file(%s)",filename );
            return -1;
        }
        usleep(5000);
        sleep(2);
    } // while

    ret = NULL;
    fclose(fp);

    return 0;
}

static int fstSendJsonToServer(SSL *ssl, int code, char *jsonData, int jsonSize)
{
    int     rxt = 0;
    int     retryCnt = 0;
    char    *data = NULL;
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

    rxt = fsock_SslSocketSend(ssl, data, sizeof(CRhead)+jsonSize);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"[REQ] Retry, because fail in send, errno(%d)code(%d)",
                errno,
                code);

        retryCnt = 0;

        while(retryCnt < g_stProcVwlogInfo.nCfgRetryAckCount)
        {
            fcom_Msleep(g_stProcVwlogInfo.nCfgRetryAckSleep);
            rxt = fsock_SslSocketSend(ssl, data, sizeof(CRhead) + jsonSize);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == g_stProcVwlogInfo.nCfgRetryAckCount)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"[REQ] Fail in send, retry(%d)errno(%d)code(%d)",
                           g_stProcVwlogInfo.nCfgRetryAckCount,
                            errno,
                            code);

            fcom_MallocFree((void**)&data);
            return -1;
        }
    }

    fcom_MallocFree((void**)&data);

    return 0;
}

void fstAlarmInterrupt(int signo)
{
    WRITE_CRITICAL(CATEGORY_DEBUG,"%s Interruptted by alarm signal!!",g_stServerInfo.stDapComnInfo.szProcName);
//    exit_server(1);
    exit(signo);
}

static void fstHandleSignal(int sid)
{
    WRITE_CRITICAL(CATEGORY_DEBUG,"Exit program, received signal-[%d],pid-[%d]",
                   sid,
                   getpid());

    exit(sid);
}

static int fstSigHandler(void)
{
    signal( SIGCLD, SIG_IGN   );                 /* 18: child status change */
    signal( SIGWINCH, SIG_IGN );                 /* 20: window size change */
    signal( SIGHUP,    SIG_IGN        );   /* 1 : hangup */
    signal( SIGINT,    fstHandleSignal   );   /* 2 : interrupt (rubout) */
    signal( SIGQUIT,   fstHandleSignal   );   /* 3 : quit (ASCII FS) */
    signal( SIGILL,    fstHandleSignal   );   /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP,   fstHandleSignal   );   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT,    fstHandleSignal   );   /* 6 : IOT instruction */
    signal( SIGABRT,   fstHandleSignal   );   /* 6 : used by abort,replace SIGIOT in the future */
//    signal( SIGEMT,    fstHandleSignal   );   /* 7 : EMT instruction */
    signal( SIGFPE,    fstHandleSignal   );   /* 8 : floating point exception */
    signal( SIGKILL ,  fstHandleSignal   );   /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS ,   fstHandleSignal   );   /* 10: bus error */
    signal( SIGSEGV,   fstHandleSignal   );   /* 11: segmentation violation */
    signal( SIGSYS ,   fstHandleSignal   );   /* 12: bad argument to system call */
    signal( SIGPIPE,   fstHandleSignal   );   /* 13: write on a pipe with no one to read it */
    signal( SIGALRM,   fstHandleSignal   );   /* 14: alarm clock */
    signal( SIGTERM,   fstHandleSignal   );   /* 15: software termination signal from kill */
    signal( SIGUSR1,   fstHandleSignal   );   /* 16: user defined signal 1 */
    signal( SIGUSR2,   fstHandleSignal   );   /* 17: user defined signal 2 */
    signal( SIGPWR ,   fstHandleSignal   );   /* 19: power-fail restart */
    signal( SIGURG  ,  fstHandleSignal   );   /* 21: urgent socket condition */
    signal( SIGPOLL ,  fstHandleSignal   );   /* 22: pollable event occured */
    signal( SIGIO   ,  fstHandleSignal   );   /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP ,  fstHandleSignal   );   /* 23: stop (cannot be caught or ignored) */
    signal( SIGTTIN ,  fstHandleSignal   );   /* 26: background tty read attempted */
    signal( SIGTTOU ,  fstHandleSignal   );   /* 27: background tty write attempted */
    signal( SIGVTALRM, fstHandleSignal   );   /* 28: virtual timer expired */
    signal( SIGPROF ,  fstHandleSignal   );   /* 29: profiling timer expired */
    signal( SIGXCPU ,  fstHandleSignal   );   /* 30: exceeded cpu limit */
    signal( SIGXFSZ ,  fstHandleSignal   );   /* 31: exceeded file size limit */

    return RET_SUCC;
}


static void fstReloadCfgFile( )
{
    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}
