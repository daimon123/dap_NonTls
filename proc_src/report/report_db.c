
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

int freport_InsertKeyValue(char** param_KeyArray, int param_nKeyCnt, char* param_InsertKey)
{
    int local_nLoop = 0;

    for ( local_nLoop = 0; local_nLoop < param_nKeyCnt; local_nLoop++ )
    {
        if ( param_KeyArray[local_nLoop][0] == 0x00 )
        {
            snprintf(param_KeyArray[local_nLoop], 64, "%s", param_InsertKey);
            return 0;
        }
    }

    return (-1);
}

int freport_CheckKeyValue( char** param_KeyArray, int param_nKeyCnt, char* param_FindKey )
{
    int local_nLoop = 0;

    for ( local_nLoop = 0; local_nLoop < param_nKeyCnt; local_nLoop++ )
    {
        if ( memcmp(param_KeyArray[local_nLoop], param_FindKey, strlen(param_FindKey) ) == 0) //키 찾음
        {
            return local_nLoop;
        }
    }

    return (-1); //키 못찾음
}



int freport_GetGroupInfo( struct _STD_EVENT_INFO* param_StdEventInfo, int param_EventCnt)
{
    int local_nLoop = 0, local_nLoop2 = 0;
    int local_nRow = 0;
    int local_nIdx = 0;
    char local_szQuery[2024] = {0x00,};
    struct _USER_LINK_INFO
    {
        long nUgSq;
        long nUsSq;
    };
    struct _USER_LINK_INFO* local_ptrUserLinkInfo = NULL;

    // TEMP 테이블 생성
    snprintf(local_szQuery, sizeof(local_szQuery),"select UG_SQ,US_SQ from DAP_DB.USER_LINK_TB ");

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if ( local_nRow > 0)
    {
        fcom_malloc((void**) &local_ptrUserLinkInfo, sizeof(struct _USER_LINK_INFO) * local_nRow );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            local_ptrUserLinkInfo[local_nIdx].nUgSq = atol(g_stMyCon->row[0]);
            local_ptrUserLinkInfo[local_nIdx].nUsSq = atol(g_stMyCon->row[1]);
            local_nIdx++;
        }

    }
    fdb_SqlFreeResult(g_stMyCon);


    for ( local_nLoop = 0; local_nLoop < param_EventCnt; local_nLoop++ )
    {
        for( local_nLoop2 = 0; local_nLoop2 < local_nIdx; local_nLoop2++ )
        {
            if ( param_StdEventInfo[local_nLoop].nEventType != 0 )
            {
                if ( local_ptrUserLinkInfo[local_nLoop2].nUsSq == param_StdEventInfo[local_nLoop].nUsSq )
                {
                    param_StdEventInfo[local_nLoop].nUgSq = local_ptrUserLinkInfo[local_nLoop2].nUgSq;
                    break;
                }
            }
        }
    }

    return 0;

}

int freport_InsertStdEvent( struct _STD_EVENT_INFO* param_StdEventInfo, int param_Eventcnt, char* param_WhereTime, char param_cRealTimeFlag )
{
    int local_nLoop = 0;
    int local_nRowCnt = 0;
    char local_szQuery[2024] = {0x00,};

    time_t  YesterDay_t;
    char    local_szWhereTime[12] = {0x00,};
    YesterDay_t = time(NULL)-86400;

    if ( param_cRealTimeFlag != 0x00 ) //실시간 보고서
    {
        snprintf(local_szWhereTime, sizeof(local_szWhereTime), "%s", param_WhereTime);
    }
    else //정기보고서
    {
        // 1 day age
        fcom_time2str(YesterDay_t, local_szWhereTime, "YYYY-MM-DD");
    }
    strcat(local_szWhereTime,"%");

    snprintf(local_szQuery, sizeof(local_szQuery), "SELECT 1 FROM Information_schema.tables "
                                                   "WHERE table_schema = 'DAP_DB' "
                                                   "  AND table_name = 'STD_EVENT_TB_TEMP'" );
    local_nRowCnt = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    fdb_SqlFreeResult(g_stMyCon);

    if ( local_nRowCnt <= 0)
    {
        // TEMP 테이블 생성
        memset( local_szQuery, 0x00, sizeof(local_szQuery) );
        snprintf(local_szQuery, sizeof(local_szQuery),
                 "create table DAP_DB.STD_EVENT_TB_TEMP "
                 "( "
                 "    STD_SQ          bigint unsigned zerofill auto_increment comment '고유 번호' "
                 "        primary key, "
                 "    STD_RECORD_TIME varchar(13)   not null comment '데이터 검출 시간(HOUR)', "
                 "    STD_IPADDR      varchar(15)   not null comment '이벤트발생 IP', "
                 "    STD_US_SQ       bigint        not null comment '이벤트발생 US_SQ', "
                 "    STD_UG_SQ       bigint        not null comment '이벤트발생 UG_SQ', "
                 "    STD_TYPE        smallint(3)   not null comment '이벤트 유형', "
                 "    STD_LEVEL       smallint(1)   not null comment '이벤트 레벨', "
                 "    STD_EXIST       smallint(1)   not null comment '0:해제,1:발생,2:중복,3:강제확인,4:강제삭제' "
                 ") engine = Aria charset = utf8 ");
        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }


    memset( local_szQuery, 0x00, sizeof(local_szQuery) );
    snprintf(local_szQuery,
             sizeof(local_szQuery)," DELETE FROM DAP_DB.STD_EVENT_TB_TEMP ");
    fdb_SqlQuery2(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    for ( local_nLoop = 0; local_nLoop < param_Eventcnt; local_nLoop++ )
    {
        memset( local_szQuery, 0x00, sizeof(local_szQuery) );

        snprintf(local_szQuery, sizeof(local_szQuery),
                 "INSERT INTO DAP_DB.STD_EVENT_TB_TEMP( "
                 "   STD_RECORD_TIME, "
                 "   STD_IPADDR, "
                 "   STD_US_SQ, "
                 "   STD_UG_SQ, "
                 "   STD_TYPE, "
                 "   STD_LEVEL, "
                 "   STD_EXIST "
                 "   ) VALUES ('%s', '%s', %d, %d, %d, %d, %d)",
                 param_StdEventInfo[local_nLoop].szRecordTime,
                 param_StdEventInfo[local_nLoop].szIpAddr,
                 param_StdEventInfo[local_nLoop].nUsSq,
                 param_StdEventInfo[local_nLoop].nUgSq,
                 param_StdEventInfo[local_nLoop].nEventType,
                 param_StdEventInfo[local_nLoop].nEventLevel,
                 param_StdEventInfo[local_nLoop].nEventExist );


        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            if (g_stMyCon->nErrCode == 2014)
            {
                fdb_SqlFreeResult(g_stMyCon);
            }
        }

        fcom_SleepWait(2);
        fcom_SleepWait(2);
    }


    if ( param_cRealTimeFlag != 0x00 ) //실시간 보고서
    {
        // Key를 없앴으니 중복될수 있으므로 해당 날짜의 Delete 쿼리를 추가.
        memset ( local_szQuery, 0x00, sizeof(local_szQuery));
        snprintf(local_szQuery, sizeof(local_szQuery),
                 "DELETE FROM DAP_DB.STD_EVENT_REALTIME_TB WHERE STDR_RECORD_TIME LIKE '%s%%' ", local_szWhereTime);
        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }


        memset( local_szQuery, 0x00, sizeof(local_szQuery) );
        snprintf(local_szQuery, sizeof(local_szQuery), "%s",
                 "INSERT INTO DAP_DB.STD_EVENT_REALTIME_TB "
                 " ( "
                 "           STDR_RECORD_TIME, STDR_IPADDR, STDR_US_SQ, STDR_UG_SQ, STDR_TYPE, STDR_LEVEL, STDR_EXIST, STDR_COUNT "
                 " ) "
                 "   SELECT STD_RECORD_TIME, STD_IPADDR, STD_US_SQ, STD_UG_SQ, STD_TYPE, STD_LEVEL, STD_EXIST, count(*) as STD_COUNT "
                 " FROM STD_EVENT_TB_TEMP "
                 "group by STD_RECORD_TIME, STD_IPADDR, STD_US_SQ, STD_TYPE, STD_LEVEL, STD_EXIST ");

        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }

    }
    else //정기 보고서
    {
        // Key를 없앴으니 중복될수 있으므로 해당 날짜의 Delete 쿼리를 추가.
        memset ( local_szQuery, 0x00, sizeof(local_szQuery));
        snprintf(local_szQuery, sizeof(local_szQuery),
                 "DELETE FROM DAP_DB.STD_EVENT_TB WHERE STD_RECORD_TIME LIKE '%s%%' ", local_szWhereTime);
        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }

        memset( local_szQuery, 0x00, sizeof(local_szQuery) );
        snprintf(local_szQuery, sizeof(local_szQuery), "%s",
                 "INSERT INTO DAP_DB.STD_EVENT_TB ( "
                 "           STD_RECORD_TIME, STD_IPADDR, STD_US_SQ, STD_UG_SQ, STD_TYPE, STD_LEVEL, STD_EXIST, STD_COUNT ) "
                 "   SELECT STD_RECORD_TIME, STD_IPADDR, STD_US_SQ, STD_UG_SQ, STD_TYPE, STD_LEVEL, STD_EXIST, count(*) as STD_COUNT "
                 " FROM STD_EVENT_TB_TEMP "
                 "group by STD_RECORD_TIME, STD_IPADDR, STD_US_SQ, STD_TYPE, STD_LEVEL, STD_EXIST ");

        fdb_SqlQuery2(g_stMyCon, local_szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }


    return 0;
}

int freport_SelectEvent(struct _EVENT_INFO* param_EventInfo,
                        char* param_DbPreFix, char* param_TablePreFix, char* param_WhereTime)
{
    int         local_nRow                      = 0;
    int         local_nIdx                      = 0;
    int         local_nEventType                = 0;
    int         local_nHbSq                     = 0;

    char        local_szQuery[1023 +1]          = {0x00,};
    char        local_szTableName[31 +1]        = {0x00,};
    char        local_szDbName[31 +1]           = {0x00,};

    // Set Database
    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_DB" );

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "EVENT_TB" );

    // DETECT
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT HB_SQ, EV_TYPE, EV_DETECT_TIME, EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
             " FROM %s.%s "
             " WHERE EV_DETECT_TIME LIKE '%s%%' "
             " AND EV_TYPE NOT IN(40,41,42) "
             " group by HB_SQ, EV_TYPE, EV_DETECT_TIME ",
             local_szDbName, local_szTableName, param_WhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Select Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        fcom_malloc((void**) &(param_EventInfo->ptrEventTbDetect),
                    sizeof(struct _EVENT_TB_INFO) * local_nRow );
        param_EventInfo->nEventCnt = local_nRow;
        WRITE_DEBUG(CATEGORY_DEBUG,"[Select Event TB] Cnt : [%d] [%d]",
                    param_EventInfo->nEventCnt, local_nRow);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // HB_SQ
            local_nHbSq = atoi(g_stMyCon->row[0]);
            param_EventInfo->ptrEventTbDetect[local_nIdx].nHbSq = local_nHbSq;

            // EVENT TYPE
            local_nEventType = atoi(g_stMyCon->row[1]);
            param_EventInfo->ptrEventTbDetect[local_nIdx].nEvType = local_nEventType;

            // DETECT TIME
            if ( g_stMyCon->row[2] != NULL )
            {
                snprintf( param_EventInfo->ptrEventTbDetect[local_nIdx].szDetectTime,
                          sizeof(param_EventInfo->ptrEventTbDetect[local_nIdx].szDetectTime),
                          "%s", g_stMyCon->row[2]);

                // RECORD_TIME
                snprintf(param_EventInfo->ptrEventTbDetect[local_nIdx].szRecordTime,
                         sizeof(param_EventInfo->ptrEventTbDetect[local_nIdx].szRecordTime),
                         "%s", param_EventInfo->ptrEventTbDetect[local_nIdx].szDetectTime);
            }

            // IP ADDR
            if ( g_stMyCon->row[3] != NULL )
            {
                snprintf( param_EventInfo->ptrEventTbDetect[local_nIdx].szIpAddr,
                          sizeof(param_EventInfo->ptrEventTbDetect[local_nIdx].szIpAddr),
                          "%s", g_stMyCon->row[3]);
            }

            // EVENT LEVEL
            param_EventInfo->ptrEventTbDetect[local_nIdx].nEvLevel = atoi(g_stMyCon->row[4]);

            // EVENT EXIST
            param_EventInfo->ptrEventTbDetect[local_nIdx].nEvExist = atoi(g_stMyCon->row[5]);

            // US_SQ
            param_EventInfo->ptrEventTbDetect[local_nIdx].nUsSq = atoi(g_stMyCon->row[6]);

            local_nIdx++;
        }

    }

    fdb_SqlFreeResult(g_stMyCon);

    // DUP_DETECT
    local_nIdx = 0;
    memset( local_szQuery, 0x00, sizeof(local_szQuery) );
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT HB_SQ, EV_TYPE, EV_DUP_DETECT_TIME, EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
             " FROM %s.%s "
            " WHERE EV_DUP_DETECT_TIME LIKE '%s%%' "
            " AND EV_TYPE NOT IN(40,41,42) "
             " group by HB_SQ, EV_TYPE, EV_DUP_DETECT_TIME ",
             local_szDbName, local_szTableName, param_WhereTime );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        fcom_malloc((void**) &(param_EventInfo->ptrEventDuplDetect),
                    sizeof(struct _EVENT_TB_INFO) * local_nRow );
        param_EventInfo->nDuplCnt = local_nRow;
        WRITE_DEBUG(CATEGORY_DEBUG,"[Select Dupl Event TB] Cnt : [%d] [%d]",
                    param_EventInfo->nDuplCnt, local_nRow);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // HB_SQ
            local_nHbSq = atoi(g_stMyCon->row[0]);
            param_EventInfo->ptrEventDuplDetect[local_nIdx].nHbSq = local_nHbSq;

            // EVENT TYPE
            local_nEventType = atoi(g_stMyCon->row[1]);
            param_EventInfo->ptrEventDuplDetect[local_nIdx].nEvType = local_nEventType;

            // DUP DETECT TIME
            if ( g_stMyCon->row[2] != NULL )
            {
                snprintf( param_EventInfo->ptrEventDuplDetect[local_nIdx].szDetectTime,
                          sizeof(param_EventInfo->ptrEventDuplDetect[local_nIdx].szDetectTime),
                          "%s", g_stMyCon->row[2]);

                // RECORD TIME
                snprintf(param_EventInfo->ptrEventDuplDetect[local_nIdx].szRecordTime,
                         sizeof(param_EventInfo->ptrEventDuplDetect[local_nIdx].szRecordTime),
                         "%s", param_EventInfo->ptrEventDuplDetect[local_nIdx].szDetectTime);
            }

            // IP ADDR
            if ( g_stMyCon->row[3] != NULL )
            {
                snprintf( param_EventInfo->ptrEventDuplDetect[local_nIdx].szIpAddr,
                          sizeof(param_EventInfo->ptrEventDuplDetect[local_nIdx].szIpAddr),
                          "%s", g_stMyCon->row[3]);
            }

            // EVENT LEVEL
            param_EventInfo->ptrEventDuplDetect[local_nIdx].nEvLevel = atoi(g_stMyCon->row[4]);

            // EVENT EXIST
            param_EventInfo->ptrEventDuplDetect[local_nIdx].nEvExist = atoi(g_stMyCon->row[5]);

            // US_SQ
            param_EventInfo->ptrEventDuplDetect[local_nIdx].nUsSq = atoi(g_stMyCon->row[6]);

            local_nIdx++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);


    return 0;
}
//int freport_SelectEventHistory(struct _EVENT_HISTORY*       param_EventHistInfo,
//                               char* param_DbPreFix, char*  param_TablePreFix)
//{
//    int         local_nIdx                      = 0;
//    int         local_nRow                      = 0;
//
//    int         local_nEventType                = 0;
//    int         local_nHbSq                     = 0;
//
//    char        local_szWhereTime[19 +1]        = {0x00,}; // WHERE 조건시간
//    char        local_szTableName[31 +1]        = {0x00,};
//    char        local_szDbName[31 +1]           = {0x00,};
//    char        local_szQuery[1023 +1]          = {0x00,};
//    char        local_szGetTime[14 +1]     = {0x00,};
//    char        local_szYYYY[4 +1]         = {0x00,};
//    char        local_szMM[2 +1]           = {0x00,};
//    char        local_szDD[2 +1]           = {0x00,};
//
//    // Get Time, 어제 날짜를 가져온다
//    fcom_GetTime(local_szGetTime, 86400 );
//
//    memcpy( local_szYYYY, &local_szGetTime[0], 4 ); // YYYY
//    memcpy( local_szMM, &local_szGetTime[4], 2 );   // MM
//    memcpy ( local_szDD, &local_szGetTime[6], 2);   // DD
//
//    // Set Database
//    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_HISTORY_%s", param_DbPreFix );
//
//    // Set TableName
//    snprintf(local_szTableName, sizeof(local_szTableName), "EVENT_HISTORY_%s", param_TablePreFix );
//
//    snprintf(local_szWhereTime, sizeof(local_szWhereTime), "%s-%s-%s", local_szYYYY, local_szMM, local_szDD);
//
//    // DETECT TIME
//    snprintf(local_szQuery, sizeof(local_szQuery),
//             "SELECT HB_SQ, EV_TYPE, EV_DETECT_TIME, EV_DUP_DETECT_TIME, EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
//             " FROM %s.%s "
//             " WHERE EV_DETECT_TIME LIKE '%s%%' OR EV_DUP_DETECT_TIME LIKE '%s%%' "
//             " group by HB_SQ, EV_TYPE, EV_DETECT_TIME, EV_DUP_DETECT_TIME ",
//             local_szDbName, local_szTableName, local_szWhereTime, local_szWhereTime );
//
//    WRITE_DEBUG(CATEGORY_DEBUG,"@@ Execute Query [%s] ", local_szQuery );
//
//    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
//    if(g_stMyCon->nErrCode != 0)
//    {
//        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
//                       g_stMyCon->nErrCode,
//                       g_stMyCon->cpErrMsg);
//    }
//
//    if( local_nRow > 0 )
//    {
//        fcom_malloc((void**) &param_EventHistInfo->ptrEventHistoryDetect,
//                    sizeof(struct _EVENT_HISTORY_INFO) * local_nRow );
//        fcom_malloc((void**) &param_EventHistInfo->ptrEventHistoryDuplDetect,
//                    sizeof(struct _EVENT_HISTORY_INFO) * local_nRow );
//        param_EventHistInfo->nHistCnt = local_nRow;
//        WRITE_DEBUG(CATEGORY_DEBUG,"[Select Event History] Cnt : [%d] [%d]",
//                    param_EventHistInfo->nHistCnt, local_nRow);
//
//        while(fdb_SqlFetchRow(g_stMyCon) == 0)
//        {
//            // HB_SQ
//            local_nHbSq = atoi(g_stMyCon->row[0]);
//            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nHbSq =
//            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nHbSq = local_nHbSq;
//
//            // EVENT TYPE
//            local_nEventType = atoi(g_stMyCon->row[1]);
//            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvType =
//            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvType = local_nEventType;
//
//            // DETECT TIME
//            if ( g_stMyCon->row[2] != NULL )
//            {
//                snprintf( param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime,
//                          sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime),
//                          "%s", g_stMyCon->row[2]);
//
//                // RECORD TIME
//                snprintf(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szRecordTime,
//                         sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szRecordTime),
//                         "%s", param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime);
//
//            }
//
//            // DUP DETECT TIME
//            if ( g_stMyCon->row[3] != NULL )
//            {
//                snprintf( param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDupDetectTime,
//                          sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDupDetectTime),
//                          "%s", g_stMyCon->row[3]);
//
//                // RECORD TIME
//                snprintf(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szRecordTime,
//                         sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szRecordTime),
//                         "%s", param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDetectTime);
//            }
//
//            // IP ADDR
//            if ( g_stMyCon->row[4] != NULL )
//            {
//                snprintf( param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szIpAddr,
//                          sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szIpAddr),
//                          "%s", g_stMyCon->row[4]);
//                snprintf( param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szIpAddr,
//                          sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szIpAddr),
//                          "%s", param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szIpAddr);
//            }
//
//            // EVENT LEVEL
//            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvLevel = atoi(g_stMyCon->row[5]);
//            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvLevel =
//                    param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvLevel;
//
//            // EVENT EXIST
//            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvExist = atoi(g_stMyCon->row[6]);
//            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvExist =
//                    param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvExist;
//
//            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nUsSq = atoi(g_stMyCon->row[7]);
//            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nUsSq =
//                    param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nUsSq;
//
//
//
//            local_nIdx++;
//        }
//    }
//
//    fdb_SqlFreeResult(g_stMyCon);
//
//    return 0;
//
//
//}

int freport_CpuUsageAlarm( char* param_DbPreFix, char* param_TablePreFix, char* param_WhereTime, struct _STD_EVENT_RESULT* param_ResultInfo )
{
    int         local_nRowCnt                   = 0;
    int         local_nResultCnt                = param_ResultInfo->EventCnt;
    char        local_szTableName[31 +1]        = {0x00,};
    char        local_szDbName[31 +1]           = {0x00,};
    char        local_szEventTableName[31 +1]   = {0x00,};
    char        local_szEventDbName[31 +1]      = {0x00,};

    char        local_szQuery[2047 +1]          = {0x00,};
    struct _STD_EVENT_INFO* local_ptrTemp = NULL;

    // Set Database
    snprintf(local_szDbName, sizeof(local_szDbName),
             "DAP_HISTORY_%s", param_DbPreFix );
    snprintf(local_szEventDbName, sizeof(local_szEventDbName), "DAP_HISTORY_%s", param_DbPreFix);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName),
             "CTRL_PROCESS_CPU_HISTORY_%s", param_TablePreFix );
    snprintf(local_szEventTableName, sizeof(local_szEventTableName),
             "EVENT_HISTORY_%s", param_TablePreFix );


    // Select CTRL_PROCESS_CPU_HISTORY
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT a.CP_DETECT_TIME, date_format(a.CP_DETECT_TIME, '%%Y-%%m-%%d %%H') as RECORD_TIME, b.EV_IPADDR, b.US_SQ, b.EV_TYPE, b.EV_LEVEL, b.EV_EXIST , b.HB_SQ "
             "FROM "
             "    ( "
             "        select HB_SQ, CP_DETECT_TIME, CP_ALARM_TYPE "
             "        from %s.%s "
             "        where CP_DETECT_TIME LIKE '%s%%' "
             "          and CP_IS_DAP_FLAG = 0 "
             "          AND CP_NEW_DATA_FLAG = 1 "
             "          AND CP_STATUS NOT IN (1, 3, 6) "
             "        group by CP_DETECT_TIME, CP_ALARM_TYPE "
             "    ) a, "
             "    ( "
             "        SELECT HB_SQ, EV_TYPE, EV_DETECT_TIME, EV_DUP_DETECT_TIME,EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
             "        FROM %s.%s "
             "        WHERE EV_DETECT_TIME LIKE '%s%%' OR EV_DUP_DETECT_TIME LIKE '%s%%'  "
             "          AND EV_TYPE IN (40,41,42) "
             "        group by HB_SQ, EV_TYPE, EV_DETECT_TIME,EV_DUP_DETECT_TIME "
             "    )b "
             "WHERE a.CP_DETECT_TIME = b.EV_DETECT_TIME OR a.CP_DETECT_TIME = b.EV_DUP_DETECT_TIME "
             "group by CP_DETECT_TIME,CP_ALARM_TYPE ",
             local_szDbName, local_szTableName,
             param_WhereTime,
             local_szEventDbName, local_szEventTableName,
             param_WhereTime, param_WhereTime);

    WRITE_DEBUG(CATEGORY_DEBUG,"CPU Ctrl History Execute Query [%s] ", local_szQuery );

    local_nRowCnt = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if ( local_nRowCnt > 0 )
    {
        local_ptrTemp = param_ResultInfo->ptrStdEventInfo;
        param_ResultInfo->ptrStdEventInfo = realloc(param_ResultInfo->ptrStdEventInfo, sizeof(struct _STD_EVENT_INFO) * (param_ResultInfo->EventCnt + local_nRowCnt) );
        if ( param_ResultInfo->ptrStdEventInfo == NULL)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Realloc() Failed");
            free(local_ptrTemp);
            fdb_SqlFreeResult(g_stMyCon);
            return (-1);
        }
        memset( &param_ResultInfo->ptrStdEventInfo[param_ResultInfo->EventCnt], 0x00,
                sizeof(struct _STD_EVENT_INFO) * (param_ResultInfo->EventCnt + local_nRowCnt) - sizeof(struct _STD_EVENT_INFO) * param_ResultInfo->EventCnt);

        while(fdb_SqlFetchRow(g_stMyCon) == 0 )
        {
            // DETECT_TIME
            snprintf(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szDetectTime, sizeof(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szDetectTime),
                     "%s", g_stMyCon->row[0]);

            // RECORD_TIME
            snprintf(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szRecordTime, sizeof(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szRecordTime),
                     "%s", g_stMyCon->row[1]);

            //IPADDR
            snprintf(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szIpAddr, sizeof(param_ResultInfo->ptrStdEventInfo[local_nResultCnt].szIpAddr),
                     "%s", g_stMyCon->row[2]);

            //US_SQ
            param_ResultInfo->ptrStdEventInfo[local_nResultCnt].nUsSq = atoi(g_stMyCon->row[3]);

            //EV_TYPE
            param_ResultInfo->ptrStdEventInfo[local_nResultCnt].nEventType = atoi(g_stMyCon->row[4]);

            //EV_LEVEL
            param_ResultInfo->ptrStdEventInfo[local_nResultCnt].nEventLevel = atoi(g_stMyCon->row[5]);

            //EV_EXIST
            param_ResultInfo->ptrStdEventInfo[local_nResultCnt].nEventExist = atoi(g_stMyCon->row[6]);

            //HB_SQ
            param_ResultInfo->ptrStdEventInfo[local_nResultCnt].nHbSq = atoi(g_stMyCon->row[7]);

            local_nResultCnt++;
        }
    }


    fdb_SqlFreeResult(g_stMyCon);
    param_ResultInfo->EventCnt = local_nResultCnt;


    return 0;
}

int freport_SelectEventHistory(struct _EVENT_HISTORY*       param_EventHistInfo,
                               char* param_DbPreFix, char*  param_TablePreFix, char* param_WhereTime)
{
    int         local_nIdx                      = 0;
    int         local_nRow                      = 0;
    int         local_nEventType                = 0;
    int         local_nHbSq                     = 0;


    char        local_szTableName[31 +1]        = {0x00,};
    char        local_szDbName[31 +1]           = {0x00,};
    char        local_szQuery[1023 +1]          = {0x00,};

    // Set Database
    memset( local_szDbName, 0x00, sizeof(local_szDbName) );
    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_HISTORY_%s", param_DbPreFix );

    // Set TableName
    memset( local_szTableName, 0x00, sizeof(local_szTableName) );
    snprintf(local_szTableName, sizeof(local_szTableName), "EVENT_HISTORY_%s", param_TablePreFix );

    local_nIdx = 0;
    memset( local_szQuery, 0x00, sizeof(local_szQuery) );

    // SELECT Event History
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT HB_SQ, EV_TYPE, EV_DETECT_TIME, EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
             " FROM %s.%s "
             " WHERE EV_DETECT_TIME LIKE '%s%%' "
             " AND EV_TYPE NOT IN (40,41,42) "
             " group by HB_SQ, EV_TYPE, EV_DETECT_TIME ",
             local_szDbName, local_szTableName, param_WhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"SELECT Event Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        fcom_malloc((void**) &(param_EventHistInfo->ptrEventHistoryDetect),
                    sizeof(struct _EVENT_HISTORY_INFO) * local_nRow );
        WRITE_DEBUG(CATEGORY_DEBUG,"[Select Event History] Cnt : [%d]",local_nRow);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // HB_SQ
            local_nHbSq = atoi(g_stMyCon->row[0]);
            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nHbSq = local_nHbSq;

            // EVENT TYPE
            local_nEventType = atoi(g_stMyCon->row[1]);
            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvType = local_nEventType;

            // DUP DETECT TIME
            if ( g_stMyCon->row[2] != NULL )
            {
                snprintf( param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime,
                          sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime),
                          "%s", g_stMyCon->row[2]);

                // RECORD TIME
                snprintf(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szRecordTime,
                         sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szRecordTime),
                         "%s", param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szDetectTime);
            }

            // IP ADDR
            if ( g_stMyCon->row[3] != NULL )
            {
                snprintf( param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szIpAddr,
                          sizeof(param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].szIpAddr),
                          "%s", g_stMyCon->row[3]);
            }

            // EVENT LEVEL
            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvLevel = atoi(g_stMyCon->row[4]);

            // EVENT EXIST
            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nEvExist = atoi(g_stMyCon->row[5]);

            // US_SQ
            param_EventHistInfo->ptrEventHistoryDetect[local_nIdx].nUsSq = atoi(g_stMyCon->row[6]);

            local_nIdx++;
        }
    }

    param_EventHistInfo->nDetectHistCnt = local_nIdx;
    fdb_SqlFreeResult(g_stMyCon);



    local_nIdx = 0;
    memset( local_szQuery, 0x00, sizeof(local_szQuery) );

    // SELECT Event History
    snprintf(local_szQuery, sizeof(local_szQuery),
             "SELECT HB_SQ, EV_TYPE, EV_DUP_DETECT_TIME, EV_IPADDR, EV_LEVEL, EV_EXIST, US_SQ "
             " FROM %s.%s "
             " WHERE EV_DUP_DETECT_TIME LIKE '%s%%' "
             " AND EV_TYPE NOT IN (40,41,42) "
             " group by HB_SQ, EV_TYPE, EV_DUP_DETECT_TIME ",
             local_szDbName, local_szTableName, param_WhereTime );

    WRITE_DEBUG(CATEGORY_DEBUG,"Select Event History Execute Query [%s] ", local_szQuery );

    local_nRow = fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if( local_nRow > 0 )
    {
        fcom_malloc((void**) &(param_EventHistInfo->ptrEventHistoryDuplDetect),
                    sizeof(struct _EVENT_HISTORY_INFO) * local_nRow );
        WRITE_DEBUG(CATEGORY_DEBUG,"[Select Event Dupl History] Cnt : [%d] [%d]",
                    param_EventHistInfo->nDuplHistCnt, local_nRow);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // HB_SQ
            local_nHbSq = atoi(g_stMyCon->row[0]);
            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nHbSq = local_nHbSq;

            // EVENT TYPE
            local_nEventType = atoi(g_stMyCon->row[1]);
            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvType = local_nEventType;

            // DUP DETECT TIME
            if ( g_stMyCon->row[2] != NULL )
            {
                snprintf( param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDetectTime,
                          sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDetectTime),
                          "%s", g_stMyCon->row[2]);

                // RECORD TIME
                snprintf(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szRecordTime,
                         sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szRecordTime),
                         "%s", param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szDetectTime);
            }

            // IP ADDR
            if ( g_stMyCon->row[3] != NULL )
            {
                snprintf( param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szIpAddr,
                          sizeof(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].szIpAddr),
                          "%s", g_stMyCon->row[3]);
            }

            // EVENT LEVEL
            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvLevel = atoi(g_stMyCon->row[4]);

            // EVENT EXIST
            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nEvExist = atoi(g_stMyCon->row[5]);

            // US_SQ
            param_EventHistInfo->ptrEventHistoryDuplDetect[local_nIdx].nUsSq = atoi(g_stMyCon->row[6]);

            local_nIdx++;
        }
    }

    param_EventHistInfo->nDuplHistCnt = local_nIdx;
    fdb_SqlFreeResult(g_stMyCon);


    return 0;
}

int freport_CopyEventHistResult(struct _EVENT_HISTORY_INFO*      param_EventHistInfo,
                            struct _STD_EVENT_INFO*              param_EventResult,
                            int*                                 param_KeyCnt,
                            char**                               param_KeyArray,
                            char*                                param_FindKey)
{
    // IP ADDR
    snprintf(param_EventResult->szIpAddr, sizeof(param_EventResult->szIpAddr),
             "%s", param_EventHistInfo->szIpAddr);

    // DETECT TIME
    snprintf(param_EventResult->szDetectTime, sizeof(param_EventResult->szDetectTime),
             "%s", param_EventHistInfo->szDetectTime);

    // RECORD TIME
    snprintf( param_EventResult->szRecordTime, sizeof(param_EventResult->szRecordTime),
              "%s", param_EventHistInfo->szRecordTime);

    param_EventResult->nHbSq = param_EventHistInfo->nHbSq;
    param_EventResult->nUsSq = param_EventHistInfo->nUsSq;

    param_EventResult->nEventType  = param_EventHistInfo->nEvType;
    param_EventResult->nEventLevel = param_EventHistInfo->nEvLevel;
    param_EventResult->nEventExist = param_EventHistInfo->nEvExist;

    (*param_KeyCnt)++;

    // Insert Key Value
    if ( freport_InsertKeyValue( param_KeyArray, *param_KeyCnt, param_FindKey ) < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Insert Key (%s) Failed", param_FindKey);
        return (-1);
    }

    return 0;
}


int freport_CopyEventResult(struct _EVENT_TB_INFO*      param_EventInfo,
                            struct _STD_EVENT_INFO*     param_EventResult,
                            int*                        param_KeyCnt,
                            char**                      param_KeyArray,
                            char*                       param_FindKey)
{
    // IP ADDR
    snprintf(param_EventResult->szIpAddr, sizeof(param_EventResult->szIpAddr),
             "%s", param_EventInfo->szIpAddr);

    // DETECT TIME
    snprintf(param_EventResult->szDetectTime, sizeof(param_EventResult->szDetectTime),
             "%s", param_EventInfo->szDetectTime);

    // RECORD TIME
    snprintf( param_EventResult->szRecordTime, sizeof(param_EventResult->szRecordTime),
              "%s", param_EventInfo->szRecordTime);

    param_EventResult->nHbSq = param_EventInfo->nHbSq;
    param_EventResult->nUsSq = param_EventInfo->nUsSq;

    param_EventResult->nEventType  = param_EventInfo->nEvType;
    param_EventResult->nEventLevel = param_EventInfo->nEvLevel;
    param_EventResult->nEventExist = param_EventInfo->nEvExist;

    (*param_KeyCnt)++;

    // Insert Key Value
    if ( freport_InsertKeyValue( param_KeyArray, *param_KeyCnt, param_FindKey ) < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Insert Key (%s) Failed", param_FindKey);
        return (-1);
    }

    return 0;

}

//int freport_CopyEventHistory(struct _STD_EVENT_INFO*     param_StdEventResult,
//                             struct _STD_EVENT_INFO*     param_StdData,
//                             struct _EVENT_HISTORY_INFO* param_HistInfo,
//                             int*                        param_KeyCnt,
//                             char**                      param_KeyArray,
//                             char*                       param_FindKey)
//{
//    // IP ADDR
//    snprintf(param_StdEventResult->szIpAddr, sizeof(param_StdEventResult->szIpAddr),
//             "%s", param_HistInfo->szIpAddr);
//
//    // DETECT TIME
//    snprintf(param_StdEventResult->szDetectTime, sizeof(param_StdEventResult->szDetectTime),
//             "%s", param_StdData->szDetectTime);
//    // RECORD TIME
//    snprintf( param_StdEventResult->szRecordTime, sizeof(param_StdEventResult->szRecordTime),
//              "%s", param_StdData->szRecordTime);
//
//    param_StdEventResult->nHbSq = param_StdData->nHbSq;
//    param_StdEventResult->nUsSq = param_StdData->nUsSq;
////    param_StdEventResult->nUgSq = param_StdData->nUgSq;
//
//    param_StdEventResult->nEventType = param_HistInfo->nEvType;
//    param_StdEventResult->nEventLevel = param_HistInfo->nEvLevel;
//    param_StdEventResult->nEventExist = param_HistInfo->nEvExist;
//
//    (*param_KeyCnt)++;
//    // Insert Key Value
//    if ( freport_InsertKeyValue( param_KeyArray, *param_KeyCnt, param_FindKey ) < 0 )
//    {
//        WRITE_CRITICAL(CATEGORY_DEBUG,"Insert Key (%s) Failed", param_FindKey);
//        return (-1);
//    }
//
//    return 0;
//
//}

int freport_MergeEvent(struct _EVENT_INFO* param_EventInfo,
                       struct _EVENT_HISTORY* param_EventHistInfo,
                       struct _STD_EVENT_RESULT* param_ResultInfo )
{
    int  i = 0;
    int  local_nKeyCnt  = 0;
    int  local_nResultCnt = 0;
    int  local_nEventLoop = 0, local_nEventHistLoop = 0;
    int  local_nEventTotCnt = param_EventInfo->nEventCnt + param_EventInfo->nDuplCnt + param_EventHistInfo->nDetectHistCnt + param_EventHistInfo->nDuplHistCnt ;
    char local_szFindKey[64 +1] = {0x00,};
    char**  local_KeyArray = NULL;

    fcom_malloc((void**) &(param_ResultInfo->ptrStdEventInfo), sizeof(struct _STD_EVENT_INFO) * local_nEventTotCnt );

    local_KeyArray = (char**) malloc(sizeof(char*) * local_nEventTotCnt );
    for ( i = 0; i < local_nEventTotCnt; i++ )
    {
        local_KeyArray[i] = (char*)malloc(sizeof(char) * 64);
        memset( local_KeyArray[i], 0x00, sizeof(char) * 64);
    }

    WRITE_DEBUG(CATEGORY_DEBUG," Total Event+Event Hist Cnt : %d, "
                               " Event Cnt : %d Dupl Event Cnt : %d "
                               " Event Hist Cnt : %d Dupl Event Hist Cnt : %d ",
                local_nEventTotCnt,
                param_EventInfo->nEventCnt,
                param_EventInfo->nDuplCnt,
                param_EventHistInfo->nDetectHistCnt,
                param_EventHistInfo->nDuplHistCnt);

    local_nKeyCnt = 0;

    // EVENT_TB 처리
    for ( local_nEventLoop = 0; local_nEventLoop < param_EventInfo->nEventCnt; local_nEventLoop++ )
    {
        // 키값 셋팅(HB_SQ+EVENT_TYPE+DETECT_TIME)
        memset(local_szFindKey, 0x00, sizeof(local_szFindKey) );
        snprintf(local_szFindKey, sizeof(local_szFindKey), "%d%d%s",
                 param_EventInfo->ptrEventTbDetect[local_nEventLoop].nHbSq,
                 param_EventInfo->ptrEventTbDetect[local_nEventLoop].nEvType,
                 param_EventInfo->ptrEventTbDetect[local_nEventLoop].szDetectTime);

        // 키 검사하여 없는 key값이면 저장
        if ( freport_CheckKeyValue( local_KeyArray, local_nKeyCnt, local_szFindKey) < 0 ) //없는 키 값이면
        {
            // Result 구조체에 데이터 Copy 후 Key Insert 처리
            freport_CopyEventResult( &(param_EventInfo->ptrEventTbDetect[local_nEventLoop]),
                                     &(param_ResultInfo->ptrStdEventInfo[local_nResultCnt]),
                                     &local_nKeyCnt, local_KeyArray, local_szFindKey);
            local_nResultCnt++;
        }
    }

    for ( local_nEventLoop = 0; local_nEventLoop < param_EventInfo->nDuplCnt; local_nEventLoop++ )
    {
        // 키값 셋팅(HB_SQ+EVENT_TYPE+DETECT_DUP_TIME)
        memset(local_szFindKey, 0x00, sizeof(local_szFindKey) );
        snprintf(local_szFindKey, sizeof(local_szFindKey), "%d%d%s",
                 param_EventInfo->ptrEventDuplDetect[local_nEventLoop].nHbSq,
                 param_EventInfo->ptrEventDuplDetect[local_nEventLoop].nEvType,
                 param_EventInfo->ptrEventDuplDetect[local_nEventLoop].szDetectTime);

        // 키 검사하여 없는 key값이면 저장
        if ( freport_CheckKeyValue( local_KeyArray, local_nKeyCnt, local_szFindKey) < 0 ) //없는 키 값이면
        {
            // Result 구조체에 데이터 Copy 후 Key Insert 처리
            freport_CopyEventResult( &(param_EventInfo->ptrEventDuplDetect[local_nEventLoop]),
                                     &(param_ResultInfo->ptrStdEventInfo[local_nResultCnt]),
                                     &local_nKeyCnt, local_KeyArray, local_szFindKey);
            local_nResultCnt++;
        }
    }

    // EVENT_HISTORY 처리
    for ( local_nEventHistLoop = 0; local_nEventHistLoop < param_EventHistInfo->nDetectHistCnt; local_nEventHistLoop++ )
    {
        // 키값 셋팅(HB_SQ+EVENT_TYPE+DETECT_TIME)
        memset(local_szFindKey, 0x00, sizeof(local_szFindKey) );
        snprintf(local_szFindKey, sizeof(local_szFindKey), "%d%d%s",
                 param_EventHistInfo->ptrEventHistoryDetect[local_nEventHistLoop].nHbSq,
                 param_EventHistInfo->ptrEventHistoryDetect[local_nEventHistLoop].nEvType,
                 param_EventHistInfo->ptrEventHistoryDetect[local_nEventHistLoop].szDetectTime);

        // 키 검사하여 없는 key값이면 저장
        if ( freport_CheckKeyValue( local_KeyArray, local_nKeyCnt, local_szFindKey) < 0 ) //없는 키 값이면
        {
            // Result 구조체에 데이터 Copy 후 Key Insert 처리
            freport_CopyEventHistResult(    &(param_EventHistInfo->ptrEventHistoryDetect[local_nEventHistLoop]),
                                            &(param_ResultInfo->ptrStdEventInfo[local_nResultCnt]),
                                     &local_nKeyCnt, local_KeyArray, local_szFindKey);
            local_nResultCnt++;
        }
    }

    for ( local_nEventHistLoop = 0; local_nEventHistLoop < param_EventHistInfo->nDuplHistCnt; local_nEventHistLoop++ )
    {
        // 키값 셋팅(HB_SQ+EVENT_TYPE+DETECT_DUP_TIME)
        memset(local_szFindKey, 0x00, sizeof(local_szFindKey) );
        snprintf(local_szFindKey, sizeof(local_szFindKey), "%d%d%s",
                 param_EventHistInfo->ptrEventHistoryDuplDetect[local_nEventHistLoop].nHbSq,
                 param_EventHistInfo->ptrEventHistoryDuplDetect[local_nEventHistLoop].nEvType,
                 param_EventHistInfo->ptrEventHistoryDuplDetect[local_nEventHistLoop].szDetectTime);

        // 키 검사하여 없는 key값이면 저장
        if ( freport_CheckKeyValue( local_KeyArray, local_nKeyCnt, local_szFindKey) < 0 ) //없는 키 값이면
        {
            // Result 구조체에 데이터 Copy 후 Key Insert 처리
            freport_CopyEventHistResult(    &(param_EventHistInfo->ptrEventHistoryDuplDetect[local_nEventHistLoop]),
                                            &(param_ResultInfo->ptrStdEventInfo[local_nResultCnt]),
                                            &local_nKeyCnt, local_KeyArray, local_szFindKey);
            local_nResultCnt++;
        }
    }

    param_ResultInfo->EventCnt = local_nResultCnt;
    for( i = 0; i < local_nEventTotCnt; i++)
    {
        if ( local_KeyArray[i] != NULL ) { free(local_KeyArray[i]); }
    }
    if ( local_KeyArray != NULL) { free(local_KeyArray); }

    return 0;

}

// 2022.02.05 RealTimeFlag 0x00 : 정기보고서 , 0x01 : 실시간 보고서
int freport_TaskReport(char* param_DbPreFix, char* param_TablePreFix)
{
    int         local_nLoop = 0;
    char        local_szGetTime[14 +1]     = {0x00,};
    char        local_szWhereTime[19 +1]   = {0x00,}; // WHERE 조건시간
    char        local_szYYYY[4 +1]         = {0x00,};
    char        local_szMM[2 +1]           = {0x00,};
    char        local_szDD[2 +1]           = {0x00,};
    char        local_szTableName[31 +1]   = {0x00,};
    char        local_szDbName[31 +1]      = {0x00,};
    struct _EVENT_INFO          local_stEventTb;
    struct _EVENT_HISTORY       local_stEventHistory;
    struct _STD_EVENT_RESULT    local_stResultEventInfo;  //EVENT_TB + EVENT_HISTORY 데이터 중복제거 후 데이터

    memset( &local_stEventHistory, 0x00, sizeof(struct _EVENT_HISTORY) );
    memset( &local_stEventTb, 0x00, sizeof(struct _EVENT_INFO) );
    memset( &local_stResultEventInfo, 0x00, sizeof(struct _STD_EVENT_RESULT) );

    // Get Time, 어제 날짜를 가져온다
    fcom_GetTime(local_szGetTime, 86400 );

    memcpy( local_szYYYY, &local_szGetTime[0], 4 ); // YYYY
    memcpy( local_szMM, &local_szGetTime[4], 2 );   // MM
    memcpy ( local_szDD, &local_szGetTime[6], 2);   // DD

    // Set Database
    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_HISTORY_%s", param_DbPreFix );

    snprintf(local_szWhereTime, sizeof(local_szWhereTime), "%s-%s-%s", local_szYYYY, local_szMM, local_szDD);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "EVENT_HISTORY_%s", param_TablePreFix );

    // EVENT HISTORY Select
    freport_SelectEventHistory( &local_stEventHistory, param_DbPreFix, param_TablePreFix, local_szWhereTime );

    // EVENT TB Select
    freport_SelectEvent( &local_stEventTb, param_DbPreFix, param_TablePreFix, local_szWhereTime );

    // 중복 제거
    freport_MergeEvent(&local_stEventTb, &local_stEventHistory, &local_stResultEventInfo);

    // 예외 이벤트 처리.
    // CPU 알람 및 CPU 통제만 추가로 이벤트를 구한다.
    freport_CpuUsageAlarm( param_DbPreFix, param_TablePreFix, local_szWhereTime, &local_stResultEventInfo);

    // Get Group Info
    freport_GetGroupInfo( local_stResultEventInfo.ptrStdEventInfo, local_stResultEventInfo.EventCnt );

    int temp = 0;
    for ( local_nLoop = 0; local_nLoop < local_stResultEventInfo.EventCnt; local_nLoop++ )
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"%d. DETECT : [%s] EV_TYPE : [%d] EV_LEVEL : [%d]",
                    local_nLoop,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].szDetectTime,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventLevel);
        if ( local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType == 41 ||
             local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType == 42)
        {
            temp++;
        }
    }
    WRITE_DEBUG(CATEGORY_DEBUG,"Result Cpu Cnt: [%d]", temp);

    WRITE_DEBUG(CATEGORY_DEBUG,"[Result Report Data Cnt] : %d ", local_stResultEventInfo.EventCnt);

    // STD_EVENT_TB / STD_EVENT_REALTIME_TB Insert
    freport_InsertStdEvent( local_stResultEventInfo.ptrStdEventInfo,
                            local_stResultEventInfo.EventCnt,
                            NULL,
                            0x00);

    fcom_MallocFree((void**) &(local_stResultEventInfo.ptrStdEventInfo));

    fcom_MallocFree((void**) &(local_stEventTb.ptrEventTbDetect));
    fcom_MallocFree((void**) &(local_stEventTb.ptrEventDuplDetect));

    fcom_MallocFree((void**) &(local_stEventHistory.ptrEventHistoryDetect));
    fcom_MallocFree((void**) &(local_stEventHistory.ptrEventHistoryDuplDetect));

    return 0;
}


// 2022.02.05 RealTimeFlag 0x00 : 정기보고서 , 0x01 : 실시간 보고서
int freport_TaskReportRealTime(char* param_YYYY, char* param_MM, char* param_DD)
{
    int         local_nLoop = 0;
    char        local_szGetTime[14 +1]     = {0x00,};
    char        local_szWhereTime[19 +1]   = {0x00,}; // WHERE 조건시간
    char        local_szTableName[31 +1]   = {0x00,};
    char        local_szDbName[31 +1]      = {0x00,};
    struct _EVENT_INFO          local_stEventTb;
    struct _EVENT_HISTORY       local_stEventHistory;
    struct _STD_EVENT_RESULT    local_stResultEventInfo;  //EVENT_TB + EVENT_HISTORY 데이터 중복제거 후 데이터

    memset( &local_stEventHistory, 0x00, sizeof(struct _EVENT_HISTORY) );
    memset( &local_stEventTb, 0x00, sizeof(struct _EVENT_INFO) );
    memset( &local_stResultEventInfo, 0x00, sizeof(struct _STD_EVENT_RESULT) );

    // Set Database
    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_HISTORY_%s", param_YYYY );

    snprintf(local_szWhereTime, sizeof(local_szWhereTime), "%s-%s-%s", param_YYYY, param_MM, param_DD);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "EVENT_HISTORY_%s", param_MM );

    // EVENT HISTORY Select
    freport_SelectEventHistory( &local_stEventHistory, param_YYYY, param_MM, local_szWhereTime );

    // EVENT TB Select
    freport_SelectEvent( &local_stEventTb, param_YYYY, param_MM, local_szWhereTime );

    // 중복 제거
    freport_MergeEvent(&local_stEventTb, &local_stEventHistory, &local_stResultEventInfo);

    // 예외 이벤트 처리.
    // CPU 알람 및 CPU 통제만 추가로 이벤트를 구한다.
    freport_CpuUsageAlarm( param_YYYY, param_MM, local_szWhereTime, &local_stResultEventInfo);

    // Get Group Info
    freport_GetGroupInfo( local_stResultEventInfo.ptrStdEventInfo, local_stResultEventInfo.EventCnt );

    int temp = 0;
    for ( local_nLoop = 0; local_nLoop < local_stResultEventInfo.EventCnt; local_nLoop++ )
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"%d. DETECT : [%s] EV_TYPE : [%d] EV_LEVEL : [%d]",
                    local_nLoop,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].szDetectTime,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType,
                    local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventLevel);
        if ( local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType == 41 ||
             local_stResultEventInfo.ptrStdEventInfo[local_nLoop].nEventType == 42)
        {
            temp++;
        }
    }
    WRITE_DEBUG(CATEGORY_DEBUG,"Result Cpu Cnt: [%d]", temp);

    WRITE_DEBUG(CATEGORY_DEBUG,"[Result Report Data Cnt] : %d ", local_stResultEventInfo.EventCnt);

    // STD_EVENT_TB / STD_EVENT_REALTIME_TB Insert
    freport_InsertStdEvent( local_stResultEventInfo.ptrStdEventInfo,
                            local_stResultEventInfo.EventCnt,
                            local_szWhereTime,
                            0x01);

    fcom_MallocFree((void**) &(local_stResultEventInfo.ptrStdEventInfo));

    fcom_MallocFree((void**) &(local_stEventTb.ptrEventTbDetect));
    fcom_MallocFree((void**) &(local_stEventTb.ptrEventDuplDetect));

    fcom_MallocFree((void**) &(local_stEventHistory.ptrEventHistoryDetect));
    fcom_MallocFree((void**) &(local_stEventHistory.ptrEventHistoryDuplDetect));

    return 0;
}




int freport_GetReportMailInfo(char *p_confMailFrom,
                          char *p_confMailToDay,
                          char *p_confMailToWeek,
                          char *p_confMailToMonth)
{
    char    sqlBuf[128] = {0x00,};
    int		rowCnt = 0;

    strcpy(sqlBuf, "select CF_NAME, CF_VALUE from CONFIG_TB "
                   "where CF_NAME like 'REPORT_MAIL_%'");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_LANG"))
                {
                    if(g_stMyCon->row[1] != NULL)
                        strcpy(g_stProcReportInfo.szConfMailLang, g_stMyCon->row[1]);
                    else
                        strcpy(g_stProcReportInfo.szConfMailLang, "kr");
                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_USER_VIEW")) // 'ip','id','sno','name'
                {
                    if(g_stMyCon->row[1] != NULL)
                        strcpy(g_stProcReportInfo.szConfMailUserView, g_stMyCon->row[1]);
                    else
                        strcpy(g_stProcReportInfo.szConfMailUserView, "ip");

                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_FROM"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_confMailFrom, g_stMyCon->row[1]);
                    else						strcpy(p_confMailFrom, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_TO_DAY"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_confMailToDay, g_stMyCon->row[1]);
                    else						strcpy(p_confMailToDay, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_TO_WEEK"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_confMailToWeek, g_stMyCon->row[1]);
                    else						strcpy(p_confMailToWeek, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_TO_MONTH"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_confMailToMonth, g_stMyCon->row[1]);
                    else						strcpy(p_confMailToMonth, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "REPORT_MAIL_CLOSED_VIEW"))
                {
                    if(g_stMyCon->row[1] != NULL)
                        strcpy(g_stProcReportInfo.szConfMailClosedView, g_stMyCon->row[1]);
                    else
                        strcpy(g_stProcReportInfo.szConfMailClosedView, "");
                }
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO,"confMailLang : %s  ",g_stProcReportInfo.szConfMailLang );
    WRITE_INFO(CATEGORY_INFO,"confMailUserView :   %s",g_stProcReportInfo.szConfMailUserView );
    WRITE_INFO(CATEGORY_INFO,"confMailFrom : %s  ",p_confMailFrom );
    WRITE_INFO(CATEGORY_INFO,"confMailToDay : %s  ",p_confMailToDay);
    WRITE_INFO(CATEGORY_INFO,"confMailToWeek : %s  ",p_confMailToWeek);
    WRITE_INFO(CATEGORY_INFO,"confMailToMonth : %s  ",p_confMailToMonth);
    WRITE_INFO(CATEGORY_INFO,"confMailClosedView : %s ",g_stProcReportInfo.szConfMailClosedView );

    return rowCnt;
}

int freport_GetReportRuntime(char *p_confMailRunTime)
{
    char    sqlBuf[128] = {0x00,};
    int		rowCnt = 0;

    strcpy(sqlBuf, "select CF_VALUE from CONFIG_TB "
                   "where CF_NAME = 'REPORT_MAIL_RUNTIME'");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)	strcpy(p_confMailRunTime, g_stMyCon->row[0]);
            else						    strcpy(p_confMailRunTime, "0100");
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO, "Proc rowCnt(%d)p_confMailRunTime(%s)",
               rowCnt,
               p_confMailRunTime);

    return rowCnt;
}
int freport_GetRankIp(
        char *gubun,
        char *rankIp,
        char *sCloseVal,
        char *uWarnVal,
        char *uCritVal,
        char *uBlokVal,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp,
        char *lang,
        char param_cRealTimeFlag
)
{
    int		i = 0;
    int		idx = 0;
    int		rowCnt = 0;
    int		topCnt = 0;
//    int		sqlRankExist = 0;
    int		sqlRankLevel = 0;
    int		uSqlTopSum = 0;
    int		uSqlRankSum = 0;
//    int     sSqlRankVal[10][3] = {{0},{0}};
    int     uSqlRankVal[10][3] = {{0},{0}};
    char	sqlRankIp[10][15+1] = {{0x00,},{0x00,}};
    char	sqlBuf[4096] = {0x00,};

    for(i=0; i<10; i++)
    {
        memset(sqlRankIp[i], 0x00, sizeof(sqlRankIp[i]));
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf,	"select STDR_IPADDR, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB  "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }

                strcat(sqlBuf, "group by STDR_IPADDR "
                               "order by CNT desc, STDR_IPADDR asc limit 10");

            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
                sprintf(sqlBuf,	"select STDR_IPADDR, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB  "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STDR_IPADDR "
                               "order by CNT desc, STDR_IPADDR asc limit 10");
            }
            else
            {
                sprintf(sqlBuf,	"select STDR_IPADDR, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB  "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' " , sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STDR_IPADDR "
                               "order by CNT desc, STDR_IPADDR asc limit 10");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf,	"select STDR_IPADDR, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB  "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "between adddate(curdate(), - weekday(curdate()) -7) "
                                  "and  adddate(curdate(), - weekday(curdate()) -1) " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STDR_IPADDR "
                               "order by CNT desc, STDR_IPADDR asc limit 10");
            }
            else
            {
                sprintf(sqlBuf,	"select STWR_IPADDR, sum(STWR_COUNT) as CNT "
                                   "from STW_EVENT_REALTIME_TB  "
                                   "where STWR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STWR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STWR_IPADDR "
                               "order by CNT desc, STWR_IPADDR asc limit 10");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf,	"select STDR_IPADDR, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m') "
                                  "= date_format(curdate() - interval 1 month, '%Y-%m') " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STDR_IPADDR "
                               "order by CNT desc, STDR_IPADDR asc limit 10");
            }
            else
            {
                sprintf(sqlBuf,	"select STMR_IPADDR, sum(STMR_COUNT) as CNT "
                                   "from STM_EVENT_REALTIME_TB  "
                                   "where STMR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STMR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STMR_IPADDR "
                               "order by CNT desc, STMR_IPADDR asc limit 10");
            }
        }
    }
    else // 정기 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf,	"select STD_IPADDR,count(*) as CNT "
                strcpy(sqlBuf,	"select STD_IPADDR, sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB  "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
//                              "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }

                strcat(sqlBuf, "group by STD_IPADDR "
                               "order by CNT desc, STD_IPADDR asc limit 10");

            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
//            sprintf(sqlBuf,	"select STD_IPADDR, count(*) as CNT "
                sprintf(sqlBuf,	"select STD_IPADDR, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB  "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
//                               "and STD_EXIST = 1 ", sDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STD_IPADDR "
                               "order by CNT desc, STD_IPADDR asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf,	"select STD_IPADDR, count(*) as CNT "
                sprintf(sqlBuf,	"select STD_IPADDR, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB  "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' " , sDate, eDate);
//                               "and STD_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STD_IPADDR "
                               "order by CNT desc, STD_IPADDR asc limit 10");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf,	"select STD_IPADDR,count(*) as CNT "
                strcpy(sqlBuf,	"select STD_IPADDR, sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB  "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "between adddate(curdate(), - weekday(curdate()) -7) "
                                  "and  adddate(curdate(), - weekday(curdate()) -1) " );
//                              "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STD_IPADDR "
                               "order by CNT desc, STD_IPADDR asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf,	"select STW_IPADDR,count(*) as CNT "
                sprintf(sqlBuf,	"select STW_IPADDR, sum(STW_COUNT) as CNT "
                                   "from STW_EVENT_TB  "
                                   "where STW_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                               "and STW_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STW_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STW_IPADDR "
                               "order by CNT desc, STW_IPADDR asc limit 10");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf,	"select STD_IPADDR,count(*) as CNT "
                strcpy(sqlBuf,	"select STD_IPADDR,sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m') "
                                  "= date_format(curdate() - interval 1 month, '%Y-%m') " );
//                              "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STD_IPADDR "
                               "order by CNT desc, STD_IPADDR asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf,	"select STM_IPADDR,count(*) as CNT "
                sprintf(sqlBuf,	"select STM_IPADDR,sum(STM_COUNT) as CNT "
                                   "from STM_EVENT_TB  "
                                   "where STM_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                               "and STM_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STM_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STM_IPADDR "
                               "order by CNT desc, STM_IPADDR asc limit 10");
            }
        }

    }

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_INFO,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        idx = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(sqlRankIp[idx], g_stMyCon->row[0]);
            idx++;
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Proc rowCnt(%d)",rowCnt );
    fdb_SqlFreeResult(g_stMyCon);

    for(i=0; i<10; i++)
    {
        if(strlen(sqlRankIp[i]) < 6)
            break;

        WRITE_INFO(CATEGORY_INFO,"sqlRankIp[%d]: (%s) ",i,sqlRankIp[i] );

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
        {
            if(!strcmp(gubun, "day"))
            {
                if(strlen(sDate) == 0)
                {
                    //원래 코드
                    sprintf(sqlBuf,	"select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                       "from STD_EVENT_REALTIME_TB "
                                       "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "= date_format(curdate() - interval 1 day, '%%Y-%%m-%%d') "
                                       "and STDR_IPADDR = '%s' "
                                       "group by STDR_LEVEL", sqlRankIp[i]);
                }
                else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
                {
                    sprintf(sqlBuf,	"select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                       "from STD_EVENT_REALTIME_TB "
                                       "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' "
                                       "and STDR_IPADDR = '%s' "
                                       "group by STDR_LEVEL", sDate,sqlRankIp[i]);
                }
                else
                {
                    sprintf(sqlBuf,	"select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                       "from STD_EVENT_REALTIME_TB "
                                       "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "between '%s' and '%s' "
                                       "and STDR_IPADDR = '%s' "
                                       "group by STDR_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }
            else if(!strcmp(gubun, "week"))
            {
                if(strlen(sDate) == 0)
                {
                    sprintf(sqlBuf,	"select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                       "from STD_EVENT_REALTIME_TB "
                                       "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "between adddate(curdate(), - weekday(curdate()) -7) "
                                       "and adddate(curdate(), - weekday(curdate()) -1) "
                                       "and STDR_IPADDR = '%s' "
                                       "group by STDR_LEVEL", sqlRankIp[i]);
                }
                else
                {
                    sprintf(sqlBuf,	"select STWR_LEVEL, sum(STWR_COUNT) as CNT "
                                       "from STW_EVENT_REALTIME_TB "
                                       "where STWR_RECORD_TIME between '%s' and '%s' "
                                       "and STWR_IPADDR = '%s' "
                                       "group by STWR_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }
            else if(!strcmp(gubun, "month"))
            {
                if(strlen(sDate) == 0)
                {
                    sprintf(sqlBuf,	"select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                       "from STD_EVENT_REALTIME_TB "
                                       "where date_format(STDR_RECORD_TIME, '%%Y-%%m') "
                                       "= date_format(curdate() - interval 1 month, '%%Y-%%m') "
                                       "and STDR_IPADDR = '%s' "
                                       "group by STDR_LEVEL", sqlRankIp[i]);
                }
                else
                {
                    sprintf(sqlBuf,	"select STMR_LEVEL, sum(STMR_COUNT) as CNT "
                                       "from STM_EVENT_REALTIME_TB "
                                       "where STMR_RECORD_TIME between '%s' and '%s' "
                                       "and STMR_IPADDR = '%s' "
                                       "group by STMR_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }

        }
        else //정기 보고서
        {
            if(!strcmp(gubun, "day"))
            {
                if(strlen(sDate) == 0)
                {
                    //원래 코드
//                sprintf(sqlBuf,	"select STD_EXIST,STD_LEVEL,count(*) as CNT "
                    sprintf(sqlBuf,	"select STD_LEVEL, sum(STD_COUNT) as CNT "
                                       "from STD_EVENT_TB "
                                       "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "= date_format(curdate() - interval 1 day, '%%Y-%%m-%%d') "
                                       "and STD_IPADDR = '%s' "
                                       //                                   "group by STD_EXIST,STD_LEVEL", sqlRankIp[i]);
                                       "group by STD_LEVEL", sqlRankIp[i]);
                }
                else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
                {
                    sprintf(sqlBuf,	"select STD_LEVEL, sum(STD_COUNT) as CNT "
                                       "from STD_EVENT_TB "
                                       "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' "
                                       "and STD_IPADDR = '%s' "
                                       //                                   "group by STD_EXIST,STD_LEVEL", sDate,sqlRankIp[i]);
                                       "group by STD_LEVEL", sDate,sqlRankIp[i]);
                }
                else
                {
                    sprintf(sqlBuf,	"select STD_LEVEL, sum(STD_COUNT) as CNT "
                                       "from STD_EVENT_TB "
                                       "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "between '%s' and '%s' "
                                       "and STD_IPADDR = '%s' "
                                       "group by STD_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }
            else if(!strcmp(gubun, "week"))
            {
                if(strlen(sDate) == 0)
                {
//                sprintf(sqlBuf,	"select STD_EXIST,STD_LEVEL,count(*) as CNT "
                    sprintf(sqlBuf,	"select STD_LEVEL,sum(STD_COUNT) as CNT "
                                       "from STD_EVENT_TB "
                                       "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                       "between adddate(curdate(), - weekday(curdate()) -7) "
                                       "and adddate(curdate(), - weekday(curdate()) -1) "
                                       "and STD_IPADDR = '%s' "
                                       //                                   "group by STD_EXIST,STD_LEVEL", sqlRankIp[i]);
                                       "group by STD_LEVEL", sqlRankIp[i]);
                }
                else
                {
//                sprintf(sqlBuf,	"select STW_EXIST,STW_LEVEL,count(*) as CNT "
                    sprintf(sqlBuf,	"select STW_LEVEL,sum(STW_COUNT) as CNT "
                                       "from STW_EVENT_TB "
                                       "where STW_RECORD_TIME between '%s' and '%s' "
                                       "and STW_IPADDR = '%s' "
                                       //                                   "group by STW_EXIST,STW_LEVEL", sDate,eDate,sqlRankIp[i]);
                                       "group by STW_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }
            else if(!strcmp(gubun, "month"))
            {
                if(strlen(sDate) == 0)
                {
//                sprintf(sqlBuf,	"select STD_LEVEL,count(*) as CNT "
                    sprintf(sqlBuf,	"select STD_LEVEL,sum(STD_COUNT) as CNT "
                                       "from STD_EVENT_TB "
                                       "where date_format(STD_RECORD_TIME, '%%Y-%%m') "
                                       "= date_format(curdate() - interval 1 month, '%%Y-%%m') "
                                       "and STD_IPADDR = '%s' "
                                       //                                   "group by STD_EXIST,STD_LEVEL", sqlRankIp[i]);
                                       "group by STD_LEVEL", sqlRankIp[i]);
                }
                else
                {
//                sprintf(sqlBuf,	"select STM_LEVEL,count(*) as CNT "
                    sprintf(sqlBuf,	"select STM_LEVEL,sum(STM_COUNT) as CNT "
                                       "from STM_EVENT_TB "
                                       "where STM_RECORD_TIME between '%s' and '%s' "
                                       "and STM_IPADDR = '%s' "
                                       //                                   "group by STM_EXIST,STM_LEVEL", sDate,eDate,sqlRankIp[i]);
                                       "group by STM_LEVEL", sDate,eDate,sqlRankIp[i]);
                }
            }

        }


        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
        }

        WRITE_DEBUG(CATEGORY_DEBUG,"RANKIP Exec Query : [%s]", sqlBuf);

        if(rowCnt > 0)
        {
            while(fdb_SqlFetchRow(g_stMyCon) == 0)
            {
//                sqlRankExist = atoi(g_stMyCon->row[0]);
//                sqlRankLevel = atoi(g_stMyCon->row[1]);
                sqlRankLevel = atoi(g_stMyCon->row[0]);

//                if(sqlRankExist == 0) //해지
//                {
//                    if		(sqlRankLevel == WARNING)	sSqlRankVal[i][0] += atoi(g_stMyCon->row[2]);
//                    else if	(sqlRankLevel == CRITICAL) 	sSqlRankVal[i][1] += atoi(g_stMyCon->row[2]);
//                    else if	(sqlRankLevel == BLOCK) 	sSqlRankVal[i][2] += atoi(g_stMyCon->row[2]);
//                }
//                else //미해지
//                {
//                    if		(sqlRankLevel == WARNING)	uSqlRankVal[i][0] += atoi(g_stMyCon->row[2]);
//                    else if	(sqlRankLevel == CRITICAL) 	uSqlRankVal[i][1] += atoi(g_stMyCon->row[2]);
//                    else if	(sqlRankLevel == BLOCK) 	uSqlRankVal[i][2] += atoi(g_stMyCon->row[2]);
                    if		(sqlRankLevel == WARNING)	uSqlRankVal[i][0] += atoi(g_stMyCon->row[1]);
                    else if	(sqlRankLevel == CRITICAL) 	uSqlRankVal[i][1] += atoi(g_stMyCon->row[1]);
                    else if	(sqlRankLevel == BLOCK) 	uSqlRankVal[i][2] += atoi(g_stMyCon->row[1]);
//                }
            }
        }
    }//for

    WRITE_INFO(CATEGORY_INFO,"Proc rowCnt(%d)",rowCnt );
    fdb_SqlFreeResult(g_stMyCon);

    topCnt = 0;
    for(i=0; i<10; i++)
    {
        if(strlen(sqlRankIp[i]) < 6) break;
        sprintf(rankIp+strlen(rankIp), "'%s',", sqlRankIp[i]);
//        sprintf(sCloseVal+strlen(sCloseVal), "%d,",
//                sSqlRankVal[i][0]+sSqlRankVal[i][1]+sSqlRankVal[i][2]);
        sprintf(uWarnVal+strlen(uWarnVal), "%d,", uSqlRankVal[i][0]);
        sprintf(uCritVal+strlen(uCritVal), "%d,", uSqlRankVal[i][1]);
        sprintf(uBlokVal+strlen(uBlokVal), "%d,", uSqlRankVal[i][2]);

        //Top 1을 구한다. 카운트가 같으면 추가
        if(i == 0)
        {
            uSqlTopSum = uSqlRankVal[i][0]+uSqlRankVal[i][1]+uSqlRankVal[i][2];
            sprintf(topVal, "%s", sqlRankIp[i]);
        }
        else
        {
            uSqlRankSum = uSqlRankVal[i][0]+uSqlRankVal[i][1]+uSqlRankVal[i][2];
            if(uSqlTopSum == uSqlRankSum)
            {
                topCnt++;
            }
        }
    } //for

    if(topCnt != 0)
    {
        if(!strcasecmp(lang, "kr"))
        {
            sprintf(topVal+strlen(topVal), " 외%d개", topCnt);
        }
        else
        {
            sprintf(topVal+strlen(topVal), " other %d ", topCnt);
        }
    }

    return rowCnt;
}
int freport_GetRankGroupBysq(
        char*	gubun,
        char*	rankGroup,
        char*	sCloseVal,
        char*	uWarnVal,
        char*	uCritVal,
        char*	uBlokVal,
        char*	topVal,
        char*	sDate,
        char*	eDate,
        char*	arrGrp,
        char*	lang, char param_cRealTimeFlag)
{
    int		i = 0;
    int		idx = 0;
    int		rowCnt = 0;
    int		topCnt = 0;
//    int		sqlGroupExist = 0;
    int		sqlGroupLevel = 0;
    int		uSqlTopSum = 0;
    int		uSqlGroupSum = 0;
//    int		sSqlGroupVal[10][3] = {{0},{0}};
    int		uSqlGroupVal[10][3] = {{0},{0}};
    char	tmpGroupName[30] = {0x00,};
    unsigned long long	sqlTopGroupSq[10] = {0,};
    char	sqlBuf[8192] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
                // 원래코드
                strcpy(sqlBuf,	"select STDR_UG_SQ, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STDR_UG_SQ "
                                  "order by CNT desc, STDR_UG_SQ asc limit 10");

            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
                sprintf(sqlBuf,	"select STDR_UG_SQ, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STDR_UG_SQ "
                                  "order by CNT desc, STDR_UG_SQ asc limit 10");
            }
            else
            {
                sprintf(sqlBuf,	"select STDR_UG_SQ, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STDR_UG_SQ "
                               "order by CNT desc, STDR_UG_SQ asc limit 10");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf, "select STDR_UG_SQ, sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STDR_UG_SQ "
                                  "order by CNT desc, STDR_UG_SQ asc limit 10");
            }
            else
            {
                sprintf(sqlBuf, "select STWR_UG_SQ, sum(STWR_COUNT) as CNT "
                                "from STW_EVENT_REALTIME_TB "
                                "where STWR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STWR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STWR_UG_SQ "
                                  "order by CNT desc, STWR_UG_SQ asc limit 10");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf, "select STDR_UG_SQ, sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STDR_UG_SQ "
                                  "order by CNT desc, STDR_UG_SQ asc limit 10");
            }
            else
            {
                sprintf(sqlBuf, "select STMR_UG_SQ, sum(STMR_COUNT) as CNT "
                                "from STM_EVENT_REALTIME_TB "
                                "where STMR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STMR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STMR_UG_SQ "
                                  "order by CNT desc, STMR_UG_SQ asc limit 10");
            }
        }

    }
    else //정기 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
                // 원래코드
//            strcpy(sqlBuf,	"select STD_UG_SQ, count(*) as CNT "
                strcpy(sqlBuf,	"select STD_UG_SQ, sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
//                              "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STD_UG_SQ "
                                  "order by CNT desc, STD_UG_SQ asc limit 10");

            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
//            sprintf(sqlBuf,	"select STD_UG_SQ, count(*) as CNT "
                sprintf(sqlBuf,	"select STD_UG_SQ, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
//                               "and STD_EXIST = 1 ", sDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STD_UG_SQ "
                                  "order by CNT desc, STD_UG_SQ asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf,	"select STD_UG_SQ, count(*) as CNT "
                sprintf(sqlBuf,	"select STD_UG_SQ, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
//                               "and STD_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf, "group by STD_UG_SQ "
                               "order by CNT desc, STD_UG_SQ asc limit 10");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf, "select STD_UG_SQ, count(*) as CNT "
                strcpy(sqlBuf, "select STD_UG_SQ, sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STD_UG_SQ "
                                  "order by CNT desc, STD_UG_SQ asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf, "select STW_UG_SQ, count(*) as CNT "
                sprintf(sqlBuf, "select STW_UG_SQ, sum(STW_COUNT) as CNT "
                                "from STW_EVENT_TB "
                                "where STW_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STW_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STW_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STW_UG_SQ "
                                  "order by CNT desc, STW_UG_SQ asc limit 10");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf, "select STD_UG_SQ, count(*) as CNT "
                strcpy(sqlBuf, "select STD_UG_SQ, sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STD_UG_SQ "
                                  "order by CNT desc, STD_UG_SQ asc limit 10");
            }
            else
            {
//            sprintf(sqlBuf, "select STM_UG_SQ, count(*) as CNT "
                sprintf(sqlBuf, "select STM_UG_SQ, sum(STM_COUNT) as CNT "
                                "from STM_EVENT_TB "
                                "where STM_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STM_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STM_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by STM_UG_SQ "
                                  "order by CNT desc, STM_UG_SQ asc limit 10");
            }
        }
    }

    WRITE_DEBUG(CATEGORY_DEBUG,"Rank Group Exec Query : [%s]", sqlBuf);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_INFO,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        idx = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sqlTopGroupSq[idx] = atol(g_stMyCon->row[0]); //group sq
            idx++;
        }

        fdb_SqlFreeResult(g_stMyCon); // 아래 select가 들어가므로 여기서 free한다.

        topCnt = 0;
        for(i=0; i < idx; i++)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
            {
                if (!strcmp(gubun, "day"))
                {
                    if (strlen(sDate) == 0)
                    {
                        //원래 코드
                        sprintf(sqlBuf, "select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                        "from STD_EVENT_REALTIME_TB "
                                        "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "= date_format(curdate() - interval 1 day, '%%Y-%%m-%%d') "
                                        "and STDR_UG_SQ = %llu "
                                        "group by STDR_LEVEL", sqlTopGroupSq[i]);

                    }
                    else if (strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
                    {
                        sprintf(sqlBuf, "select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                        "from STD_EVENT_REALTIME_TB "
                                        "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' "
                                        "and STDR_UG_SQ = %llu "
                                        "group by STDR_LEVEL", sDate,sqlTopGroupSq[i]);
                    }
                    else
                    {
                        sprintf(sqlBuf, "select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                        "from STD_EVENT_REALTIME_TB "
                                        "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "between '%s' and '%s' "
                                        "and STDR_UG_SQ = %llu "
                                        "group by STDR_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }
                else if (!strcmp(gubun, "week"))
                {
                    if (strlen(sDate) == 0)
                    {
                        sprintf(sqlBuf, "select STDR_LEVEL, sum(STDR_COUNT) as CNT "
                                        "from STD_EVENT_REALTIME_TB "
                                        "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "between adddate(curdate(), - weekday(curdate()) -7) "
                                        "and adddate(curdate(), - weekday(curdate()) -1) "
                                        "and STDR_UG_SQ = %llu "
                                        "group by STDR_LEVEL", sqlTopGroupSq[i]);
                    }
                    else
                    {
                        sprintf(sqlBuf, "select STWR_LEVEL, sum(STWR_COUNT) as CNT "
                                        "from STW_EVENT_REALTIME_TB "
                                        "where STWR_RECORD_TIME between '%s' and '%s' "
                                        "and STWR_UG_SQ = %llu "
                                        "group by STWR_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }
                else if(!strcmp(gubun, "month"))
                {
                    if(strlen(sDate) == 0)
                    {
                        sprintf(sqlBuf, "select STDR_LEVEL,sum(STDR_COUNT) as CNT "
                                        "from STD_EVENT_REALTIME_TB "
                                        "where date_format(STDR_RECORD_TIME, '%%Y-%%m') "
                                        "= date_format(curdate() - interval 1 month, '%%Y-%%m') "
                                        "and STDR_UG_SQ = %llu "
                                        "group by STDR_LEVEL", sqlTopGroupSq[i]);
                    }
                    else
                    {
                        sprintf(sqlBuf, "select STMR_LEVEL, sum(STMR_COUNT) as CNT "
                                        "from STM_EVENT_REALTIME_TB "
                                        "where STMR_RECORD_TIME between '%s' and '%s' "
                                        "and STMR_UG_SQ = %llu "
                                        "group by STMR_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }
            }

            else //정기보고서
            {
                if (!strcmp(gubun, "day"))
                {
                    if (strlen(sDate) == 0)
                    {
                        //원래 코드
//                    sprintf(sqlBuf, "select STD_EXIST,STD_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STD_LEVEL, sum(STD_COUNT) as CNT "
                                        "from STD_EVENT_TB "
                                        "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "= date_format(curdate() - interval 1 day, '%%Y-%%m-%%d') "
                                        "and STD_UG_SQ = %llu "
                                        //                                    "group by STD_EXIST,STD_LEVEL", sqlTopGroupSq[i]);
                                        "group by STD_LEVEL", sqlTopGroupSq[i]);

                    }
                    else if (strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
                    {
//                    sprintf(sqlBuf, "select STD_EXIST,STD_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STD_LEVEL,sum(STD_COUNT) as CNT "
                                        "from STD_EVENT_TB "
                                        "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' "
                                        "and STD_UG_SQ = %llu "
                                        //                                    "group by STD_EXIST,STD_LEVEL", sDate,sqlTopGroupSq[i]);
                                        "group by STD_LEVEL", sDate,sqlTopGroupSq[i]);
                    }
                    else
                    {
//                    sprintf(sqlBuf, "select STD_EXIST,STD_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STD_LEVEL,sum(STD_COUNT) as CNT "
                                        "from STD_EVENT_TB "
                                        "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "between '%s' and '%s' "
                                        "and STD_UG_SQ = %llu "
                                        //                                    "group by STD_EXIST,STD_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                                        "group by STD_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }
                else if (!strcmp(gubun, "week"))
                {
                    if (strlen(sDate) == 0)
                    {
//                    sprintf(sqlBuf, "select STD_EXIST,STD_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STD_LEVEL, sum(STD_COUNT) as CNT "
                                        "from STD_EVENT_TB "
                                        "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                        "between adddate(curdate(), - weekday(curdate()) -7) "
                                        "and adddate(curdate(), - weekday(curdate()) -1) "
                                        "and STD_UG_SQ = %llu "
                                        //                                    "group by STD_EXIST,STD_LEVEL", sqlTopGroupSq[i]);
                                        "group by STD_LEVEL", sqlTopGroupSq[i]);
                    }
                    else
                    {
//                    sprintf(sqlBuf, "select STW_EXIST,STW_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STW_LEVEL,sum(STW_COUNT) as CNT "
                                        "from STW_EVENT_TB "
                                        "where STW_RECORD_TIME between '%s' and '%s' "
                                        "and STW_UG_SQ = %llu "
                                        //                                    "group by STW_EXIST,STW_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                                        "group by STW_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }
                else if(!strcmp(gubun, "month"))
                {
                    if(strlen(sDate) == 0)
                    {
//                    sprintf(sqlBuf, "select STD_EXIST,STD_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STD_LEVEL,sum(STD_COUNT) as CNT "
                                        "from STD_EVENT_TB "
                                        "where date_format(STD_RECORD_TIME, '%%Y-%%m') "
                                        "= date_format(curdate() - interval 1 month, '%%Y-%%m') "
                                        "and STD_UG_SQ = %llu "
                                        //                                    "group by STD_EXIST,STD_LEVEL", sqlTopGroupSq[i]);
                                        "group by STD_LEVEL", sqlTopGroupSq[i]);
                    }
                    else
                    {
//                    sprintf(sqlBuf, "select STM_EXIST,STM_LEVEL,count(*) as CNT "
                        sprintf(sqlBuf, "select STM_LEVEL,sum(STM_COUNT) as CNT "
                                        "from STM_EVENT_TB "
                                        "where STM_RECORD_TIME between '%s' and '%s' "
                                        "and STM_UG_SQ = %llu "
                                        //                                    "group by STM_EXIST,STM_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                                        "group by STM_LEVEL", sDate,eDate,sqlTopGroupSq[i]);
                    }
                }

            }


            WRITE_DEBUG(CATEGORY_DEBUG,"Rank Group Exec Query : [%s]",sqlBuf);
            rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
            }

            if(rowCnt > 0)
            {
                while(fdb_SqlFetchRow(g_stMyCon) == 0)
                {
//                    sqlGroupExist = atoi(g_stMyCon->row[0]);
//                    sqlGroupLevel = atoi(g_stMyCon->row[1]);
                    sqlGroupLevel = atoi(g_stMyCon->row[0]);
//                    if(sqlGroupExist == 0) //해지
//                    {
//                        if      (sqlGroupLevel == WARNING)   sSqlGroupVal[i][0] += atoi(g_stMyCon->row[2]);
//                        else if (sqlGroupLevel == CRITICAL)  sSqlGroupVal[i][1] += atoi(g_stMyCon->row[2]);
//                        else if (sqlGroupLevel == BLOCK)     sSqlGroupVal[i][2] += atoi(g_stMyCon->row[2]);
//                    }
//                    else //미해지
//                    {
                        if      (sqlGroupLevel == WARNING)   uSqlGroupVal[i][0] += atoi(g_stMyCon->row[1]);
                        else if (sqlGroupLevel == CRITICAL)  uSqlGroupVal[i][1] += atoi(g_stMyCon->row[1]);
                        else if (sqlGroupLevel == BLOCK)     uSqlGroupVal[i][2] += atoi(g_stMyCon->row[1]);
//                    }
                }
            }
            fdb_SqlFreeResult(g_stMyCon);
        }

        WRITE_INFO(CATEGORY_DB,"Proc rowCnt(%d) ",rowCnt );
        fdb_SqlFreeResult(g_stMyCon);

        topCnt = 0;
        for(i=0; i<idx; i++)
        {
            memset(tmpGroupName, 0x00, sizeof(tmpGroupName));
            fdb_GetGroupName(sqlTopGroupSq[i], tmpGroupName);

            sprintf(rankGroup+strlen(rankGroup), "'%s',", tmpGroupName);
//            sprintf(sCloseVal+strlen(sCloseVal), "%d,",
//                    sSqlGroupVal[i][0]+sSqlGroupVal[i][1]+sSqlGroupVal[i][2]);
            sprintf(uWarnVal+strlen(uWarnVal), "%d,", uSqlGroupVal[i][0]);
            sprintf(uCritVal+strlen(uCritVal), "%d,", uSqlGroupVal[i][1]);
            sprintf(uBlokVal+strlen(uBlokVal), "%d,", uSqlGroupVal[i][2]);

            //Top 1을 구한다. 카운트가 같으면 추가
            if(i == 0)
            {
                uSqlTopSum = uSqlGroupVal[i][0]+uSqlGroupVal[i][1]+uSqlGroupVal[i][2];
                sprintf(topVal, "%s", tmpGroupName);
            }
            else
            {
                uSqlGroupSum = uSqlGroupVal[i][0]+uSqlGroupVal[i][1]+uSqlGroupVal[i][2];
                if(uSqlTopSum == uSqlGroupSum)
                {
                    topCnt++;
                }
            }
        } //for

        if(topCnt != 0)
        {
            if(!strcasecmp(lang, "kr"))
            {
                sprintf(topVal+strlen(topVal), " 외%d개", topCnt);
            }
            else
            {
                sprintf(topVal+strlen(topVal), " other %d ", topCnt);
            }
        }
    }

    return rowCnt;
}


int freport_GetRankTimeBysq(
        char *gubun,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp, char param_cRealTimeFlag
)
{
    int		i = 0;
    int		idx = 0;
    int		rowCnt = 0;
    char    sqlTopTime[MAX_CUSTOM_ARRAY][15] = {{0x00,},{0x00,}};
    int     sqlTopVal[MAX_CUSTOM_ARRAY] = {0};
    char	sqlBuf[8192] = {0x00,};

    for(i=0; i<MAX_CUSTOM_ARRAY; i++)
    {
        memset(sqlTopTime[i], 0x00, sizeof(sqlTopTime[i]));
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0) //정규time
            {
                strcpy(sqlBuf,	"select date_format(STDR_RECORD_TIME, '%H') as TIME, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by date_format(STDR_RECORD_TIME, '%H') "
                                  "order by CNT desc, TIME asc");
            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
                sprintf(sqlBuf,	"select date_format(STDR_RECORD_TIME, '%%H') as TIME, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ",sDate );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by date_format(STDR_RECORD_TIME, '%H') "
                                  "order by CNT desc, TIME asc");

            }
            else //daily
            {
                sprintf(sqlBuf,	"select date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') as TIME, SUM(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "order by CNT desc, TIME asc");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0) //일주일정규보고서
            {
                strcpy(sqlBuf, "select date_format(STDR_RECORD_TIME, '%Y-%m-%d') as TIME, sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by TIME order by CNT desc, TIME asc");
            }
            else  //1Week 맞춤보고서
            {
                sprintf(sqlBuf, "select STWR_RECORD_TIME, sum(STWR_COUNT) as CNT "
                                "from STW_EVENT_REALTIME_TB "
                                "where STWR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STWR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STWR_RECORD_TIME order by CNT desc, STWR_RECORD_TIME asc");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0) //정규 전달일자별통계
            {
                strcpy(sqlBuf, "select date_format(STDR_RECORD_TIME, '%Y-%m-%d') as TIME,sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by TIME order by CNT desc, TIME asc");
            }
            else //맞춤월별통계
            {
                sprintf(sqlBuf, "select STMR_RECORD_TIME, sum(STMR_COUNT) as CNT "
                                "from STM_EVENT_REALTIME_TB "
                                "where STMR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STMR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STMR_RECORD_TIME "
                               "order by CNT desc, STMR_RECORD_TIME asc");
            }
        }
    }
    else //정기 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0) //정규time
            {
//            strcpy(sqlBuf,	"select date_format(STD_RECORD_TIME, '%H') as TIME,count(*) as CNT "
                strcpy(sqlBuf,	"select date_format(STD_RECORD_TIME, '%H') as TIME, sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
//                              "and STD_EXIST = 1 "); // 발생이외에도 TOP을 구해야 하므로
                if(strlen(arrGrp) > 0)
                {
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                }
                strcat(sqlBuf,	"group by date_format(STD_RECORD_TIME, '%H') "
                                  "order by CNT desc, TIME asc");
            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
//            sprintf(sqlBuf,	"select date_format(STD_RECORD_TIME, '%%H') as TIME,count(*) as CNT "
                sprintf(sqlBuf,	"select date_format(STD_RECORD_TIME, '%%H') as TIME, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ",sDate );
//                               "and STD_EXIST = 1 ", sDate); // 발생 이외에도 TOP을 구해야 하므로
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by date_format(STD_RECORD_TIME, '%H') "
                                  "order by CNT desc, TIME asc");

            }
            else //daily
            {
//            sprintf(sqlBuf,	"select date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') as TIME,count(*) as CNT "
                sprintf(sqlBuf,	"select date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') as TIME, SUM(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
//                               "and STD_EXIST = 1 ", sDate,eDate); // 발생 이외에도 TOP을 구해야 하므로
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "order by CNT desc, TIME asc");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0) //일주일정규보고서
            {
//            strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%Y-%m-%d') as TIME,count(*) as CNT "
                strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%Y-%m-%d') as TIME, sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by TIME order by CNT desc, TIME asc");
            }
            else  //1Week 맞춤보고서
            {
//            sprintf(sqlBuf, "select STW_RECORD_TIME, count(*) as CNT "
                sprintf(sqlBuf, "select STW_RECORD_TIME, sum(STW_COUNT) as CNT "
                                "from STW_EVENT_TB "
                                "where STW_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STW_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STW_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STW_RECORD_TIME order by CNT desc, STW_RECORD_TIME asc");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0) //정규 전달일자별통계
            {
//            strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%Y-%m-%d') as TIME,count(*) as CNT "
                strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%Y-%m-%d') as TIME,sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by TIME order by CNT desc, TIME asc");
            }
            else //맞춤월별통계
            {
//            sprintf(sqlBuf, "select STM_RECORD_TIME,count(*) as CNT "
                sprintf(sqlBuf, "select STM_RECORD_TIME,sum(STM_COUNT) as CNT "
                                "from STM_EVENT_TB "
                                "where STM_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STM_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STM_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STM_RECORD_TIME "
                               "order by CNT desc, STM_RECORD_TIME asc");
            }
        }
    }


    WRITE_DEBUG(CATEGORY_DEBUG,"Time Line Execute Query : [%s]", sqlBuf);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        idx = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(sqlTopTime[idx], g_stMyCon->row[0]); //time
            sqlTopVal[idx] = atoi(g_stMyCon->row[1]); //cnt
            idx++;
        }

        for(i=0; i<MAX_CUSTOM_ARRAY; i++)
        {
            //Top 1을 구한다. 카운트가 같으면 추가
            if(i == 0)
            {
                sprintf(topVal, "%s", sqlTopTime[i]);
            }
            else
            {
                if(sqlTopVal[0] == sqlTopVal[i])
                {
                    sprintf(topVal+strlen(topVal), ", %s", sqlTopTime[i]);
                }
            }
        } //for
    }
    fdb_SqlFreeResult(g_stMyCon);

    return rowCnt;
}

int freport_GetRankTypeBysq(
        char *gubun,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp,
        char *lang, char param_cRealTimeFlag)
{
    int		i = 0;
    int		idx = 0;
    int		rowCnt = 0;
    int		topCnt = 0;
    int     sqlTopType[CUR_RULE_CNT][2] = {{0},{0}};
    char	sqlBuf[8192] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    if ( param_cRealTimeFlag != 0x00 ) //실시간 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
                //원래 코드
                strcpy(sqlBuf,	"select STDR_TYPE, sum(STDR_COUNT) as CNT "
                                  "from STD_EVENT_REALTIME_TB "
                                  "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STDR_TYPE "
                               "order by CNT desc, STDR_TYPE asc");
            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
                sprintf(sqlBuf,	"select STDR_TYPE, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STDR_TYPE "
                                  "order by CNT desc, STDR_TYPE asc");
            }
            else
            {
                sprintf(sqlBuf,	"select STDR_TYPE, sum(STDR_COUNT) as CNT "
                                   "from STD_EVENT_REALTIME_TB "
                                   "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STDR_TYPE "
                                  "order by CNT desc, STDR_TYPE asc");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf, "select STDR_TYPE, sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STDR_TYPE "
                               "order by CNT desc, STDR_TYPE asc");
            }
            else
            {
                sprintf(sqlBuf, "select STWR_TYPE, sum(STWR_COUNT) as CNT "
                                "from STW_EVENT_REALTIME_TB "
                                "where STWR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STWR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STWR_TYPE "
                                  "order by CNT desc, STWR_TYPE asc");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
                strcpy(sqlBuf, "select STDR_TYPE, sum(STDR_COUNT) as CNT "
                               "from STD_EVENT_REALTIME_TB "
                               "where date_format(STDR_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STDR_TYPE "
                                  "order by CNT desc, STDR_TYPE asc");
            }
            else
            {
                sprintf(sqlBuf, "select STMR_TYPE, sum(STMR_COUNT) as CNT "
                                "from STM_EVENT_REALTIME_TB "
                                "where STMR_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STMR_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STMR_TYPE "
                                  "order by CNT desc, STMR_TYPE asc");
            }
        }
    }
    else //정기 보고서
    {
        if(!strcmp(gubun, "day"))
        {
            if(strlen(sDate) == 0)
            {
                //원래 코드
                strcpy(sqlBuf,	"select STD_TYPE, sum(STD_COUNT) as CNT "
                                  "from STD_EVENT_TB "
                                  "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                                  "= date_format(curdate() - interval 1 day, '%Y-%m-%d') " );
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STD_TYPE "
                               "order by CNT desc, STD_TYPE asc");
            }
            else if(strlen(sDate) != 0 && !strcmp(sDate,eDate)) //맞춤24time
            {
//            sprintf(sqlBuf,	"select STD_TYPE, count(*) as CNT "
                sprintf(sqlBuf,	"select STD_TYPE, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", sDate);
//                               "and STD_EXIST = 1 ", sDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STD_TYPE "
                                  "order by CNT desc, STD_TYPE asc");
            }
            else
            {
//            sprintf(sqlBuf,	"select STD_TYPE,count(*) as CNT "
                sprintf(sqlBuf,	"select STD_TYPE, sum(STD_COUNT) as CNT "
                                   "from STD_EVENT_TB "
                                   "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                   "between '%s' and '%s' ", sDate, eDate);
//                               "and STD_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STD_TYPE "
                                  "order by CNT desc, STD_TYPE asc");
            }
        }
        else if(!strcmp(gubun, "week"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf, "select STD_TYPE,count(*) as CNT "
                strcpy(sqlBuf, "select STD_TYPE,sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                               "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                               "and adddate(curdate(), - WEEKDAY(curdate()) -1) " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf, "group by STD_TYPE "
                               "order by CNT desc, STD_TYPE asc");
            }
            else
            {
//            sprintf(sqlBuf, "select STW_TYPE,count(*) as CNT "
//            sprintf(sqlBuf, "select STW_TYPE,sum(*) as CNT "
                sprintf(sqlBuf, "select STW_TYPE,sum(STW_COUNT) as CNT "
                                "from STW_EVENT_TB "
                                "where STW_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STW_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STW_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STW_TYPE "
                                  "order by CNT desc, STW_TYPE asc");
            }
        }
        else if(!strcmp(gubun, "month"))
        {
            if(strlen(sDate) == 0)
            {
//            strcpy(sqlBuf, "select STD_TYPE,sum(*) as CNT "
                strcpy(sqlBuf, "select STD_TYPE,sum(STD_COUNT) as CNT "
                               "from STD_EVENT_TB "
                               "where date_format(STD_RECORD_TIME, '%Y-%m') "
                               "= date_format(CURDATE() - interval 1 month, '%Y-%m') " );
//                           "and STD_EXIST = 1 ");
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STD_TYPE "
                                  "order by CNT desc, STD_TYPE asc");
            }
            else
            {
//            sprintf(sqlBuf, "select STM_TYPE,count(*) as CNT "
                sprintf(sqlBuf, "select STM_TYPE,sum(STM_COUNT) as CNT "
                                "from STM_EVENT_TB "
                                "where STM_RECORD_TIME between '%s' and '%s' ", sDate, eDate);
//                            "and STM_EXIST = 1 ", sDate,eDate);
                if(strlen(arrGrp) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STM_UG_SQ in (%s) ", arrGrp);
                strcat(sqlBuf,	"group by STM_TYPE "
                                  "order by CNT desc, STM_TYPE asc");
            }
        }

    }


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        idx = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sqlTopType[idx][0] = atoi(g_stMyCon->row[0]); //type
            sqlTopType[idx][1] = atoi(g_stMyCon->row[1]); //cnt
            idx++;
        }

        topCnt = 0;
        for(i=0; i<CUR_RULE_CNT; i++)
        {
            //Top 1을 구한다. 카운트가 같으면 추가
            if(i == 0)
            {
//                fcom_GetStrType2(sqlTopType[i][0],topVal,fcom_GetCustLang());
                sprintf(topVal, "%s", fcom_getStrType(sqlTopType[i][0]));

            }
            else
            {
                if(sqlTopType[0][1] == sqlTopType[i][1])
                {
                    topCnt++;
                }
            }
        } //for

        if(topCnt != 0)
        {
            if(!strcasecmp(lang, "kr"))
            {
                sprintf(topVal+strlen(topVal), " 외%d개", topCnt);
            }
            else
            {
                sprintf(topVal+strlen(topVal), " other %d ", topCnt);
            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Proc rowCnt(%d)",
            rowCnt);

    fdb_SqlFreeResult(g_stMyCon);

    return rowCnt;
}



int freport_DeployReportDailyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// char result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly,	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// char size
        char                param_cRealTimeFlag
)
{
    int		i = 0, rowCnt = 0,nRes = 0, rxt = 0;
    int		sqlType = 0, sqlHour = 0, sqlLevel = 0;
    //daily count
    int		sDayTypeTot = 0;

    char	sqlRange[10 +1] = {0x00,};
    char	sqlBuf[8192 +1] = {0x00,};
    char	sCategory[1024] = {0x00,};
    char	topValue[512 +1] = {0x00,};
    char	strFontWarn[50 +1] = {0x00,};
    char	strFontCrit[50 +1] = {0x00,};
    char	strFontBlok[50 +1] = {0x00,};
    char	refGroupSq[4096 +1] = {0x00,}; //조건절 그룹명in리스트

//    int     sTotDay[3] = {0}; //해지 1:경고,2:위험,3:차단
//    int     sDayValue[24][3] = {{0},{0}};
//    int     sDayTypeValue[CUR_RULE_CNT][3] = {{0},{0}};

    int     uTotDay[3] = {0}; //미해지 1:경고,2:위험,3:차단
    int     uDayValue[24][3] = {{0},{0}};
    int     uDayTypeValue[CUR_RULE_CNT][3] = {{0},{0}};

    //daily html data
    char    pieTotDay[64 +1] = {0x00,};		//해지 총합계+미해지
    char    sStrCloseDay[512 +1] = {0x00,};	//해지 총합계
    char    uStrWarnDay[512 +1] = {0x00,};	//미해결 경고
    char    uStrCritDay[512 +1] = {0x00,};	//미해결 위험
    char    uStrBlokDay[512 +1] = {0x00,};	//미해결 차단
    char	repFullPath[256 +1] = {0x00,};
    char	repTmpBuf[2048 +1] = {0x00,};
    char	currLang[2 +1] = {0x00,};


    if(strlen(fcom_GetCustLang()) > 0)
    {
        snprintf(currLang, sizeof(currLang), "%s", fcom_GetCustLang());
    }
    else
    {
        snprintf(currLang, sizeof(currLang), "%s", g_stProcReportInfo.szConfMailLang);
    }

    //0. date
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select date_format(curdate() - interval 1 day, '%%Y-%%m-%%d')");

    WRITE_INFO(CATEGORY_INFO,"sqlBuf(%s) ",sqlBuf );

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            snprintf(sqlRange, sizeof(sqlRange), "%s",g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    strcpy(resRange, sqlRange);

    // 하위그룹 sq를 구한다.
    memset(refGroupSq, 0x00, sizeof(refGroupSq));
    if(grpSq != 0)
    {
        fdb_GetSubGroupSq(grpSq, refGroupSq);
    }

    if ( param_cRealTimeFlag != 0x00 ) //실시간 보고서
    {
        //1.daily_total
        strcpy(sqlBuf, "select date_format(STDR_RECORD_TIME, '%H'), "
                       "STDR_TYPE, STDR_LEVEL, sum(STDR_COUNT) as CNT "
                       "from STD_EVENT_REALTIME_TB "
                       "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                       "= date_format(curdate() - interval 1 day, '%Y-%m-%d') ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, 	"group by date_format(STDR_RECORD_TIME, '%H'), "
                          "STDR_TYPE, STDR_LEVEL, STDR_EXIST ");
    }
    else //정기 보고서
    {
        //1.daily_total
        strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%H'), "
                       "STD_TYPE, STD_LEVEL, sum(STD_COUNT) as CNT "
                       "from STD_EVENT_TB "
                       "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                       "= date_format(curdate() - interval 1 day, '%Y-%m-%d') ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, 	"group by date_format(STD_RECORD_TIME, '%H'), "
                          "STD_TYPE, STD_LEVEL, STD_EXIST ");
    }


    WRITE_DEBUG(CATEGORY_DEBUG,"Report Weekly Exec Sql : [%s]", sqlBuf);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sqlHour = atoi(g_stMyCon->row[0]);
            sqlType = atoi(g_stMyCon->row[1]);
            sqlLevel = atoi(g_stMyCon->row[2]);
//            sqlExist = atoi(g_stMyCon->row[3]);

            //시간대별 정리
            for(i=0; i<24; i++)
            {
                if(sqlHour == i)
                {
//                    if(sqlExist == 0) //해지
//                    {
//                        if		(sqlLevel == WARNING) 	sDayValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sDayValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sDayValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else
//                    { //미해지
                        if		(sqlLevel == WARNING) 	uDayValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL)	uDayValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uDayValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            } //for

            //항목별 정리
            for(i=0; i<CUR_RULE_CNT; i++)
            {
                if(sqlType == i)
                {
//                    if(sqlExist == 0)
//                    { //해지
//                        if      (sqlLevel == WARNING) 	sDayTypeValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sDayTypeValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sDayTypeValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else
//                    {
                        if		(sqlLevel == WARNING) 	uDayTypeValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uDayTypeValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uDayTypeValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            } //for
        } //while
    }

    fdb_SqlFreeResult(g_stMyCon);

    //tot_day
    for(i=0; i<24; i++)
    {
//        sTotDay[0] += sDayValue[i][0];
//        sTotDay[1] += sDayValue[i][1];
//        sTotDay[2] += sDayValue[i][2];
        uTotDay[0] += uDayValue[i][0];
        uTotDay[1] += uDayValue[i][1];
        uTotDay[2] += uDayValue[i][2];
    }
    memset(repFullPath, 0x00, sizeof(repFullPath));
    sprintf(repFullPath,"%s/%s", p_repPath, "template");

    WRITE_INFO(CATEGORY_INFO,"[DAILY]1. Make pie-chart " );


    memset(pieTotDay, 0x00, sizeof(pieTotDay));
    memset(strFontWarn, 0x00, sizeof(strFontWarn));
    memset(strFontCrit, 0x000, sizeof(strFontCrit));
    memset(strFontBlok, 0x00, sizeof(strFontBlok));
    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));

    if( strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") != 0 )
    {
//        sprintf(pieTotDay, "%d,%d,%d,%d,", sTotDay[0]+sTotDay[1]+sTotDay[2],uTotDay[0],uTotDay[1],uTotDay[2]);
        sprintf(pieTotDay, "%d,%d,%d,", uTotDay[0],uTotDay[1],uTotDay[2]);

        if(uTotDay[0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uTotDay[0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uTotDay[0]);
        if(uTotDay[1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uTotDay[1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uTotDay[1]);
        if(uTotDay[2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uTotDay[2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uTotDay[2]);

        if(!strncmp(currLang, "kr", 2))
        {
            if((uTotDay[0]+uTotDay[1]+uTotDay[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ 0 ]건 입니다.");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ %d ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
                        uTotDay[0]+uTotDay[1]+uTotDay[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        else
        {
            if((uTotDay[0]+uTotDay[1]+uTotDay[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ 0 ].");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ %d ] (Warning: %s, Critical: %s, Block: %s).",
                        uTotDay[0]+uTotDay[1]+uTotDay[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }

//        if(sTotDay[0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sTotDay[0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", sTotDay[0]);
//        if(sTotDay[1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sTotDay[1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sTotDay[1]);
//        if(sTotDay[2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sTotDay[2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sTotDay[2]);
//
//        if(!strncmp(currLang, "kr", 2))
//        {
//            if((sTotDay[0]+sTotDay[1]+sTotDay[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">0</font> ]건 입니다.");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">%d</font> ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
//                        sTotDay[0]+sTotDay[1]+sTotDay[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
//        else
//        {
//            if((sTotDay[0]+sTotDay[1]+sTotDay[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">0</font> ].");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">%d</font> ] (Warning: %s, Critical: %s, Block: %s).",
//                        sTotDay[0]+sTotDay[1]+sTotDay[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
        nRes = fcom_GetReportFile(repFullPath, "pie.js", repTmpBuf);
    }
    else
    {
        sprintf(pieTotDay, "%d,%d,%d,", uTotDay[0],uTotDay[1],uTotDay[2]);
        if(uTotDay[0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uTotDay[0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uTotDay[0]);
        if(uTotDay[1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uTotDay[1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uTotDay[1]);
        if(uTotDay[2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uTotDay[2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uTotDay[2]);

        if(!strncmp(currLang, "kr", 2))
        {
            if((uTotDay[0]+uTotDay[1]+uTotDay[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ 0 ]건 입니다.");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ %d ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
                        uTotDay[0]+uTotDay[1]+uTotDay[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        else
        {
            if((uTotDay[0]+uTotDay[1]+uTotDay[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ 0 ].");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ %d ] (Warning: %s, Critical: %s, Block: %s).",
                        uTotDay[0]+uTotDay[1]+uTotDay[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        nRes = fcom_GetReportFile(repFullPath, "pie2.js", repTmpBuf);
    }
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- pie-chart pieTotDay(%s) ",pieTotDay );


    nRes = fcom_ReportPieFilter(repTmpBuf, "varPieDay", pieTotDay,
            g_stProcReportInfo.szConfMailLang,
            g_stProcReportInfo.szConfMailClosedView);

    WRITE_INFO(CATEGORY_INFO,"[DAILY]- pie-chart size(%d) ",
            nRes);
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO,"[DAILY]2. Make timeline-chart" );


//    memset(sStrCloseDay, 0x00, sizeof(sStrCloseDay));
//    for(i=0; i<24; i++)
//    {
//        sprintf(sStrCloseDay+strlen(sStrCloseDay), "%d,",
//                sDayValue[i][0] +sDayValue[i][1] +sDayValue[i][2]);
//    }

    //미해지 경고건수
    memset(uStrWarnDay, 0x00, sizeof(uStrWarnDay));
    for(i=0; i<24; i++)
    {
        sprintf(uStrWarnDay+strlen(uStrWarnDay), "%d,", uDayValue[i][0]);
    }
    //미해지 위험건수
    memset(uStrCritDay, 0x00, sizeof(uStrCritDay));
    for(i=0; i<24; i++)
    {
        sprintf(uStrCritDay+strlen(uStrCritDay), "%d,", uDayValue[i][1]);
    }
    //미해지 차단건수
    memset(uStrBlokDay, 0x00, sizeof(uStrBlokDay));
    for(i=0; i<24; i++)
    {
        sprintf(uStrBlokDay+strlen(uStrBlokDay), "%d,", uDayValue[i][2]);
    }

    strcpy(sCategory,
            "'00','01','02','03','04','05','06','07','08','09','10','11','12','13','14','15','16','17','18','19','20','21','22','23'");


    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart sCategory(%s) ",sCategory      );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart sStrCloseDay(%s) ",sStrCloseDay);
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart uStrWarnDay(%s) ",uStrWarnDay  );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart uStrCritDay(%s) ",uStrCritDay  );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart uStrBlokDay(%s) ",uStrBlokDay  );

    memset(repTmpBuf, 0x00 ,sizeof(repTmpBuf));

    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic2.js", repTmpBuf);
    }
    nRes = fcom_ReportLineFilter(repTmpBuf, "varLineDay",
                              sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay);

    WRITE_INFO(CATEGORY_INFO,"[DAILY]- timeline-chart size(%d) ",nRes );
    strcat(resChart, repTmpBuf);

    //분석용랭킹구하기
    memset(topValue, 0x00, sizeof(topValue));
    rxt = freport_GetRankTimeBysq("day", topValue, "", "", refGroupSq, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTimeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTimeAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTimeAnaly, "ㆍ 이벤트 발생이 가장 많은 시간대는 [ %s ]시 입니다.", topValue);
        else
            sprintf(resTimeAnaly, "ㆍThe most frequent occurrence of the event is at [ %s ].", topValue);
    }


    WRITE_INFO(CATEGORY_INFO,"[DAILY]3. Make rank-chart" );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseDay, 0x00, sizeof(sStrCloseDay));
    memset(uStrWarnDay, 0x00, sizeof(uStrWarnDay));
    memset(uStrBlokDay, 0x00, sizeof(uStrBlokDay));
    memset(uStrCritDay, 0x00, sizeof(uStrCritDay));


    rxt = freport_GetRankIp("day",
                      sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay,
                      topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);

    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resRankAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resRankAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP ID는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 사번은 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 이름은 [ %s ] 입니다.", topValue);
            else
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
        }
        else
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍThe ID with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍThe SNO with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍThe NAME with the most event occurrences is [ %s ].", topValue);
            else
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
        }
    }


    WRITE_INFO(CATEGORY_INFO, "[DAILY]- rank-chart sCategory(%s)",sCategory );
    WRITE_INFO(CATEGORY_INFO, "[DAILY]- rank-chart sStrCloseDay(%s)",sStrCloseDay );
    WRITE_INFO(CATEGORY_INFO, "[DAILY]- rank-chart uStrWarnDay(%s)",uStrWarnDay );
    WRITE_INFO(CATEGORY_INFO, "[DAILY]- rank-chart uStrCritDay(%s)",uStrCritDay);
    WRITE_INFO(CATEGORY_INFO, "[DAILY]- rank-chart uStrBlokDay(%s)",uStrBlokDay);

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "rankChartDay",
                             sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay);
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- rank-chart size(%d) ",nRes);
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO,"[DAILY]4. Make type-chart " );


    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseDay, 0x00, sizeof(sStrCloseDay));
    memset(uStrWarnDay, 0x00, sizeof(uStrWarnDay));
    memset(uStrCritDay, 0x00, sizeof(uStrCritDay));
    memset(uStrBlokDay, 0x00, sizeof(uStrBlokDay));

    for(i=0; i<CUR_RULE_CNT; i++)
    {
        //모든 합이 0이 아니라면
        if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
        {
//            sDayTypeTot = sDayTypeValue[i][0]+sDayTypeValue[i][1]+sDayTypeValue[i][2]
//                          + uDayTypeValue[i][0]+uDayTypeValue[i][1]+uDayTypeValue[i][2];
            sDayTypeTot =  uDayTypeValue[i][0]+uDayTypeValue[i][1]+uDayTypeValue[i][2];
        }
        else
        {
            sDayTypeTot = uDayTypeValue[i][0]+uDayTypeValue[i][1]+uDayTypeValue[i][2];
        }

        if(sDayTypeTot != 0)
        {
//            fcom_GetStrType2(i,sCategory+strlen(sCategory),fcom_GetCustLang());
            sprintf(sCategory+strlen(sCategory), "'%s',", fcom_getStrType(i));
//            sprintf(sStrCloseDay+strlen(sStrCloseDay), "%d,",
//                    sDayTypeValue[i][0]+sDayTypeValue[i][1]+sDayTypeValue[i][2]);
            sprintf(uStrWarnDay+strlen(uStrWarnDay), "%d,", uDayTypeValue[i][0]);
            sprintf(uStrCritDay+strlen(uStrCritDay), "%d,", uDayTypeValue[i][1]);
            sprintf(uStrBlokDay+strlen(uStrBlokDay), "%d,", uDayTypeValue[i][2]);
        }
    }

    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart sCategory(%s)  ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart sStrCloseDay(%s) ",sStrCloseDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart uStrWarnDay(%s) ",uStrWarnDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart uStrCritDay(%s) ",uStrCritDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart uStrBlokDay(%s) ",uStrBlokDay );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "typeChartDay",
                             sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay);

    WRITE_INFO(CATEGORY_INFO,"[DAILY]- type-chart size(%d) ",nRes );
    strcat(resChart, repTmpBuf);
    memset(topValue, 0x00, sizeof(topValue));
    rxt = freport_GetRankTypeBysq("day", topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTypeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTypeAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTypeAnaly, "ㆍ 이벤트 발생이 가장 많은 항목은 [ %s ] 입니다.", topValue);
        else
            sprintf(resTypeAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[DAILY]5. Make group-chart" );


    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseDay, 0x00, sizeof(sStrCloseDay));
    memset(uStrWarnDay, 0x00, sizeof(uStrWarnDay));
    memset(uStrCritDay, 0x00, sizeof(uStrCritDay));
    memset(uStrBlokDay, 0x00, sizeof(uStrBlokDay));
    memset(topValue, 0x00, sizeof(topValue));

    rxt = freport_GetRankGroupBysq("day",
                              sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay,
                              topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt < 1)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resGroupAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resGroupAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resGroupAnaly, "ㆍ 이벤트 발생이 가장 많은 그룹은 [ %s ] 입니다.", topValue);
        else
            sprintf(resGroupAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart sCategory(%s) ", sCategory );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart sStrCloseDay(%s) ", sStrCloseDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart uStrWarnDay(%s) ", uStrWarnDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart uStrCritDay(%s) ", uStrCritDay );
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart uStrBlokDay(%s) ", uStrBlokDay );


    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "groupChartDay",
                             sCategory, sStrCloseDay, uStrWarnDay, uStrCritDay, uStrBlokDay);
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- group-chart size(%d) ",nRes );

    strcat(resChart, repTmpBuf);
    *chartSize = strlen(resChart);
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- all chart tot size(%d) ",*chartSize );

    WRITE_INFO(CATEGORY_INFO,"[DAILY]6. Make table" );

//    nRes = freport_MakeTableDay(resTable, sDayValue, uDayValue);
    nRes = freport_MakeTableDay(resTable, uDayValue);
    WRITE_INFO(CATEGORY_INFO,"[DAILY]- table size(%d) ",nRes );

    return strlen(resTable);
}

int freport_DeployReportMonthlyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// chart size
        char                param_cRealTimeFlag
)
{
    int		i;
    int		idx;
    int		nRes;
    int		rxt;
    int		rowCnt;

    int		monthNum;
    int		sqlType;
    int		sqlLevel;
//    int		sqlExist;
    char	sqlRange[7+1];
    char	*tmpRange;
    char	sqlMonth[31][10+1];
    char	sqlBuf[8192];
    char	tmpType[13+1];
    char	sCategory[1024];
    char	topValue[512];
    char	strFontWarn[50];
    char	strFontCrit[50];
    char	strFontBlok[50];

    char	refGroupSq[4096];

    //monthly count
    int		sMonthTypeTot = 0;

//    int     sTotMonth[3] = {0};
//    int     sMonthValue[31][3] = {{0},{0}};
//    int     sMonthTypeValue[CUR_RULE_CNT][3] = {{0},{0}};

    int     uTotMonth[3] = {0};
    int     uMonthValue[31][3] = {{0},{0}};
    int     uMonthTypeValue[CUR_RULE_CNT][3] = {{0},{0}};


    //monthly html data
    char    pieTotMonth[64];	//해지총합계+미해지
    char	sStrCloseMonth[128];
    char    uStrWarnMonth[128];
    char    uStrCritMonth[128];
    char    uStrBlokMonth[128];

    char	repFullPath[256];
    char	repTmpBuf[2048];
    char	currLang[2+1];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        strcpy(currLang, fcom_GetCustLang());
    }
    else
    {
        strcpy(currLang, g_stProcReportInfo.szConfMailLang);
    }

    strcpy(sqlBuf, "select a.Date from ("
               "select curdate() - INTERVAL (a.a + (10 * b.a) + (100 * c.a)) DAY as Date "
               "from (select 0 as a union all select 1 union all select 2  "
               "	union all select 3 "
               "	union all select 4 "
               "	union all select 5 "
               "	union all select 6 "
               "	union all select 7 "
               "	union all select 8 "
               "	union all select 9) as a "
               "	cross join ( "
               "		select 0 as a "
               "		union all select 1 "
               "		union all select 2 "
               "		union all select 3 "
               "		union all select 4 "
               "		union all select 5 "
               "		union all select 6 "
               "		union all select 7 "
               "		union all select 8 "
               "		union all select 9) as b "
               "	cross join ( "
               "		select 0 as a "
               "		union all select 1 "
               "		union all select 2 "
               "		union all select 3 "
               "		union all select 4 "
               "		union all select 5 "
               "		union all select 6 "
               "		union all select 7 "
               "		union all select 8 "
               "		union all select 9) as c "
               ") a where date_format(a.Date, '%Y-%m') "
               "= date_format(CURDATE() - interval 1 month, '%Y-%m') order by Date asc");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        idx = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(sqlMonth[idx], g_stMyCon->row[0]);
            idx++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    monthNum = rowCnt;

    tmpRange = NULL;
    tmpRange = strrchr(sqlMonth[0], '-');

    memset(sqlRange, 0x00, sizeof(sqlRange));
    strncpy(sqlRange, sqlMonth[0], tmpRange-sqlMonth[0]);
    tmpRange = NULL;
    strcpy(resRange, sqlRange);

    // 하위그룹 sq를 구한다.
    memset(refGroupSq, 0x00, sizeof(refGroupSq));
    if(grpSq != 0)
        fdb_GetSubGroupSq(grpSq, refGroupSq);

    if ( param_cRealTimeFlag != 0x00 ) //실시간 보고서
    {
        //1.monthly_total
        strcpy(sqlBuf, 	"select "
                          "STDR_RECORD_TIME, STDR_TYPE, STDR_LEVEL, sum(STDR_COUNT) as CNT "
                          "from STD_EVENT_REALTIME_TB "
                          "where date_format(STDR_RECORD_TIME, '%Y-%m') "
                          "= date_format(CURDATE() - interval 1 month, '%Y-%m') ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, "group by STDR_RECORD_TIME, STDR_TYPE, STDR_LEVEL, STDR_EXIST");


    }
    else // 정기 보고서
    {
        //1.monthly_total
        strcpy(sqlBuf, 	"select "
                          //                  "STD_RECORD_TIME,STD_TYPE,STD_LEVEL,STD_EXIST,count(*) as CNT "
                          "STD_RECORD_TIME,STD_TYPE,STD_LEVEL,sum(STD_COUNT) as CNT "
                          "from STD_EVENT_TB "
                          "where date_format(STD_RECORD_TIME, '%Y-%m') "
                          "= date_format(CURDATE() - interval 1 month, '%Y-%m') ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, "group by STD_RECORD_TIME,STD_TYPE,STD_LEVEL,STD_EXIST");


    }


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sqlType		= atoi(g_stMyCon->row[1]);
            sqlLevel	= atoi(g_stMyCon->row[2]);
//            sqlExist	= atoi(g_stMyCon->row[3]);
            for(i=0; i<monthNum; i++)
            {
                if (!strncmp(sqlMonth[i], g_stMyCon->row[0], 10))
                {
//                    if(sqlExist == 0) //해지
//                    {
//                        if		(sqlLevel == WARNING) 	sMonthValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sMonthValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sMonthValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else //미해지
//                    {
                        if		(sqlLevel == WARNING) 	uMonthValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uMonthValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uMonthValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            }//for
            //항목별 정리
            for(i=0; i<CUR_RULE_CNT; i++)
            {
                if(sqlType == i)
                {
//                    if(sqlExist == 0) //해지
//                    {
//                        if      (sqlLevel == WARNING) 	sMonthTypeValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sMonthTypeValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sMonthTypeValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else
//                    {
                        if		(sqlLevel == WARNING) 	uMonthTypeValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uMonthTypeValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uMonthTypeValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            } //for
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    //tot_day
    for(i=0; i<monthNum; i++)
    {
//        sTotMonth[0] += sMonthValue[i][0];
//        sTotMonth[1] += sMonthValue[i][1];
//        sTotMonth[2] += sMonthValue[i][2];
        uTotMonth[0] += uMonthValue[i][0];
        uTotMonth[1] += uMonthValue[i][1];
        uTotMonth[2] += uMonthValue[i][2];
    }

    //tot_day html data
    memset(repFullPath, 0x00, sizeof(repFullPath));
    sprintf(repFullPath,"%s/%s", p_repPath, "template");

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]1. Make pie-chart" );

    memset(pieTotMonth, 0x00, sizeof(pieTotMonth));
    memset(strFontWarn, 0x00, sizeof(strFontWarn));
    memset(strFontCrit, 0x00, sizeof(strFontCrit));
    memset(strFontBlok, 0x00, sizeof(strFontBlok));
    memset(repTmpBuf,   0x00, sizeof(repTmpBuf));

    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
//        sprintf(pieTotMonth, "%d,%d,%d,%d,", sTotMonth[0]+sTotMonth[1]+sTotMonth[2],uTotMonth[0],uTotMonth[1],uTotMonth[2]);
        sprintf(pieTotMonth, "%d,%d,%d,", uTotMonth[0],uTotMonth[1],uTotMonth[2]);

//        if(sTotMonth[0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sTotMonth[0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", sTotMonth[0]);
//        if(sTotMonth[1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sTotMonth[1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sTotMonth[1]);
//        if(sTotMonth[1] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sTotMonth[2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sTotMonth[2]);

//        if(!strncmp(currLang, "kr", 2))
//        {
//            if((sTotMonth[0]+sTotMonth[1]+sTotMonth[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">0</font> ]건 입니다.");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">%d</font> ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
//                        sTotMonth[0]+sTotMonth[1]+sTotMonth[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
//        else
//        {
//            if((sTotMonth[0]+sTotMonth[1]+sTotMonth[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">0</font> ].");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">%d</font> ] (Warning: %s, Critical: %s, Block: %s).",
//                        sTotMonth[0]+sTotMonth[1]+sTotMonth[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
        nRes = fcom_GetReportFile(repFullPath, "pie.js", repTmpBuf);
    }
    else
    {
        sprintf(pieTotMonth, "%d,%d,%d,", uTotMonth[0],uTotMonth[1],uTotMonth[2]);
        if(uTotMonth[0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uTotMonth[0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uTotMonth[0]);
        if(uTotMonth[1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uTotMonth[1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uTotMonth[1]);
        if(uTotMonth[1] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uTotMonth[2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uTotMonth[2]);

        if(!strncmp(currLang, "kr", 2))
        {
            if((uTotMonth[0]+uTotMonth[1]+uTotMonth[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ 0 ]건 입니다.");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ %d ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
                        uTotMonth[0]+uTotMonth[1]+uTotMonth[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        else
        {
            if((uTotMonth[0]+uTotMonth[1]+uTotMonth[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ 0 ].");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ %d ] (Warning: %s, Critical: %s, Block: %s).",
                        uTotMonth[0]+uTotMonth[1]+uTotMonth[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        nRes = fcom_GetReportFile(repFullPath, "pie2.js", repTmpBuf);
    }
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- pie-chart pieTotMonth(%s) ",pieTotMonth );

    nRes = fcom_ReportPieFilter(repTmpBuf, "varPieMonth", pieTotMonth,
            g_stProcReportInfo.szConfMailLang,
            g_stProcReportInfo.szConfMailClosedView);

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- pie-chart size(%d) ",nRes);
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]2. Make timeline-chart " );
    //해지된총건수
//    memset(sStrCloseMonth, 0x00, sizeof(sStrCloseMonth));
//    for(i=0; i<monthNum; i++)
//    {
//        sprintf(sStrCloseMonth+strlen(sStrCloseMonth), "%d,",
//                sMonthValue[i][0]+sMonthValue[i][1]+sMonthValue[i][2]);
//    }
    //미해지 경고 총건수
    memset(uStrWarnMonth, 0x00, sizeof(uStrWarnMonth));
    for(i=0; i<monthNum; i++)
    {
        sprintf(uStrWarnMonth+strlen(uStrWarnMonth), "%d,", uMonthValue[i][0]);
    }
    //미해지 위험 총건수
    memset(uStrCritMonth, 0x00, sizeof(uStrCritMonth));
    for(i=0; i<monthNum; i++)
    {
        sprintf(uStrCritMonth+strlen(uStrCritMonth), "%d,", uMonthValue[i][1]);
    }
    //미해지 차단 총건수
    memset(uStrBlokMonth, 0x00, sizeof(uStrBlokMonth));
    for(i=0; i<monthNum; i++)
    {
        sprintf(uStrBlokMonth+strlen(uStrBlokMonth), "%d,", uMonthValue[i][2]);
    }
    memset(sCategory, 0x00, sizeof(sCategory));
    for(i=0; i<monthNum; i++)
    {
        if(sqlMonth[i] != NULL)
        {
            memset(tmpType, 0x00, sizeof(tmpType));
            sprintf(tmpType, "'%s',", sqlMonth[i]);
            strcat(sCategory, tmpType);
        }
    }

    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- timeline-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- timeline-chart sStrCloseMonth(%s) ",sStrCloseMonth );
    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- timeline-chart uStrWarnMonth(%s) ",uStrWarnMonth );
    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- timeline-chart uStrCritMonth(%s) ",uStrCritMonth );
    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- timeline-chart uStrBlokMonth(%s) ",uStrBlokMonth );


    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic2.js", repTmpBuf);
    }

    nRes = fcom_ReportLineFilter(repTmpBuf, "varLineMonth",
                              sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth);

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- timeline-chart size(%d) ",nRes );
    strcat(resChart, repTmpBuf);
    //분석용랭킹구하기
    memset(topValue, 0x00, sizeof(topValue));
    rxt = freport_GetRankTimeBysq("month", topValue, "", "", refGroupSq, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTimeAnaly, "<li>데이터가 존재하지 않습니다.</li>");
        else
            strcpy(resTimeAnaly, "<li>Data does not exist.</li>");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTimeAnaly, "<li>이벤트 발생이 가장 많은 일자는 [ %s ] 입니다.</li>", topValue);
        else
            sprintf(resTimeAnaly, "<li>The most frequent occurrence of the event is [ %s ].</li>", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]3. Make rank-chart" );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseMonth, 0x00, sizeof(sStrCloseMonth));
    memset(uStrWarnMonth, 0x00, sizeof(uStrWarnMonth));
    memset(uStrCritMonth, 0x00, sizeof(uStrCritMonth));
    memset(uStrBlokMonth, 0x00, sizeof(uStrBlokMonth));
    //분석코멘트
    memset(topValue, 0x00, sizeof(topValue));

    rxt = freport_GetRankIp("month",
                      sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth,
                      topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resRankAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resRankAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP ID는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 사번은 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 이름은 [ %s ] 입니다.", topValue);
            else
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
        }
        else
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍThe ID with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍThe SNO with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍThe NAME with the most event occurrences is [ %s ].", topValue);
            else
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
        }
    }


    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart sStrCloseMonth(%s) ",sStrCloseMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart uStrWarnMonth(%s) ",uStrWarnMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart uStrCritMonth(%s) ",uStrCritMonth  );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart uStrBlokMonth(%s) ",uStrBlokMonth );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "rankChartMonth",
                             sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth);
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- rank-chart size(%d)",nRes );
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]4. Make type-chart " );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseMonth, 0x00, sizeof(sStrCloseMonth));
    memset(uStrWarnMonth, 0x00, sizeof(uStrWarnMonth));
    memset(uStrCritMonth, 0x00, sizeof(uStrCritMonth));
    memset(uStrBlokMonth, 0x00, sizeof(uStrBlokMonth));

    for(i=0; i<CUR_RULE_CNT; i++)
    {
        //모든 합이 0이 아니라면
        if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
        {
//            sMonthTypeTot = sMonthTypeValue[i][0]+sMonthTypeValue[i][1]+sMonthTypeValue[i][2]
//                            + uMonthTypeValue[i][0]+uMonthTypeValue[i][1]+uMonthTypeValue[i][2];
            sMonthTypeTot = uMonthTypeValue[i][0]+uMonthTypeValue[i][1]+uMonthTypeValue[i][2];
        }
        else
        {
            sMonthTypeTot = uMonthTypeValue[i][0]+uMonthTypeValue[i][1]+uMonthTypeValue[i][2];
        }

        if(sMonthTypeTot != 0)
        {
//            fcom_GetStrType2(i,sCategory+strlen(sCategory),fcom_GetCustLang());
            sprintf(sCategory+strlen(sCategory), "'%s',", fcom_getStrType(i));
//            sprintf(sStrCloseMonth+strlen(sStrCloseMonth), "%d,",
//                    sMonthTypeValue[i][0]+sMonthTypeValue[i][1]+sMonthTypeValue[i][2]);
            sprintf(uStrWarnMonth+strlen(uStrWarnMonth), "%d,", uMonthTypeValue[i][0]);
            sprintf(uStrCritMonth+strlen(uStrCritMonth), "%d,", uMonthTypeValue[i][1]);
            sprintf(uStrBlokMonth+strlen(uStrBlokMonth), "%d,", uMonthTypeValue[i][2]);
        }
    }

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart sStrCloseMonth(%s) ",sStrCloseMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart uStrWarnMonth(%s) ",uStrWarnMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart uStrCritMonth(%s) ",uStrCritMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart uStrBlokMonth(%s) ",uStrBlokMonth );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "typeChartMonth",
                             sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth);

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- type-chart size(%d) ",nRes );

    strcat(resChart, repTmpBuf);

    memset(topValue, 0x00, sizeof(topValue));
    rxt = freport_GetRankTypeBysq("month", topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTypeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTypeAnaly, "ㆍ Data does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTypeAnaly, "ㆍ 이벤트 발생이 가장 많은 항목은 [ %s ] 입니다.", topValue);
        else
            sprintf(resTypeAnaly, "ㆍ The most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]5. Make group-chart" );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseMonth, 0x00, sizeof(sStrCloseMonth));
    memset(uStrWarnMonth, 0x00, sizeof(uStrWarnMonth));
    memset(uStrCritMonth, 0x00, sizeof(uStrCritMonth));
    memset(uStrBlokMonth, 0x00, sizeof(uStrBlokMonth));
    memset(topValue, 0x00, sizeof(topValue));


    rxt = freport_GetRankGroupBysq("month",
                              sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth,
                              topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt < 1) //Group 미등록 1개밖에 없으면
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resGroupAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resGroupAnaly, "ㆍ Data does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resGroupAnaly, "ㆍ 이벤트 발생이 가장 많은 항목은 [ %s ] 입니다.", topValue);
        else
            sprintf(resGroupAnaly, "ㆍ The most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart sCategory(%s)  ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart sStrCloseMonth(%s) ",sStrCloseMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart uStrWarnMonth(%s) ",uStrWarnMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart uStrCritMonth(%s) ",uStrCritMonth );
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart uStrBlokMonth(%s) ",uStrBlokMonth );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "groupChartMonth",
                             sCategory, sStrCloseMonth, uStrWarnMonth, uStrCritMonth, uStrBlokMonth);
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- group-chart size(%d) ",nRes);

    strcat(resChart, repTmpBuf);
    *chartSize = strlen(resChart);
    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]- all chart tot size(%d) ",*chartSize );

    WRITE_INFO(CATEGORY_INFO,"[MONTHLY]6. Make table " );

//    nRes = freport_MakeTableMonth(resTable, sqlMonth, sMonthValue, uMonthValue, monthNum);
    nRes = freport_MakeTableMonth(resTable, sqlMonth, uMonthValue, monthNum);
    WRITE_INFO(CATEGORY_INFO, "[MONTHLY]- table size(%d) ", nRes );

    return strlen(resTable);
}


int freport_DeployReportWeeklyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart,		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly,	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// chart size
        char                param_cRealTimeFlag
)
{
    int		i;

    int		nRes;
    int		rxt;
    int		rowCnt;

    int		sqlType;
    int		sqlLevel;
//    int		sqlExist;
    char	sqlBuf[8192]    = {0x00,};
    char	sqlWeek[7][10+1] = { {0x00},{0x00}};
    char	sCategory[1024];
    char	tmpType[13+1];
    char	sqlRange[23+1];
    char	topValue[512];
    char	strFontWarn[50];
    char	strFontCrit[50];
    char	strFontBlok[50];

    char	refGroupSq[4096];

    //weekly count
    int		sWeekTypeTot = 0;

//    int     sTotWeek[3] = {0};
//    int     sWeekValue[7][3] = {{0},{0}};
//    int     sWeekTypeValue[CUR_RULE_CNT][3] = {{0},{0}};

    int     uTotWeek[3] = {0};
    int     uWeekValue[7][3] = {{0},{0}};
    int     uWeekTypeValue[CUR_RULE_CNT][3] = {{0},{0}};


    char    pieTotWeek[64];		//해지 총합계+미해지
    char    sStrCloseWeek[128]; //해지총 합계
    char    uStrWarnWeek[128];	//미해지 경고
    char    uStrCritWeek[128];	//미해지 위험
    char    uStrBlokWeek[128];  //미해지 차단

    char	repFullPath[256];
    char	repTmpBuf[2048];
    char	currLang[2+1];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        strcpy(currLang, fcom_GetCustLang());
    }
    else
    {
        strcpy(currLang, g_stProcReportInfo.szConfMailLang);
    }

    //1.weekly_total
    //정기주간보고서만 월화수목금토일(근무요일고려,week단위가아니라day를일주일로나열함것)
    //주간요청보고서는 일월화수목금토(달력한줄씩,week단위로합산)

    strcpy(sqlBuf, "select adddate(CURDATE(), - WEEKDAY(CURDATE()) -7) as week_1,"
               "adddate(CURDATE(), - WEEKDAY(CURDATE()) -6) as week_2,"
               "adddate(CURDATE(), - WEEKDAY(CURDATE()) -5) as week_3,"
               "adddate(CURDATE(), - WEEKDAY(CURDATE()) -4) as week_4,"
               "adddate(CURDATE(), - WEEKDAY(CURDATE()) -3) as week_5,"
               "adddate(CURDATE(), - WEEKDAY(CURDATE()) -2) as week_6,"
               "adddate(curdate(), - WEEKDAY(curdate()) -1) as week_7");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(sqlWeek[0], g_stMyCon->row[0]);
            strcpy(sqlWeek[1], g_stMyCon->row[1]);
            strcpy(sqlWeek[2], g_stMyCon->row[2]);
            strcpy(sqlWeek[3], g_stMyCon->row[3]);
            strcpy(sqlWeek[4], g_stMyCon->row[4]);
            strcpy(sqlWeek[5], g_stMyCon->row[5]);
            strcpy(sqlWeek[6], g_stMyCon->row[6]);
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    memset(sqlRange, 0x00, sizeof(sqlRange));
    strcat(sqlRange, sqlWeek[0]);
    strcat(sqlRange, " ~ ");
    strcat(sqlRange, sqlWeek[6]);
    strcpy(resRange, sqlRange);

    // 하위그룹 sq를 구한다.
    memset(refGroupSq, 0x00, sizeof(refGroupSq));
    if(grpSq != 0)
        fdb_GetSubGroupSq(grpSq, refGroupSq);

    if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
    {
        strcpy(sqlBuf, "select date_format(STDR_RECORD_TIME, '%Y-%m-%d'),"
                       "STDR_TYPE,STDR_LEVEL,STDR_EXIST,sum(STDR_COUNT) as CNT "
                       "from STD_EVENT_REALTIME_TB "
                       "where date_format(STDR_RECORD_TIME, '%Y-%m-%d') "
                       "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                       "and adddate(curdate(), - WEEKDAY(curdate()) -1) ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, 	"group by STDR_RECORD_TIME,STDR_TYPE,STDR_LEVEL,STDR_EXIST");

    }
    else //정기 보고서
    {
        strcpy(sqlBuf, "select date_format(STD_RECORD_TIME, '%Y-%m-%d'),"
                       "STD_TYPE,STD_LEVEL,STD_EXIST,sum(STD_COUNT) as CNT "
                       "from STD_EVENT_TB "
                       "where date_format(STD_RECORD_TIME, '%Y-%m-%d') "
                       "between ADDDATE(CURDATE(), - WEEKDAY(CURDATE()) -7) "
                       "and adddate(curdate(), - WEEKDAY(curdate()) -1) ");
        if(strlen(refGroupSq) > 0)
            sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", refGroupSq);
        strcat(sqlBuf, 	"group by STD_RECORD_TIME,STD_TYPE,STD_LEVEL,STD_EXIST");
    }



    WRITE_DEBUG(CATEGORY_DEBUG,"Report Weekly Exec Sql : [%s]", sqlBuf);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sqlType		= atoi(g_stMyCon->row[1]);
            sqlLevel	= atoi(g_stMyCon->row[2]);
//            sqlExist	= atoi(g_stMyCon->row[3]);
            for(i=0; i<7; i++)
            {
                if (!strncmp(sqlWeek[i], g_stMyCon->row[0], 10))
                {
//                    if(sqlExist == 0) //해지
//                    {
//                        if		(sqlLevel == WARNING) 	sWeekValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sWeekValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sWeekValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else //미해지
//                    {
                        if		(sqlLevel == WARNING) 	uWeekValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uWeekValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uWeekValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            }//for
            //항목별 정리
            for(i=0; i<CUR_RULE_CNT; i++)
            {
                if(sqlType == i)
                {
//                    if(sqlExist == 0)
//                    {
//                        //해지
//                        if      (sqlLevel == WARNING)	sWeekTypeValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sWeekTypeValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sWeekTypeValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else
//                    {
                        if		(sqlLevel == WARNING) 	uWeekTypeValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uWeekTypeValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uWeekTypeValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            } //for
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    //tot_day
    for(i=0; i<7; i++)
    {
//        sTotWeek[0] += sWeekValue[i][0];
//        sTotWeek[1] += sWeekValue[i][1];
//        sTotWeek[2] += sWeekValue[i][2];
        uTotWeek[0] += uWeekValue[i][0];
        uTotWeek[1] += uWeekValue[i][1];
        uTotWeek[2] += uWeekValue[i][2];
    }

    //tot_day html data
    memset(repFullPath, 0x00, sizeof(repFullPath));


    sprintf(repFullPath,"%s/%s", p_repPath, "template");

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]1. Make pie-chart" );

    memset(pieTotWeek, 0x00, sizeof(pieTotWeek));
    memset(strFontWarn, 0x00, sizeof(strFontWarn));
    memset(strFontCrit, 0x00, sizeof(strFontCrit));
    memset(strFontBlok, 0x00, sizeof(strFontBlok));
    memset(repTmpBuf,0x00, sizeof(repTmpBuf));

    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes"))
    {
//        sprintf(pieTotWeek, "%d,%d,%d,%d,", sTotWeek[0]+sTotWeek[1]+sTotWeek[2],uTotWeek[0],uTotWeek[1],uTotWeek[2]);
        sprintf(pieTotWeek, "%d,%d,%d,", uTotWeek[0],uTotWeek[1],uTotWeek[2]);

//        if(sTotWeek[0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sTotWeek[0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", sTotWeek[0]);
//        if(sTotWeek[1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sTotWeek[1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sTotWeek[1]);
//        if(sTotWeek[2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sTotWeek[2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sTotWeek[2]);
//
//        if(!strncmp(currLang, "kr", 2))
//        {
//            if((sTotWeek[0]+sTotWeek[1]+sTotWeek[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">0</font> ]건 입니다.");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">%d</font> ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
//                        sTotWeek[0]+sTotWeek[1]+sTotWeek[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
//        else
//        {
//            if((sTotWeek[0]+sTotWeek[1]+sTotWeek[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">0</font> ].");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">%d</font> ] (Warning: %s, Critical: %s, Block: %s).",
//                        sTotWeek[0]+sTotWeek[1]+sTotWeek[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
        nRes = fcom_GetReportFile(repFullPath, "pie.js", repTmpBuf);
    }
    else
    {
        sprintf(pieTotWeek, "%d,%d,%d,", uTotWeek[0],uTotWeek[1],uTotWeek[2]);
        if(uTotWeek[0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uTotWeek[0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uTotWeek[0]);
        if(uTotWeek[1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uTotWeek[1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uTotWeek[1]);
        if(uTotWeek[2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uTotWeek[2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uTotWeek[2]);

        if(!strncmp(currLang, "kr", 2))
        {
            if((uTotWeek[0]+uTotWeek[1]+uTotWeek[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ 0 ]건 입니다.");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ %d ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
                        uTotWeek[0]+uTotWeek[1]+uTotWeek[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        else
        {
            if((uTotWeek[0]+uTotWeek[1]+uTotWeek[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ 0 ].");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ %d ] (Warning: %s, Critical: %s, Block: %s).",
                        uTotWeek[0]+uTotWeek[1]+uTotWeek[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        nRes = fcom_GetReportFile(repFullPath, "pie2.js", repTmpBuf);
    }
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- pie-chart pieTotWeek(%s) ",pieTotWeek );

    nRes = fcom_ReportPieFilter(repTmpBuf, "varPieWeek", pieTotWeek,
            g_stProcReportInfo.szConfMailLang,
            g_stProcReportInfo.szConfMailClosedView);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- pie-chart size(%d) ",nRes );
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]2. Make timeline-chart" );

    //해지된총건수
//    memset(sStrCloseWeek, 0x00, sizeof(sStrCloseWeek));
//    for(i=0; i<7; i++)
//    {
//        sprintf(sStrCloseWeek+strlen(sStrCloseWeek), "%d,",
//                sWeekValue[i][0]+sWeekValue[i][1]+sWeekValue[i][2]);
//    }
    //미해지 경고 총건수
    memset(uStrWarnWeek, 0x00, sizeof(uStrWarnWeek));
    for(i=0; i<7; i++)
    {
        sprintf(uStrWarnWeek+strlen(uStrWarnWeek), "%d,", uWeekValue[i][0]);
    }
    //미해지 위험 총건수
    memset(uStrCritWeek, 0x00, sizeof(uStrCritWeek));
    for(i=0; i<7; i++)
    {
        sprintf(uStrCritWeek+strlen(uStrCritWeek), "%d,", uWeekValue[i][1]);
    }
    //미해지 차단 총건수
    memset(uStrBlokWeek, 0x00, sizeof(uStrBlokWeek));
    for(i=0; i<7; i++)
    {
        sprintf(uStrBlokWeek+strlen(uStrBlokWeek), "%d,", uWeekValue[i][2]);
    }

    memset(sCategory, 0x00, sizeof(sCategory));
    for(i=0; i<7; i++)
    {
        memset(tmpType, 0x00, sizeof(tmpType));
        sprintf(tmpType, "'%s',", sqlWeek[i]);
        strcat(sCategory, tmpType);
    }

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart sStrCloseWeek(%s) ",sStrCloseWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart uStrWarnWeek(%s) ",uStrWarnWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart uStrCritWeek(%s) ",uStrCritWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart uStrBlokWeek(%s) ",uStrBlokWeek );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic2.js", repTmpBuf);
    }

    nRes = fcom_ReportLineFilter(repTmpBuf, "varLineWeek",
                              sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- timeline-chart size(%d)",nRes );
    strcat(resChart, repTmpBuf);
    //분석용랭킹구하기

    memset(topValue, 0x00, sizeof(topValue));

    rxt = freport_GetRankTimeBysq("week", topValue, "", "", refGroupSq, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTimeAnaly, "<li>데이터가 존재하지 않습니다.</li>");
        else
            strcpy(resTimeAnaly, "<li>Data does not exist.</li>");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTimeAnaly, "<li>이벤트 발생이 가장 많은 일자는 [ %s ] 입니다.</li>", topValue);
        else
            sprintf(resTimeAnaly, "<li>The most frequent occurrence of the event is [ %s ].</li>", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]3. Make rank-chart" );


    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseWeek, 0x00, sizeof(sStrCloseWeek));
    memset(uStrWarnWeek, 0x00, sizeof(uStrWarnWeek));
    memset(uStrCritWeek, 0x00, sizeof(uStrCritWeek));
    memset(uStrBlokWeek, 0x00, sizeof(uStrBlokWeek));

    //분석용코멘트
    memset(topValue, 0x00, sizeof(topValue));

    rxt = freport_GetRankIp("week",
                      sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek,
                      topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resRankAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resRankAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP ID는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 사번은 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 이름은 [ %s ] 입니다.", topValue);
            else
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
        }
        else
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍThe ID with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍThe SNO with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍThe NAME with the most event occurrences is [ %s ].", topValue);
            else
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
        }
    }

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart sCategory(%s) ", sCategory );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart sStrCloseWeek(%s) ", sStrCloseWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart uStrWarnWeek(%s) ", uStrWarnWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart uStrCritWeek(%s) ", uStrCritWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart uStrBlokWeek(%s) ", uStrBlokWeek );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView,"yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }
    nRes = fcom_ReportBarFilter(repTmpBuf, "rankChartWeek", sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- rank-chart size(%d) ",nRes );
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO, "[WEEKLY]4. Make type-chart" );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseWeek, 0x00, sizeof(sStrCloseWeek));
    memset(uStrWarnWeek, 0x00, sizeof(uStrWarnWeek));
    memset(uStrCritWeek, 0x00, sizeof(uStrCritWeek));
    memset(uStrBlokWeek, 0x00, sizeof(uStrBlokWeek));

    for(i=0; i<CUR_RULE_CNT; i++)
    {
        //모든 합이 0이 아니라면
        if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
        {
//            sWeekTypeTot = sWeekTypeValue[i][0]+sWeekTypeValue[i][1]+sWeekTypeValue[i][2]
//                           + uWeekTypeValue[i][0]+uWeekTypeValue[i][1]+uWeekTypeValue[i][2];
            sWeekTypeTot = uWeekTypeValue[i][0]+uWeekTypeValue[i][1]+uWeekTypeValue[i][2];
        }
        else
        {
            sWeekTypeTot = uWeekTypeValue[i][0]+uWeekTypeValue[i][1]+uWeekTypeValue[i][2];
        }

        if(sWeekTypeTot != 0)
        {
//            fcom_GetStrType2(i, sCategory+strlen(sCategory),fcom_GetCustLang());
            sprintf(sCategory+strlen(sCategory), "'%s',", fcom_getStrType(i));
//            sprintf(sStrCloseWeek+strlen(sStrCloseWeek), "%d,",
//                    sWeekTypeValue[i][0]+sWeekTypeValue[i][1]+sWeekTypeValue[i][2]);
            sprintf(uStrWarnWeek+strlen(uStrWarnWeek), "%d,", uWeekTypeValue[i][0]);
            sprintf(uStrCritWeek+strlen(uStrCritWeek), "%d,", uWeekTypeValue[i][1]);
            sprintf(uStrBlokWeek+strlen(uStrBlokWeek), "%d,", uWeekTypeValue[i][2]);
        }
    }

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart sStrCloseWeek(%s) ",sStrCloseWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart uStrWarnWeek(%s) ",uStrWarnWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart uStrCritWeek(%s) ",uStrCritWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart uStrBlokWeek(%s) ",uStrBlokWeek );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }
    nRes = fcom_ReportBarFilter(repTmpBuf, "typeChartWeek", sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- type-chart size(%d) ",nRes);
    strcat(resChart, repTmpBuf);
    memset(topValue, 0x00, sizeof(topValue));
    rxt = freport_GetRankTypeBysq("week", topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTypeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTypeAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTypeAnaly, "ㆍ 이벤트 발생이 가장 많은 항목은 [ %s ] 입니다.", topValue);
        else
            sprintf(resTypeAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO, "[WEEKLY]5. Make group-chart" );

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseWeek, 0x00, sizeof(sStrCloseWeek));
    memset(uStrWarnWeek, 0x00, sizeof(uStrWarnWeek));
    memset(uStrCritWeek, 0x00, sizeof(uStrCritWeek));
    memset(uStrBlokWeek, 0x00, sizeof(uStrBlokWeek));
    memset(topValue, 0x00, sizeof(topValue));


    rxt = freport_GetRankGroupBysq("week",
                              sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek,
                              topValue, "", "", refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt < 1)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resGroupAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resGroupAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resGroupAnaly, "ㆍ 이벤트 발생이 가장 많은 그룹은 [ %s ] 입니다.", topValue);
        else
            sprintf(resGroupAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart sStrCloseWeek(%s) ",sStrCloseWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart uStrWarnWeek(%s) ",uStrWarnWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart uStrCritWeek(%s) ",uStrCritWeek );
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart uStrBlokWeek(%s) ",uStrBlokWeek );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "groupChartWeek",
                             sCategory, sStrCloseWeek, uStrWarnWeek, uStrCritWeek, uStrBlokWeek);

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- group-chart size(%d) ",nRes );

    strcat(resChart, repTmpBuf);
    *chartSize = strlen(resChart);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- all chart tot size(%d) ",*chartSize );

    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]6. Make table " );

    nRes = freport_MakeTableWeek(resTable, sqlWeek,  uWeekValue);
    WRITE_INFO(CATEGORY_INFO,"[WEEKLY]- table size(%d) ",
            nRes);

    return strlen(resTable);
}

int freport_DeployReportCustomBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        int*				chartSize,		// chart size
        char*				beginDate, 		// begin date
        char*				endDate,		// end date
        unsigned long long	grpSq,			// group sequence
        int					isCust,         // custom status
        char                param_cRealTimeFlag)
{
    int		i;
    int		idx;
    int		nRes;
    int		rxt;
    int		rowCnt;
    int		loop;
    int		sqlHour;
    int		sqlType;
    int		sqlLevel;
//    int		sqlExist;
    int		sqlCustNum = 0;

    int		bDayTime = 0;

    char	sqlCust[MAX_CUSTOM_ARRAY][13+1] = { {0x00},{0x00}};
    char	sqlBuf[8192]    = {0x00,};
    char	tmpType[13+1]   = {0x00,};
    char	sCategory[1024] = {0x00,};
    char	topValue[512]   = {0x00,};
    char	strFontWarn[50] = {0x00,};
    char	strFontCrit[50] = {0x00,};
    char	strFontBlok[50] = {0x00,};
    //unsigned long long	arrGroupSq[MAX_GROUP_CNT];
    char	refGroupSq[4096] = {0x00,};

    //monthly count
    int		sCustTypeTot = 0;

//    int     sTotCust[3] = {0};
//    int     sCustValue[MAX_CUSTOM_ARRAY][3] = {{0,0},{0,0}};
//    int     sCustTypeValue[CUR_RULE_CNT][3] = {{0,0},{0,0}};

    int     uTotCust[3] = {0};
    int     uCustValue[MAX_CUSTOM_ARRAY][3] = {{0,0},{0,0}};
    int     uCustTypeValue[CUR_RULE_CNT][3] = {{0,0},{0,0}};


    //monthly html data
    char    pieTotCust[64] = {0x00,};	//해지총합계+미해지
    char	sStrCloseCust[256] = {0x00,};
    char    uStrWarnCust[256] = {0x00,};
    char    uStrCritCust[256] = {0x00,};
    char    uStrBlokCust[256] = {0x00,};

    char	repFullPath[256] = {0x00,};
    char	repTmpBuf[4096] = {0x00,};
    char	currLang[2+1] = {0x00,};

    if(strlen(fcom_GetCustLang()) > 0)
    {
        strcpy(currLang, fcom_GetCustLang());
    }
    else
    {
        strcpy(currLang, g_stProcReportInfo.szConfMailLang);
    }


    //sqlCust 초기화
    for(i=0; i<MAX_CUSTOM_ARRAY; i++)
    {
        memset(sqlCust[i], 0x00, sizeof(sqlCust[i]));
    }

    // 하위그룹 sq를 구한다.
    memset(refGroupSq, 0x00, sizeof(refGroupSq));
    if(grpSq != 0)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        rowCnt = fdb_GetSubGroupSq(grpSq, refGroupSq);
        if(rowCnt <= 0) //최하위 단일그룹1개만
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf(sqlBuf, "select UG_SQ from USER_GROUP_TB where UG_SQ in ("
                            "	select UG_SQ_C from USER_GROUP_LINK_TB where UG_SQ_C=%llu"
                            ")", grpSq);

            rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
            }
            memset(refGroupSq, 0x00, sizeof(refGroupSq));
            if(rowCnt > 0)
            {
                loop = 0;
                while(fdb_SqlFetchRow(g_stMyCon) == 0)
                {
                    if(g_stMyCon->row[0] != NULL)
                    {
                        if(loop == 0)
                            sprintf(refGroupSq, "'%llu'", atoll(g_stMyCon->row[0]));
                        else
                            sprintf(refGroupSq+strlen(refGroupSq), ",'%llu'", atoll(g_stMyCon->row[0]));
                    }
                    loop++;
                }
                //sqlGroupNum = rowCnt;
            }
            fdb_SqlFreeResult(g_stMyCon);
        }
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    if(isCust == 1 && !strcmp(beginDate, endDate))
    {
        bDayTime = 1;
        for(i=0; i<24; i++) //24시간
        {
            if(i < 10)
            {
                sprintf(sqlCust[i], "0%d", i);
            }
            else
            {
                sprintf(sqlCust[i], "%d", i);
            }
            sqlCustNum++;
        }
    }
    else if(isCust == 2)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "select CONCAT( date_format(a.Date, '%%Y-%%m'),'-',"
                        "floor( (date_format(a.Date,'%%d') + "
                        "    (date_format(date_format(a.Date,'%%Y%%m%%01'),'%%w')-1))/7)+1, 'W' "
                        ") as WEEK from ( "
                        "select curdate() - INTERVAL (a.a + (10 * b.a) + (100 * c.a)) DAY as Date "
                        "from (select 0 as a union all select 1 union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as a "
                        "cross join ( "
                        "select 0 as a "
                        "union all select 1 "
                        "union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as b "
                        "cross join ( "
                        "select 0 as a "
                        "union all select 1 "
                        "union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as c "
                        ") a where CONCAT( date_format(a.Date,'%%Y-%%m'),'-',"
                        "floor(  (date_format(a.Date,'%%d') + "
                        "(date_format(date_format(a.Date,'%%Y%%m%%01'),'%%w')-1))/7)+1, 'W' "
                        ") "
                        "between '%s' and '%s' group by WEEK order by Date asc",
                beginDate,endDate);

        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
        }

        if(rowCnt > 0)
        {
            idx = 0;
            while(fdb_SqlFetchRow(g_stMyCon) == 0)
            {
                strcpy(sqlCust[idx], g_stMyCon->row[0]);
                idx++;
            }
        }
        sqlCustNum = rowCnt;
        fdb_SqlFreeResult(g_stMyCon);
    }
    else if(isCust == 3)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "select date_format(a.Date, '%%Y-%%m') as MONTH "
                        "from ( "
                        "select curdate() - INTERVAL (a.a + (10 * b.a) + (100 * c.a)) DAY as Date "
                        "from (select 0 as a union all select 1 union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as a "
                        "cross join ( "
                        "select 0 as a "
                        "union all select 1 "
                        "union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as b "
                        "cross join ( "
                        "select 0 as a "
                        "union all select 1 "
                        "union all select 2 "
                        "union all select 3 "
                        "union all select 4 "
                        "union all select 5 "
                        "union all select 6 "
                        "union all select 7 "
                        "union all select 8 "
                        "union all select 9) as c "
                        ") a where date_format(a.Date,'%%Y-%%m') "
                        "between '%s' and '%s' group by MONTH order by Date asc",
                beginDate,endDate);

        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
        }

        if(rowCnt > 0)
        {
            idx = 0;
            while(fdb_SqlFetchRow(g_stMyCon) == 0)
            {
                strcpy(sqlCust[idx], g_stMyCon->row[0]);
                idx++;
            }
        }
        sqlCustNum = rowCnt;
        fdb_SqlFreeResult(g_stMyCon);
    }
    else  //3개월미만 주간
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "select a.Date from ("
                        "select curdate() - INTERVAL (a.a + (10 * b.a) + (100 * c.a)) DAY as Date "
                        "from (select 0 as a union all select 1 union all select 2  "
                        "	union all select 3 "
                        "	union all select 4 "
                        "	union all select 5 "
                        "	union all select 6 "
                        "	union all select 7 "
                        "	union all select 8 "
                        "	union all select 9) as a "
                        "	cross join ( "
                        "		select 0 as a "
                        "		union all select 1 "
                        "		union all select 2 "
                        "		union all select 3 "
                        "		union all select 4 "
                        "		union all select 5 "
                        "		union all select 6 "
                        "		union all select 7 "
                        "		union all select 8 "
                        "		union all select 9) as b "
                        "	cross join ( "
                        "		select 0 as a "
                        "		union all select 1 "
                        "		union all select 2 "
                        "		union all select 3 "
                        "		union all select 4 "
                        "		union all select 5 "
                        "		union all select 6 "
                        "		union all select 7 "
                        "		union all select 8 "
                        "		union all select 9) as c "
                        ") a where date_format(a.Date, '%%Y-%%m-%%d') "
                        "between '%s' and '%s' order by Date asc", beginDate,endDate);

        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
        }

        if(rowCnt > 0)
        {
            idx = 0;
            while(fdb_SqlFetchRow(g_stMyCon) == 0)
            {
                strcpy(sqlCust[idx], g_stMyCon->row[0]);
                idx++;
            }
        }
        sqlCustNum = rowCnt;
        fdb_SqlFreeResult(g_stMyCon);
    }


    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    if ( param_cRealTimeFlag != 0x00 ) // 실시간 보고서
    {
        if(isCust == 1) //day
        {
            if(bDayTime == 1)
            {
                strcpy(resRange, beginDate);
                sprintf(sqlBuf, "select date_format(STDR_RECORD_TIME, '%%H'),"
                                "STDR_TYPE,STDR_LEVEL,sum(STDR_COUNT) as CNT "
                                "from STD_EVENT_REALTIME_TB "
                                "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", beginDate);
                if(strlen(refGroupSq) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", refGroupSq);
                strcat(sqlBuf,	"group by date_format(STDR_RECORD_TIME, '%H'),"
                                  "STDR_TYPE,STDR_LEVEL,STDR_EXIST");
            }
            else //3개월이하 daily
            {
                sprintf(resRange, "%s ~ %s", beginDate,endDate);
                sprintf(sqlBuf, "select date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d'),"
                                "STDR_TYPE,STDR_LEVEL,sum(STDR_COUNT) as CNT "
                                "from STD_EVENT_REALTIME_TB "
                                "where date_format(STDR_RECORD_TIME, '%%Y-%%m-%%d') "
                                "between '%s' and '%s' ", beginDate,endDate);
                if(strlen(refGroupSq) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STDR_UG_SQ in (%s) ", refGroupSq);
                strcat(sqlBuf,	"group by date_format(STDR_RECORD_TIME, '%Y-%m-%d'),"
                                  "STDR_TYPE,STDR_LEVEL,STDR_EXIST");
            }
        }
        else if(isCust == 2) //3개월이상 week
        {
            sprintf(resRange, "%s ~ %s", beginDate,endDate);
            sprintf(sqlBuf, "select STWR_RECORD_TIME as TIME,"
                            "STWR_TYPE,STWR_LEVEL,sum(STWR_COUNT) as CNT "
                            "from STW_EVENT_REALTIME_TB "
                            "where STWR_RECORD_TIME between '%s' and '%s' ", beginDate,endDate);
            if(strlen(refGroupSq) > 0)
                sprintf(sqlBuf+strlen(sqlBuf), "and STWR_UG_SQ in (%s) ", refGroupSq);
            strcat(sqlBuf, "group by TIME,STWR_TYPE,STWR_LEVEL,STWR_EXIST");
        }
        else //3개월이상 month
        {
            sprintf(resRange, "%s ~ %s", beginDate,endDate);
            sprintf(sqlBuf, "select STMR_RECORD_TIME,STMR_TYPE,STMR_LEVEL,sum(STMR_COUNT) as CNT "
                            "from STM_EVENT_REALTIME_TB "
                            "where STMR_RECORD_TIME between '%s' and '%s' ", beginDate,endDate);
            if(strlen(refGroupSq) > 0)
                sprintf(sqlBuf+strlen(sqlBuf), "and STMR_UG_SQ in (%s) ", refGroupSq);
            strcat(sqlBuf, "group by STMR_RECORD_TIME,STMR_TYPE,STMR_LEVEL,STMR_EXIST");
        }
    }
    else //정기 보고서
    {
        if(isCust == 1) //day
        {
            if(bDayTime == 1)
            {
                strcpy(resRange, beginDate);
                sprintf(sqlBuf, "select date_format(STD_RECORD_TIME, '%%H'),"
                                //                            "STD_TYPE,STD_LEVEL,STD_EXIST,count(*) as CNT "
                                "STD_TYPE,STD_LEVEL,sum(STD_COUNT) as CNT "
                                "from STD_EVENT_TB "
                                "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') = '%s' ", beginDate);
                if(strlen(refGroupSq) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", refGroupSq);
                strcat(sqlBuf,	"group by date_format(STD_RECORD_TIME, '%H'),"
                                  "STD_TYPE,STD_LEVEL,STD_EXIST");
            }
            else //3개월이하 daily
            {
                sprintf(resRange, "%s ~ %s", beginDate,endDate);
                sprintf(sqlBuf, "select date_format(STD_RECORD_TIME, '%%Y-%%m-%%d'),"
                                //                            "STD_TYPE,STD_LEVEL,STD_EXIST,count(*) as CNT "
                                "STD_TYPE,STD_LEVEL,sum(STD_COUNT) as CNT "
                                "from STD_EVENT_TB "
                                "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                                "between '%s' and '%s' ", beginDate,endDate);
                if(strlen(refGroupSq) > 0)
                    sprintf(sqlBuf+strlen(sqlBuf), "and STD_UG_SQ in (%s) ", refGroupSq);
                strcat(sqlBuf,	"group by date_format(STD_RECORD_TIME, '%Y-%m-%d'),"
                                  "STD_TYPE,STD_LEVEL,STD_EXIST");
            }
        }
        else if(isCust == 2) //3개월이상 week
        {
            sprintf(resRange, "%s ~ %s", beginDate,endDate);
            sprintf(sqlBuf, "select STW_RECORD_TIME as TIME,"
                            //                        "STW_TYPE,STW_LEVEL,STW_EXIST,count(*) as CNT "
                            "STW_TYPE,STW_LEVEL,sum(STW_COUNT) as CNT "
                            "from STW_EVENT_TB "
                            "where STW_RECORD_TIME between '%s' and '%s' ", beginDate,endDate);
            if(strlen(refGroupSq) > 0)
                sprintf(sqlBuf+strlen(sqlBuf), "and STW_UG_SQ in (%s) ", refGroupSq);
            strcat(sqlBuf, "group by TIME,STW_TYPE,STW_LEVEL,STW_EXIST");
        }
        else //3개월이상 month
        {
            sprintf(resRange, "%s ~ %s", beginDate,endDate);
//        sprintf(sqlBuf, "select STM_RECORD_TIME,STM_TYPE,STM_LEVEL,STM_EXIST,count(*) as CNT "
            sprintf(sqlBuf, "select STM_RECORD_TIME,STM_TYPE,STM_LEVEL,sum(STM_COUNT) as CNT "
                            "from STM_EVENT_TB "
                            "where STM_RECORD_TIME between '%s' and '%s' ", beginDate,endDate);
            if(strlen(refGroupSq) > 0)
                sprintf(sqlBuf+strlen(sqlBuf), "and STM_UG_SQ in (%s) ", refGroupSq);
            strcat(sqlBuf, "group by STM_RECORD_TIME,STM_TYPE,STM_LEVEL,STM_EXIST");
        }

    }

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(bDayTime == 1)
            {
                sqlHour		= atoi(g_stMyCon->row[0]);
            }
            sqlType		= atoi(g_stMyCon->row[1]);
            sqlLevel	= atoi(g_stMyCon->row[2]);
//            sqlExist	= atoi(g_stMyCon->row[3]);
            for(i=0; i<sqlCustNum; i++)
            {
                if(bDayTime == 1)
                {
                    if(sqlHour == i)
                    {
//                        if(sqlExist == 0) //해지
//                        {
//                            if		(sqlLevel == WARNING) 	sCustValue[i][0] += atoi(g_stMyCon->row[4]);
//                            else if	(sqlLevel == CRITICAL) 	sCustValue[i][1] += atoi(g_stMyCon->row[4]);
//                            else if	(sqlLevel == BLOCK) 	sCustValue[i][2] += atoi(g_stMyCon->row[4]);
//                        }
//                        else  //미해지
//                        {
                            if		(sqlLevel == WARNING) 	uCustValue[i][0] += atoi(g_stMyCon->row[3]);
                            else if	(sqlLevel == CRITICAL) 	uCustValue[i][1] += atoi(g_stMyCon->row[3]);
                            else if	(sqlLevel == BLOCK) 	uCustValue[i][2] += atoi(g_stMyCon->row[3]);
//                        }
                    }
                }
                else
                {
                    if(!strcmp(sqlCust[i], g_stMyCon->row[0]))
                    {
//                        if(sqlExist == 0) //해지
//                        {
//                            if		(sqlLevel == WARNING) 	sCustValue[i][0] += atoi(g_stMyCon->row[4]);
//                            else if	(sqlLevel == CRITICAL) 	sCustValue[i][1] += atoi(g_stMyCon->row[4]);
//                            else if	(sqlLevel == BLOCK) 	sCustValue[i][2] += atoi(g_stMyCon->row[4]);
//                        }
//                        else //미해지
//                        {
                            if		(sqlLevel == WARNING) 	uCustValue[i][0] += atoi(g_stMyCon->row[3]);
                            else if	(sqlLevel == CRITICAL) 	uCustValue[i][1] += atoi(g_stMyCon->row[3]);
                            else if	(sqlLevel == BLOCK) 	uCustValue[i][2] += atoi(g_stMyCon->row[3]);
//                        }
                    }
                }
            }//for
            //항목별 정리
            for(i=0; i<CUR_RULE_CNT; i++)
            {
                if(sqlType == i)
                {
//                    if(sqlExist == 0) //해지
//                    {
//                        if      (sqlLevel == WARNING) 	sCustTypeValue[i][0] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == CRITICAL) 	sCustTypeValue[i][1] += atoi(g_stMyCon->row[4]);
//                        else if	(sqlLevel == BLOCK) 	sCustTypeValue[i][2] += atoi(g_stMyCon->row[4]);
//                    }
//                    else
//                    {
                        if		(sqlLevel == WARNING) 	uCustTypeValue[i][0] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == CRITICAL) 	uCustTypeValue[i][1] += atoi(g_stMyCon->row[3]);
                        else if	(sqlLevel == BLOCK) 	uCustTypeValue[i][2] += atoi(g_stMyCon->row[3]);
//                    }
                }
            } //for
        } //while
    } //if

    fdb_SqlFreeResult(g_stMyCon);

    //tot_day
    for(i=0; i<sqlCustNum; i++)
    {
//        sTotCust[0] += sCustValue[i][0];
//        sTotCust[1] += sCustValue[i][1];
//        sTotCust[2] += sCustValue[i][2];
        uTotCust[0] += uCustValue[i][0];
        uTotCust[1] += uCustValue[i][1];
        uTotCust[2] += uCustValue[i][2];
    }

    //tot_day html data
    memset(repFullPath, 0x00, sizeof(repFullPath));
    sprintf(repFullPath,"%s/%s", p_repPath, "template");

    /*
     *	1. Make pie-chart
     */
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]1. Make pie-chart ");


    memset(pieTotCust ,0x00, sizeof(pieTotCust));
    memset(strFontWarn,0x00, sizeof(strFontWarn));
    memset(strFontCrit,0x00, sizeof(strFontCrit));
    memset(strFontBlok,0x00, sizeof(strFontBlok));
    memset(repTmpBuf  ,0x00, sizeof(repTmpBuf));

    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") != 0 )
    {
//        sprintf(pieTotCust, "%d,%d,%d,%d,", sTotCust[0]+sTotCust[1]+sTotCust[2],uTotCust[0],uTotCust[1],uTotCust[2]);
        sprintf(pieTotCust, "%d,%d,%d,", uTotCust[0],uTotCust[1],uTotCust[2]);

//        if(sTotCust[0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sTotCust[0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", sTotCust[0]);
//        if(sTotCust[1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sTotCust[1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sTotCust[1]);
//        if(sTotCust[2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sTotCust[2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sTotCust[2]);

//        if(!strncmp(currLang, "kr", 2))
//        {
//            if((sTotCust[0]+sTotCust[1]+sTotCust[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">0</font> ]건 입니다.");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ 해지(<font color=\"#00c3c0\">Close</font>)된 건수는 총 [ <font color=\"#00c3c0\">%d</font> ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
//                        sTotCust[0]+sTotCust[1]+sTotCust[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
//        else
//        {
//            if((sTotCust[0]+sTotCust[1]+sTotCust[2]) == 0)
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">0</font> ].");
//            }
//            else
//            {
//                sprintf(resPieAnaly, "ㆍ The (<font color=\"#00c3c0\">Close</font>) count is [ <font color=\"#00c3c0\">%d</font> ] (Warning: %s, Critical: %s, Block: %s).",
//                        sTotCust[0]+sTotCust[1]+sTotCust[2],strFontWarn,strFontCrit,strFontBlok);
//            }
//        }
        nRes = fcom_GetReportFile(repFullPath, "pie.js", repTmpBuf);
    }
    else
    {
        sprintf(pieTotCust, "%d,%d,%d,", uTotCust[0],uTotCust[1],uTotCust[2]);
        if(uTotCust[0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uTotCust[0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uTotCust[0]);
        if(uTotCust[1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uTotCust[1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uTotCust[1]);
        if(uTotCust[2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uTotCust[2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uTotCust[2]);

        if(!strncmp(currLang, "kr", 2))
        {
            if((uTotCust[0]+uTotCust[1]+uTotCust[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ 0 ]건 입니다.");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ 이벤트 발생 건수는 총 [ %d ]건 (경고: %s건, 위험: %s건, 차단: %s건) 입니다.",
                        uTotCust[0]+uTotCust[1]+uTotCust[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        else
        {
            if((uTotCust[0]+uTotCust[1]+uTotCust[2]) == 0)
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ 0 ].");
            }
            else
            {
                sprintf(resPieAnaly, "ㆍ The event count is [ %d ] (Warning: %s, Critical: %s, Block: %s).",
                        uTotCust[0]+uTotCust[1]+uTotCust[2],strFontWarn,strFontCrit,strFontBlok);
            }
        }
        nRes = fcom_GetReportFile(repFullPath, "pie2.js", repTmpBuf);
    }
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- pie-chart pieTotCust(%s) ",pieTotCust );

    nRes = fcom_ReportPieFilter(repTmpBuf, "varPieCust", pieTotCust,
            g_stProcReportInfo.szConfMailLang,
            g_stProcReportInfo.szConfMailClosedView);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- pie-chart size(%d) ", nRes );
    strcat(resChart, repTmpBuf);

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]2. Make timeline-chart ");

    memset(sStrCloseCust, 0x00, sizeof(sStrCloseCust));
    //해지된총건수
//    for(i=0; i<sqlCustNum; i++)
//    {
//        sprintf(sStrCloseCust+strlen(sStrCloseCust), "%d,",
//                sCustValue[i][0]+sCustValue[i][1]+sCustValue[i][2]);
//    }
    //미해지 경고 총건수
    memset(uStrWarnCust, 0x00, sizeof(uStrWarnCust));
    for(i=0; i<sqlCustNum; i++)
    {
        sprintf(uStrWarnCust+strlen(uStrWarnCust), "%d,", uCustValue[i][0]);
    }
    //미해지 위험 총건수
    memset(uStrCritCust, 0x00, sizeof(uStrCritCust));
    for(i=0; i<sqlCustNum; i++)
    {
        sprintf(uStrCritCust+strlen(uStrCritCust), "%d,", uCustValue[i][1]);
    }
    //미해지 차단 총건수
    memset(uStrBlokCust, 0x00, sizeof(uStrBlokCust));
    for(i=0; i<sqlCustNum; i++)
    {
        sprintf(uStrBlokCust+strlen(uStrBlokCust), "%d,", uCustValue[i][2]);
    }

    //sCategory 설정
    memset(sCategory, 0x00, sizeof(sCategory));
    for(i=0; i<sqlCustNum; i++)
    {
        if(sqlCust[i] != NULL && strlen(sqlCust[i]) > 0)
        {
            memset(tmpType, 0x00, sizeof(tmpType));
            sprintf(tmpType, "'%s',", sqlCust[i]);
            strcat(sCategory, tmpType);
        }
    }

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart sCategory(%s)    ",    sCategory);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart sStrCloseCust(%s)",sStrCloseCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart uStrWarnCust(%s) ", uStrWarnCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart uStrCritCust(%s) ", uStrCritCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart uStrBlokCust(%s) ", uStrBlokCust);

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "line_basic2.js", repTmpBuf);
    }
    nRes = fcom_ReportLineFilter(repTmpBuf, "varLineCust", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- timeline-chart size(%d)", nRes );
    strcat(resChart, repTmpBuf);

    //분석용랭킹구하기
    memset(topValue, 0x00, sizeof(topValue));
    if(isCust == 1)		rxt = freport_GetRankTimeBysq("day", topValue, beginDate, endDate, refGroupSq, param_cRealTimeFlag);
    else if(isCust == 2)rxt = freport_GetRankTimeBysq("week", topValue, beginDate, endDate, refGroupSq, param_cRealTimeFlag);
    else				rxt = freport_GetRankTimeBysq("month", topValue, beginDate, endDate, refGroupSq, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTimeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTimeAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
        {
            if(bDayTime == 1)
            {
                sprintf(resTimeAnaly, "ㆍ 이벤트 발생이 가장 많은 시간대는 [ %s ]시 입니다.", topValue);
            }
            else
            {
                sprintf(resTimeAnaly, "ㆍ 이벤트 발생이 가장 많은 일자는 [ %s ] 입니다.", topValue);
            }
        }
        else
        {
            sprintf(resTimeAnaly, "ㆍThe most frequent occurrence of the event is [ %s ].", topValue);
        }
    }

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]3. Make rank-chart ");

    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseCust, 0x00, sizeof(sStrCloseCust));
    memset(uStrWarnCust, 0x00, sizeof(uStrWarnCust));
    memset(uStrCritCust, 0x00, sizeof(uStrCritCust));
    memset(uStrBlokCust, 0x00, sizeof(uStrBlokCust));
    //분석코멘트
    memset(topValue,0x00, sizeof(topValue));
    if(isCust == 1)		rxt = freport_GetRankIp("day", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust, topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    else if(isCust == 2)rxt = freport_GetRankIp("week", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust, topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    else				rxt = freport_GetRankIp("month", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust, topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resRankAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resRankAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP ID는 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 사번은 [ %s ] 입니다.", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP 이름은 [ %s ] 입니다.", topValue);
            else
                sprintf(resRankAnaly, "ㆍ 이벤트 발생이 가장 많은 TOP IP는 [ %s ] 입니다.", topValue);
        }
        else
        {
            if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "ip"))
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "id"))
                sprintf(resRankAnaly, "ㆍThe ID with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "sno"))
                sprintf(resRankAnaly, "ㆍThe SNO with the most event occurrences is [ %s ].", topValue);
            else if (!strcasecmp(g_stProcReportInfo.szConfMailUserView, "name"))
                sprintf(resRankAnaly, "ㆍThe NAME with the most event occurrences is [ %s ].", topValue);
            else
                sprintf(resRankAnaly, "ㆍThe IP with the most event occurrences is [ %s ].", topValue);
        }
    }

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart sCategory(%s) ",    sCategory );
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart sStrCloseCust(%s) ",sStrCloseCust );
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart uStrWarnCust(%s) ", uStrWarnCust );
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart uStrCritCust(%s) ", uStrCritCust );
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart uStrBlokCust(%s) ", uStrBlokCust );

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }
    nRes = fcom_ReportBarFilter(repTmpBuf, "rankChartCust", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- rank-chart size(%d) ", nRes );
    strcat(resChart, repTmpBuf);


    memset(sCategory, 0x00, sizeof(sCategory));
    memset(sStrCloseCust, 0x00, sizeof(sStrCloseCust));
    memset(uStrWarnCust, 0x00, sizeof(uStrWarnCust));
    memset(uStrCritCust, 0x00, sizeof(uStrCritCust));
    memset(uStrBlokCust, 0x00, sizeof(uStrBlokCust));

    for(i=0; i<CUR_RULE_CNT; i++)
    {
        //모든 합이 0이 아니라면
        if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
        {
//            sCustTypeTot = sCustTypeValue[i][0]+sCustTypeValue[i][1]+sCustTypeValue[i][2]
//                           + uCustTypeValue[i][0]+uCustTypeValue[i][1]+uCustTypeValue[i][2];
            sCustTypeTot = uCustTypeValue[i][0]+uCustTypeValue[i][1]+uCustTypeValue[i][2];
        }
        else
        {
            sCustTypeTot = uCustTypeValue[i][0]+uCustTypeValue[i][1]+uCustTypeValue[i][2];
        }
        if(sCustTypeTot != 0)
        {
//            fcom_GetStrType2(i,sCategory+strlen(sCategory),fcom_GetCustLang());
            sprintf(sCategory+strlen(sCategory), "'%s',", fcom_getStrType(i));
//            sprintf(sStrCloseCust+strlen(sStrCloseCust), "%d,",
//                    sCustTypeValue[i][0]+sCustTypeValue[i][1]+sCustTypeValue[i][2]);
            sprintf(uStrWarnCust+strlen(uStrWarnCust), "%d,", uCustTypeValue[i][0]);
            sprintf(uStrCritCust+strlen(uStrCritCust), "%d,", uCustTypeValue[i][1]);
            sprintf(uStrBlokCust+strlen(uStrBlokCust), "%d,", uCustTypeValue[i][2]);
        }
    }
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart sCategory(%s) ", sCategory );
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart sStrCloseCust(%s) ",  sStrCloseCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart uStrWarnCust(%s) ",  uStrWarnCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart uStrCritCust(%s) ",  uStrCritCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart uStrBlokCust(%s) ",  uStrBlokCust);

    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_horizontal2.js", repTmpBuf);
    }
    nRes = fcom_ReportBarFilter(repTmpBuf, "typeChartCust", sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust);

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- type-chart size(%d) ", nRes );

    strcat(resChart, repTmpBuf);
    //분석코멘트
    memset(topValue, 0x00, sizeof(topValue));
    if(isCust == 1)		rxt = freport_GetRankTypeBysq("day", topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    else if(isCust == 2)rxt = freport_GetRankTypeBysq("week", topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    else				rxt = freport_GetRankTypeBysq("month", topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    if(rxt == 0)
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resTypeAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resTypeAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resTypeAnaly, "ㆍ 이벤트 발생이 가장 많은 항목은 [ %s ] 입니다.", topValue);
        else
            sprintf(resTypeAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }

    WRITE_INFO(CATEGORY_DEBUG, "[CUSTOM]5. Make group-chart ");

    memset(sCategory,     0x00, sizeof(sCategory));
    memset(sStrCloseCust, 0x00, sizeof(sStrCloseCust));
    memset(uStrWarnCust,  0x00, sizeof(uStrWarnCust));
    memset(uStrCritCust,  0x00, sizeof(uStrCritCust));
    memset(uStrBlokCust,  0x00, sizeof(uStrBlokCust));
    if(isCust == 1)
    {

        rxt = freport_GetRankGroupBysq("day",
                                  sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust,
                                  topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    }
    else if(isCust == 2)
    {

        rxt = freport_GetRankGroupBysq("week",
                                  sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust,
                                  topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    }
    else
    {

        rxt = freport_GetRankGroupBysq("month",
                                  sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust,
                                  topValue, beginDate, endDate, refGroupSq, currLang, param_cRealTimeFlag);
    }
    if(rxt < 1) //Group 미등록1개밖에 없으면
    {
        if(!strncmp(currLang, "kr", 2))
            strcpy(resGroupAnaly, "ㆍ 데이터가 존재하지 않습니다.");
        else
            strcpy(resGroupAnaly, "ㆍData does not exist.");
    }
    else
    {
        if(!strncmp(currLang, "kr", 2))
            sprintf(resGroupAnaly, "ㆍ 이벤트 발생이 가장 많은 그룹은 [ %s ] 입니다.", topValue);
        else
            sprintf(resGroupAnaly, "ㆍThe most frequent occurrence of an event is [ %s ].", topValue);
    }


    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- group-chart sCategory(%s) ",sCategory );
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- group-chart sStrCloseCust(%s) ",sStrCloseCust );
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- group-chart uStrWarnCust(%s) ",uStrWarnCust );
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- group-chart uStrCritCust(%s) ",uStrCritCust );
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- group-chart uStrBlokCust(%s) ",uStrBlokCust );


    memset(repTmpBuf, 0x00, sizeof(repTmpBuf));
    if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical.js", repTmpBuf);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "bar_vertical2.js", repTmpBuf);
    }

    nRes = fcom_ReportBarFilter(repTmpBuf, "groupChartCust",
                             sCategory, sStrCloseCust, uStrWarnCust, uStrCritCust, uStrBlokCust);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- group-chart size(%d) ",  nRes );

    strcat(resChart, repTmpBuf);
    *chartSize = strlen(resChart);
    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]- all chart tot size(%d) ",
            *chartSize);

    WRITE_INFO(CATEGORY_INFO, "[CUSTOM]6. Make table " );

//   nRes = freport_MakeTableCustom(resTable, sqlCust, sCustValue, uCustValue, sqlCustNum, bDayTime);
   nRes = freport_MakeTableCustom(resTable, sqlCust,  uCustValue, sqlCustNum, bDayTime);
    WRITE_INFO(CATEGORY_INFO,"[CUSTOM]- table size(%d) ",nRes );

    return strlen(resTable);
}


int freport_MergeReportDayBysq(char *p_histEventName, int mLimit, char *p_prefix, char *p_postfix)
{
    int		rxt;
    int		mergeCnt;
    int		deleteCnt;

    char	sqlBuf[4096];
    char	histTableName[50];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf,
            "insert into STD_EVENT_TB\n"
            "(\n"
            "	STD_RECORD_TIME,\n"
            "	STD_IPADDR,\n"
            "	STD_US_SQ,\n"
            "	STD_UG_SQ,\n"
            "	STD_TYPE,\n"
            "	STD_LEVEL,\n"
            "	STD_EXIST,\n"
            "	STD_COUNT\n"
            ")\n"
            "select DAY_TIME, EV_IPADDR, US_SQ, ifnull(UG_SQ, 1), EV_TYPE, EV_LEVEL, EV_EXIST, CNT\n"
            "from (\n"
            "	select D.DAY_TIME, D.EV_IPADDR, D.US_SQ, D.UG_SQ, D.EV_TYPE, D.EV_LEVEL, D.EV_EXIST, sum(D.COUNT) as CNT\n"
            "	from (\n"
            "		select  date_format(A.EV_RECORD_TIME,'%%Y-%%m-%%d %%H') as DAY_TIME,\n"
            "				A.EV_IPADDR,\n"
            "				A.US_SQ,\n"
            "				D.UG_SQ,\n"
            "				A.EV_TYPE,\n"
            "				A.EV_LEVEL,\n"
            "				A.EV_EXIST,\n"
            "				count(*) as COUNT\n"
            "		from EVENT_TB as A\n"
            "			 left join (  select B.US_SQ, C.UG_SQ\n"
            "						  from USER_LINK_TB as B, USER_GROUP_TB as C\n"
            "						  where B.UG_SQ = C.UG_SQ\n"
            "					   ) D on A.US_SQ = D.US_SQ\n"
            "		where date_format(A.EV_RECORD_TIME,'%%Y-%%m-%%d') = date_format(curdate() - interval 1 day, '%%Y-%%m-%%d')\n"
            "		group by DAY_TIME, A.EV_IPADDR, A.US_SQ, A.EV_TYPE, A.EV_LEVEL, A.EV_EXIST\n"
            "		union all\n"
            "		select  date_format(A.EVH_RECORD_TIME,'%%Y-%%m-%%d %%H') as DAY_TIME,\n"
            "				A.EV_IPADDR,\n"
            "				A.US_SQ,\n"
            "				D.UG_SQ,\n"
            "				A.EV_TYPE,\n"
            "				A.EV_LEVEL,\n"
            "				A.EV_EXIST,\n"
            "				count(*) as COUNT\n"
            "		from %s as A\n"
            "			left join ( select B.US_SQ, C.UG_SQ\n"
            "						from USER_LINK_TB as B, USER_GROUP_TB as C\n"
            "						where B.UG_SQ = C.UG_SQ\n"
            "					  ) D on A.US_SQ = D.US_SQ\n"
            "		where EV_EXIST = 0\n"
            "		and date_format(A.EVH_RECORD_TIME,'%%Y-%%m-%%d') = date_format(curdate() - interval 1 day, '%%Y-%%m-%%d')\n"
            "		group by DAY_TIME, A.EV_IPADDR, A.US_SQ, A.EV_TYPE, A.EV_LEVEL, A.EV_EXIST\n"
            "	) as D\n"
            "	group by DAY_TIME, EV_IPADDR, US_SQ, EV_TYPE, EV_LEVEL, EV_EXIST order by DAY_TIME\n"
            ") as T\n"
            " on duplicate key update\n"
            "	STD_RECORD_TIME = T.DAY_TIME,\n"
            "	STD_IPADDR = T.EV_IPADDR,\n"
            "	STD_US_SQ = T.US_SQ,\n"
            "	STD_UG_SQ = ifnull(T.UG_SQ, 1),\n"
            "	STD_TYPE = T.EV_TYPE,\n"
            "	STD_LEVEL = T.EV_LEVEL,\n"
            "	STD_EXIST = T.EV_EXIST,\n"
            "	STD_COUNT = T.CNT", p_histEventName);

    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Proc mergeCnt(%d)", mergeCnt );

    memset(histTableName, 0x00, sizeof(histTableName));
    // Get STD_EVENT_HISTORY Table Name
    fdb_GetHistoryTableName("STD_EVENT_TB", histTableName, p_prefix, p_postfix);

    rxt = fdb_InsertReportHistoryTb("day", histTableName);
    if(rxt == 0) // Insert 성공시
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "delete from STD_EVENT_TB\n"
                        "where date_format(STD_RECORD_TIME, '%%Y-%%m')\n "
                        "< date_format(CURDATE() - interval %d month, '%%Y-%%m')", mLimit);

        deleteCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
            return -1;
        }

        WRITE_INFO(CATEGORY_DB,"Proc deleteCnt(%d) ",deleteCnt );
    }

//    fdb_SqlCommit(g_stMyCon);

    return 0;
}

int freport_MergeReportWeekBysq(int mLimit, char *p_prefix, char *p_postfix)
{
    int		rxt;
    int		mergeCnt;

    char	sqlBuf[4096] = {0x00,};
    char	histTableName[50] = {0x00,};

    time_t  YesterDay_t;
    char    local_szWhereTime[12] = {0x00,};
    YesterDay_t = time(NULL)-86400;
    // 1 day age
    fcom_time2str(YesterDay_t, local_szWhereTime, "YYYY-MM-DD");
    strcat(local_szWhereTime,"%");

    //일월화수목금토 달력의가로한줄을 Week로한다.
    snprintf(sqlBuf, sizeof(sqlBuf),
       "insert into STW_EVENT_TB "
       "( "
       "	STW_RECORD_TIME, "
       "	STW_IPADDR, "
       "	STW_US_SQ, "
       "	STW_UG_SQ, "
       "	STW_TYPE, "
       "	STW_LEVEL, "
       "	STW_EXIST, "
       "	STW_COUNT "
       ") "
       "select WEEK_TIME, "
       "		STD_IPADDR, "
       "		STD_US_SQ, "
       "		STD_UG_SQ, "
       "		STD_TYPE, "
       "		STD_LEVEL, "
       "		STD_EXIST, "
       "		CNT "
       "from ( "
       "	select  CONCAT(	date_format(STD_RECORD_TIME,'%%Y-%%m'),'-', "
       "					floor(	(date_format(STD_RECORD_TIME,'%%d') + "
       "							(date_format(date_format(STD_RECORD_TIME,'%%Y%%m%%01'),'%w')-1))/7)+1, 'W'"
       "			) as WEEK_TIME, "
       "			STD_IPADDR, "
       "			STD_US_SQ, "
       "			STD_UG_SQ, "
       "			STD_TYPE, "
       "			STD_LEVEL, "
       "			STD_EXIST, "
       "			sum(STD_COUNT) as CNT "
       "	from STD_EVENT_TB "
       "	where STD_RECORD_TIME LIKE '%s' "
       "	group by WEEK_TIME, STD_IPADDR, STD_US_SQ, STD_TYPE, STD_LEVEL, STD_EXIST "
       ") as T "
       " on duplicate key update "
       "	STW_RECORD_TIME = T.WEEK_TIME, "
       "	STW_IPADDR = T.STD_IPADDR, "
       "	STW_US_SQ = T.STD_US_SQ, "
       "	STW_UG_SQ = T.STD_UG_SQ, "
       "	STW_TYPE = T.STD_TYPE, "
       "	STW_LEVEL = T.STD_LEVEL, "
       "	STW_EXIST = T.STD_EXIST, "
       "	STW_COUNT = T.CNT", local_szWhereTime );

    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }


    WRITE_INFO(CATEGORY_INFO,"Proc mergeCnt(%d)", mergeCnt);


    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("STW_EVENT_TB", histTableName, p_prefix, p_postfix);

    rxt = fdb_InsertReportHistoryTb("week", histTableName);
    if(rxt == 0)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "delete from STW_EVENT_TB\n"
                        "where date_format(STW_RECORD_TIME, '%%Y-%%m')\n "
                        "< date_format(CURDATE() - interval %d month, '%%Y-%%m')", mLimit);

        mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

    }


    return 0;
}
int freport_MergeReportMonthBysq(int mLimit, char *p_prefix, char *p_postfix)
{
    int		rxt;
    int		mergeCnt;
    time_t  YesterDay_t;
    char	sqlBuf[4096] = {0x00,};
    char	histTableName[50] = {0x00,};
    char    local_szWhereTime[12] = {0x00,};

    YesterDay_t = time(NULL)-86400;

    // 1 day age
    fcom_time2str(YesterDay_t, local_szWhereTime, "YYYY-MM-DD");
    strcat(local_szWhereTime,"%");

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

//    sprintf(sqlBuf,
    snprintf(sqlBuf, sizeof(sqlBuf),
            "insert into STM_EVENT_TB "
            "( "
            "	STM_RECORD_TIME, "
            "	STM_IPADDR,"
            "	STM_US_SQ, "
            "	STM_UG_SQ, "
            "	STM_TYPE, "
            "	STM_LEVEL, "
            "	STM_EXIST, "
            "	STM_COUNT "
            ") "
            "select MONTH_TIME, "
            "		STD_IPADDR, "
            "		STD_US_SQ, "
            "		STD_UG_SQ, "
            "		STD_TYPE, "
            "		STD_LEVEL, "
            "		STD_EXIST, "
            "		CNT "
            "from ( "
            "	select  date_format(STD_RECORD_TIME,'%%Y-%%m') as MONTH_TIME, "
            "			STD_IPADDR, "
            "			STD_US_SQ, "
            "			STD_UG_SQ, "
            "			STD_TYPE, "
            "			STD_LEVEL, "
            "			STD_EXIST, "
//            "			count(*) as CNT "
            "			sum(STD_COUNT) as CNT "
            "	from STD_EVENT_TB "
//            "	where date_format(STD_RECORD_TIME,'%%Y-%%m-%%d') = date_format(CURDATE() - interval 1 day, '%%Y-%%m-%%d') "
            "	where  STD_RECORD_TIME LIKE '%s' "
            "	group by MONTH_TIME, STD_IPADDR, STD_US_SQ, STD_TYPE, STD_LEVEL, STD_EXIST "
            ") as T "
            " on duplicate key update "
            "	STM_RECORD_TIME = T.MONTH_TIME, "
            "	STM_IPADDR = T.STD_IPADDR, "
            "	STM_US_SQ = T.STD_US_SQ, "
            "	STM_UG_SQ = T.STD_UG_SQ, "
            "	STM_TYPE = T.STD_TYPE, "
            "	STM_LEVEL = T.STD_LEVEL, "
            "	STM_EXIST = T.STD_EXIST, "
            "	STM_COUNT = T.CNT", local_szWhereTime );

    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }


    WRITE_INFO(CATEGORY_INFO, "Proc mergeCnt(%d) ", mergeCnt);


    memset(histTableName, 0x00, sizeof(histTableName));

    fdb_GetHistoryTableName("STM_EVENT_TB", histTableName, p_prefix, p_postfix);

    rxt = fdb_InsertReportHistoryTb("month", histTableName);
    if(rxt == 0)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "delete from STM_EVENT_TB\n"
                        "where date_format(STM_RECORD_TIME, '%%Y-%%m')\n "
                        "< date_format(CURDATE() - interval %d month, '%%Y-%%m')", mLimit);

        mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }
    }

    return 0;
}

#if 0
int freport_TaskReport_Old(char* DbPreFix, char* TablePreFix)
{
    unsigned int         local_nPrevCnt     = 0;
    int         local_nTotalCnt             = 0;

    int         local_nResultIdx                  = 0;

    char        local_szGetTime[14 +1]     = {0x00,};
    char        local_szWhereTime[19 +1]   = {0x00,}; // WHERE 조건시간
    char        local_szYYYY[4 +1]         = {0x00,};
    char        local_szMM[2 +1]           = {0x00,};
    char        local_szDD[2 +1]           = {0x00,};
    char        local_szTableName[31 +1]   = {0x00,};
    char        local_szDbName[31 +1]      = {0x00,};

    struct _STD_EVENT_RESULT local_stResultEventInfo;
    struct _EVENT_HISTORY   local_stEventHistory;

    memset( &local_stEventHistory, 0x00, sizeof(struct _EVENT_HISTORY) );
    memset( &local_stResultEventInfo, 0x00, sizeof(struct _STD_EVENT_RESULT) );


    // Get Time, 어제 날짜를 가져온다
    fcom_GetTime(local_szGetTime, 86400 );

    memcpy( local_szYYYY, &local_szGetTime[0], 4 ); // YYYY
    memcpy( local_szMM, &local_szGetTime[4], 2 );   // MM
    memcpy ( local_szDD, &local_szGetTime[6], 2);   // DD

    // Set Database
    snprintf(local_szDbName, sizeof(local_szDbName), "DAP_HISTORY_%s", DbPreFix );

    snprintf(local_szWhereTime, sizeof(local_szWhereTime), "%s-%s-%s", local_szYYYY, local_szMM, local_szDD);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "CPU_HISTORY_%s", TablePreFix );

    // EVENT HISTORY Select
    freport_SelectEventHistory( &local_stEventHistory, DbPreFix, TablePreFix );

    // CPU HISTORY
    freport_CpuHistory(  local_szDbName,
                        local_szTableName,
                        local_szWhereTime,
                           &local_nTotalCnt,
                           &local_nPrevCnt,
                            &local_nResultIdx,
                          &local_stResultEventInfo, &local_stEventHistory);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "PROCESS_HISTORY_%s", TablePreFix );

    // Process History
    freport_ProcessHistory(local_szDbName,
                          local_szTableName,
                          local_szWhereTime,
                             &local_nTotalCnt,
                             &local_nPrevCnt,
                            &local_stResultEventInfo,
                            &local_stEventHistory);

    // MainBoard History -> 정보성 이력만 있으므로 애초에 제외.


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "DISK_HISTORY_%s", TablePreFix );

    // Disk History
    freport_DiskHistory(local_szDbName,
                        local_szTableName,
                        local_szWhereTime,
                        &local_nTotalCnt,
                        &local_nPrevCnt,
                         &local_stResultEventInfo,
                        &local_stEventHistory);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "NET_ADAPTER_HISTORY_%s", TablePreFix );

    // Network Adapter
    freport_NetadapterHistory(local_szDbName,
                              local_szTableName,
                              local_szWhereTime,
                              &local_nTotalCnt,
                              &local_nPrevCnt,
                              &local_stResultEventInfo,
                              &local_stEventHistory);

    // Set TableName
     snprintf(local_szTableName, sizeof(local_szTableName), "NET_DRIVE_HISTORY_%s", TablePreFix );

    // Network Driver
    freport_NetdriveHistory(local_szDbName,
                            local_szTableName,
                            local_szWhereTime,
                            &local_nTotalCnt,
                            &local_nPrevCnt,
                            &local_stResultEventInfo,
                            &local_stEventHistory);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "WIFI_HISTORY_%s", TablePreFix );

    // 와이파이
    freport_WifiHistory(local_szDbName,
                        local_szTableName,
                        local_szWhereTime,
                        &local_nTotalCnt,
                        &local_nPrevCnt,
                        &local_stResultEventInfo,
                        &local_stEventHistory);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "NET_PRINTER_HISTORY_%s", TablePreFix );

    // Network Printer
    freport_NetPrinterHistory(local_szDbName,
                              local_szTableName,
                              local_szWhereTime,
                              &local_nTotalCnt,
                              &local_nPrevCnt,
                              &local_stResultEventInfo,
                              &local_stEventHistory);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "NET_CONNECTION_HISTORY_%s", TablePreFix );

    // Network Connection
    freport_NetConnectionHistory(local_szDbName,
                                 local_szTableName,
                                 local_szWhereTime,
                                 &local_nTotalCnt,
                                 &local_nPrevCnt,
                                 &local_stResultEventInfo,
                                 &local_stEventHistory);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "BLUETOOTH_HISTORY_%s", TablePreFix );

    // 블루투스
    freport_BlueToothHistory(local_szDbName,
                             local_szTableName,
                             local_szWhereTime,
                             &local_nTotalCnt,
                             &local_nPrevCnt,
                             &local_stResultEventInfo,
                             &local_stEventHistory);



    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "INFRARED_DEVICE_HISTORY_%s", TablePreFix );

    // 적외선 장치
    freport_InfraredHistory(local_szDbName,
                            local_szTableName,
                            local_szWhereTime,
                            &local_nTotalCnt,
                            &local_nPrevCnt,
                            &local_stResultEventInfo,
                            &local_stEventHistory);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "ROUTER_HISTORY_%s", TablePreFix );

    // 공유기
    freport_RouterHistory( local_szDbName,
                           local_szTableName,
                           local_szWhereTime,
                           &local_nTotalCnt,
                           &local_nPrevCnt,
                           &local_stResultEventInfo,
                           &local_stEventHistory);


    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "SHARE_FOLDER_HISTORY_%s", TablePreFix );

    // 공유폴더
    freport_ShareFolderHistory(local_szDbName,
                               local_szTableName,
                               local_szWhereTime,
                               &local_nTotalCnt,
                               &local_nPrevCnt,
                               &local_stResultEventInfo,
                               &local_stEventHistory);

    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "SYSTEM_HISTORY_%s", TablePreFix );

    // 가상머신
    freport_SystemHistory( local_szDbName,
                           local_szTableName,
                           local_szWhereTime,
                           &local_nTotalCnt,
                           &local_nPrevCnt,
                           &local_stResultEventInfo,
                           &local_stEventHistory);
    // Set TableName
    snprintf(local_szTableName, sizeof(local_szTableName), "CONNECT_EXT_HISTORY_%s", TablePreFix );

    // 외부 네트워크 접속
    freport_ConnExtHistory( local_szDbName,
                            local_szTableName,
                            local_szWhereTime,
                            &local_nTotalCnt,
                            &local_nPrevCnt,
                            &local_stResultEventInfo,
                            &local_stEventHistory);
//
//    // OS -> 이력에 정보성만 남으므로 제외.
//
//    // OS 계정 -> 이력에 정보성만 남으므로 제외.
//
//    // 유출된 장비
//
    // 원격 데스크탑
    // Set TableName
     snprintf(local_szTableName, sizeof(local_szTableName), "RDP_SESSION_HISTORY_%s", TablePreFix );

    freport_RdpSessionHistory( local_szDbName,
                               local_szTableName,
                               local_szWhereTime,
                               &local_nTotalCnt,
                               &local_nPrevCnt,
                               &local_stResultEventInfo,
                               &local_stEventHistory);


    // CPU 사용통제
    snprintf( local_szTableName, sizeof(local_szTableName), "CTRL_PROCESS_CPU_HISTORY_%s", TablePreFix);

    freport_CpuCtrlHistory(    local_szDbName,
                               local_szTableName,
                               local_szWhereTime,
                               &local_nTotalCnt,
                               &local_nPrevCnt,
                               &local_stResultEventInfo,
                               &local_stEventHistory);

    // Get Group Info
    freport_GetGroupInfo( local_stResultEventInfo.ptrStdEventInfo, local_stResultEventInfo.EventCnt );


    WRITE_DEBUG(CATEGORY_DEBUG,"[Result Report Data Cnt] : %d ", local_stResultEventInfo.EventCnt);


    // STD_EVENT_TB Insert
    freport_InsertStdEvent( local_stResultEventInfo.ptrStdEventInfo, local_stResultEventInfo.EventCnt);

    fcom_MallocFree((void**) &local_stResultEventInfo.ptrStdEventInfo);
    fcom_MallocFree((void**) &local_stEventHistory.ptrEventHistoryDetect);
    fcom_MallocFree((void**) &local_stEventHistory.ptrEventHistoryDuplDetect);

    return 0;


}
#endif