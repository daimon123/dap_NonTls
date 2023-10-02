//
// Created by KimByoungGook on 2021-12-30.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "com/dap_com.h"
#include "report.h"

#if 0

// CPU HISTORY
void freport_CpuHistory(char* param_szDbName, char* param_szTableName, char* param_szWhereTime,
                       int* param_TotalCnt, int* param_PrevCnt, int* param_ResultIdx,
                       struct _STD_EVENT_RESULT* param_ResultEvent,
                       struct _EVENT_HISTORY* param_EventHistory)
{
    int     local_nRow = 0;
    int     local_nIdx = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    // CPU History
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT CU_DETECT_TIME, HB_SQ, US_SQ  FROM %s.%s WHERE CU_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, CU_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"CPU History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );

            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );
            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
              param_EventHistory,
            param_ResultEvent, ENUM_CPU);

    fcom_MallocFree((void**) &local_ptrDetectInfo );
    return;
}

// PROCESS HISTORY
void freport_ProcessHistory(char* param_szDbName,
                            char* param_szTableName,
                            char* param_szWhereTime,
                            int* param_TotalCnt,
                            int* param_PrevCnt,
                            struct _STD_EVENT_RESULT* param_ResultEvent,
                            struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    // Process History
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT PS_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE PS_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, PS_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Process History History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
              param_EventHistory,
            param_ResultEvent, ENUM_PROCESS);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Netadapter History
void freport_NetadapterHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT NA_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE NA_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, NA_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Net Adapter History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_NET_ADAPTER);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// DISK History
void freport_DiskHistory(   char* param_szDbName,
                            char* param_szTableName,
                            char* param_szWhereTime,
                            int* param_TotalCnt,
                            int* param_PrevCnt,
                            struct _STD_EVENT_RESULT* param_ResultEvent,
                            struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT DK_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE DK_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, DK_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Disk History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_DISK);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}


// Netdrive History
void freport_NetdriveHistory(char* param_szDbName,
                             char* param_szTableName,
                             char* param_szWhereTime,
                             int* param_TotalCnt,
                             int* param_PrevCnt,
                             struct _STD_EVENT_RESULT* param_ResultEvent,
                             struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT ND_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE ND_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, ND_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"NetDrive History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent,
            ENUM_NET_DRIVE
            );

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Net Printer History
void freport_NetPrinterHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT NP_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE NP_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, NP_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"NetPrinter History Execute Query [%s] ", local_szQuery );


    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_NET_PRINTER);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Network Connection History
void freport_NetConnectionHistory(char* param_szDbName,
                                  char* param_szTableName,
                                  char* param_szWhereTime,
                                  int* param_TotalCnt,
                                  int* param_PrevCnt,
                                  struct _STD_EVENT_RESULT* param_ResultEvent,
                                  struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT NC_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE NC_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, NC_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"NetConnection History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_NET_CONNECTION);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Bluetooth History
void freport_BlueToothHistory(char* param_szDbName,
                              char* param_szTableName,
                              char* param_szWhereTime,
                              int* param_TotalCnt,
                              int* param_PrevCnt,
                              struct _STD_EVENT_RESULT* param_ResultEvent,
                              struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT BT_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE BT_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, BT_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"BlueTooth History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_WIFI);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Infrared History
void freport_InfraredHistory(char* param_szDbName,
                             char* param_szTableName,
                             char* param_szWhereTime,
                             int* param_TotalCnt,
                             int* param_PrevCnt,
                             struct _STD_EVENT_RESULT* param_ResultEvent,
                             struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT ID_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE ID_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, ID_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Infrared History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_INFRARE);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// Router History
void freport_RouterHistory(char* param_szDbName,
                           char* param_szTableName,
                           char* param_szWhereTime,
                           int* param_TotalCnt,
                           int* param_PrevCnt,
                           struct _STD_EVENT_RESULT* param_ResultEvent,
                           struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT RT_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE RT_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, RT_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Router History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_ROUTER);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// WIFI History
void freport_WifiHistory(char* param_szDbName,
                         char* param_szTableName,
                         char* param_szWhereTime,
                         int* param_TotalCnt,
                         int* param_PrevCnt,
                         struct _STD_EVENT_RESULT* param_ResultEvent,
                         struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT WF_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE WF_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, WF_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Wifi History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_WIFI);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// ShareFolder History
void freport_ShareFolderHistory(char* param_szDbName,
                                char* param_szTableName,
                                char* param_szWhereTime,
                                int* param_TotalCnt,
                                int* param_PrevCnt,
                                struct _STD_EVENT_RESULT* param_ResultEvent,
                                struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT SF_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE SF_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, SF_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"ShareFolder History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_SHARE_FOLDER);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// System History
void freport_SystemHistory(char* param_szDbName,
                           char* param_szTableName,
                           char* param_szWhereTime,
                           int* param_TotalCnt,
                           int* param_PrevCnt,
                           struct _STD_EVENT_RESULT* param_ResultEvent,
                           struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT ST_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE ST_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, ST_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"System History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_VIRTUAL_MACHINE);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// ConnExt History
void freport_ConnExtHistory(char* param_szDbName,
                            char* param_szTableName,
                            char* param_szWhereTime,
                            int* param_TotalCnt,
                            int* param_PrevCnt,
                            struct _STD_EVENT_RESULT* param_ResultEvent,
                            struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT CE_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE CE_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, CE_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"System History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_CONNEXT);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}

// RDP Session History
void freport_RdpSessionHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT RDP_DETECT_TIME, HB_SQ, US_SQ FROM %s.%s WHERE RDP_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, RDP_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"RDP Session History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_RDP);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}


// CPU Ctrl History
void freport_CpuCtrlHistory(   char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory)
{
    int     local_nRow = 0;
    char    local_szQuery[1023 +1]     = {0x00,};
    int     local_nIdx = 0;
    struct _STD_EVENT_INFO* local_ptrDetectInfo = NULL;

    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT CP_DETECT_TIME, HB_SQ, US_SQ from %s.%s WHERE CP_DETECT_TIME LIKE '%s%%' "
             " GROUP BY HB_SQ, CP_DETECT_TIME ",
             param_szDbName, param_szTableName, param_szWhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"CPU Ctrl History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        *param_TotalCnt += local_nRow;
        // 검출 데이터
        fcom_malloc((void**)&local_ptrDetectInfo, sizeof(struct _STD_EVENT_INFO) * local_nRow );
        if ( param_ResultEvent->ptrStdEventInfo == NULL )
        {
            fcom_malloc((void**)&param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            *param_PrevCnt = *param_TotalCnt;
        }
        else
        {
            struct _STD_EVENT_INFO* local_ptrTemp = param_ResultEvent->ptrStdEventInfo;
            param_ResultEvent->ptrStdEventInfo = realloc( param_ResultEvent->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt) );
            // 추가한 메모리 초기화
            memset( &(param_ResultEvent->ptrStdEventInfo[*param_PrevCnt]), 0x00,
                    (sizeof(struct _STD_EVENT_INFO) * (*param_TotalCnt)) - sizeof(struct _STD_EVENT_INFO) * (*param_PrevCnt) );

            *param_PrevCnt = *param_TotalCnt;
            if ( param_ResultEvent->ptrStdEventInfo == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc Failed");
                free(local_ptrTemp);
                free(local_ptrDetectInfo);
                return;
            }
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // DETECT_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szDetectTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(local_ptrDetectInfo[local_nIdx].szRecordTime,
                     sizeof(local_ptrDetectInfo[local_nIdx].szRecordTime),
                     "%s", local_ptrDetectInfo[local_nIdx].szDetectTime);
            local_ptrDetectInfo[local_nIdx].nHbSq = atol(g_stMyCon->row[1]);
            local_ptrDetectInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[2]);
            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    // Compare Event History
    freport_CompEventHistory(
            local_nRow,
            local_ptrDetectInfo,
            param_EventHistory,
            param_ResultEvent, ENUM_CPU_CONTROL);

    fcom_MallocFree((void**) &local_ptrDetectInfo );

    return;
}
#endif