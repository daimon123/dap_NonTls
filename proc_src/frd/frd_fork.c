

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "ipc/dap_qdef.h"

#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_req.h"
#include "json/dap_json.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "frd.h"
#include "dap_version.h"


int frd_ForkWork(void)
{
    struct          timeval tv;
    int             local_FpChangeFlag          = 0;
    int             local_nRet                  = 0;
    int             local_nForkIdx              = 0;
    int             local_nFileFd               = 0;
    int             local_nPackType             = 0;
    char            local_szFileName[32+1]      = {0x00,};
    char            local_szTmp[32 +1]          = {0x00,};
    char            local_szFilePath[256 +1]    = {0x00,};
    fd_set          local_ReadFd, local_BackupFd;

    _DAP_QUEUE_BUF  local_DbData;
    FILE*           local_Fp = NULL;

    local_nForkIdx = g_nForkIdx;

    memset( &local_DbData, 0x00, sizeof(_DAP_QUEUE_BUF) );

    /** 부모에서 등록한 SIGCHILD해제, system()함수 내부적으로 shell fork되면서 SIGCHILD 반횐된다. **/
    signal( SIGCHLD, SIG_IGN );

    WRITE_DEBUG(CATEGORY_DEBUG,"Fork Created (%d)", g_nForkIdx);

    /** DB Connection **/
    local_nRet = fdb_ConnectDB();
    if(local_nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in db connection ");
        exit(0);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Succeed in db connection ");
    }

    /** 정책 데이터 :  가장 마지막 File뺴고 다 제거. 가장 마지막 File 기준으로 DB처리(현재시점 기준)
     *  검출 데이터 :  가장 처음파일 부터 파일사이즈가 0인거는 제거.
     *               가장 마지막 파일은 사이즈가 0이어도 쓰는중이므로 제거하지 않는다.
     *               파일사이즈가 0이 아닌 가장 처음파일부터 DB처리.
     * **/
    frd_InitClearFile( local_nForkIdx );

    /** Set DB Execute Fail **/
    frd_InitDbExecFail( local_nForkIdx );

    /** 파일명 Set **/
    /** 검출데이터는 오늘 날짜의 가장 마지막의 파일 Name을 가져온다. **/
    /** 정책은 가장 최신의 파일 Name을 가져온다. **/
    frd_GetFileName( local_nForkIdx, local_szFileName, sizeof(local_szFileName));

    /** 정책  **/
    if ( local_nForkIdx == FORK_POLICY )
    {
        snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s", g_stProcFrdInfo.szPolicyFilePath, local_szFileName);

        /** 현재 File Index +1이 MAX보다 크면, 파일번호 1로 회귀. **/
        if ( g_stProcFrdInfo.nFileIdx + 1 > g_stProcFrdInfo.nCfgMaxFileIndex )
        {
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_POLICY", 1);
        }
        else
        {
            /** Next File Name 설정 **/
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_POLICY",
                      g_stProcFrdInfo.nFileIdx +1);
        }

        snprintf(g_stProcFrdInfo.szFileNextPath,
                 sizeof(g_stProcFrdInfo.szFileNextPath),
                 "%s%s",
                 g_stProcFrdInfo.szPolicyFilePath,
                 g_stProcFrdInfo.szNextFileName);
    }

    else if( local_nForkIdx == FORK_SERVICE )
    {
        snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s", g_stProcFrdInfo.szPolicyFilePath, local_szFileName);

        /** 현재 File Index +1이 MAX보다 크면, 파일번호 1로 회귀. **/
        if ( g_stProcFrdInfo.nFileIdx + 1 > g_stProcFrdInfo.nCfgMaxFileIndex )
        {
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_SERVICE",
                      1);
        }
        else
        {
            /** Next File Name 설정 **/
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_SERVICE",
                      g_stProcFrdInfo.nFileIdx +1);
        }

        snprintf(g_stProcFrdInfo.szFileNextPath,
                 sizeof(g_stProcFrdInfo.szFileNextPath),
                 "%s%s",
                 g_stProcFrdInfo.szPolicyFilePath,
                 g_stProcFrdInfo.szNextFileName);
    }
    else /** 검출  **/
    {
        snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s", g_stProcFrdInfo.szDtFilePath, local_szFileName);
        snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", local_nForkIdx - FORK_DETECT );

        if ( g_stProcFrdInfo.nFileIdx +1 > g_stProcFrdInfo.nCfgMaxFileIndex )
        {
            /** 파일인덱스 1번으로 회귀 **/
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      local_szTmp,
                      1);
        }
        else
        {
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      local_szTmp,
                      g_stProcFrdInfo.nFileIdx +1);
        }

        snprintf(g_stProcFrdInfo.szFileNextPath,
                 sizeof(g_stProcFrdInfo.szFileNextPath),
                 "%s%s",
                 g_stProcFrdInfo.szDtFilePath,
                 g_stProcFrdInfo.szNextFileName);
    }

    g_stProcFrdInfo.nNextIdx = g_stProcFrdInfo.nFileIdx+1;

    if ( g_stProcFrdInfo.nNextIdx > g_stProcFrdInfo.nCfgMaxFileIndex )
    {
        g_stProcFrdInfo.nNextIdx = 1;
    }


    WRITE_DEBUG(CATEGORY_DEBUG, "Fork (%d) File Read Path : [%s] ", local_nForkIdx, local_szFilePath);
    local_Fp = fopen(local_szFilePath, "rb");
    if(local_Fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File (%s) Open Failed ", local_szFilePath);
        sleep(1);
        return (-1);
    }

    snprintf( g_stProcFrdInfo.szFileName,       sizeof(g_stProcFrdInfo.szFileName),
              "%s", local_szFileName);
    snprintf(g_stProcFrdInfo.szFileFullPath,    sizeof(g_stProcFrdInfo.szFileFullPath),
             "%s", local_szFilePath );
    g_stProcFrdInfo.Fp = local_Fp;

    if( local_nForkIdx == FORK_POLICY || local_nForkIdx == FORK_SERVICE)
    {
        fseek(local_Fp, 0, SEEK_END);
    }
    else
    {
        /** 검출데이터는 중복 처리 방지를 위해 이전에 Read하던 파일 포인터의 위치를 가져온다. **/
        frd_GetLastPosition( );
        fseek(local_Fp, g_stProcFrdInfo.nLastReadPosition, SEEK_CUR);
    }

    local_nFileFd = fileno(local_Fp);
    WRITE_DEBUG(CATEGORY_DEBUG, "Open File Name : [%s], Next : [%s] Fd : [%d] Pos : [%d] ",
                g_stProcFrdInfo.szFileName,
                g_stProcFrdInfo.szNextFileName,
                local_nFileFd,
                g_stProcFrdInfo.nLastReadPosition );


    /** File Select **/
    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    FD_ZERO(&local_ReadFd);
    FD_SET(local_nFileFd, &local_ReadFd);
    local_BackupFd = local_ReadFd; /* -- Backup -- */

    while(1)
    {
        local_ReadFd = local_BackupFd;

        local_nRet = select(local_nFileFd+1, &local_ReadFd, NULL, NULL, &tv);

        if (local_nRet > 0)
        {
            if (FD_ISSET(local_nFileFd, &local_ReadFd) )
            {
                memset( &local_DbData,          0x00,    sizeof(_DAP_QUEUE_BUF)    );
                if ( frd_FileRead( local_nForkIdx, &g_stProcFrdInfo.Fp, &local_DbData, &local_nPackType, &local_FpChangeFlag ) <= 0)
                {
                    fcom_SleepWait(3);
                    continue; // Data Is Null
                }
                else
                {
                    frd_BufferToDBMS(local_nForkIdx, &local_DbData);
                    if ( local_FpChangeFlag == 1)
                    {
                        /** Select FD 배열 재 설정 **/
                        local_nFileFd = fileno(g_stProcFrdInfo.Fp);
                        FD_ZERO(&local_ReadFd);
                        FD_SET(local_nFileFd, &local_ReadFd);
                        local_BackupFd = local_ReadFd; /* -- Backup -- */

                        local_FpChangeFlag = 0;
                    }
                }
            }
        }
        else
        {
            if ( local_nRet == 0) // TimeOut
            {
                if(fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Select Is Timeout Next File (%s) Check Exist",
                                   g_stProcFrdInfo.szFileNextPath );
                    frd_GetNextFile( local_nForkIdx );

                    /** Select FD 배열 재 설정 **/
                    local_nFileFd = fileno(g_stProcFrdInfo.Fp);
                    FD_ZERO(&local_ReadFd);
                    FD_SET(local_nFileFd, &local_ReadFd);
                    local_BackupFd = local_ReadFd; /* -- Backup -- */
                }
            }
        }
    }

    return 0;

}

/** 정책 데이터 :  가장 마지막 File뺴고 다 제거. 가장 마지막 File 기준으로 DB처리(Now)
     *  검출 데이터 :  가장 처음파일 부터 파일사이즈가 0인거는 제거.
     *               가장 마지막 파일은 사이즈가 0이어도 쓰는중이므로 제거하지 않는다.
     *               파일사이즈가 0이 아닌 가장 처음파일부터 DB처리.
     * **/

int frd_InitClearFile( int param_ForkIdx )
{
    int  local_nFileIndex = 0;

    if ( param_ForkIdx == FORK_POLICY )
    {
        /** 마지막 File Index를 가져와서 File 정리. **/
        local_nFileIndex = frd_GetLastFileIndex("FW_POLICY");
        frd_ClearPolicyFile("FW_POLICY", local_nFileIndex);
    }
    else if ( param_ForkIdx == FORK_SERVICE )
    {
        /** 마지막 File Index를 가져와서 File 정리. **/
        local_nFileIndex = frd_GetLastFileIndex("FW_SERVICE");
        frd_ClearPolicyFile("FW_SERVICE", local_nFileIndex);
    }

    /** 검출 데이터 주석처리, 현재 사용하지 않음. **/
    /*else
    {
        *//** 마지막 File Index를 가져와서 File 정리. **//*
        snprintf(local_szTemp, sizeof(local_szTemp), "%s_%02d", "FW_DT", param_ForkIdx-1 );
        local_nFileIndex = frd_GetLastFileIndex( local_szTemp );

        *//** Set 삭제 예외 파일명
         * 마지막 File Index는 파일 사이즈가 0이여도 신규로 쓰여질 파일이기 때문에 지우지 않는다.
         * **//*
        snprintf( local_szFileName, sizeof(local_szFileName), "%s.%05d",
                  local_szTemp,
                  local_nFileIndex);
        frd_ClearDtFile(local_szTemp, local_nFileIndex, local_szFileName);
    }*/

    return 0;

}

int frd_InitDbExecFail(int param_ForkIdx)
{
    char local_szFilePath[256 +1] = {0x00,};

    /** 정책 **/
    if ( param_ForkIdx == FORK_POLICY || param_ForkIdx == FORK_SERVICE)
    {
        snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s", g_stProcFrdInfo.szPolicyFilePath, SQL_FAIL_NAME_POLICY);
    }
    else /** 검출 **/
    {
        snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s_%d",
                 g_stProcFrdInfo.szDtFilePath,
                 SQL_FAIL_NAME_DT,
                 param_ForkIdx - FORK_DETECT);
    }

    g_stProcFrdInfo.SqlFailFp = fopen(local_szFilePath, "a, ccs=UTF-8");
    if(g_stProcFrdInfo.SqlFailFp == NULL )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "DB Exec Fail File (%s) Open Fail", local_szFilePath);
        return (-1);
    }


    return 0;

}


int frd_BufferToDBMS(int param_ForkIdx, _DAP_QUEUE_BUF* param_QueueBuf)
{
    int		i = 0;
    int		rxt = 0;
    int		buffLen = 0;
    int		retryBuffLen = 0;
    int		eventCount = 0;
    int		stLen = 0;
    int		bBtDanger = 0;
    int 	bEmptyBase = 0;
    int		bNotiFlag = 0;
    int		hbExt = 0;
    int     CurrMin = 0;
    char	*arrTokenKey = NULL;
    char	*tokenKey = NULL;
    char    *ptrTemp = NULL;
    _DAP_COMN_INFO* pstComnInfo = NULL;
    char	sMsgLen[5 +1] = {0x00,};
    char	prefix[6 +1] = {0x00,};
    char	postfix[4 +1] = {0x00,};
    char	cmdEventChange[128 +1] = {0x00,};
    char	cmdBaseChange[128 +1] = {0x00,};
    char	cmdCpChange[128 +1] = {0x00,};
    char	currDate[19 +1] = {0x00,};
    time_t	currTime = 0;

    _DAP_QUEUE_BUF 	    RsltQData;
    _DAP_QUEUE_BUF	    RetryQData;
    _DAP_QUEUE_BUF	    EventQData;
    _DAP_QUEUE_BUF      AlarmQData;
    _DAP_AGENT_INFO     AgentInfo;
    _DAP_DETECT         Detect;
    _DAP_EventParam     EventParam;
    DETECT_ITEM         DetectItem; //ENUM

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    memset(cmdEventChange,  0x00, sizeof(cmdEventChange));
    memset(cmdBaseChange,   0x00, sizeof(cmdBaseChange) );
    memset(cmdCpChange,     0x00, sizeof(cmdCpChange)   );

    sprintf(cmdEventChange  , "touch %s%s", getenv("DAP_HOME"), "/config/event_change"  );
    sprintf(cmdBaseChange   , "touch %s%s", getenv("DAP_HOME"), "/config/base_change"   );
    sprintf(cmdCpChange     , "touch %s%s", getenv("DAP_HOME"), "/config/cp_change"     );


    g_stProcFrdInfo.nIngJobTime = time(NULL);
    CurrMin = fcom_GetSysMinute();

    memset((void *)&RsltQData,0x00,sizeof(RsltQData)); //????????
    memset((void *)&RetryQData,0x00,sizeof(RetryQData)); //Retry?? All ??????
    memset((void *)&EventQData,0x00,sizeof(EventQData)); //Retry?? Event ??????

    if((g_stProcFrdInfo.nIngJobTime % g_stProcFrdInfo.nCfgPrmonInterval) == 0) //foreach 1min
    {
        fipc_PrmonLinker(pstComnInfo->szDebugName, 0, &g_stProcFrdInfo.nLastSendTime);
    }

    if( (CurrMin % 3) == 0 )
    {
        if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcFrdInfo.nCfgLastModify) == 1)
        {
            frd_ReloadCfgFile();
        }
    }

    memcpy( RsltQData.buf, param_QueueBuf->buf, sizeof(RsltQData.buf));
    RsltQData.packtype = param_QueueBuf->packtype;

    g_stProcFrdInfo.nLastJobTime    = time(NULL);

    memset(prefix,  0x00, sizeof(prefix));
    memset(postfix, 0x00, sizeof(postfix));

    fdb_GetHistoryDbTime(prefix, 0);
    fdb_GetHistoryTableTime(postfix, NULL, 0);

    WRITE_INFO(CATEGORY_INFO, "prefix(%s) postfix(%s)", prefix, postfix);

    if(	RsltQData.packtype == DATACODE_REQUEST_CFG  ||
           RsltQData.packtype == EXTERNAL_REQUEST_CFG   )
    {
        memset(&AgentInfo, 0x00, sizeof(AgentInfo));
        memcpy((void *)&AgentInfo, (void *)&RsltQData.buf, sizeof(AgentInfo));

        if(RsltQData.packtype == DATACODE_REQUEST_CFG)
        {
            /* HW_BASE_TB UPDATE */
            frd_UpdateAccessByHwBaseTb(&AgentInfo, prefix, postfix, &hbExt);
            if(hbExt == 1)
            {
                currTime = time((time_t) 0);
                fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");

                memset(&EventParam, 0x00, sizeof(EventParam));

                strcpy(	EventParam.user_key,	AgentInfo.user_key);
                strcpy(	EventParam.user_ip,		AgentInfo.user_ip);
                EventParam.user_seq		      = AgentInfo.user_seq;
                strcpy(	EventParam.detect_time,	currDate);
                EventParam.ev_type		      = EXTERNAL_CONN;
                EventParam.ev_level		      = CRITICAL;
                strcpy(	EventParam.prefix,		prefix);
                strcpy(	EventParam.postfix,		postfix);
                strcpy(	EventParam.ev_context, "");
                EventParam.ru_sq		      = 0;

                rxt = frd_MergeEventTb(&EventParam, 0);
                if(rxt > 0)
                    bNotiFlag = 1;
            }
            
            system(cmdBaseChange);

            WRITE_DEBUG(CATEGORY_DEBUG,"Policy Update Key (%s)", AgentInfo.user_key);

        }

        else if(RsltQData.packtype == EXTERNAL_REQUEST_CFG)
        {
            WRITE_INFO(CATEGORY_INFO,  "[DQUE](%d) EXTERNAL_REQUEST_CFG ", fipc_SQGetElCnt());
            rxt = frd_UpdateExternalByHwBaseTb(&AgentInfo, &hbExt);
            if(hbExt == 0)
            {
                currTime = time((time_t) 0);
                fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");
                memset(&EventParam, 0x02, sizeof(EventParam));
                strcpy(	EventParam.user_key,	AgentInfo.user_key);
                strcpy(	EventParam.user_ip,		AgentInfo.user_ip);
                EventParam.user_seq 	= AgentInfo.user_seq;
                strcpy(	EventParam.detect_time,	currDate);
                EventParam.ev_type		= EXTERNAL_CONN;
                EventParam.ev_level		= CRITICAL;
                strcpy(	EventParam.prefix,		prefix);
                strcpy(	EventParam.postfix,		postfix);
                strcpy(	EventParam.ev_context, "ext");
                EventParam.ru_sq		= 0;
                rxt = frd_MergeEventTb(&EventParam, 1);
                if(rxt > 0)
                {
                    bNotiFlag = 1;
                    frd_FQEventToPcif(&EventParam);
                    if(g_stProcFrdInfo.nCfgAlarmActivation == 1)
                    {
                        memset(&AlarmQData, 0x00, sizeof(AlarmQData));
                        frd_FQEventToAlarm(&AlarmQData, &EventParam);
                    }

                    if(g_stProcFrdInfo.nEorActive == 1)
                    {
                        frd_WriteEor( &EventParam );
                    }
                }
            }
        }

        if(bNotiFlag)
        {
            WRITE_INFO(CATEGORY_INFO,	"[NOTI](%s) userKey(%s)type(%d)level(%d)",
                       EventParam.user_ip,Detect.AgentInfo.user_key,
                       EventParam.ev_type,EventParam.ev_level);
            system(cmdEventChange);
            bNotiFlag = 0;
        }
        // update user_tb
        rxt = frd_UpdateUserTb(&AgentInfo, prefix, postfix);
        if (rxt > 0)
        {
            system(cmdCpChange);
            WRITE_DEBUG(CATEGORY_DEBUG,"User (%s) Ip is Changed cmd (%s)",EventParam.user_ip,cmdCpChange);

        }

    }
    else if(	RsltQData.packtype == DATACODE_DETECT_DATA ||
                RsltQData.packtype == EXTERNAL_DETECT_DATA ||
                RsltQData.packtype == DATACODE_ADDRESS_LIST )
    {
        if(RsltQData.packtype == DATACODE_DETECT_DATA)
        {
            WRITE_INFO(CATEGORY_IPC, "DATACODE_DETECT_DATA ");
            RetryQData.packtype = DATACODE_DETECT_DATA;
        }
        else if(RsltQData.packtype == EXTERNAL_DETECT_DATA)
        {
            WRITE_INFO(CATEGORY_IPC, "[DQUE] EXTERNAL_DETECT_DATA ");
            RetryQData.packtype = EXTERNAL_DETECT_DATA;
        }
        else if(RsltQData.packtype == DATACODE_ADDRESS_LIST)
        {
            RetryQData.packtype = DATACODE_ADDRESS_LIST;
        }

        bEmptyBase          = 0;
        EventQData.packtype = RETRY_EVENT_ALARM;

        memset(&Detect, 0x00, sizeof(Detect));

        //1. msgLen
        memset(sMsgLen, 0x00, sizeof(sMsgLen));
        memcpy(sMsgLen, (void *)&RsltQData.buf, sizeof(sMsgLen));
        buffLen      = sizeof(sMsgLen);
        retryBuffLen = sizeof(sMsgLen);

        //2. Detect
        memcpy((void *)&Detect, (void *)&RsltQData.buf[buffLen], sizeof(Detect));
        memcpy((void *)&RetryQData.buf[retryBuffLen], (void *)&RsltQData.buf[buffLen], sizeof(Detect));

        buffLen         += sizeof(Detect);
        retryBuffLen    += sizeof(Detect);

        fjson_PrintAllDetect(&Detect);

        if(fcom_malloc((void**)&arrTokenKey, sizeof(char)*(strlen(Detect.change_item)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
            return (-1);
        }

        snprintf(arrTokenKey,strlen(Detect.change_item)+1,"%s",Detect.change_item );
        WRITE_INFO(CATEGORY_INFO, "arrTokenKey(%s)", arrTokenKey);

        tokenKey = strtok_r(arrTokenKey, ",",&ptrTemp);

        while(tokenKey != NULL)
        {
            WRITE_INFO(CATEGORY_INFO, "tokenKey(%s) ", tokenKey);
            DetectItem = fjson_GetDetectItem(tokenKey);

            switch(DetectItem)
            {
                case MAIN_BOARD:
                    frd_TaskMainboard(  &RsltQData,
                                        &RetryQData,
                                        &Detect,
                                        prefix,
                                        postfix,
                                        cmdBaseChange,
                                        &retryBuffLen,
                                        &buffLen);
                    break;
                case SYSTEM:
                    frd_TaskSystem(     &RsltQData,
                                        &EventQData,
                                        &AlarmQData,
                                        &RetryQData,
                                        &EventParam,
                                        &Detect,
                                        prefix,
                                        postfix,
                                        &bEmptyBase,
                                        &eventCount,
                                        &bNotiFlag,
                                        &retryBuffLen,
                                        &buffLen );

                    break;

                case CONNECT_EXT_SVR:
                    frd_TaskConnExt(&RsltQData,
                                    &EventQData,
                                    &AlarmQData,
                                    &RetryQData,
                                    &EventParam,
                                    &Detect,
                                    prefix,
                                    postfix,
                                    &bEmptyBase,
                                    &eventCount,
                                    &bNotiFlag,
                                    &retryBuffLen,
                                    &buffLen );
                    break;
                case OPERATING_SYSTEM:

                    frd_TaskOperation(&RsltQData,
                                      &RetryQData,
                                      &Detect,
                                      prefix,
                                      postfix,
                                      &bEmptyBase,
                                      &retryBuffLen,
                                      &buffLen );
                    break;
                case CPU:
                    frd_TaskCpu(   &RsltQData,
                                   &RetryQData,
                                   &Detect,
                                   prefix,
                                   postfix,
                                   &bEmptyBase,
                                   &retryBuffLen,
                                   &buffLen );
                    break;
                case NET_ADAPTER:
                    frd_TaskNetAdapter(   &RsltQData,
                                          &EventQData,
                                          &AlarmQData,
                                          &RetryQData,
                                          &EventParam,
                                          &Detect,
                                          prefix,
                                          postfix,
                                          &bEmptyBase,
                                          &eventCount,
                                          &bNotiFlag,
                                          &retryBuffLen,
                                          &buffLen );

                    break;
                case WIFI:
                    frd_TaskWifi(  &RsltQData,
                                   &EventQData,
                                   &AlarmQData,
                                   &RetryQData,
                                   &EventParam,
                                   &Detect,
                                   prefix,
                                   postfix,
                                   &bEmptyBase,
                                   &eventCount,
                                   &bNotiFlag,
                                   &retryBuffLen,
                                   &buffLen );

                    break;
                case BLUETOOTH:
                    frd_TaskBlueTooth(  &RsltQData,
                                        &EventQData,
                                        &AlarmQData,
                                        &RetryQData,
                                        &EventParam,
                                        &Detect,
                                        prefix,
                                        postfix,
                                        &bEmptyBase,
                                        &eventCount,
                                        &bNotiFlag,
                                        &retryBuffLen,
                                        &bBtDanger,
                                        &buffLen );

                    break;
                case NET_CONNECTION:
                    frd_TaskNetConnection(  &RsltQData,
                                            &EventQData,
                                            &AlarmQData,
                                            &RetryQData,
                                            &EventParam,
                                            &Detect,
                                            prefix,
                                            postfix,
                                            &bEmptyBase,
                                            &eventCount,
                                            &bNotiFlag,
                                            &retryBuffLen,
                                            &buffLen );
                    break;
                case DISK:
                    frd_TaskDisk(  &RsltQData,
                                   &EventQData,
                                   &AlarmQData,
                                   &RetryQData,
                                   &EventParam,
                                   &Detect,
                                   prefix,
                                   postfix,
                                   &bEmptyBase,
                                   &eventCount,
                                   &bNotiFlag,
                                   &retryBuffLen,
                                   &buffLen );
                    break;
                case NET_DRIVE:
                    frd_TaskNetDrive(  &RsltQData,
                                       &EventQData,
                                       &AlarmQData,
                                       &RetryQData,
                                       &EventParam,
                                       &Detect,
                                       prefix,
                                       postfix,
                                       &bEmptyBase,
                                       &eventCount,
                                       &bNotiFlag,
                                       &retryBuffLen,
                                       &buffLen );

                    break;
                case OS_ACCOUNT:
                    frd_TaskOsAccount( &RsltQData,
                                       &RetryQData,
                                       &Detect,
                                       prefix,
                                       postfix,
                                       &bEmptyBase,
                                       &retryBuffLen,
                                       &buffLen );


                    break;
                case SHARE_FOLDER:
                    frd_TaskShareFolder(  &RsltQData,
                                          &EventQData,
                                          &AlarmQData,
                                          &RetryQData,
                                          &EventParam,
                                          &Detect,
                                          prefix,
                                          postfix,
                                          &bEmptyBase,
                                          &eventCount,
                                          &bNotiFlag,
                                          &retryBuffLen,
                                          &buffLen );

                    break;
                case INFRARED_DEVICE:
                    frd_TaskInfrared( &RsltQData,
                                      &EventQData,
                                      &AlarmQData,
                                      &RetryQData,
                                      &EventParam,
                                      &Detect,
                                      prefix,
                                      postfix,
                                      &bEmptyBase,
                                      &eventCount,
                                      &bNotiFlag,
                                      &retryBuffLen,
                                      &buffLen     );

                    break;
                case PROCESS:
                    frd_TaskProcess(  &RsltQData,
                                      &EventQData,
                                      &AlarmQData,
                                      &RetryQData,
                                      &EventParam,
                                      &Detect,
                                      prefix,
                                      postfix,
                                      &bEmptyBase,
                                      &eventCount,
                                      &bNotiFlag,
                                      &retryBuffLen,
                                      &buffLen);

                    break;
                case ROUTER:
                    frd_TaskRouter( &RsltQData,
                                    &EventQData,
                                    &AlarmQData,
                                    &RetryQData,
                                    &EventParam,
                                    &Detect,
                                    prefix,
                                    postfix,
                                    &bEmptyBase,
                                    &eventCount,
                                    &bNotiFlag,
                                    &retryBuffLen,
                                    &buffLen);

                    break;
                case NET_PRINTER:
                    frd_TaskNetPrinter(  &RsltQData,
                                         &EventQData,
                                         &AlarmQData,
                                         &RetryQData,
                                         &EventParam,
                                         &Detect,
                                         prefix,
                                         postfix,
                                         &bEmptyBase,
                                         &eventCount,
                                         &bNotiFlag,
                                         &retryBuffLen,
                                         &buffLen   );

                    break;
/*
            case ARP:

                memcpy(	(void *)&DetectData.Arp,
                        (void *)&RsltQData.buf[buffLen],
                        sizeof(DetectData.Arp) );
                buffLen += sizeof(DetectData.Arp);
                print_all_arp(&DetectData.Arp);
                rxt = merge_arp_tb(Detect.AgentInfo.user_key, &DetectData.Arp, prefix, postfix);
                if(rxt < 0) {
                    if(rxt == -2) bEmptyBase = 1;
                    memcpy(	(void *)&RetryQData.buf[retryBuffLen],
                            (void *)&DetectData.Arp,
                            sizeof(DetectData.Arp) );
                    retryBuffLen += sizeof(DetectData.Arp);
                }
                break;
*/
                case NET_SCAN:
                    frd_TaskNetScan(  &RsltQData,
                                      &RetryQData,
                                      &Detect,
                                      prefix,
                                      postfix,
                                      &bEmptyBase,
                                      &retryBuffLen,
                                      &buffLen);

                    break;
                case SSO_CERT:
                    frd_TaskSsoCert(  &RsltQData,
                                      &EventQData,
                                      &AlarmQData,
                                      &EventParam,
                                      &Detect,
                                      prefix,
                                      postfix,
                                      &eventCount,
                                      &bNotiFlag,
                                      &buffLen);

                    break;
                case WIN_DRV:
                    frd_TaskWinDrv(  &RsltQData,
                                     &RetryQData,
                                     &Detect,
                                     prefix,
                                     postfix,
                                     &bEmptyBase,
                                     &retryBuffLen,
                                     &buffLen );

                    break;
                case RDP_SESSION:
                    frd_TaskRdpSession(    &RsltQData,
                                           &EventQData,
                                           &AlarmQData,
                                           &RetryQData,
                                           &EventParam,
                                           &Detect,
                                           prefix,
                                           postfix,
                                           &bEmptyBase,
                                           &eventCount,
                                           &bNotiFlag,
                                           &retryBuffLen,
                                           &buffLen );

                    break;
                case CPU_USAGE:
                    frd_TaskCpuUsage(   &RsltQData,
                                        &EventQData,
                                        &AlarmQData,
                                        &RetryQData,
                                        &EventParam,
                                        &Detect,
                                        prefix,
                                        postfix,
                                        &bEmptyBase,
                                        &eventCount,
                                        &bNotiFlag,
                                        &retryBuffLen,
                                        &buffLen );
                    break;
                default:
                WRITE_INFO(CATEGORY_DB, "Unknown detect item(%d) ", DetectItem);
                    break;
            }//switch

            if( bNotiFlag )
            {
                WRITE_INFO(CATEGORY_DB,	"[NOTI](%s) userKey(%s)type(%d)level(%d)",
                           EventParam.user_ip,Detect.AgentInfo.user_key,
                           EventParam.ev_type,EventParam.ev_level);

                system(cmdEventChange);
                bNotiFlag = 0;
            }
            tokenKey = strtok_r(NULL, ",",&ptrTemp);
        }//while

        fcom_MallocFree((void**)&arrTokenKey);
        tokenKey = NULL;

        if(bEmptyBase)
        {
            rxt = frd_InsertEmptyBase(&Detect.AgentInfo);

            if(rxt < 0)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in insert_empty_base ");
            }
            else
            {
                WRITE_INFO(CATEGORY_DB, "Succeed in insert_empty_base ");
                system(cmdBaseChange);
            }
        }
    }
    else if(RsltQData.packtype == RETRY_EVENT_ALARM)
    {
        WRITE_INFO(CATEGORY_IPC, "[DQUE](%d) RETRY_EVENT_ALARM ", fipc_SQGetElCnt() );
        memset(sMsgLen, 0x00,                    sizeof(sMsgLen));
        memcpy(sMsgLen, (void *)&EventQData.buf,    sizeof(sMsgLen));
        WRITE_INFO(CATEGORY_IPC, "sMsgLen(%d)", atoi(sMsgLen));
        eventCount = atoi(sMsgLen);

        for(i=0; i<eventCount; i++)
        {
            memset(&EventParam, 0x00, sizeof(EventParam));
            memcpy(	(void *)&EventParam,
                       (void *)&EventQData.buf[eventCount*sizeof(EventParam)],
                       sizeof(EventParam) );
            rxt = frd_MergeEventTb(&EventParam, stLen);
            if(rxt > 0)
            {
                WRITE_INFO(CATEGORY_INFO, "[NOTI](%s) userKey(%s)type(%d)level(%d)",
                           EventParam.user_ip,Detect.AgentInfo.user_key,
                           EventParam.ev_type,EventParam.ev_level);

                system(cmdEventChange);
            }
            if(g_stProcFrdInfo.nEorActive == 1)
            {
                frd_WriteEor(&EventParam);
            }

        }
    }
    else if ( RsltQData.packtype == DATACODE_SERVICE_STATUS )
    {
        int  local_nOffset = 0;
        char local_szAgentStatus[1 +1] = {0x00,};
        char local_szHbSq[20 +1]       = {0x00,};

        WRITE_INFO(CATEGORY_IPC, "[DBMS] SERVICE STATUS ");

        memset(&AgentInfo, 0x00, sizeof(AgentInfo));
        memcpy((void *)&AgentInfo, (void *)&RsltQData.buf, sizeof(AgentInfo));

        local_nOffset += sizeof(AgentInfo);
        snprintf(local_szAgentStatus, sizeof(local_szAgentStatus), "%s", &RsltQData.buf[local_nOffset]);

        local_nOffset += sizeof(local_szAgentStatus);
        snprintf(local_szHbSq, sizeof(local_szHbSq), "%s", &RsltQData.buf[local_nOffset]);

        WRITE_DEBUG(CATEGORY_DEBUG,"Service Status : [%s] [%s] [%s]", AgentInfo.user_key, local_szAgentStatus, local_szHbSq);

        /** Insert Table **/
        if ( frd_InsertAgentStatusHist(&AgentInfo, local_szAgentStatus, local_szHbSq) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Insert Agent Status Failed");
            return (-1);
        }
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_IPC, "Unknown result queue type(%d)", RsltQData.packtype);
    }

    return 0;
}


//int frd_ConnDbPool(int param_ForkIdx)
//{
//    int local_nRet = 0;
//
//    local_nRet = fdb_ConnectDbPool( g_stFrdDbInfo.stMyCon, FRD_MAX_THREAD);
//    if(local_nRet < 0)
//    {
//        WRITE_CRITICAL(CATEGORY_DB, "DB Conn Failed ");
//        return (-1);
//    }
//    else
//    {
//        WRITE_INFO(CATEGORY_DB," Success In DB Conn");
//        return 0;
//    }
//
//}
