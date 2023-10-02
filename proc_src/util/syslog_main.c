//
// Created by KimByoungGook on 2021-11-23.
//


#include "syslog_mon.h"


int fsock_ConnectNonBlock(int param_nSocket, struct sockaddr_in* param_stptrServerAddress, socklen_t param_nAddrlen, int param_nTimesec)
{
    int         local_SockFlag = 0;
    int         local_nRet = 0;
    char        local_optVal = 1;
    int		    local_error = 0, local_len = 0;
    fd_set	    local_ReadSet;
    fd_set	    local_WriteSet;
    socklen_t   local_OptLen;
    struct		timeval		local_stTval;

    memset( &local_stTval, 0x00, sizeof(struct timeval) );

    local_OptLen = sizeof(local_optVal);

    setsockopt( param_nSocket, SOL_SOCKET, SO_KEEPALIVE, (char *) &local_optVal, local_OptLen );

    local_SockFlag = fcntl( param_nSocket, F_GETFL, 0 );
    if(local_SockFlag < 0)
    {
        WRITE_LOG("Socket Flag Is Invalid");

        return -1;
    }

    if(fcntl( param_nSocket, F_SETFL, local_SockFlag | O_NONBLOCK ) < 0)
    {
        WRITE_LOG("Socket NONBLOCK Set Failed");

        return -1;
    }

    local_nRet = connect(param_nSocket, (struct sockaddr*)param_stptrServerAddress, param_nAddrlen);
    if ( local_nRet < 0)
    {
        if ( errno == EINPROGRESS )
        {
            fcom_SleepWait(4);

            int local_nCheckCnt = 0;

            while(1)
            {
                FD_ZERO(&local_ReadSet  );
                FD_SET( param_nSocket, &local_ReadSet );

                local_WriteSet = local_ReadSet;

                if( param_nTimesec != 0 )
                {
                    local_stTval.tv_sec  = param_nTimesec;
                    local_stTval.tv_usec = 0;
                }
                else
                {
                    local_stTval.tv_sec  = 0;
                    local_stTval.tv_usec = 250000; // 0.25
                }

                local_nRet = select( param_nSocket + 1, &local_ReadSet, &local_WriteSet, NULL, &local_stTval);
                if (local_nRet < 0 && errno != EINTR)
                {
                    local_SockFlag = fcntl(param_nSocket, F_GETFL, 0);
                    if (local_SockFlag < 0)
                    {
                        WRITE_LOG("fcntl: F_GETFL Fail");
                        return (-1);
                    }
                    /* Block Mode 변경 */
                    if (fcntl(param_nSocket, F_SETFL, local_SockFlag & (~O_NONBLOCK)) < 0)
                    {
                        WRITE_LOG("fcntl: F_SETFL Fail");
                        return (-1);
                    }

                    return (-1);
                }
                else // >= 0
                {
                    if(FD_ISSET(param_nSocket, &local_WriteSet) || FD_ISSET(param_nSocket, &local_ReadSet))
                    {
                        local_SockFlag = fcntl(param_nSocket, F_GETFL, 0);
                        if (local_SockFlag < 0)
                        {
                            WRITE_LOG("fcntl: F_GETFL Fail");
                            return (-1);
                        }
                        /* Block Mode 변경 */
                        if (fcntl(param_nSocket, F_SETFL, local_SockFlag & (~O_NONBLOCK)) != 0)
                        {
                            WRITE_LOG("fcntl: F_SETFL Fail");
                            return (-1);
                        }
                        /* Socket error 없는지 한번 더 검사. */
                        if( getsockopt( param_nSocket , SOL_SOCKET , SO_ERROR , &local_error ,(socklen_t *) &local_len ) < 0 )
                        {
                            WRITE_LOG("Get Sockopt Error ");
                            return (-1);
                        }

                        /* connect Success */
//                        WRITE_DEBUG("Connect non-block success ");

                        return 0;
                    }
                    else if( local_nRet == 0) /* timeout */
                    {
                        local_nCheckCnt++;

                        if(local_nCheckCnt > 3)
                        {
                            fcntl( param_nSocket , F_SETFL, local_SockFlag );

                            return (-1);
                        }
                        else
                            continue;
                    }
                }
            }
        }
        else
        {
            WRITE_LOG("Conect Error ");
            local_SockFlag = fcntl(param_nSocket, F_GETFL, 0);
            if (local_SockFlag < 0)
            {
                WRITE_LOG("fcntl: F_GETFL Fail");
                return (-1);
            }
            /* Block Mode 변경 */
            if (fcntl(param_nSocket, F_SETFL, local_SockFlag & (~O_NONBLOCK)) < 0)
            {
                WRITE_LOG("fcntl: F_SETFL Fail");
                return (-1);
            }

            return (-1);
        }
    }

    local_SockFlag = fcntl(param_nSocket, F_GETFL, 0);
    if (local_SockFlag < 0)
    {
        WRITE_LOG("fcntl: F_GETFL Fail");
        return (-1);
    }
    /* Block Mode 변경 */
    if (fcntl(param_nSocket, F_SETFL, local_SockFlag & (~O_NONBLOCK)) < 0)
    {
        WRITE_LOG("fcntl: F_SETFL Fail");
        return (-1);
    }

    return 0;

}

void fsyslog_EventMsgOut(int param_EventType, char* param_EventMsg, int param_EventMsgSize, _THREAD_ARG* param_ThreadArg)
{
    switch (param_EventType)
    {
        case MAIN_BOARD: //1
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "메인보드",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case SYSTEM: //2
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "시스템",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case OPERATING_SYSTEM: //3
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "운영체제",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case CPU: //4
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "CPU",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_ADAPTER: //5
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크어댑터",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case WIFI: //6
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "와이파이",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case BLUETOOTH: //7
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "블루투스",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_CONNECTION: //8
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크연결",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK: //9
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "디스크",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_DRIVE: //10
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크드라이브",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case OS_ACCOUNT: //11
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "운영체제계정",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case SHARE_FOLDER: //12
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "공유폴더",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case INFRARED_DEVICE: //13
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "적외선장치",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case PROCESS: //14
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "프로세스",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case ROUTER: //15
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "공유기",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_PRINTER: //16
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크프린터",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_SCAN: //17
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크스캔",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case ARP: //18
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "ARP",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case VIRTUAL_MACHINE: //19
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "가상머신",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }


        case CONNECT_EXT_SVR: //20
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "외부네트워크",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }


        case NET_ADAPTER_OVER: //21
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "네트워크어댑터2개이상",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }


        case NET_ADAPTER_DUPIP: //22
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "IP주소중복",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }


        case NET_ADAPTER_DUPMAC: //23
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "MAC주소중복",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_REG: //24
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "미등록디스크",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_HIDDEN: //25
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "디스크숨김",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_NEW: //26
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "새로운디스크",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_MOBILE: //27
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "이동식디스크",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_MOBILE_READ: //28
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "디스크읽기권한",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case DISK_MOBILE_WRITE: //29
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "디스크쓰기권한",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case PROCESS_WHITE: //30
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "화이트리스트 프로세스",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case PROCESS_BLACK: //31
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "블랙리스트 프로세스",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case PROCESS_ACCESSMON: //32
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "접속감시대상 프로세스",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case NET_ADAPTER_MULIP: //33
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "IP주소 다중설정탐지",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case EXTERNAL_CONN: //34
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "외부접속탐지",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case WIN_DRV: //35
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "윈도우드라이버장치",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case RDP_SESSION: //36
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "원격데스크톱연결탐지",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case SSO_CERT: //37
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "SSO인증여부",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case PROCESS_BLOCK: //38
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "프로세스차단",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case CPU_USAGE_ALARM: //39
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "CPU사용률알람",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        case CPU_USAGE_CONTROL: //40
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "CPU사용통제",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }

        default: //41
        {
            snprintf(param_EventMsg, param_EventMsgSize, "%s %s",
                     "알수없음",
                     fsyslog_GetEventStr(param_ThreadArg->stCommInfo.szEvent));
            break;
        }
    }
}

void fsyslog_PrintOut(void)
{
    printf("Usage syslog_mon dap/fi/ni \n");
    printf("dap : Detect Abnormal Processing \n");
    printf("fi : FileInspector \n");
    printf("ni : NetInspector \n");

    return;
}

char* fsyslog_GetEventStr(char* param_Event)
{
    if ( param_Event[0] == 'D') //DETECT
    {
        return "탐지";
    }
    else if ( param_Event[0] == 'B') //BLOCK
    {
        return "차단";
    }

    return "DAP";
}


int fsyslog_LogInit(char* param_LogPath)
{
    int    nRet = 0;
    struct tm stTm;
    time_t tTime;
    tTime = time(NULL);

    memset(&stTm, 0x00, sizeof(struct tm));
    localtime_r(&tTime,&stTm);

    // Path는 파일명까지 들어가야 함.
    snprintf(g_szLogPath, sizeof(g_szLogPath), "%s", param_LogPath);

    printf("Log Path : [%s] \n",g_szLogPath);

    if(access(g_szLogPath, F_OK) != 0)
    {
        if((nRet = fcom_MkPath(g_szLogPath, 0755)) != RET_SUCC)
        {
            return RET_FAIL;
        }
    }
    return RET_SUCC;
}

int fsyslog_GetLogTime(char *timebuf)
{
    struct timeb t;
    struct tm timeinfo;
    int len;

    ftime(&t);

    localtime_r(&t.time, &timeinfo);

    len = sprintf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d.%03u",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec,
                  t.millitm);

    return len;
}

int fsyslog_GetSendTime(char *timebuf)
{
    struct timeb t;
    struct tm timeinfo;
    int len;

    ftime(&t);

    localtime_r(&t.time, &timeinfo);

    len = sprintf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec);

    return len;
}

void fsyslog_LogWrite(const char *fmt, ...)
{
    char szVarLog [1024 +1] = {0x00,};
    char szLogBuf [2048 +1] = {0x00,};
    char szTimeBuf[  24 +1] = {0x00,};
    va_list     args;

    FILE* fp = NULL;

    fp = fopen(g_szLogPath,"a, ccs=UTF-8");
    if(fp == NULL)
    {
        perror("fcom_LogWrite Is NULL! :");
        return;
    }

    va_start(args,fmt);
    /* Log Formatting */
    /* [Time][ProcessID:ThreadID][LogLevel][Category][Message]  */
    fsyslog_GetLogTime(szTimeBuf);
    vsnprintf(szVarLog, sizeof(szVarLog) -1, fmt, args);
    va_end(args);


    snprintf(szLogBuf, sizeof(szLogBuf) -1,
             "[%-24.24s][%d:%lu][%.*s]",
             szTimeBuf,
             (int)getpid() ,
             (unsigned long)pthread_self(),
             (int)strlen(szVarLog),
             szVarLog       );

    if(fp != NULL)
    {
        fprintf(fp,"%s\n",szLogBuf);
        fclose(fp);
    }

    return;

}

int fsyslog_UdpGetData(_THREAD_ARG* param_ThreadArg)
{
    int local_nRet      = 0;
    int local_nLocalFd  = 0;
    int local_nCliLen   = sizeof(struct sockaddr_un);
    struct sockaddr_un  local_stClientAddr;
    struct pollfd       local_stFds;

    memset(&local_stClientAddr, 0x00, sizeof(struct sockaddr_un));
    memset(&local_stFds,        0x00, sizeof(struct pollfd)     );

    local_nLocalFd = g_UdsSockFd;

    local_stFds.fd = local_nLocalFd;
    local_stFds.events = POLLIN;

    local_nRet = poll(&local_stFds, 1, 2000);
    if (local_nRet == 0) // timeout
    {
        return 0;
    }
    else if (local_nRet < 0) // error
    {
        return local_nRet;
    }

    local_nRet = recvfrom(local_nLocalFd, (void *)&param_ThreadArg->stCommInfo, sizeof(param_ThreadArg->stCommInfo),
                          0, (struct sockaddr *)&local_stClientAddr, (socklen_t *)&local_nCliLen);

    param_ThreadArg->nReadSize = local_nRet; //recv size

    return local_nRet;
}

int fsyslog_ArgParse(int argc, char **argv)
{
    char* cptrProcName    = NULL;
    char* cptrSep         = NULL;

    cptrSep       = NULL;
    cptrProcName  = NULL;

    if( (cptrSep = strrchr(argv[0], '/') ) != NULL )
    {
        cptrProcName = (cptrSep + 1);
    }
    else
    {
        cptrProcName = argv[0];
    }

    if ( argc < 2 )
    {
        fsyslog_PrintOut();
        exit(0);
    }

    printf("cptrProcName : [%s] [%s]\n", cptrProcName, argv[1]);

    snprintf(g_szProcessName, sizeof(g_szProcessName), "%s",  cptrProcName);


    if ( strcmp( argv[1], "dap") == 0 )
    {
        g_cProdType = PRODUCT_DAP;
    }

    else if ( strcmp( argv[1], "fi") == 0 )
    {
        g_cProdType = PRODUCT_FI;
    }

    else if ( strcmp( argv[1], "ni") == 0 )
    {
        g_cProdType = PRODUCT_NI;
    }

    else
    {
        fsyslog_PrintOut();
        exit(0);
    }

    return 0;

}

pid_t fsyslog_SetDeamon(void)
{
    pid_t local_pid = 0;

    /** Parent Process **/
    if((local_pid = fork()) > 0)
    {
        printf("[ START ] \n");
        exit(0);
    }
    else
    {
        /** Child Process **/
        chdir("/");
        close(0); // or dup(0) or dup2(0,STDOUT_FILENO)
        close(1); // or dup(1) or dup2(1,STDOUT_FILENO)
        close(2); // or dup(2) or dup2(2,STDOUT_FILENO)
        open("/dev/null", O_RDONLY); /* 0 */
        open("/dev/null", O_WRONLY); /* 1 */
        open("/dev/null", O_WRONLY); /* 2 */

        return 0;
    }

    return local_pid;
}

void fsyslog_Init(void)
{
    char local_szHome[256]      = {0x00,};
    char local_szLogHome[256]   = {0x00,};
    char local_szCfgHome[256]   = {0x00,};
    char local_szPidPath[256]   = {0x00,};

    switch(g_cProdType)
    {
        case PRODUCT_DAP:
        {
            snprintf(local_szHome, sizeof(local_szHome), "%s", getenv("DAP_HOME"));
            if ( strlen(local_szHome) <= 0)
            {
                snprintf(local_szHome, sizeof(local_szHome), "%s/%s", getenv("HOME"), "dap");
            }
            snprintf(local_szCfgHome, sizeof(local_szCfgHome), "%s/%s/%s", local_szHome, "config", DF_CONFIG_FILE_NAME);
            break;
        }

        case PRODUCT_FI:
        {
            snprintf(local_szHome, sizeof(local_szHome), "%s/%s", getenv("HOME"), "file-inspector");
            if ( strlen(local_szHome) <= 0)
            {
                snprintf(local_szHome, sizeof(local_szHome), "%s/%s", getenv("HOME"), "file-inspector");
            }
            snprintf(local_szCfgHome, sizeof(local_szCfgHome), "%s/%s/%s", local_szHome, "cnf", DF_CONFIG_FILE_NAME);
            break;
        }

        case PRODUCT_NI:
        {
            snprintf(local_szHome, sizeof(local_szHome), "%s", getenv("NI_HOME"));
            if ( strlen(local_szHome) <= 0)
            {
                snprintf(local_szHome, sizeof(local_szHome), "%s/%s", getenv("HOME"), "NI");
            }
            snprintf(local_szCfgHome, sizeof(local_szCfgHome), "%s/%s/%s", local_szHome, "config", DF_CONFIG_FILE_NAME);
            break;
        }
    }
    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", local_szHome);

    printf("Process Home : [%s] cfg home : [%s] pid : [%s] \n",local_szHome, local_szCfgHome, local_szPidPath);



    //Load
    fcom_SetIniName(local_szCfgHome);

    //Get Config
    fcom_GetProfile("EVENT_LOGGING","SERVER_IP"   ,g_stCfgInfo.szServerIp    ,"");
    fcom_GetProfile("EVENT_LOGGING","SERVER_PORT" ,g_stCfgInfo.szServerPort  ,"");
    fcom_GetProfile("EVENT_LOGGING","PROTOCOL"    ,g_stCfgInfo.szProtocol    ,"");
    fcom_GetProfile("EVENT_LOGGING","TOKEN_STR"   ,g_stCfgInfo.szTokenStr    ,"");

    printf("[%s] [%s] [%s] [%s] \n",
           g_stCfgInfo.szServerIp, g_stCfgInfo.szServerPort, g_stCfgInfo.szProtocol, g_stCfgInfo.szTokenStr);

    if ( g_stCfgInfo.szServerIp[0]    ==  0x00 ||
         g_stCfgInfo.szServerPort[0]  ==  0x00 ||
         g_stCfgInfo.szProtocol[0]    ==  0x00 ||
         g_stCfgInfo.szTokenStr[0]    ==  0x00  )
    {

        fsyslog_PrintOut();
        exit(0);
    }
    snprintf(g_szProcessHome, sizeof(g_szProcessHome), "%s", local_szHome);

    // Log Init
    snprintf(local_szLogHome, sizeof(local_szLogHome), "%s/syslog/%s.log",g_szProcessHome, g_szProcessName);
    fsyslog_LogInit(local_szLogHome);

    if ( strcasecmp(g_stCfgInfo.szProtocol,"udp") == 0 )
    {
        g_cProtocolType = PROTOCOL_UDP;
    }
    else if( strcasecmp(g_stCfgInfo.szProtocol, "tcp") == 0 )
    {
        g_cProtocolType = PROTOCOL_TCP;
    }
    else if( strcasecmp(g_stCfgInfo.szProtocol, "syslog") == 0 )
    {
        g_cProtocolType = PROTOCOL_SYSLOG;
    }
    else
    {
        WRITE_LOG("Invalid Config Protocol Type (%s)", g_stCfgInfo.szProtocol);
        exit(0);
    }

    // Iface name
    if ( fsyslog_GetNic(g_szIface) < 0)
    {
        snprintf(g_szMyIp, sizeof(g_szMyIp), "%s", "UNKNOWN");
    }
    fsyslog_GetIp(g_szIface, g_szMyIp);


    fcom_GetProfile("EVENT_LOGGING","MSG_CHARSET", g_szCfgCharset,"UTF-8");
    if ( strcmp(g_szCfgCharset, "UTF-8") != 0) //cfg charset이 UTF-8이 아니면
    {
        // iconv open
        WRITE_LOG("iconv open [%s] -> [%s]", "UTF-8", g_szCfgCharset);
        g_iconv_t = iconv_open(g_szCfgCharset, "UTF-8");
    }

    // Daemonize
    fsyslog_SetDeamon();

    // Make Master Pid File
    if (fFile_CheckMasterPid(local_szPidPath, "syslog_mon") == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, "syslog_mon");
        exit(0);
    }

    // Make Master Pid File
    fFile_MakeMasterPid(local_szPidPath, "syslog_mon");

    return;

}


void fsyslog_HandleSignal(int sid)
{
    if(sid == SIGALRM)
    {
        return ;
    }

    iconv_close(g_iconv_t);

    WRITE_LOG("Exit signal(%d)pid(%d)ppid(%d)", sid,getpid(),getppid());

    exit(0);

}

int fsyslog_SignalInit(void)
{
    signal( SIGHUP, fsyslog_HandleSignal);    /* 1 : hangup */
    signal( SIGINT, fsyslog_HandleSignal);    /* 2 : interrupt (rubout) */
    signal( SIGQUIT, fsyslog_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal( SIGILL, fsyslog_HandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP, fsyslog_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT, fsyslog_HandleSignal);    /* 6 : IOT instruction */
    signal( SIGABRT, fsyslog_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal( SIGFPE, fsyslog_HandleSignal);    /* 8 : floating point exception */
    signal( SIGKILL , fsyslog_HandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS , fsyslog_HandleSignal);   /* 10: bus error */
    signal( SIGSEGV, fsyslog_HandleSignal);   /* 11: segmentation violation */
    signal( SIGSYS , fsyslog_HandleSignal);   /* 12: bad argument to system call */
    signal( SIGPIPE, SIG_IGN );              /* 13: write on a pipe with no one to read it */

    signal( SIGALRM, fsyslog_HandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fsyslog_HandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fsyslog_HandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fsyslog_HandleSignal);   /* 17: user defined signal 2 */
    signal( SIGCLD,SIG_IGN );              /* 18: child status change */
    signal( SIGCHLD,SIG_IGN );             /* 18: child status change alias (POSIX) */
    signal( SIGPWR , fsyslog_HandleSignal);   /* 19: power-fail restart */
    signal( SIGWINCH, SIG_IGN );            /* 20: window size change */
    signal( SIGURG  , fsyslog_HandleSignal);  /* 21: urgent socket condition */
    signal( SIGPOLL , fsyslog_HandleSignal);  /* 22: pollable event occured */
    signal( SIGIO   , fsyslog_HandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP , fsyslog_HandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal( SIGTSTP , fsyslog_HandleSignal);  /* 24: user stop requested from tty */
    signal( SIGCONT , fsyslog_HandleSignal);  /* 25: stopped process has been continued */
    signal( SIGTTIN , fsyslog_HandleSignal);  /* 26: background tty read attempted */
    signal( SIGTTOU , fsyslog_HandleSignal);  /* 27: background tty write attempted */
    signal( SIGVTALRM , fsyslog_HandleSignal);/* 28: virtual timer expired */
    signal( SIGPROF , fsyslog_HandleSignal);  /* 29: profiling timer expired */
    signal( SIGXCPU , fsyslog_HandleSignal);  /* 30: exceeded cpu limit */
    signal( SIGXFSZ , fsyslog_HandleSignal);  /* 31: exceeded file size limit */

    return 0;
}

int fsyslog_UnixSockInit(void)
{
    char local_UdsFileName[31 +1] = {0x00,};

    snprintf(local_UdsFileName, sizeof(local_UdsFileName), "%s", "SYSLOG");

    switch (g_cProdType)
    {
        case PRODUCT_DAP:
        {
            snprintf(g_szUnixSockPath, sizeof(g_szUnixSockPath), "%s/%s/%s", g_szProcessHome, ".DAPQ",local_UdsFileName );
            break;
        }

        case PRODUCT_FI:
        {
            snprintf(g_szUnixSockPath, sizeof(g_szUnixSockPath), "%s/%s", g_szProcessHome,local_UdsFileName );
            break;
        }

        case PRODUCT_NI:
        {
            snprintf(g_szUnixSockPath, sizeof(g_szUnixSockPath), "%s/%s/%s", g_szProcessHome, "UDS/SERVER",local_UdsFileName );
            break;
        }
    }

    if( (g_UdsSockFd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        WRITE_LOG("UDP UDS Socket Create Fail (%d:%s)",errno, strerror(errno));
        return (-1);
    }

    if(access(g_szUnixSockPath, F_OK) != 0)
    {
        fcom_MkPath(g_szUnixSockPath, 0755);
    }

    unlink(g_szUnixSockPath);

    bzero(&g_stUdsSockAddr, sizeof(struct sockaddr_un));
    g_stUdsSockAddr.sun_family = AF_UNIX;
    strcpy(g_stUdsSockAddr.sun_path, g_szUnixSockPath);

    if( bind(g_UdsSockFd, (struct sockaddr *)&g_stUdsSockAddr, sizeof(struct sockaddr_un)) < 0)
    {
        WRITE_LOG("Udp UDS Bind Error (%d:%s)",errno, strerror(errno));
        return (-1);
    }

    WRITE_LOG("Unix Doamin Socket UDP Conntrack Init Path : [%s] ", g_szUnixSockPath);


    return 0;
}

void* fsyslog_SendThread(void* param_ThreadArg)
{
    int         local_nByteRead                 = 0;
    size_t      local_nInSize, local_nOutSize   = 0;
    int         local_nOffset                   = 0;
    int	        local_nTargetSock               = 0;
    int         local_nTargetPort               = 0;
    int         local_nEventType                = 0;
    int         local_nError                    = 0;
    socklen_t   local_targetSocklen             = 0;

    char local_szSendTime[19 +1]                = {0x00,};
    char local_szTargetIp[15 +1]                = {0x00,};
    char local_szMyIp[15 +1]                    = {0x00,};
    char local_szTokenStr[8 +1]                 = {0x00,};
    char local_szEventMsg[256 +1]               = {0x00,};
    char local_szBuffer[DF_BUFFER_SIZE]         = {0x00,};
    char local_szIconvBuffer[DF_BUFFER_SIZE]    = {0x00,};

    struct sockaddr_in local_stTargetInfo;
    _THREAD_ARG        local_stThreadArg;
    char*              local_ptrInBuffer        = NULL;
    char*              local_ptrIconvBuffer     = NULL;

    memset( &local_stTargetInfo, 0x00, sizeof(struct sockaddr_in) );
    memset( &local_stThreadArg, 0x00, sizeof(local_stThreadArg) );

    // 스레드 아규먼트 Copy
    memcpy( &local_stThreadArg, (_THREAD_ARG*)param_ThreadArg, sizeof(_THREAD_ARG) );
    local_nByteRead     = local_stThreadArg.nReadSize;    //read size

    snprintf(local_szTokenStr, sizeof(local_szTokenStr) ,"%s", g_stCfgInfo.szTokenStr);

    // 전송시간
    fsyslog_GetLogTime(local_szSendTime);

    // Server IP(내 IP)
    snprintf(local_szMyIp, sizeof(local_szMyIp), "%s", g_szMyIp);

    // 이벤트 메시지
    local_nEventType = atoi(local_stThreadArg.stCommInfo.szEventType);

    if ( g_cProdType == PRODUCT_DAP )
    {
        fsyslog_EventMsgOut( local_nEventType, local_szEventMsg, sizeof(local_szEventMsg), &local_stThreadArg);
    }
    else if ( g_cProdType == PRODUCT_FI )
    {
        snprintf(local_szEventMsg, sizeof(local_szEventMsg), "%s", "FileInspector DETECT APT HASH");
    }


    if ( strcmp(g_szCfgCharset, "UTF-8") != 0)          //cfg charset이 UTF-8이 아니면
    {
        local_ptrInBuffer = (char*)local_szEventMsg;
        local_nInSize = strlen(local_szEventMsg);

        local_ptrIconvBuffer = local_szIconvBuffer;
        local_nOutSize = sizeof(local_szIconvBuffer);

        if (iconv (g_iconv_t, &local_ptrInBuffer, &local_nInSize, &local_ptrIconvBuffer, &local_nOutSize) < 0)
        {
            if (errno == E2BIG || errno == EILSEQ || errno == EINVAL )
            {
                WRITE_LOG("Iconv Failed ");
            }
        }
        else
        {
            WRITE_LOG("Iconv Success (%s) (%s)", local_szEventMsg, local_szIconvBuffer);
            snprintf(local_szEventMsg, sizeof(local_szEventMsg), "%s", local_szIconvBuffer);
        }
    }

    // 로깅
    WRITE_LOG("Facility : [%s] Severity Level : [%s] time : [%s]  Client IP: [%s] Server IP : [%s] "
              "ProdType : [%s] Event Level : : [%s] Event Type : [%s] Event : [%s] Msg : [%s] " ,
              local_stThreadArg.stCommInfo.szFacility,
              local_stThreadArg.stCommInfo.szSeverityLevel,
              local_szSendTime,
              local_stThreadArg.stCommInfo.szClientIp,
              local_szMyIp,
              local_stThreadArg.stCommInfo.szProdType,
              local_stThreadArg.stCommInfo.szEventLevel,
              local_stThreadArg.stCommInfo.szEventType,
              local_stThreadArg.stCommInfo.szEvent,
              local_szEventMsg );

    // Send Buffer Packing
    memset( local_szBuffer, 0x00, sizeof(local_szBuffer) );


    if ( local_stThreadArg.nProtocolType != PROTOCOL_SYSLOG )
    {
        memcpy( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szFacility,
                strlen(local_stThreadArg.stCommInfo.szFacility));
        local_nOffset += strlen(local_stThreadArg.stCommInfo.szFacility); // Facility

        strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
        local_nOffset += strlen(local_szTokenStr);

        memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szSeverityLevel,
                 strlen(local_stThreadArg.stCommInfo.szSeverityLevel) );
        local_nOffset += strlen(local_stThreadArg.stCommInfo.szSeverityLevel); // Severity Level

        strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
        local_nOffset += strlen(local_szTokenStr);
    }

    memcpy ( &local_szBuffer[local_nOffset], local_szSendTime,strlen(local_szSendTime) ); // Send Time
    local_nOffset += strlen(local_szSendTime);

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szClientIp,
             strlen(local_stThreadArg.stCommInfo.szClientIp));
    local_nOffset += strlen(local_stThreadArg.stCommInfo.szClientIp);  // Client IP

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_szMyIp, sizeof(local_szMyIp)-1 );
    local_nOffset += strlen(local_szMyIp);                           // Server IP

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szProdType,
             strlen(local_stThreadArg.stCommInfo.szProdType));
    local_nOffset += strlen(local_stThreadArg.stCommInfo.szProdType);    // Product Type

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szEventLevel,
             strlen(local_stThreadArg.stCommInfo.szEventLevel));
    local_nOffset += strlen(local_stThreadArg.stCommInfo.szEventLevel);  // Event Level

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szEventType,
             strlen(local_stThreadArg.stCommInfo.szEventType));
    local_nOffset += strlen(local_stThreadArg.stCommInfo.szEventType);   // Event Type

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_stThreadArg.stCommInfo.szEvent,
             strlen(local_stThreadArg.stCommInfo.szEvent));
    local_nOffset += strlen(local_stThreadArg.stCommInfo.szEvent);        // Event

    strcat(&local_szBuffer[local_nOffset], local_szTokenStr);
    local_nOffset += strlen(local_szTokenStr);

    memcpy ( &local_szBuffer[local_nOffset], local_szEventMsg, sizeof(local_szEventMsg)-1 );


    WRITE_LOG("Protocol : [%s] Send Buffer : [%s]",
              (local_stThreadArg.nProtocolType == PROTOCOL_TCP) ? "TCP" :
              (local_stThreadArg.nProtocolType == PROTOCOL_UDP) ? "UDP" : "SYSLOG",
              local_szBuffer);

    switch (local_stThreadArg.nProtocolType)
    {
        case PROTOCOL_UDP:
        {
            local_targetSocklen = sizeof(struct sockaddr_in);
            local_nTargetPort   = atoi(g_stCfgInfo.szServerPort);                                       //server port
            snprintf(local_szTargetIp, sizeof(local_szTargetIp), "%s", g_stCfgInfo.szServerIp);  //server ip

            local_stTargetInfo.sin_family = AF_INET;
            local_stTargetInfo.sin_port = htons(local_nTargetPort);
            fsock_InetPton(AF_INET, local_szTargetIp, &local_stTargetInfo.sin_addr);


            WRITE_LOG("Send IP : [%s] Port : [%d] \n",local_szTargetIp, local_nTargetPort);
            local_nTargetSock = fsock_Socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(local_nTargetSock < 0)
            {
                WRITE_LOG("Error while socket creation (%d:%s)",errno ,strerror(errno) );
                goto _out_;
            }

            sendto(local_nTargetSock, local_szBuffer, sizeof(local_szBuffer), MSG_DONTWAIT,
                   (struct sockaddr *)&local_stTargetInfo, local_targetSocklen);

            break;
        }

        case PROTOCOL_TCP:
        {
            local_targetSocklen = sizeof(struct sockaddr_in);
            local_nTargetPort   = atoi(g_stCfgInfo.szServerPort);                                       //server port
            snprintf(local_szTargetIp, sizeof(local_szTargetIp), "%s", g_stCfgInfo.szServerIp);  //server ip

            local_stTargetInfo.sin_family = PF_INET;
            local_stTargetInfo.sin_port = htons(local_nTargetPort);
            fsock_InetPton(PF_INET, local_szTargetIp, &(local_stTargetInfo.sin_addr));

            WRITE_LOG("Send IP : [%s] Port : [%d] \n",local_szTargetIp, local_nTargetPort);
            local_nTargetSock = fsock_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(local_nTargetSock < 0)
            {
                WRITE_LOG("Error while socket creation (%d:%s)",errno ,strerror(errno) );
                goto _out_;
            }

            if ( fsock_ConnectNonBlock( local_nTargetSock, &local_stTargetInfo, local_targetSocklen, 0) != 0)
            {
                if( errno != EINPROGRESS )
                {
                    WRITE_LOG( "Fail in connect (%d:%s) ", errno, strerror(errno) );
                }

                goto _out_;
            }

            fsock_Send(local_nTargetSock, local_szBuffer, sizeof(local_szBuffer));
            break;
        }

        case PROTOCOL_SYSLOG:
        {
            int local_nFacility = 0;
            int local_nLevel    = 0;

            local_nFacility = atoi(local_stThreadArg.stCommInfo.szFacility);
            local_nLevel    = atoi(local_stThreadArg.stCommInfo.szSeverityLevel);

            openlog("DAP", LOG_CONS, local_nFacility<<3);
            syslog(local_nLevel,"%s",local_szBuffer);
            closelog();

            break;
        }
    }

_out_:
    if ( local_nTargetSock > 0 )
    {
        close(local_nTargetSock);
        local_nTargetSock = 0;
    }

    pthread_exit(NULL);


}

int fsyslog_GetNic(char* param_NicName)
{
    int local_nSock = 0;
    int local_nIfs = 0;
    int local_nLoop = 0;
    struct ifconf stIfconf;
    struct ifreq  stIfr[50];

    local_nSock = socket(AF_INET, SOCK_STREAM, 0);
    if (local_nSock < 0)
    {
        return (-1);
    }

    stIfconf.ifc_buf = (char *) stIfr;
    stIfconf.ifc_len = sizeof( stIfr );

    if (ioctl(local_nSock, SIOCGIFCONF, &stIfconf) == -1)
    {
        return (-1);
    }

    local_nIfs = stIfconf.ifc_len / sizeof(stIfr[0]);

    for (local_nLoop = 0; local_nLoop < local_nIfs; local_nLoop++)
    {
        if(strcmp(stIfr[local_nLoop].ifr_name, "lo") != 0)
        {
            strcpy(param_NicName, stIfr[local_nLoop].ifr_name);
            break;
        }
    }

    if( local_nSock > 0)
    {
        close(local_nSock);
        local_nSock = 0;
    }

    return 1;

}

void fsyslog_GetIp(char* param_InterfaceName, char* param_Ip)
{
    struct ifreq ifr;
    int s;
    s = socket(AF_INET, SOCK_DGRAM, 0); strncpy(ifr.ifr_name, param_InterfaceName, IFNAMSIZ);
    if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
    {
        printf("Error");
    }
    else
    {
        inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, param_Ip, sizeof(struct sockaddr));
    }

    return;
}



int main(int argc, char** argv)
{
    int  local_nRet               = 0;
    pthread_t local_pthread_t     = 0;
    _THREAD_ARG local_stThreadArg;

    // 아규먼트 파싱
    fsyslog_ArgParse(argc, argv);

    // Init
    fsyslog_Init();

    // Signal Init
    fsyslog_SignalInit();

    // Unix Socket Init
    fsyslog_UnixSockInit();

    // syslog recvfrom
    while(1)
    {
        memset( &local_stThreadArg, 0x00, sizeof(local_stThreadArg)  );

        local_nRet = fsyslog_UdpGetData( &local_stThreadArg);
        if ( local_nRet > 0 )
        {
            // read size
            local_stThreadArg.nReadSize = local_nRet;

            // protocol type
            local_stThreadArg.nProtocolType = g_cProtocolType;

            // thread create, syslog sendto
            fcom_ThreadCreate( &local_pthread_t, fsyslog_SendThread, &local_stThreadArg, 4*1024*1024);
        }

        fcom_SleepWait(5);
    }

    return 0;
}
