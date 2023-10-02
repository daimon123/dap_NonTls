//
// Created by KimByoungGook on 2020-09-07.
//
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>



#include "sock/dap_sock.h"
#include "pcif.h"
#include "com/dap_req.h"


const char g_LogCategoryList[13][32] = {
        {"pcif"},
        {"dbif"},
        {"alarm"},
        {"master"},
        {"dblog"},
        {"fwder"},
        {"prmon"},
        {"proxy"},
        {"replmon"},
        {"report"},
        {"schd"},
        {"sysman"},
        {"vwlog"}};

TAIL* fpcif_OpenTail(char *fname)
{
    TAIL *MTAIL = NULL;
    struct stat fbuf;

    if(fcom_malloc((void**)&MTAIL, sizeof(TAIL)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return NULL;
    }
    if ((MTAIL->fp = fopen(fname, "r")) == NULL)
    {
        fcom_MallocFree((void**)&MTAIL);
        return NULL;
    }

    if (stat(fname, &fbuf) < 0)
    {
        fcom_MallocFree((void**)&MTAIL);
        return NULL;
    }

    strncpy(MTAIL->filename, fname, 255);

    // 제일 끝으로 보내서 위치를 저장한다.
    fseek( MTAIL->fp , 0L, SEEK_END);

    // 열린 파일 스트림의 파일지정자를 얻어온다.
    MTAIL->fd = fileno(MTAIL->fp);
    MTAIL->revsize = fbuf.st_size;
    return MTAIL;
}
void fpcif_DistKillTail(char *logip, char* clientIp, char* clientPath)
{
    int		loop = 0;
    int		jsonSize = 0;
    int		convMsgCode = 0;
    int		tokenCnt = 0;
    char	distServerIp[100] = {0x00,};
    char*	tokenIp = NULL;
    char*	jsonData = NULL;
    char*   TempPtr  = NULL;

    // Deliver distribution server
    fcom_GetProfile("PCIF", "DISTRIBUTION_SERVER_IP", distServerIp, "");
    if( strlen(distServerIp) > 6 )
    {
        json_t* root = json_pack("{s:s, s:s}",	"manager_ip",clientIp,"file_name",clientPath);
        jsonData = json_dumps(root, JSON_INDENT(0));
        if( jsonData == NULL )
        {
            json_decref(root);
            WRITE_CRITICAL_IP(logip, "Json pack error");
            return;
        }
        json_decref(root);

        jsonSize = strlen(jsonData);

        WRITE_DEBUG_JSON(logip, "Dump json(%s) ", jsonData);

        convMsgCode = MANAGE_FORWARD_SERVER_TERM;

        tokenCnt = fcom_TokenCnt(distServerIp, ",");
        if(tokenCnt > 0)
        {
            loop = 0;

            tokenIp = strtok_r(distServerIp, ",",&TempPtr);
            while(tokenIp != NULL)
            {
                fpcif_DistServerTail(logip, tokenIp, convMsgCode, jsonData, jsonSize, NULL);
                loop++;

                tokenIp = strtok_r(NULL, ",",&TempPtr);
            }
        }
        else
        {
            fpcif_DistServerTail(logip, distServerIp, convMsgCode, jsonData, jsonSize, NULL);
        }

        fcom_MallocFree((void**)&jsonData);
    }
}

void fpcif_KillTail(char *logip, char* clientIp, char* clientPath)
{
    int     pid = 0;
    int		serverId = 0;
    char    cmd[128 +1]         = {0x00,};
    char    buff[10 +1]         = {0x00,};
    char	strServerId[5 +1]   = {0x00,};
    char	strFilePath[30 +1]  = {0x00,};
    char*	strPos = NULL;
    FILE*   fp = NULL;

    memset  (cmd, 0x00, sizeof(cmd));
    if( !strncasecmp(clientPath, "all", 3) )
    {
        sprintf (cmd, "ps -eo pid,ppid,comm,cmd |grep dap_tail|grep %s|grep -v grep|awk '{print $1}'",
                 clientIp);
    }
    else
    {
        strPos = NULL;
        strPos = strchr(clientPath, '/');
        memset(strServerId, 0x00, sizeof(strServerId));
        strncpy(strServerId, clientPath, strPos-clientPath);
        serverId = atoi(strServerId);
        WRITE_INFO_IP(logip, "serverId(%d) ", serverId);
        if( serverId == g_stServerInfo.stDapComnInfo.nCfgMgwId )
        {
            memset(strFilePath, 0x00, sizeof(strFilePath));
            strcpy(strFilePath, clientPath+strlen(strServerId)+1);
            WRITE_INFO_IP(logip, "strFilePath(%s)", strFilePath);
            sprintf (cmd, "ps -eo pid,ppid,comm,cmd |grep dap_tail|grep %s|grep %s|grep -v grep|awk '{print $1}'",
                     clientIp,strFilePath);
        }
        else
        {
            return;
        }
    }

    fp = popen(cmd, "r");
    if( fp == NULL )
    {
        WRITE_INFO_IP(logip, "Fail in popen, cmd(%s)", cmd);
        return;
    }

    memset(buff, 0x00, sizeof(buff));
    while(fgets(buff, 10, fp) != NULL)
    {
        pid = atoi(buff);
        if( kill(pid, SIGTERM) < 0 )
        {
            WRITE_INFO_IP(logip, "Can't kill process, pid(%d)sig(%d)", pid,SIGTERM);
        }
        else
        {
            WRITE_INFO_IP(logip, "Killed process, pid(%d)sig(%d)", pid,SIGTERM );
        }
    }
    pclose(fp);
}

/*
 * sec시간 간격으로 파일에 추가된 내용을 읽어온다.
 * 만약 지금의 파일크기가 이전 파일크기 보다 작다면
 * 파일이 truncate() 되었다고 가정하고 첫라인 부터
 * 다시 읽어 들인다.
 */
int fpcif_Readtail(TAIL *LTAIL, char *buf, size_t size, int usec, char* FlagFilePath)
{
    fd_set rfds;
    struct timeval tv;
    int retval  = 0;
    char *ret   = NULL;
    struct stat fbuf;

    FD_ZERO(&rfds);
    FD_SET(LTAIL->fd, &rfds);
    tv.tv_sec  = 0;
    tv.tv_usec = usec;

    memset(&fbuf, 0x00, sizeof(struct stat));

    while(1)
    {
        if(fcom_fileCheckStatus(FlagFilePath) == 0)
        {
            break;
        }
        retval = select(LTAIL->fd+1, &rfds, NULL, NULL, &tv);
        if (retval)
        {
            memset(buf, 0x00, 4096);
            ret = fgets(buf, size, LTAIL->fp);
            if (stat(LTAIL->filename, &fbuf) < 0)
            {
                return -1;
            }
            if (ret == NULL)
            {
                // 현재 파일크기가 이전 파일 크기 보다
                // 작다면 rewind()시킨다.
                if (fbuf.st_size < LTAIL->revsize)
                {
                    rewind(LTAIL->fp);
                }
                LTAIL->revsize = fbuf.st_size;
                sleep(1);
                continue;
            }
            LTAIL->revsize = fbuf.st_size;
            return 1;
        }
        else
        {
            return -1;
        }
    }
    return 1;
}

void* fpcif_ReqServerLogTail(void* arg)
{
    int i = 0, jsonSize = 0, rxt = 0, send_cnt = 0;

    char szCopyTemp[256 +1]      = {0x00,};
    char szReplacePath[256 +1]   = {0x00,};

    struct _STLOGTAIL stTail;
    _DAP_EventParam EventParam;

    TAIL* tf        = NULL;
    char* buf       = NULL;
    char* resJson   = NULL;
    char* RemainPtr = NULL;
    struct _STLOGTAIL* pstTail = NULL;

    memcpy(&stTail, (struct _STLOGTAIL*)arg,sizeof(struct _STLOGTAIL));

    pstTail = &stTail;

    i = jsonSize = rxt = send_cnt = 0;

    memset(szCopyTemp, 0x00, sizeof(szCopyTemp));
    memset(szReplacePath, 0x00, sizeof(szReplacePath));
    send_cnt = 0;

    for(i = 0; i < 20; i++)
    {
        if(strstr(pstTail->FilePath,g_LogCategoryList[i]) != NULL)
        {
            sprintf(szCopyTemp,"%s",pstTail->FilePath);
            strtok_r(szCopyTemp,"/",&RemainPtr);
            strtok_r(NULL,"/",&RemainPtr);
            sprintf(szReplacePath,"/%s",RemainPtr);
            break;
        }
    }
    /* 파일이 없거나 서버의 mgwid != 서버 mgwid인 경우 */
    if(fcom_fileCheckStatus(szReplacePath) != 0 && (pstTail->mgwid != g_stServerInfo.stDapComnInfo.nCfgMgwId))
    {
        pthread_exit(NULL);
    }
    if ((tf = fpcif_OpenTail( szReplacePath)) == NULL)
    {
        perror("error ");
        exit(0);
    }
//    buf = (char *)malloc(4096);
//    if(buf == NULL)
//    {
//        exit(0);
//    }
    if(fcom_malloc((void**)&buf,4096) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        exit(0);
    }
    while(1)
    {
        if(send_cnt > 150)
        {
            break;
        }

        memset(buf, 0x00, 4096);
        fpcif_Readtail(tf, buf, 4095, 10, pstTail->FlagFilePath);
        fflush(stdout);
        if((fcom_fileCheckStatus(pstTail->FlagFilePath)) == 0)
        {
            unlink(pstTail->FlagFilePath);
            break;
        }
        memset(&EventParam, 0x00, sizeof(EventParam));
        EventParam.ev_type = MANAGE_RSP_LOGTAIL;
        EventParam.ev_level = g_stServerInfo.stDapComnInfo.nCfgMgwId;
        strncpy(EventParam.ev_context, buf, sizeof(EventParam.ev_context));
        jsonSize = fpcif_MakeJsonTail(EventParam.ev_level,
                                    pstTail->FilePath,
                                    (char *)EventParam.ev_context,
                                    &resJson );
        if( jsonSize < 0 )
        {
            continue;
        }
        rxt = fpcif_SendJsonToManager(pstTail->ssl, EventParam.ev_type, resJson, jsonSize);
        if( rxt < 0 )
        {
            fcom_MallocFree((void**)&resJson);
            continue;
        }
        fcom_MallocFree((void**)&resJson);
        fcom_Msleep(100);
        send_cnt++;
    }
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
    pthread_exit(NULL);

}

void* fpcif_DistServerLogTail(void* arg)
{
    int rxt     = 0;
    int	msgLeng = 0;
    int sendCnt = 0;
    char msgType[10+1] = {0x00,};
    char* tmpJson = NULL;
    CRhead	RHead;
    struct _DistTailInfo* pstTail = NULL;
    struct _DistTailInfo stTail;

    memcpy(&stTail, (struct _DistTailInfo*)arg, sizeof(struct _DistTailInfo));
    pstTail = &stTail;

    while(1)
    {
        memset(&RHead, 0x00, sizeof(CRhead));

        if(sendCnt > 150)
            break;

        if(fcom_fileCheckStatus(pstTail->FlagFilePath) == 0)
        {
            unlink(pstTail->FlagFilePath);
            break;
        }

        // PCIF(Distribution) -> PCIF
        rxt = fsock_SslSocketRecv(pstTail->sslcon->ssl, (char *)&RHead, sizeof(RHead));
        if(rxt <=  0)
        {
            WRITE_CRITICAL_IP(pstTail->cpip,"Fail in socket head, rxt(%d)errno(%d)pid(%d)",rxt,errno,getpid());
            break;
        }

        memset(msgType, 0x00, sizeof(msgType));
        strcpy(msgType, RHead.msgtype);
        msgLeng = ntohl(RHead.msgleng);

        if(fcom_malloc((void**)&tmpJson, sizeof(char)*((msgLeng-2)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            exit(0);
        }

        // PCIF(Distribution) -> PCIF
        rxt = fsock_SslSocketRecv(pstTail->sslcon->ssl, tmpJson, msgLeng-2);
        if(rxt <=  0)
        {
            WRITE_CRITICAL_IP(pstTail->cpip,"Fail in socket head, rxt(%d)errno(%d)pid(%d)",
                              rxt,errno,getpid());
            break;
        }

        // PCIF -> Manager
        rxt = fpcif_SendJsonToManager(pstTail->p_manager_ssl, MANAGE_RSP_LOGTAIL, tmpJson, msgLeng-2);

        WRITE_INFO_IP(pstTail->cpip,"[REQ] Succeed in send json to server(%s) ",tmpJson);

        sendCnt++;
        fcom_Msleep(500);
    }

    if(tmpJson != NULL)
        free(tmpJson);

    ssl_disconnect(pstTail->sslcon);
    fpcif_DistKillTail(pstTail->cpip, pstTail->cpip, "all");

    pthread_exit(NULL);

}

int fpcif_ReqForwardLogTail(struct _STLOGTAIL* pstTail)
{
    TAIL* tf = NULL;
    char* buf = NULL;
    int i = 0, jsonSize = 0, rxt = 0, send_cnt = 0;
    char *resJson = NULL;
    char *szPtrTmp = NULL;
    char *RemainPtr = NULL;
    char *TempPtr   = NULL;
    char UnpackName[256 +1]     = {0x00,};
    char szCopyTemp[256 +1]     = {0x00,};
    char szReplacePath[256 +1]  = {0x00,};
    char szCopyFilePath[256 +1] = {0x00,};

    int localMgwId = 0;

    _DAP_EventParam EventParam;

    memset(szCopyTemp, 0x00, sizeof(szCopyTemp));
    memset(szReplacePath, 0x00, sizeof(szReplacePath));
    memset(UnpackName, 0x00, sizeof(UnpackName));

    sprintf(UnpackName, "%s", pstTail->FilePath);

    sprintf(szCopyFilePath, "%s", pstTail->FilePath);
//    szPtrTmp = strtok(szCopyFilePath, "/");
    szPtrTmp = strtok_r(szCopyFilePath, "/",&TempPtr);
    localMgwId = atoi(szPtrTmp);


    send_cnt = 0;

    for (i = 0; i < 20; i++)
    {
        if (strstr(pstTail->FilePath, g_LogCategoryList[i]) != NULL)
        {
            sprintf(szCopyTemp, "%s", pstTail->FilePath);
            strtok_r(szCopyTemp, "/", &RemainPtr);
            strtok_r(NULL, "/", &RemainPtr);
            sprintf(szReplacePath, "/%s", RemainPtr);
            break;
        }
    }
    if (fcom_fileCheckStatus(szReplacePath) != 0 && localMgwId != g_stServerInfo.stDapComnInfo.nCfgMgwId)
    {
        return (-1);
    }
    if ((tf = fpcif_OpenTail(szReplacePath)) == NULL)
    {
        perror("error ");
        exit(0);
    }
//    buf = (char *) malloc(sizeof(char) * 4096);
    if(fcom_malloc((void**)&buf, sizeof(char) * 4096) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    while (1)
    {
        if (send_cnt > 150)
            break;


        memset(buf, 0x00, 4096);
        fpcif_Readtail(tf, buf, 4095, 10, pstTail->FlagFilePath);
        fflush(stdout);

        memset(&EventParam, 0x00, sizeof(_DAP_EventParam));
        EventParam.ev_type = MANAGE_RSP_LOGTAIL;
        EventParam.ev_level = g_stServerInfo.stDapComnInfo.nCfgMgwId;
        strncpy(EventParam.ev_context, buf, sizeof(EventParam.ev_context));

        jsonSize = fpcif_MakeJsonTail(EventParam.ev_level,
                                      UnpackName,
                                      (char *) EventParam.ev_context,
                                      &resJson);


        rxt = fpcif_SendJsonToManager(pstTail->sock, EventParam.ev_type, resJson, jsonSize);
        if (rxt < 0)
        {
//            fcom_BufferFree(resJson);
            fcom_MallocFree((void**)&resJson);
            continue;
        }

        if (fcom_fileCheckStatus(pstTail->FlagFilePath) == 0)
        {
            break;
        }
        fcom_Msleep(100);
        send_cnt++;
    }
    if (buf != NULL)
    {
        free(buf);
        buf = NULL;
    }

    unlink(pstTail->FlagFilePath);

    return (-1);
}
