//
// Created by KimByoungGook on 2021-05-10.
//

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"

#include "db/dap_mysql.h"
#include "json/dap_json.h"
#include "sock/dap_sock.h"
#include "frd.h"
#include "dap_version.h"

char g_ExecFileName[256];
int frd_DumpExec(void)
{
    int             local_nRet                  = 0;
    _DAP_QUEUE_BUF  local_DbData;

    memset( &local_DbData, 0x00, sizeof(_DAP_QUEUE_BUF) );

    /** 부모에서 등록한 SIGCHILD해제, system()함수 내부적으로 shell fork되면서 SIGCHILD 반횐된다. **/
    signal( SIGCHLD, SIG_IGN );

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

    /** FW_DT의 모든 파일 Name을 가져온다. **/
    local_nRet = frd_DumpDtLoop();
    if ( local_nRet < 0 )
    {
        printf("Detect Data Dump Exec Failed (%d) \n", local_nRet);
        return (-1);
    }

    /** FW_DT_LAST파일 삭제 **/
    frd_DumpDeleteDtLast();

    return 0;

}

int frd_DumpDtLoop(void)
{
    int  local_nReadSize            = 0;
    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};

    _DAP_QUEUE_BUF          local_QueueBuf;
    DIR* local_pDir                 = NULL;
    FILE* local_Fp                  = NULL;
    struct dirent* local_pstDirent  = NULL;

    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);

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

        if( memcmp(local_pstDirent->d_name, "FW_DT_LAST",10) == 0)
            continue;

        if( memcmp(local_pstDirent->d_name, "FW_DT_",6) != 0)
            continue;


        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        WRITE_DEBUG(CATEGORY_DEBUG,"Read DT File [%s]", local_szFileName);

        memset(local_szFilePath, 0x00, sizeof(local_szFilePath));
        snprintf(local_szFilePath, sizeof(local_szFilePath),"%s/%s", g_stProcFrdInfo.szDtFilePath, local_szFileName);
        local_Fp = fopen(local_szFilePath,"rb");
        snprintf(g_ExecFileName, sizeof(g_ExecFileName), "%s", local_szFilePath); //SEGV 경우 삭제를 위해

        while(1)
        {
            local_nReadSize = fread( &local_QueueBuf, sizeof(char), sizeof(_DAP_QUEUE_BUF), local_Fp );
            if( local_nReadSize <= 0 ) /** Reading Byte Is Zero && EOF **/
            {
                printf("File Is Deleted [%s] \n", local_szFilePath);
                unlink(local_szFilePath);

                /** 삭제 후 다음파일 처리 **/
                fcom_SleepWait(3);
                fclose(local_Fp);
                local_Fp = NULL;
                break;
            }
            else
            {
                frd_BufferToDBMS(0, &local_QueueBuf);
            }
        }

    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return 0;
}

void frd_DumpDeleteDtLast(void)
{
    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;

    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);

    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return;
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, "FW_DT_LAST",10) != 0)
            continue;


        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        WRITE_DEBUG(CATEGORY_DEBUG,"Read DT File [%s]", local_szFileName);
        printf("Read DT File [%s] \n", local_szFileName);

        memset(local_szFilePath, 0x00, sizeof(local_szFilePath));
        snprintf(local_szFilePath, sizeof(local_szFilePath),"%s/%s", g_stProcFrdInfo.szDtFilePath, local_szFileName);

        /** FW_DT_LAST* 파일 삭제 **/
        unlink(local_szFilePath);
        printf("File Deleted Open [%s] \n", local_szFilePath);

    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return;
}