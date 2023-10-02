//
// Created by KimByoungGook on 2021-03-17.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "com/dap_com.h"
#include "sock/dap_sock.h"
#include "secure/dap_secure.h"
#include "secure/bcrypt.h"
#include "ipc/dap_Queue.h"

#include "pcif.h"


int fpcif_FileWriteInit(int param_ForkIndex)
{
    int  local_nForkIndex           = 0;
    char local_szFileName[32+1]     = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};
    FILE*           local_Fp = NULL;

    local_nForkIndex = param_ForkIndex;

    /** 파일명 Set
     * 검출 데이터 File 처리 Fork
     * **/
     fpcif_GetLastFileName( local_nForkIndex, local_szFileName, sizeof(local_szFileName));
     snprintf(local_szFilePath, sizeof(local_szFilePath),
              "%s%s", g_stProcPcifInfo.szDtFilePath, local_szFileName);

    WRITE_DEBUG(CATEGORY_DEBUG, "Fork (%d) File Write Path : [%s] ", local_nForkIndex, local_szFilePath);

    local_Fp = fopen(local_szFilePath, "wb");
    if(local_Fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File (%s) Open Failed ", local_szFilePath);
        return (-1);
    }

    snprintf( g_stProcPcifInfo.szFileName, sizeof(g_stProcPcifInfo.szFileName),
              "%s", local_szFileName);
    snprintf( g_stProcPcifInfo.szFileFullPath, sizeof(g_stProcPcifInfo.szFileFullPath),
              "%s",  local_szFilePath );
    g_stProcPcifInfo.Fp = local_Fp;

    /** Queue Init **/
    g_stProcPcifInfo.nRsltQsize = g_stProcPcifInfo.cfgThreadCnt / 2;
    fipc_SQInitNotBackup( g_stProcPcifInfo.nRsltQsize );

    return 0;

}

/*int fpcif_FileWriteWrLock(pthread_rwlock_t* param_Mutex_t)
{
    int local_loopCount = 0;

    while (1)
    {
        if ( pthread_rwlock_wrlock( param_Mutex_t ) == 0)
        {
            return 0;
        }
        else
        {
            fcom_SleepWait(5); // 0.01sec
            local_loopCount++;
        }
        if (local_loopCount >= 500)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Mutex loop break ");
            break;
        }
    }

    return (-1);
}

int fpcif_FileWriteRdLock(pthread_rwlock_t* param_Mutex_t)
{
    int local_loopCount = 0;

    while (1)
    {
        if ( pthread_rwlock_tryrdlock( param_Mutex_t ) == 0)
        {
            return 0;
        }
        else
        {
            fcom_SleepWait(5); // 0.01sec
            local_loopCount++;
        }
        if (local_loopCount >= 500)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Mutex loop break ");
            break;
        }
    }

    return (-1);
}*/
int fpcif_FileMutexLock(pthread_mutex_t* param_mutex_t)
{
    int local_loopCount = 0;

    while (1)
    {
        if ( pthread_mutex_trylock( param_mutex_t ) == 0)
        {
            return 0;
        }
        else
        {
            fcom_SleepWait(4); // 0.01sec
            local_loopCount++;
        }
        if (local_loopCount >= 500)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Mutex loop break ");
            break;
        }
    }

    return (-1);
}


int fpcif_GetLastFileIndex( char* param_FileName )
{
    int  local_LastIndex = 0;
    int  local_nFileIndex = 0;

    char local_szFileName[32 +1]        = {0x00,};
    char local_szFileIndex[5 +1]        = {0x00,};
    char local_szFilePath[256 +1]       = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;


    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcPcifInfo.szDtFilePath);


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir (%s) Failed ", local_szFilePath);
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

int fpcif_GetLastFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize)
{
    int local_nFileIndex = 0;
    char local_szTmp[32 + 1] = {0x00,};

    snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", param_ForkIdx);
    local_nFileIndex = fpcif_GetLastFileIndex(local_szTmp);

    if ( local_nFileIndex +1 > g_stProcPcifInfo.nCfgMaxFileIndex )
    {
        local_nFileIndex = 0;
    }
    snprintf(param_FileName, param_FileNameSize, "%s.%05d",
             local_szTmp,
             local_nFileIndex + 1);
    g_stProcPcifInfo.nLastFileIdx = local_nFileIndex + 1;

    return 0;
}

/** 파일 Write **/
int fpcif_WriteDataFile(int param_ForkIdx, _DAP_QUEUE_BUF* param_Buffer, int param_BufferSize, FILE* param_Fp)
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
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fwrite, Fork (%d) path(%s)", g_stProcPcifInfo.szFileFullPath );
        return (-1);
    }
    fflush(param_Fp);

    local_nFileSize = ftell(param_Fp);

    /** Config Size(MB) 이상이면 Index +1하여 신규 파일에 Write. **/
    if( local_nFileSize >= g_stProcPcifInfo.nCfgMaxFileSize )
    {
        if ( fpcif_GetNextFileSize( param_ForkIdx ) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG," fw_WriteDataFile Next File Failed (%s)", g_stProcPcifInfo.szFileFullPath);
            return (-1);
        }
    }

    return 0;
}

int fpcif_GetNextFileSize(int param_ForkIdx)
{
    char local_szTmp[32 +1] = {0x00,};
    char local_szFileName[32 +1] = {0x00,};
    char local_szFilePath[256 +1] = {0x00,};

    /** 파일인덱스 +1 **/
    g_stProcPcifInfo.nLastFileIdx++;

    if ( g_stProcPcifInfo.nLastFileIdx > g_stProcPcifInfo.nCfgMaxFileIndex ) // 마지막 파일 인덱스까지 간 경우
    {
        g_stProcPcifInfo.nLastFileIdx = 1;
    }

    /** 00001번 인덱스 파일 존재여부 확인 후 없을경우 00001번 파일부터 다시 시작. **/
    snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", param_ForkIdx ); // Fork Index는 1번부터 시작.
    snprintf( local_szFileName, sizeof(local_szFileName), "%s.%05d",
              local_szTmp,
              g_stProcPcifInfo.nLastFileIdx);
    memset( local_szFilePath, 0x00, sizeof(local_szFilePath));
    snprintf(local_szFilePath, sizeof(local_szFilePath),
             "%s%s", g_stProcPcifInfo.szDtFilePath, local_szFileName);


    if ( fcom_fileCheckStatus( local_szFilePath ) == 0 ) //파일 있음
    {
        while(1)
        {
            fcom_SleepWait(5);
            if( fcom_fileCheckStatus( local_szFilePath ) != 0) //파일이 없을떄 까지 대기.
            {
                break;
            }
            else
            {
                WRITE_DEBUG(CATEGORY_DEBUG, "File Is Exist (%s) ",  local_szFilePath);
            }
        }
    }


    snprintf( g_stProcPcifInfo.szFileName, sizeof(g_stProcPcifInfo.szFileName),
              "%s", local_szFileName);
    snprintf(g_stProcPcifInfo.szFileFullPath, sizeof(g_stProcPcifInfo.szFileFullPath),
             "%s", local_szFilePath );

    WRITE_DEBUG(CATEGORY_DEBUG,"Rename File : [%s] ", g_stProcPcifInfo.szFileFullPath);

    if ( g_stProcPcifInfo.Fp != NULL)
    {
        fclose(g_stProcPcifInfo.Fp);
        g_stProcPcifInfo.Fp = fopen(g_stProcPcifInfo.szFileFullPath, "wb");
        if ( g_stProcPcifInfo.Fp == NULL)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Next File (%s) Open Failed ", g_stProcPcifInfo.szFileFullPath);
            return (-1);
        }
    }

    return 0;
}


//void* fpcif_FileWriteThread(void* param_ThreadArg)
//{
//    _DAP_QUEUE_BUF  local_DbData;
//
//    memset( &local_DbData, 0x00, sizeof(_DAP_QUEUE_BUF) );
//
//    while(1)
//    {
//        memset( &local_DbData, 0x00, sizeof(_DAP_QUEUE_BUF) );
//
//        /** Queue에서 데이터를 가져온다. **/
//        /** Mutex **/
//        if ( fpcif_FileWriteRdLock( &mutexFile ) == 0)
//        {
//            if ( fipc_SQGet( &local_DbData ) == NULL ) // Queue Empty
//            {
//                pthread_rwlock_unlock( &mutexFile );
//                fcom_SleepWait(4);
//                continue;
//            }
//            else
//            {
//                WRITE_INFO(CATEGORY_INFO, "Get queue current(%d)", fipc_SQGetElCnt() );
//                pthread_rwlock_unlock( &mutexFile );
//
//                /** File Write **/
//                fpcif_WriteDataFile(g_nPcifForkIdx, &local_DbData, sizeof(_DAP_QUEUE_BUF), g_stProcPcifInfo.Fp );
//            }
//        }
//        else
//        {
//            WRITE_CRITICAL(CATEGORY_DEBUG,"Get Mutex Try Lock Failed ");
//            fcom_SleepWait(4);
//            continue;
//        }
//
//        fcom_SleepWait(4);
//    }
//
//    pthread_exit(NULL);
//}