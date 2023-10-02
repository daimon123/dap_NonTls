//
// Created by KimByoungGook on 2021-03-10.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "ipc/dap_qdef.h"

#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_req.h"
#include "json/dap_json.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "fw.h"
#include "dap_version.h"

int fw_ForkWork(void)
{
    time_t  local_nChkTime              = 0;
    time_t  local_nExecTime             = 0;
    int     local_nRecvLen              = 0;
    int     local_nRet                  = 0;
    int     local_nForkIdx              = 0;
    char    local_szFileName[32+1]     = {0x00,};
    char    local_szFilePath[256 +1]   = {0x00,};
    char    local_szUserKey[20 +1]     = {0x00,};
    char    local_szAgentStatus[1 +1]  = {0x00,};
    char    local_szHbSq[20 +1]        = {0x00,};
    _DAP_QUEUE_BUF local_DbData;
    FILE*           local_Fp        = NULL;

    local_nForkIdx = g_nForkIdx;

    memset( &local_DbData, 0x00, sizeof(_DAP_QUEUE_BUF) );

    /** 파일명 Set **/
    fw_GetLastFileName( local_nForkIdx, local_szFileName, sizeof(local_szFileName));
    snprintf(local_szFilePath, sizeof(local_szFilePath),
                 "%s%s", g_stProcFwInfo.szPolicyFilePath, local_szFileName);

    WRITE_DEBUG(CATEGORY_DEBUG, "Fork (%d) File Write Path : [%s] ", local_nForkIdx, local_szFilePath);
    local_Fp = fopen(local_szFilePath, "wb");
    if(local_Fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File (%s) Open Failed ", local_szFilePath);
        return (-1);
    }

    snprintf( g_stProcFwInfo.szFileName, sizeof(g_stProcFwInfo.szFileName),
              "%s", local_szFileName );
    snprintf( g_stProcFwInfo.szFileFullPath, sizeof(g_stProcFwInfo.szFileFullPath),
             "%s",  local_szFilePath );
    g_stProcFwInfo.Fp = local_Fp;

    if( local_nForkIdx == FORK_POLICY )
    {
        local_nRecvLen = sizeof(_DAP_AGENT_INFO)+sizeof(local_DbData.packtype);
    }
    else if(local_nForkIdx == FORK_SERVICE )
    {
        local_nRecvLen = sizeof(_DAP_AGENT_INFO) + sizeof(local_DbData.packtype) + sizeof(local_szAgentStatus) + sizeof(local_szHbSq);
    }


    while(1)
    {
        local_nChkTime = time(NULL);

        /**  Config Reload Check **/
        if( difftime(local_nChkTime, local_nExecTime) >= 60)
        {
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcFwInfo.nCfgLastModify) == 1)
            {
                fw_ReloadCfgFile();
                local_nExecTime = time(NULL);
            }
        }

        switch(local_nForkIdx)
        {
            case FORK_POLICY:
            {
                /** UDP Recv From **/
                local_nRet = fipc_FQGetData(FW_POLICY, (char *)&local_DbData, local_nRecvLen );

                /** File Write **/
                if ( local_nRet > 0)
                {
                    memset(local_szUserKey, 0x00, sizeof(local_szUserKey));
                    memcpy(local_szUserKey,&local_DbData.buf[17], 21);
                    WRITE_DEBUG(CATEGORY_DEBUG,"Policy Key (%s) Write File", local_szUserKey );

                    fw_WriteDataFile(g_nForkIdx, &local_DbData, local_nRecvLen, g_stProcFwInfo.Fp );
                }
                break;
            }

            case FORK_SERVICE:
            {
                /** UDP Recv From **/
                local_nRet = fipc_FQGetData(FW_SERVICE, (char *)&local_DbData, local_nRecvLen);
                /** File Write **/
                if ( local_nRet > 0)
                {
                    memset(local_szUserKey, 0x00, sizeof(local_szUserKey));
                    memcpy(local_szUserKey,&local_DbData.buf[17], 21);

                    fw_WriteDataFile(g_nForkIdx, &local_DbData, local_nRecvLen, g_stProcFwInfo.Fp );
                }
                break;
            }
        }


        fcom_SleepWait(2);fcom_SleepWait(2);
    }


    return (-1);

}


/** 오늘 날짜의 마지막 FileName의 Index를 가져온다. **/
/*int fw_GetLastFileIndex(char* param_FileName, char* param_MMDD)
{
    int  local_LastIndex = 0;
    int  local_nFileIndex = 0;
    int  local_nForkIdx = 0;

    char local_szFileName[32 +1]        = {0x00,};
    char local_szFileMMDD[4 +1]         = {0x00,};
    char local_szFileIndex[5 +1]        = {0x00,};
    char local_szFilePath[256 +1]       = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;
    char* ptrTokReturn              = NULL;

    local_nForkIdx = g_nForkIdx;

    if ( local_nForkIdx == FORK_POLICY)
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFwInfo.szPolicyFilePath);
    }
    else
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFwInfo.szDtFilePath);
    }


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        *//** . 상위디렉토리 파일 Skip **//*
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName,8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileMMDD,    0x00, sizeof(local_szFileMMDD));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        ptrTokReturn = strtok_r(local_szFileName, ".", &ptrTok);
        ptrTokReturn = strtok_r(NULL, "_", &ptrTok);
        snprintf(local_szFileMMDD, sizeof(local_szFileMMDD), "%s",   ptrTokReturn );
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);


        *//** 일자가 같으면 **//*
        if( memcmp(local_szFileMMDD, param_MMDD, 4) == 0)
        {
            if( local_nFileIndex > local_LastIndex )
            {
                local_LastIndex = local_nFileIndex;
            }
        }
    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return local_LastIndex;

}*/

int fw_GetLastFileIndex( char* param_FileName )
{
    int  local_LastIndex = 0;
    int  local_nFileIndex = 0;

    char local_szFileName[32 +1]        = {0x00,};
    char local_szFileIndex[5 +1]        = {0x00,};
    char local_szFilePath[256 +1]       = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;

    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFwInfo.szPolicyFilePath);


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName,8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        strtok_r(local_szFileName, ".", &ptrTok);
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);

        if( local_nFileIndex > local_LastIndex )
        {
            local_LastIndex = local_nFileIndex;
        }
    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return local_LastIndex;

}

/** 오늘 날짜의 마지막 File Name을 가져온다. **/
/*int fw_GetLastFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize)
{
    int             local_nFileIndex = 0;
    time_t          local_get_now;
    struct  tm      local_tm;
    char            local_szMMDD[4 +1] = {0x00,};
    char            local_szTmp[32 +1] = {0x00,};

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );

    snprintf(local_szMMDD, sizeof(local_szMMDD), "%02d%02d",
             local_tm.tm_mon+1,
             local_tm.tm_mday);

    *//** Fork Index별 검출데이터 파일의 오늘 날짜의 마지막 파일 Index를 가져온다. **//*
    if ( param_ForkIdx == FORK_POLICY )
    {
        local_nFileIndex = fw_GetLastFileIndex("FW_POLICY", local_szMMDD);
        snprintf( param_FileName, param_FileNameSize, "%s.%s_%05d",
                  "FW_POLICY", local_szMMDD, local_nFileIndex + 1);
        g_stProcFwInfo.nLastFileIdx = local_nFileIndex +1;

    }
    else
    {
        // Fork Index는 1번부터 시작.
        snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", param_ForkIdx-1 );
        local_nFileIndex = fw_GetLastFileIndex(local_szTmp, local_szMMDD);
        snprintf( param_FileName, param_FileNameSize, "%s.%s_%05d",
                  local_szTmp,
                  local_szMMDD,
                  local_nFileIndex + 1);
        g_stProcFwInfo.nLastFileIdx = local_nFileIndex +1;
    }
    snprintf(g_stProcFwInfo.szMMDD, sizeof(g_stProcFwInfo.szMMDD), "%s", local_szMMDD);

    return 0;

}*/

int fw_GetLastFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize)
{
    int local_nFileIndex = 0;

    switch (param_ForkIdx)
    {
        case FORK_POLICY:
        {
            /** Fork Index별 검출데이터 파일의 오늘 날짜의 마지막 파일 Index를 가져온다. **/
            local_nFileIndex = fw_GetLastFileIndex("FW_POLICY");
            if ( local_nFileIndex +1 > g_stProcFwInfo.nCfgMaxFileIndex )
            {
                local_nFileIndex = 0;
            }
            snprintf(param_FileName, param_FileNameSize, "%s.%05d",
                     "FW_POLICY", local_nFileIndex + 1);
            g_stProcFwInfo.nLastFileIdx = local_nFileIndex + 1;
            break;
        }

        case FORK_SERVICE:
        {
            /** Fork Index별 검출데이터 파일의 오늘 날짜의 마지막 파일 Index를 가져온다. **/
            local_nFileIndex = fw_GetLastFileIndex("FW_SERVICE");
            if ( local_nFileIndex +1 > g_stProcFwInfo.nCfgMaxFileIndex )
            {
                local_nFileIndex = 0;
            }
            snprintf(param_FileName, param_FileNameSize, "%s.%05d",
                     "FW_SERVICE", local_nFileIndex + 1);
            g_stProcFwInfo.nLastFileIdx = local_nFileIndex + 1;
            break;
        }
    }



    return 0;

}



/** 파일 Write **/
int fw_WriteDataFile(int param_ForkIdx, _DAP_QUEUE_BUF* param_Buffer, int param_BufferSize, FILE* param_Fp)
{
    int             local_nFileSize = 0;
    int             local_nRet = 0;
    time_t          local_get_now;
    struct  tm      local_tm;
    char            local_szMMDD[4 +1] = {0x00,};

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );

    snprintf(local_szMMDD, sizeof(local_szMMDD), "%02d%02d",
             local_tm.tm_mon+1,
             local_tm.tm_mday);

    fseek(param_Fp, 0, SEEK_END);

    local_nRet = fwrite(param_Buffer, sizeof(char), param_BufferSize, param_Fp);
    if ( local_nRet < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fwrite, Fork (%d) path(%s)", g_stProcFwInfo.szFileFullPath );
        return (-1);
    }
    fflush(param_Fp);

    local_nFileSize = ftell(param_Fp);
    /** 20MB이상이면 Index +1하여 신규 파일에 Write. **/
    if( local_nFileSize >= g_stProcFwInfo.nCfgMaxFileSize )
    {
        if ( fw_GetNextFileSize( param_ForkIdx ) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG," fw_WriteDataFile Next File Failed (%s)", g_stProcFwInfo.szFileFullPath);
            return (-1);
        }
    }

    return 0;
}


int fw_GetNextFileSize(int param_ForkIdx)
{
    char local_szFileName[32 +1] = {0x00,};
    char local_szFilePath[256 +1] = {0x00,};

    memset( local_szFilePath, 0x00, sizeof(local_szFilePath));

    switch(param_ForkIdx)
    {
        case FORK_POLICY:
        {
            /** 파일인덱스 +1 **/
            g_stProcFwInfo.nLastFileIdx++;
            if ( g_stProcFwInfo.nLastFileIdx > g_stProcFwInfo.nCfgMaxFileIndex ) // 마지막 파일 인덱스까지 간 경우
            {
                g_stProcFwInfo.nLastFileIdx = 1;
            }

            /** 00001번 인덱스 파일 존재여부 확인 후 없을경우 00001번 파일부터 다시 시작. **/
            snprintf( local_szFileName, sizeof(local_szFileName), "%s.%05d",
                      "FW_POLICY", g_stProcFwInfo.nLastFileIdx);
            snprintf(local_szFilePath, sizeof(local_szFilePath),
                     "%s%s", g_stProcFwInfo.szPolicyFilePath, local_szFileName);
            break;
        }

        case FORK_SERVICE:
        {
            /** 파일인덱스 +1 **/
            g_stProcFwInfo.nLastFileIdx++;
            if ( g_stProcFwInfo.nLastFileIdx > g_stProcFwInfo.nCfgMaxFileIndex ) // 마지막 파일 인덱스까지 간 경우
            {
                g_stProcFwInfo.nLastFileIdx = 1;
            }

            /** 00001번 인덱스 파일 존재여부 확인 후 없을경우 00001번 파일부터 다시 시작. **/
            snprintf( local_szFileName, sizeof(local_szFileName), "%s.%05d",
                      "FW_SERVICE", g_stProcFwInfo.nLastFileIdx);
            snprintf(local_szFilePath, sizeof(local_szFilePath),
                     "%s%s", g_stProcFwInfo.szPolicyFilePath, local_szFileName);
            break;
        }
    }

    if ( fcom_fileCheckStatus( local_szFilePath ) == 0 ) //파일 있음
    {
        while(1)
        {
            fcom_SleepWait(5);
            if( fcom_fileCheckStatus( local_szFilePath ) != 0) //파일이 없을떄 까지 대기.
            {
                break;
            }
        }
    }

    snprintf( g_stProcFwInfo.szFileName, sizeof(g_stProcFwInfo.szFileName),
              "%s", local_szFileName);
    snprintf(g_stProcFwInfo.szFileFullPath, sizeof(g_stProcFwInfo.szFileFullPath),
             "%s", local_szFilePath );

    WRITE_DEBUG(CATEGORY_DEBUG,"Rename File : [%s] ", g_stProcFwInfo.szFileFullPath);

    if ( g_stProcFwInfo.Fp != NULL)
    {
        fclose(g_stProcFwInfo.Fp);
        g_stProcFwInfo.Fp = fopen(g_stProcFwInfo.szFileFullPath, "wb");
        if ( g_stProcFwInfo.Fp == NULL)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Next File (%s) Open Failed ", g_stProcFwInfo.szFileFullPath);
            return (-1);
        }
    }

    return 0;
}