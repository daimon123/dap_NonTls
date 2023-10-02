//
// Created by KimByoungGook on 2020-06-25.
//


#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

#include "com/dap_com.h"
#include "secure/dap_secure.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "db/dap_checkdb.h"
#include "ipc/dap_Queue.h"


#include "schd.h"

/* Declare : dap_checkdb.h  */
char g_exHistItem[51][TABLE_LENGTH] =
{        "SYSTEM", "OS", "CPU", "NET_ADAPTER", "WIFI",                                      //5
         "BLUETOOTH", "NET_CONNECTION", "DISK", "NET_DRIVE", "OS_ACCOUNT",                  //10
         "SHARE_FOLDER", "INFRARED_DEVICE", "PROCESS", "ROUTER", "NET_PRINTER",             //15
         "NET_SCAN", "EVENT", "CONFIG", "RULE", "RULE_EXCEPT",                              //20
         "USER_GROUP", "USER_GROUP_LINK", "USER", "USER_LINK", "MANAGER",                   //25
         "HW_BASE","DISK_REG_LINK","DETECT_PROCESS","WATCH_SERVER","DETECT_PRINTER_PORT",   //30
         "MANAGER_EVENT","STD_EVENT","STW_EVENT","STM_EVENT",                               //34
         "ALARM","DETECT_LINK","DETECT_GROUP","DETECT_GROUP_LINK","RULE_DETECT",            //39
         "RULE_DETECT_LINK","CONNECT_EXT","RULE_SCHEDULE","UPGRADE_AGENT","WIN_DRV",        //44
         "SYNC_USER","SYNC_GROUP","RDP_SESSION", "CTRL_PROCESS_CPU", "BASE_STATUS",         //49
         "SERVER_LOG", "AGENT_LOG"                                                          //51 개

};

int g_nSafeFetchRow;

void fschd_SetSafeFetchRow()
{
    g_nSafeFetchRow = fcom_GetProfileInt("MYSQL", "SAFE_FETCH_ROW", 1000);
}
int fschd_MakeStatsGwTb()
{
    int		rowCnt = 0;
    char	sqlBuf[1024 +1] = {0x00,};

    strcpy	(sqlBuf, "truncate table STATS_GW_TB");
    rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }
    WRITE_INFO(CATEGORY_INFO, "Succeed in truncate, rowCnt ");

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf	(sqlBuf, sizeof(sqlBuf),"insert into STATS_GW_TB"
                        "(SG_CLASS_IP,SG_DEFAULT_MAC,SG_RECORD_TIME) "
                        "select SUBSTRING_INDEX(B.HB_ACCESS_IP,'.',3), A.NA_DEFAULT_GW_MAC, sysdate() "
                        "from NET_ADAPTER_TB as A JOIN HW_BASE_TB as B  "
                        "on A.HB_SQ = B.HB_SQ and length(A.NA_DEFAULT_GW) > 0 "
                        "group by SUBSTRING_INDEX(B.HB_ACCESS_IP,'.',3), NA_DEFAULT_GW_MAC "
                        "having count(NA_DEFAULT_GW_MAC) > 1");

    rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_INFO, "Proc rowCnt(%d) ", rowCnt);

    return rowCnt;
}

int fschd_CreateHistoryDatabase(char *p_histDBName)
{
    char		histDBName[30 +1] = {0x00,};
    char		tmpPrefix[6 +1] = {0x00,};
    char		sqlBuf[127 +1] = {0x00,};

    _DAP_DB_CONFIG* pstDbConfig = NULL;

    pstDbConfig = &g_stServerInfo.stDapDbConfigInfo;

    memset(histDBName, 0x00, sizeof(histDBName));

    if(strlen(p_histDBName) == 0)
    {
        WRITE_INFO(CATEGORY_INFO, "histDBPrefix(%d) ", pstDbConfig->nCfgHistDBPrefix );
        if(pstDbConfig->nCfgHistDBPrefix == 0) //String
        {
            memset(tmpPrefix, 0x00, sizeof(tmpPrefix));
            fdb_GetHistoryDbTime(tmpPrefix, 0);
            sprintf(histDBName, "%s_%s", pstDbConfig->szCfgHistDBPrefix,tmpPrefix);
        }
        else if(pstDbConfig->nCfgHistDBPrefix == -1) //NO
        {
            return 0;
        }
    }
    else
    {
        strcpy(histDBName, p_histDBName);
    }

    WRITE_INFO(CATEGORY_INFO, "Current year histDBName(%s) ", histDBName);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf		(sqlBuf, "CREATE DATABASE IF NOT EXISTS %s", histDBName);
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }


    sprintf(histDBName, "%s_%d", pstDbConfig->szCfgHistDBPrefix, atoi(tmpPrefix)+1 );

    WRITE_INFO(CATEGORY_INFO, "Next year histDBName(%s) ", histDBName);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf		(sqlBuf, "CREATE DATABASE IF NOT EXISTS %s", histDBName);
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    return 0;
}

/*
 * 정책 중에 GROUP(ru_target_type=1)으로 설정된 값을 가져온다.
 */
int fschd_GetGroupValueByRule(char *res)
{
    char    sqlBuf[128 +1] = {0x00,};
    int		rowCnt = 0;
    int		bFirst = 1;

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    strcpy(sqlBuf, "select RU_TARGET_VALUE from RULE_TB where RU_TARGET_TYPE=1 and RU_USE=1");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        return -1;
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (bFirst)
            {
                strcpy(res, g_stMyCon->row[0]);
                bFirst = 0;
            }
            else
            {
                sprintf(res+strlen(res), ",%s", g_stMyCon->row[0]);
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)res(%s) ", rowCnt,res);

    return rowCnt;

}

int fschd_UgdGroupByRule()
{
    int		rowCnt = 0;
    int		updCnt = 0;
    int		tokenCnt = 0;
    char	result[128 +1] = {0x00,};
    char    sqlBuf[128 +1] = {0x00,};
    char	*tokenSeq = NULL;

    memset(result, 0x00, sizeof(result));
    rowCnt = fschd_GetGroupValueByRule(result);

    if (rowCnt > 0)
    {
        tokenCnt = fcom_TokenCnt(result, ",");
        if (tokenCnt > 0)
        {
            tokenSeq = strtok(result, ",");
            while (tokenSeq != NULL)
            {
                memset	(sqlBuf, 0x00, sizeof(sqlBuf));
                sprintf	(sqlBuf,"select UG_SQ from USER_GROUP_TB "
                                   "where UG_SQ=%s", tokenSeq);

                rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                    WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                         "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                }
                fdb_SqlFreeResult(g_stMyCon);

                // if not found group
                if (rowCnt == 0)
                {
                    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
                    sprintf	(sqlBuf,"update RULE_TB set RU_USE=0 "
                                       "where RU_TARGET_TYPE=1 "
                                       "and RU_TARGET_VALUE=%s "
                                       "and RU_USE=1", tokenSeq);


                    updCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
                    if(g_stMyCon->nErrCode != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                                       g_stMyCon->nErrCode,
                                       g_stMyCon->cpErrMsg);
                        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                    }
                }
                tokenSeq = strtok(NULL, ",");
            }
        }
        else
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"select UG_SQ from USER_GROUP_TB "
                               "where UG_SQ=%s", result);

            rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
            }
            fdb_SqlFreeResult(g_stMyCon);

            // if not found group
            if (rowCnt == 0)
            {
                memset	(sqlBuf, 0x00, sizeof(sqlBuf));

                sprintf	(sqlBuf,"update RULE_TB set RU_USE=0 "
                                   "where RU_TARGET_TYPE=1 "
                                   "and RU_TARGET_VALUE=%s "
                                   "and RU_USE=1", result);


                updCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                    WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                         "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                }
            }
        }
    }

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)updCnt(%d) ", rowCnt,updCnt);

    return updCnt;
}


int fschd_CreateHistoryTables()
{
    int		i = 0, rxt = 0;
    int     monthCnt = 0;
    int     yearCnt = 0;
    int		histItemCnt = 0;
    int     nStartMonth = 0;
    int     nEndMonth = 0;
    int     nStartYear = 0;
    int     nEndYear = 0;
    char	histDBName[30 +1] = {0x00,};
    char	logTable[64 +1] = {0x00,};
    char	tmpBuf[256 +1] = {0x00,};
    char	tmpPrefix[6 +1] = {0x00,};
    char	tmpPostfix[2 +1] = {0x00,};
    char    tmpYearfix[4 +1] = {0x00,};
    char	histScriptName[128 +1] = {0x00,};

    _DAP_DB_CONFIG* pstDbConfigInfo = NULL;

    pstDbConfigInfo = &g_stServerInfo.stDapDbConfigInfo;

    WRITE_INFO(CATEGORY_INFO, "histDBPrefix(%s) histTableKind(%d) ",
               pstDbConfigInfo->szCfgHistDBPrefix,
               pstDbConfigInfo->nCfgHistTableKind);

    memset(histDBName, 0x00, sizeof(histDBName));
    if(pstDbConfigInfo->nCfgHistDBPrefix == 0) //String
    {
        memset(tmpPrefix, 0x00, sizeof(tmpPrefix));
        fdb_GetHistoryDbTime(tmpPrefix, 0);
        sprintf(histDBName, "%s_%s", pstDbConfigInfo->szCfgHistDBPrefix,tmpPrefix);
        rxt = fdb_GetExistsDatabase(histDBName);
        if(rxt == 0)
        {
            WRITE_INFO(CATEGORY_DB, "Create db because not found, histDBName(%s) ",
                    histDBName);
            rxt = fschd_CreateHistoryDatabase(histDBName);
            if(rxt < 0)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in create db(%s) ", histDBName);
                return -1;
            }
        }
    }

    histItemCnt = sizeof(g_exHistItem)/sizeof(char[TABLE_LENGTH]);
    WRITE_INFO(CATEGORY_INFO, "histItemCnt(%d) ", histItemCnt );

    memset(tmpPostfix, 0x00, sizeof(tmpPostfix));
    fdb_GetHistoryTableTime(tmpPostfix, tmpYearfix, 0);

    // current month or day
    nStartMonth = atoi(tmpPostfix);
    nStartYear  = atoi(tmpYearfix);

    /* End Month */
    memset(tmpPostfix, 0x00, sizeof(tmpPostfix));
    fdb_GetHistoryTableTime(tmpPostfix, tmpYearfix, pstDbConfigInfo->nCfgHistTableNext);
    nEndMonth   = atoi(tmpPostfix);
    nEndYear    = atoi(tmpYearfix);

    WRITE_INFO(CATEGORY_DB, "Make current month or day, tmpPostfix (%d : %d ) -> (%d : %d)",
               nStartYear,
               nStartMonth,
               nEndYear,
               nEndMonth);

    for(i=0; i<histItemCnt; i++)
    {
        for(yearCnt = nStartYear; yearCnt <= nEndYear;  yearCnt++)
        {
            if(yearCnt == nStartYear)
            {
                for(monthCnt = nStartMonth; monthCnt <= 12; monthCnt++)
                {
                    memset(logTable, 0x00, sizeof(logTable));

                    sprintf(tmpPostfix,"%02d", monthCnt);

                    WRITE_DEBUG(CATEGORY_DEBUG,"Schedule Create History Postfix : %s",tmpPostfix);

                    rxt = fdb_GetHistoryTableName(g_exHistItem[i], logTable, tmpPrefix, tmpPostfix);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DB, "Fail in logtable name ");
                        return 0;
                    }
                    WRITE_INFO(CATEGORY_DB, "logTable(%s) ", logTable );

                    fdb_GetExistsTable(histDBName, logTable);
                    rxt = fdb_GetExistsTable(histDBName, logTable);

                    WRITE_DEBUG(CATEGORY_DEBUG,"Check Exist Table %s",logTable);

                    if(rxt == 0)
                    {
                        WRITE_INFO(CATEGORY_DB, "Run create table because not found, logTable(%s) ",
                                   logTable);
                        //create table
                        memset(histScriptName, 0x00, sizeof(histScriptName));
                        memset(tmpBuf, 0x00, sizeof(tmpBuf));

                        sprintf(histScriptName, "create_%s_HISTORY", g_exHistItem[i]);
                        sprintf(tmpBuf, "%s/script/table_script/%s.sql", getenv("DAP_HOME"),histScriptName);

                        WRITE_DEBUG(CATEGORY_DEBUG,"Run Execute Script File : [%s] ", tmpBuf);

                        fdb_ExecuteScript(logTable, tmpBuf);
                    }
                }
            }
        }
    } //for
    WRITE_INFO(CATEGORY_DB, "Make current month or day ");

    // next month or day
    WRITE_INFO(CATEGORY_DB, "Make next month or day, tmpPostfix(%s) ", tmpPostfix);

    memset(histDBName, 0x00, sizeof(histDBName));
    if(pstDbConfigInfo->nCfgHistDBPrefix == 0) //String
    {
        memset(tmpPrefix, 0x00, sizeof(tmpPrefix));
        fdb_GetHistoryDbTime(tmpPrefix, pstDbConfigInfo->nCfgHistTableNext);
        sprintf(histDBName, "%s_%s", pstDbConfigInfo->szCfgHistDBPrefix,tmpPrefix);
        rxt = fdb_GetExistsDatabase(histDBName);
        if(rxt == 0)
        {
            WRITE_INFO(CATEGORY_DB, "Create db because not found, histDBName(%s) ",
                       histDBName);
            rxt = fschd_CreateHistoryDatabase(histDBName);
            if(rxt < 0)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in create db(%s) ", histDBName);
                return -1;
            }
        }
    }

    for(i=0; i<histItemCnt; i++)
    {
        for(yearCnt = nStartYear; yearCnt <= nEndYear;  yearCnt++)
        {
            if(yearCnt == nEndYear)
            {
                for(monthCnt = 1; monthCnt <= nEndMonth; monthCnt++)
                {
                    memset(logTable, 0x00, sizeof(logTable));

                    sprintf(tmpPostfix,"%02d", monthCnt);

                    WRITE_DEBUG(CATEGORY_DEBUG,"Schedule Create History Postfix : %s",tmpPostfix);
                    rxt = fdb_GetHistoryTableName(g_exHistItem[i], logTable, tmpPrefix, tmpPostfix);
                    if(rxt < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DB, "Fail in logtable name ");
                        return 0;
                    }
                    WRITE_INFO(CATEGORY_DB, "logTable(%s) ", logTable );

                    fdb_GetExistsTable(histDBName, logTable);
                    rxt = fdb_GetExistsTable(histDBName, logTable);

                    WRITE_DEBUG(CATEGORY_DEBUG,"Check Exist Table %s",logTable);

                    if(rxt == 0)
                    {
                        WRITE_INFO(CATEGORY_DB, "Run create table because not found, logTable(%s) ",
                                   logTable);
                        //create table
                        memset(histScriptName, 0x00, sizeof(histScriptName));
                        memset(tmpBuf, 0x00, sizeof(tmpBuf));

                        sprintf(histScriptName, "create_%s_HISTORY", g_exHistItem[i]);
                        sprintf(tmpBuf, "%s/script/table_script/%s.sql", getenv("DAP_HOME"),histScriptName);
                        fdb_ExecuteScript(logTable, tmpBuf);
                    }
                }
            }
        }

    } //for
    WRITE_INFO(CATEGORY_DB, "Make next month or day ");

    return 0;
}

int fschd_MoveHistoryBaseTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[2048 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select HB_SQ from HW_BASE_TB "
                    "where HB_DEL = 9 limit %d",
                    g_nSafeFetchRow);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }


    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("HW_BASE_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_INFO, "histTableName(%s) ", histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "HB_SQ,"
                               "US_SQ,"
                               "HB_UNQ,"
                               "HB_MB_PN,"
                               "HB_MB_MF,"
                               "HB_MB_SN,"
                               "HB_FIRST_TIME,"
                               "HB_ACCESS_IP,"
                               "HB_ACCESS_MAC,"
                               "HB_SOCKET_IP,"
                               "HB_AGENT_VER,"
                               "HB_EXTERNAL,"
                               "HB_ACCESS_TIME,"
                               "HB_RECORD_TIME,"
                               "HB_PREV_HIST_SQ,"
                               "HB_DEL,"
                               "HBH_RECORD_TIME"
                               ") "
                               "select "
                               "HB_SQ,"
                               "US_SQ,"
                               "HB_UNQ,"
                               "HB_MB_PN,"
                               "HB_MB_MF,"
                               "HB_MB_SN,"
                               "HB_FIRST_TIME,"
                               "HB_ACCESS_IP,"
                               "HB_ACCESS_MAC,"
                               "HB_SOCKET_IP,"
                               "HB_AGENT_VER,"
                               "HB_EXTERNAL,"
                               "HB_ACCESS_TIME,"
                               "HB_RECORD_TIME,"
                               "HB_PREV_HIST_SQ,"
                               "HB_DEL,"
                               "sysdate() "
                               "from HW_BASE_TB "
                               "where HB_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0])
                               );
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_INFO,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                fdb_SqlFreeResult(g_stMyCon);
                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf, "delete from HW_BASE_TB where HB_SQ=%llu", atoll(g_stMyCon->row[0]));
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                fdb_SqlFreeResult(g_stMyCon);
                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ",
                rowCnt,
                insertCnt,
                resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);


    return insertCnt;
}

int fschd_MoveHistoryConfigTb()
{
    int rowCnt = 0;
    int insertCnt = 0;
    int resCnt = 0;
    char histTableName[50 +1] = {0x00,};
    char sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select CF_NAME,CF_FLAG from CONFIG_TB "
                    "where CF_FLAG in ('A','M','D') limit %d",
            g_nSafeFetchRow);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("CONFIG_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "CF_NAME,"
                               "CF_VALUE,"
                               "CF_FLAG,"
                               "CFH_RECORD_TIME"
                               ") "
                               "select "
                               "CF_NAME,"
                               "CF_VALUE,"
                               "CF_FLAG,"
                               "sysdate() "
                               "from CONFIG_TB "
                               "where CF_NAME='%s'",
                               histTableName,
                               g_stMyCon->row[0]);
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from CONFIG_TB where CF_NAME='%s'", g_stMyCon->row[0]);
            }
            else
            {
                sprintf	(sqlBuf, "update CONFIG_TB set CF_FLAG='S' where CF_NAME='%s'", g_stMyCon->row[0]);
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        } //while

        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryRuleTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50];
    char			sqlBuf[2048];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RU_SQ,RU_FLAG from RULE_TB "
                       "where RU_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }


    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("RULE_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_INFO,"histTableName(%s) ",histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "RU_SQ,"
                               "RU_MODIFY_CNT,"
                               "RU_ORDER,"
                               "MN_SQ,"
                               "RU_TARGET_TYPE,"
                               "RU_TARGET_VALUE,"
                               "RU_NET_ADAPTER,"
                               "RU_NET_ADAPTER_OVER,"
                               "RU_NET_ADAPTER_DUPIP,"
                               "RU_NET_ADAPTER_DUPMAC,"
                               "RU_NET_ADAPTER_MULIP,"
                               "RU_WIFI,"
                               "RU_BLUETOOTH,"
                               "RU_ROUTER,"
                               "RU_PRINTER,"
                               "RU_DISK,"
                               "RU_DISK_REG,"
                               "RU_DISK_HIDDEN,"
                               "RU_DISK_NEW,"
                               "RU_DISK_MOBILE,"
                               "RU_DISK_MOBILE_READ,"
                               "RU_DISK_MOBILE_WRITE,"
                               "RU_NET_DRIVE,"
                               "RU_NET_CONNECTION,"
                               "RU_SHARE_FOLDER,"
                               "RU_INFRARED_DEVICE,"
                               "RU_VIRTUAL_MACHINE,"
                               "RU_PROCESS_WHITE,"
                               "RU_PROCESS_BLACK,"
                               "RU_PROCESS_ACCESSMON,"
                               "RU_PROCESS_ACCESSMON_EXP,"
                               "RU_PROCESS_DETAILINFO,"
                               "RU_PROCESS_FORCEKILL,"
                               "RU_CONNECT_EXT_SVR,"
                               "RU_EXT_NET_DETECT_TYPE,"
                               "RU_SSO_CERT,"
                               "RU_WIN_DRV,"
                               "RU_RDP_SESSION,"
                               "RU_RDP_BLOCK_COPY,"
                               "RU_AGENT_CYCLE_PROCESS,"
                               "RU_AGENT_CYCLE_PROCESS_ACCESS,"
                               "RU_AGENT_CYCLE_NET_PRINTER,"
                               "RU_AGENT_CYCLE_NET_SCAN,"
                               "RU_AGENT_CYCLE_ROUTER,"
                               "RU_AGENT_CYCLE_EXT_ACCESS,"
                               "RU_AGENT_SSO_CHECK_CYCLE,"
                               "RU_AGENT_SSO_KEEP_TIME,"
                               "RU_ALARM_TYPE,"
                               "RU_ALARM_MN_SQ,"
                               "RU_USE,"
                               "RU_DESC,"
                               "RU_FLAG,"
                               "RUH_RECORD_TIME"
                               ") "
                               "select "
                               "RU_SQ,"
                               "RU_MODIFY_CNT,"
                               "RU_ORDER,"
                               "MN_SQ,"
                               "RU_TARGET_TYPE,"
                               "RU_TARGET_VALUE,"
                               "RU_NET_ADAPTER,"
                               "RU_NET_ADAPTER_OVER,"
                               "RU_NET_ADAPTER_DUPIP,"
                               "RU_NET_ADAPTER_DUPMAC,"
                               "RU_NET_ADAPTER_MULIP,"
                               "RU_WIFI,"
                               "RU_BLUETOOTH,"
                               "RU_ROUTER,"
                               "RU_PRINTER,"
                               "RU_DISK,"
                               "RU_DISK_REG,"
                               "RU_DISK_HIDDEN,"
                               "RU_DISK_NEW,"
                               "RU_DISK_MOBILE,"
                               "RU_DISK_MOBILE_READ,"
                               "RU_DISK_MOBILE_WRITE,"
                               "RU_NET_DRIVE,"
                               "RU_NET_CONNECTION,"
                               "RU_SHARE_FOLDER,"
                               "RU_INFRARED_DEVICE,"
                               "RU_VIRTUAL_MACHINE,"
                               "RU_PROCESS_WHITE,"
                               "RU_PROCESS_BLACK,"
                               "RU_PROCESS_ACCESSMON,"
                               "RU_PROCESS_ACCESSMON_EXP,"
                               "RU_PROCESS_DETAILINFO,"
                               "RU_PROCESS_FORCEKILL,"
                               "RU_CONNECT_EXT_SVR,"
                               "RU_EXT_NET_DETECT_TYPE,"
                               "RU_SSO_CERT,"
                               "RU_WIN_DRV,"
                               "RU_RDP_SESSION,"
                               "RU_RDP_BLOCK_COPY,"
                               "RU_AGENT_CYCLE_PROCESS,"
                               "RU_AGENT_CYCLE_PROCESS_ACCESS,"
                               "RU_AGENT_CYCLE_NET_PRINTER,"
                               "RU_AGENT_CYCLE_NET_SCAN,"
                               "RU_AGENT_CYCLE_ROUTER,"
                               "RU_AGENT_CYCLE_EXT_ACCESS,"
                               "RU_AGENT_SSO_CHECK_CYCLE,"
                               "RU_AGENT_SSO_KEEP_TIME,"
                               "RU_ALARM_TYPE,"
                               "RU_ALARM_MN_SQ,"
                               "RU_USE,"
                               "RU_DESC,"
                               "RU_FLAG,"
                               "sysdate() "
                               "from RULE_TB "
                               "where RU_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0])
                               );
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d) msg(%s)",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from RULE_TB where RU_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update RULE_TB set RU_FLAG='S' where RU_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                fdb_SqlFreeResult(g_stMyCon);
                return -1;
            }
        }

        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d)",
                rowCnt,
                insertCnt,
                resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);


    return insertCnt;
}

int fschd_MoveHistoryRuleExceptTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1023 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RE_SQ,RE_FLAG from RULE_EXCEPT_TB "
                       "where RE_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }


    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("RULE_EXCEPT_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB,"histTableName(%s) ",histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "RE_SQ,"
                               "RU_SQ,"
                               "MN_SQ,"
                               "RE_TYPE,"
                               "RE_CATEGORY,"
                               "RE_POS,"
                               "RE_VALUE,"
                               "RE_DESC,"
                               "RE_USE,"
                               "RE_FLAG,"
                               "REH_RECORD_TIME"
                               ") "
                               "select "
                               "RE_SQ,"
                               "RU_SQ,"
                               "MN_SQ,"
                               "RE_TYPE,"
                               "RE_CATEGORY,"
                               "RE_POS,"
                               "RE_VALUE,"
                               "RE_DESC,"
                               "RE_USE,"
                               "RE_FLAG,"
                               "sysdate() "
                               "from RULE_EXCEPT_TB "
                               "where RE_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from RULE_EXCEPT_TB where RE_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update RULE_EXCEPT_TB set RE_FLAG='S' where RE_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ",
                rowCnt,
                insertCnt,
                resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryRuleDetectTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RD_SQ,RD_FLAG from RULE_DETECT_TB "
                       "where RD_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("RULE_DETECT_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "RD_SQ,"
                               "RU_SQ,"
                               "RD_VALUE,"
                               "RD_TYPE,"
                               "RD_FLAG,"
                               "RDH_RECORD_TIME"
                               ") "
                               "select "
                               "RD_SQ,"
                               "RU_SQ,"
                               "RD_VALUE,"
                               "RD_TYPE,"
                               "RD_FLAG,"
                               "sysdate() "
                               "from RULE_DETECT_TB "
                               "where RD_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from RULE_DETECT_TB where RD_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update RULE_DETECT_TB set RD_FLAG='S' where RD_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ",
                rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryRuleDetectLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RU_SQ,TARGET_SQ,TARGET_IS_GROUP,TARGET_TYPE,RDL_FLAG from RULE_DETECT_LINK_TB "
                       "where RDL_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_INFO(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }


    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("RULE_DETECT_LINK_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "RU_SQ,"
                               "TARGET_SQ,"
                               "TARGET_IS_GROUP,"
                               "TARGET_TYPE,"
                               "RDL_FLAG,"
                               "RDLH_RECORD_TIME"
                               ") "
                               "select "
                               "RU_SQ,"
                               "TARGET_SQ,"
                               "TARGET_IS_GROUP,"
                               "TARGET_TYPE,"
                               "RDL_FLAG,"
                               "sysdate() "
                               "from RULE_DETECT_LINK_TB "
                               "where RU_SQ=%llu and TARGET_SQ=%llu and TARGET_IS_GROUP=%d and TARGET_TYPE=%d",
                        histTableName,
                        atoll(g_stMyCon->row[0]),
                        atoll(g_stMyCon->row[1]),
                        atoi(g_stMyCon->row[2]),
                        atoi(g_stMyCon->row[3]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[4] == 'D')
            {
                sprintf	(sqlBuf, "delete from RULE_DETECT_LINK_TB "
                                    "where RU_SQ=%llu and TARGET_SQ=%llu and TARGET_IS_GROUP=%d and TARGET_TYPE=%d",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]),
                            atoi(g_stMyCon->row[2]),
                            atoi(g_stMyCon->row[3]));
            }
            else
            {
                sprintf	(sqlBuf, "update RULE_DETECT_LINK_TB set "
                                    "RDL_FLAG='S' where RU_SQ=%llu and TARGET_SQ=%llu and TARGET_IS_GROUP=%d and TARGET_TYPE=%d",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]),
                            atoi(g_stMyCon->row[2]),
                            atoi(g_stMyCon->row[3]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryRuleScheduleTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RS_SQ,RS_FLAG from RULE_SCHEDULE_TB "
                       "where RS_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("RULE_SCHEDULE_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "RS_SQ,"
                               "RU_SQ,"
                               "RS_NAME,"
                               "RS_TYPE,"
                               "RS_WEEKOFDAY,"
                               "RS_DAYOFWEEK,"
                               "RS_DAY,"
                               "RS_DATE,"
                               "RS_TIME,"
                               "RS_EXCEPTION,"
                               "RS_USE,"
                               "RS_FLAG,"
                               "RSH_RECORD_TIME"
                               ") "
                               "select "
                               "RS_SQ,"
                               "RU_SQ,"
                               "RS_NAME,"
                               "RS_TYPE,"
                               "RS_WEEKOFDAY,"
                               "RS_DAYOFWEEK,"
                               "RS_DAY,"
                               "RS_DATE,"
                               "RS_TIME,"
                               "RS_EXCEPTION,"
                               "RS_USE,"
                               "RS_FLAG,"
                               "sysdate() "
                               "from RULE_SCHEDULE_TB "
                               "where RS_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from RULE_SCHEDULE_TB where RS_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update RULE_SCHEDULE_TB set RS_FLAG='S' where RS_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB,	"Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryUpgradeAgentTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select UA_SQ,UA_FLAG from UPGRADE_AGENT_TB "
                       "where UA_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("UPGRADE_AGENT_TB", histTableName, "", "");
        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "UA_SQ,"
                               "UA_NAME,"
                               "UA_VERSION,"
                               "UA_TARGET_TYPE,"
                               "UA_TARGET_VALUE,"
                               "UA_DATE,"
                               "UA_TIME,"
                               "UA_USE,"
                               "UA_FLAG,"
                               "UAH_RECORD_TIME"
                               ") "
                               "select "
                               "UA_SQ,"
                               "UA_NAME,"
                               "UA_VERSION,"
                               "UA_TARGET_TYPE,"
                               "UA_TARGET_VALUE,"
                               "UA_DATE,"
                               "UA_TIME,"
                               "UA_USE,"
                               "UA_FLAG,"
                               "sysdate() "
                               "from UPGRADE_AGENT_TB "
                               "where UA_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0])
                               );
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from UPGRADE_AGENT_TB where UA_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update UPGRADE_AGENT_TB set UA_FLAG='S' where UA_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB,	"Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}


int fschd_MoveHistoryUserGroupTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select UG_SQ,UG_FLAG from USER_GROUP_TB "
                       "where UG_FLAG in ('A','M','D') limit %d",
                g_nSafeFetchRow);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("USER_GROUP_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "UG_SQ,"
                               "UG_CODE,"
                               "UG_NAME,"
                               "UG_DESC,"
                               "UG_MANAGER,"
                               "UG_DATE,"
                               "UG_USE,"
                               "UG_FLAG,"
                               "UG_CREATE_TIME,"
                               "UG_MODIFY_TIME,"
                               "UGH_RECORD_TIME"
                               ") "
                               "select "
                               "UG_SQ,"
                               "UG_CODE,"
                               "UG_NAME,"
                               "UG_DESC,"
                               "UG_MANAGER,"
                               "UG_DATE,"
                               "UG_USE,"
                               "UG_FLAG,"
                               "UG_CREATE_TIME,"
                               "UG_MODIFY_TIME,"
                               "sysdate() "
                               "from USER_GROUP_TB "
                               "where UG_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf, "delete from USER_GROUP_TB where UG_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf, "update USER_GROUP_TB set UG_FLAG='S' where UG_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }

        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ",rowCnt, insertCnt, resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);


    return insertCnt;
}
int fschd_MoveHistoryUserGroupLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select UG_SQ_P,UG_SQ_C,UG_FLAG from USER_GROUP_LINK_TB "
                       "where UG_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("USER_GROUP_LINK_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));

            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "UG_SQ_P,"
                               "UG_SQ_C,"
                               "UG_FLAG,"
                               "UGH_RECORD_TIME"
                               ") "
                               "select "
                               "UG_SQ_P,"
                               "UG_SQ_C,"
                               "UG_FLAG,"
                               "sysdate() "
                               "from USER_GROUP_LINK_TB "
                               "where UG_SQ_P=%llu and UG_SQ_C=%llu",
                        histTableName,
                        atoll(g_stMyCon->row[0]),
                        atoll(g_stMyCon->row[1]));

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[2] == 'D')
            {
                sprintf	(sqlBuf,"delete from USER_GROUP_LINK_TB "
                                   "where UG_SQ_P=%llu and UG_SQ_C=%llu",
                            atoll(g_stMyCon->row[0]),atoll(g_stMyCon->row[1]));
            }
            else
            {
                sprintf	(sqlBuf,"update USER_GROUP_LINK_TB set UG_FLAG='S' "
                                   "where UG_SQ_P=%llu and UG_SQ_C=%llu",
                            atoll(g_stMyCon->row[0]),atoll(g_stMyCon->row[1]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryUserTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select US_SQ,US_FLAG from USER_TB "
                       "where US_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("USER_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "US_SQ,"
                               "US_ID,"
                               "US_PASSWD,"
                               "US_NAME,"
                               "US_SNO,"
                               "US_EMAIL,"
                               "US_PHONE,"
                               "US_CELL_PHONE,"
                               "US_IP,"
                               "US_STATE,"
                               "US_DESC,"
                               "US_DB_ROUTER,"
                               "US_LOG_LEVEL,"
                               "US_AUTO_SYNC,"
                               "US_FLAG,"
                               "US_CREATE_TIME,"
                               "US_MODIFY_TIME,"
                               "USH_RECORD_TIME"
                               ") "
                               "select "
                               "US_SQ,"
                               "US_ID,"
                               "US_PASSWD,"
                               "US_NAME,"
                               "US_SNO,"
                               "US_EMAIL,"
                               "US_PHONE,"
                               "US_CELL_PHONE,"
                               "US_IP,"
                               "US_STATE,"
                               "US_DESC,"
                               "US_DB_ROUTER,"
                               "US_LOG_LEVEL,"
                               "US_AUTO_SYNC,"
                               "US_FLAG,"
                               "US_CREATE_TIME,"
                               "US_MODIFY_TIME,"
                               "sysdate() "
                               "from USER_TB "
                               "where US_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0])
                               );

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from USER_TB where US_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update USER_TB set US_FLAG='S' where US_SQ=%llu", atoll(g_stMyCon->row[0]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        } //while
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryUserLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[511 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select UG_SQ,US_SQ,UL_FLAG from USER_LINK_TB "
                       "where UL_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("USER_LINK_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "UG_SQ,"
                               "US_SQ,"
                               "UL_FLAG,"
                               "ULH_RECORD_TIME"
                               ") "
                               "select "
                               "UG_SQ,"
                               "US_SQ,"
                               "UL_FLAG,"
                               "sysdate() "
                               "from USER_LINK_TB "
                               "where UG_SQ=%llu and US_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]),
                               atoll(g_stMyCon->row[1]));

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[2] == 'D')
            {
                sprintf	(sqlBuf,"delete from USER_LINK_TB where UG_SQ=%llu and US_SQ=%llu",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]));
            }
            else
            {
                sprintf	(sqlBuf,"update USER_LINK_TB set UL_FLAG='S' where UG_SQ=%llu and US_SQ=%llu",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryManagerTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select MN_SQ,MN_FLAG from MANAGER_TB "
                       "where MN_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("MANAGER_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "MN_SQ,"
                               "MN_LEVEL,"
                               "MN_ID,"
                               "MN_PW,"
                               "MN_NAME,"
                               "MN_IPADDR,"
                               "MN_EMAIL,"
                               "MN_CELL_PHONE,"
                               "MN_DESC,"
                               "MN_FLAG,"
                               "MN_LOGIN_STATUS,"
                               "MN_FAIL_COUNT,"
                               "MN_EVENT_NOTI,"
                               "MN_CONN_PID,"
                               "MN_CONN_FQ,"
                               "MN_CREATE_TIME,"
                               "MN_MODIFY_TIME,"
                               "MNH_RECORD_TIME"
                               ") "
                               "select "
                               "MN_SQ,"
                               "MN_LEVEL,"
                               "MN_ID,"
                               "MN_PW,"
                               "MN_NAME,"
                               "MN_IPADDR,"
                               "MN_EMAIL,"
                               "MN_CELL_PHONE,"
                               "MN_DESC,"
                               "MN_FLAG,"
                               "MN_LOGIN_STATUS,"
                               "MN_FAIL_COUNT,"
                               "MN_EVENT_NOTI,"
                               "MN_CONN_PID,"
                               "MN_CONN_FQ,"
                               "MN_CREATE_TIME,"
                               "MN_MODIFY_TIME,"
                               "sysdate() "
                               "from MANAGER_TB "
                               "where MN_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from MANAGER_TB where MN_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update MANAGER_TB set MN_FLAG='S' where MN_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryDetectGroupLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf	(sqlBuf,"select DG_SQ_P,DG_SQ_C,DGL_FLAG "
                       "from DETECT_GROUP_LINK_TB "
                       "where DGL_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DETECT_GROUP_LINK_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "DG_SQ_P,"
                               "DG_SQ_C,"
                               "DGL_FLAG,"
                               "DGLH_RECORD_TIME"
                               ") "
                               "select "
                               "DG_SQ_P,"
                               "DG_SQ_C,"
                               "DGL_FLAG,"
                               "sysdate() "
                               "from DETECT_GROUP_LINK_TB "
                               "where DG_SQ_P=%llu and DG_SQ_C=%llu",
                        histTableName,atoll(g_stMyCon->row[0]),atoll(g_stMyCon->row[1]));

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[2] == 'D')
            {
                sprintf	(sqlBuf,"delete from DETECT_GROUP_LINK_TB "
                                   "where DG_SQ_P=%llu and DG_SQ_C=%llu",
                            atoll(g_stMyCon->row[0]),atoll(g_stMyCon->row[1]));
            }
            else
            {
                sprintf	(sqlBuf,"update DETECT_GROUP_LINK_TB "
                                   "set DGL_FLAG='S' "
                                   "where DG_SQ_P=%llu and DG_SQ_C=%llu",
                            atoll(g_stMyCon->row[0]),atoll(g_stMyCon->row[1]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ",  rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryDetectGroupTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select DG_SQ,DG_FLAG from DETECT_GROUP_TB "
                       "where DG_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }


    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DETECT_GROUP_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0) {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "DG_SQ,"
                               "DG_NAME,"
                               "DG_SNO,"
                               "DG_DESC,"
                               "DG_FLAG,"
                               "DGH_RECORD_TIME"
                               ") "
                               "select "
                               "DG_SQ,"
                               "DG_NAME,"
                               "DG_SNO,"
                               "DG_DESC,"
                               "DG_FLAG,"
                               "sysdate() "
                               "from DETECT_GROUP_TB "
                               "where DG_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from DETECT_GROUP_TB "
                                   "where DG_SQ=%llu",
                                   atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update DETECT_GROUP_TB "
                                   "set DG_FLAG='S' where DG_SQ=%llu",
                                   atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistorydetectLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select DG_SQ,OBJECT_SQ,OBJECT_TYPE,DL_FLAG from DETECT_LINK_TB "
                       "where DL_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DETECT_LINK_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "DG_SQ,"
                               "OBJECT_SQ,"
                               "OBJECT_TYPE,"
                               "DL_FLAG,"
                               "DLH_RECORD_TIME"
                               ") "
                               "select "
                               "DG_SQ,"
                               "OBJECT_SQ,"
                               "OBJECT_TYPE,"
                               "DL_FLAG,"
                               "sysdate() "
                               "from DETECT_LINK_TB "
                               "where DG_SQ=%llu and OBJECT_SQ=%llu and OBJECT_TYPE=%d",
                        histTableName,
                        atoll(g_stMyCon->row[0]),
                        atoll(g_stMyCon->row[1]),
                        atoi(g_stMyCon->row[2]));

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[3] == 'D')
            {
                sprintf	(sqlBuf,"delete from DETECT_LINK_TB "
                                   "where DG_SQ=%llu and OBJECT_SQ=%llu and OBJECT_TYPE=%d",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]),
                            atoi(g_stMyCon->row[2]));
            }
            else
            {
                sprintf	(sqlBuf,"update DETECT_LINK_TB "
                                   "set DL_FLAG='S' "
                                   "where DG_SQ=%llu and OBJECT_SQ=%llu and OBJECT_TYPE=%d",
                            atoll(g_stMyCon->row[0]),
                            atoll(g_stMyCon->row[1]),
                            atoi(g_stMyCon->row[2]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt, insertCnt, resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryDetectProcessTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select DP_SQ,DP_FLAG from DETECT_PROCESS_TB "
                       "where DP_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DETECT_PROCESS_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "DP_SQ,"
                               "DP_PROCESS_NAME,"
                               "DP_DETAIL_INFO,"
                               "DP_HASH,"
                               "DP_DESC,"
                               "DP_USE,"
                               "DP_FLAG,"
                               "DPH_RECORD_TIME"
                               ") "
                               "select "
                               "DP_SQ,"
                               "DP_PROCESS_NAME,"
                               "DP_DETAIL_INFO,"
                               "DP_HASH,"
                               "DP_DESC,"
                               "DP_USE,"
                               "DP_FLAG,"
                               "sysdate() "
                               "from DETECT_PROCESS_TB "
                               "where DP_SQ='%llu'",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from DETECT_PROCESS_TB "
                                   "where DP_SQ='%llu'", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update DETECT_PROCESS_TB set DP_FLAG='S' "
                                   "where DP_SQ ='%llu'", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryWatchServerTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select WS_SQ,WS_FLAG from WATCH_SERVER_TB "
                       "where WS_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("WATCH_SERVER_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "WS_SQ,"
                               "WS_SVR,"
                               "WS_DESC,"
                               "WS_USE,"
                               "WS_FLAG,"
                               "WSH_RECORD_TIME"
                               ") "
                               "select "
                               "WS_SQ,"
                               "WS_SVR,"
                               "WS_DESC,"
                               "WS_USE,"
                               "WS_FLAG,"
                               "sysdate() "
                               "from WATCH_SERVER_TB "
                               "where WS_SQ='%llu'",
                               histTableName,atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from WATCH_SERVER_TB "
                                   "where WS_SQ='%llu'", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update WATCH_SERVER_TB set WS_FLAG='S' "
                                   "where WS_SQ='%llu'", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB,	"Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);


    return insertCnt;
}

int fschd_MoveHistoryDetectPrinterPortTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select PP_PORT,PP_FLAG from DETECT_PRINTER_PORT_TB "
                       "where PP_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    //LogDRet(5, "- rowCnt(%d)\n", STR_DEBUG,rowCnt);

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DETECT_PRINTER_PORT_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "PP_PORT,"
                               "PP_DESC,"
                               "PP_USE,"
                               "PP_FLAG,"
                               "PPH_RECORD_TIME"
                               ") "
                               "select "
                               "PP_PORT,"
                               "PP_DESC,"
                               "PP_USE,"
                               "PP_FLAG,"
                               "sysdate() "
                               "from DETECT_PRINTER_PORT_TB "
                               "where PP_PORT=%d",
                               histTableName,atoi(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from DETECT_PRINTER_PORT_TB where PP_PORT=%d", atoi(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update DETECT_PRINTER_PORT_TB set PP_FLAG='S' where PP_PORT=%d", atoi(g_stMyCon->row[0]));
            }

            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryManagerEventTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select MNE_SQ,MNE_FLAG from MANAGER_EVENT_TB "
                       "where MNE_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("MANAGER_EVENT_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "MNE_SQ,"
                               "MN_SQ,"
                               "MNE_ACTION_TYPE,"
                               "MNE_ACTION_SQ,"
                               "MNE_IPADDR,"
                               "MNE_EMAIL,"
                               "MNE_CELL_PHONE,"
                               "MNE_RECORD_TIME,"
                               "MNE_SUMMARY,"
                               "MNE_FLAG,"
                               "MNEH_RECORD_TIME"
                               ") "
                               "select "
                               "MNE_SQ,"
                               "MN_SQ,"
                               "MNE_ACTION_TYPE,"
                               "MNE_ACTION_SQ,"
                               "MNE_IPADDR,"
                               "MNE_EMAIL,"
                               "MNE_CELL_PHONE,"
                               "MNE_RECORD_TIME,"
                               "MNE_SUMMARY,"
                               "MNE_FLAG,"
                               "sysdate() "
                               "from MANAGER_EVENT_TB "
                               "where MNE_SQ=%llu",
                               histTableName,atoll(g_stMyCon->row[0]));

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from MANAGER_EVENT_TB where MNE_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update MANAGER_EVENT_TB set MNE_FLAG='S' where MNE_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DEBUG, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryEventTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1023 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select EV_SQ from EVENT_TB where EV_EXIST=3 limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("EVENT_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0) {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "EV_SQ,"
                               "US_SQ,"
                               "HB_SQ,"
                               "RU_SQ,"
                               "EV_IPADDR,"
                               "EV_TYPE,"
                               "EV_LEVEL,"
                               "EV_EXIST,"
                               "EV_EVENT_CONTEXT,"
                               "EV_DETECT_TIME,"
                               "EV_RECORD_TIME,"
                               "EVH_RECORD_TIME"
                               ") "
                               "select "
                               "EV_SQ,"
                               "US_SQ,"
                               "HB_SQ,"
                               "RU_SQ,"
                               "EV_IPADDR,"
                               "EV_TYPE,"
                               "EV_LEVEL,"
                               "EV_EXIST,"
                               "EV_EVENT_CONTEXT,"
                               "EV_DETECT_TIME,"
                               "EV_RECORD_TIME,"
                               "sysdate() "
                               "from EVENT_TB "
                               "where EV_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"delete from EVENT_TB where EV_SQ=%llu",
                    atoll(g_stMyCon->row[0]));
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);
                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

int fschd_MoveHistoryAlarmTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[1024 +1] = {0x00,};


    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select AL_SQ,AL_FLAG from ALARM_TB "
                       "where AL_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("ALARM_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "AL_SQ,"
                               "MN_SQ,"
                               "AL_DETECT_TYPE,"
                               "AL_DETECT_LEVEL,"
                               "AL_USE,"
                               "AL_FLAG,"
                               "AL_RECORD_TIME,"
                               "ALH_RECORD_TIME"
                               ") "
                               "select "
                               "AL_SQ,"
                               "MN_SQ,"
                               "AL_DETECT_TYPE,"
                               "AL_DETECT_LEVEL,"
                               "AL_USE,"
                               "AL_FLAG,"
                               "AL_RECORD_TIME,"
                               "sysdate() "
                               "from ALARM_TB "
                               "where AL_SQ=%llu",
                               histTableName,atoll(g_stMyCon->row[0]));
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from ALARM_TB where AL_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update ALARM_TB set AL_FLAG='S' where AL_SQ=%llu",
                            atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }

    fdb_SqlFreeResult(g_stMyCon);


    return insertCnt;
}


int fschd_MoveHistoryDiskRegLinkTb()
{
    int				rowCnt = 0;
    int				insertCnt = 0;
    int				resCnt = 0;
    char			histTableName[50 +1] = {0x00,};
    char			sqlBuf[512 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select DR_SQ,DR_FLAG from DISK_REG_LINK_TB "
                       "where DR_FLAG in ('A','M','D') limit %d", g_nSafeFetchRow);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        fdb_GetHistoryTableName("DISK_REG_LINK_TB", histTableName, "", "");

        WRITE_INFO(CATEGORY_DB, "histTableName(%s) ", histTableName );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into %s "
                               "("
                               "DR_SQ,"
                               "HB_SQ,"
                               "DK_PHYSICAL_SN,"
                               "DR_REG,"
                               "DR_FLAG,"
                               "DRH_RECORD_TIME"
                               ") "
                               "select "
                               "DR_SQ,"
                               "HB_SQ,"
                               "DK_PHYSICAL_SN,"
                               "DR_REG,"
                               "DR_FLAG,"
                               "sysdate() "
                               "from DISK_REG_LINK_TB "
                               "where DR_SQ=%llu",
                               histTableName,
                               atoll(g_stMyCon->row[0]) );
            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(*g_stMyCon->row[1] == 'D')
            {
                sprintf	(sqlBuf,"delete from DISK_REG_LINK_TB where DR_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            else
            {
                sprintf	(sqlBuf,"update DISK_REG_LINK_TB set DR_FLAG='S' where DR_SQ=%llu", atoll(g_stMyCon->row[0]));
            }
            resCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
        }
        WRITE_INFO(CATEGORY_DB, "Proc row(%d)insert(%d)res(%d) ", rowCnt,insertCnt,resCnt);
    }
    fdb_SqlFreeResult(g_stMyCon);

    return insertCnt;
}

void fschd_GetMoveSyncSql(char *tbName, char *histName, char *buf)
{
    char	currDate[19 +1] = {0x00,};
    time_t	currTime = time((time_t) 0);

    memset	(currDate, 0x00, sizeof(currDate));

    fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss\0");

    if (!strcmp(tbName, "SYNC_GROUP_TB")) // group
    {
        sprintf	(buf,	"insert into %s "
                            "("
                            "DEPT_CODE,"
                            "DEPT_NAME,"
                            "DEPT_DESC,"
                            "UPDEPT_CODE,"
                            "DEPT_DATE,"
                            "SGH_RECORD_TIME"
                            ") ("
                            "select "
                            "DEPT_CODE,"
                            "DEPT_NAME,"
                            "DEPT_DESC,"
                            "UPDEPT_CODE,"
                            "DEPT_DATE,"
                            "'%s' "
                            "from %s)", histName,currDate,tbName);
    }
    else if (!strcmp(tbName, "SYNC_USER_TB")) // user
    {
        sprintf	(buf,	"insert into %s "
                            "("
                            "EMN,"
                            "AEMP_NM,"
                            "BJURIS_BRCD,"
                            "CBENM,"
                            "PRS_BE_YMD,"
                            "MWK_BRCD,"
                            "GBENM,"
                            "BE_TEAM_CD,"
                            "BBENM,"
                            "BJURIS_HQCD,"
                            "EBENM,"
                            "MNDT_CD,"
                            "JBCL_CD,"
                            "JBTT_CD,"
                            "ABNM_JTM,"
                            "DUCD,"
                            "ETBN_DCD,"
                            "EMP_CPN,"
                            "EAD,"
                            "RSNO,"
                            "ENBK_YMD,"
                            "RETI_YMD,"
                            "SEUMU,"
                            "CHUL,"
                            "SUH_RECORD_TIME"
                            ") ("
                            "select "
                            "EMN,"
                            "AEMP_NM,"
                            "BJURIS_BRCD,"
                            "CBENM,"
                            "PRS_BE_YMD,"
                            "MWK_BRCD,"
                            "GBENM,"
                            "BE_TEAM_CD,"
                            "BBENM,"
                            "BJURIS_HQCD,"
                            "EBENM,"
                            "MNDT_CD,"
                            "JBCL_CD,"
                            "JBTT_CD,"
                            "ABNM_JTM,"
                            "DUCD,"
                            "ETBN_DCD,"
                            "EMP_CPN,"
                            "EAD,"
                            "RSNO,"
                            "ENBK_YMD,"
                            "RETI_YMD,"
                            "SEUMU,"
                            "CHUL,"
                            "'%s' "
                            "from %s)", histName,currDate,tbName);
    }
}

int fschd_LoadFileSyncTb(int flag, char *filePath)
{
    int		fileRowCnt = 0;
    int     nRet = 0;
    char	syncTBName[30 +1] = {0x00,};
    char	sqlBuf[1024 +1] = {0x00,};
    char	histTableName[50 +1] = {0x00,};
    char    szTemp[256 +1] = {0x00,};

    memset(syncTBName, 0x00, sizeof(syncTBName));
    memset(szTemp, 0x00, sizeof(szTemp));

    if 		(flag == 1) strcpy(syncTBName, "SYNC_GROUP_TB");
    else if (flag == 2) strcpy(syncTBName, "SYNC_USER_TB");

    else 				return -1;

    WRITE_INFO(CATEGORY_DEBUG,"SyncTB Name : (%s)",syncTBName );
    WRITE_INFO(CATEGORY_DEBUG,"File Path (%s) ",filePath );
    fileRowCnt = fcom_GetFileRows(filePath);
    WRITE_DEBUG(CATEGORY_DEBUG,"File Row Cnt : (%d) ",fileRowCnt );

    if (fileRowCnt > 0)
    {
        // get local sync group count
        fdb_GetHistoryTableName(syncTBName, histTableName, "", "");

        WRITE_INFO(CATEGORY_INFO,"Try copy to history %s from %s",histTableName,syncTBName );
        // copy to local history table
        memset(sqlBuf, 0x00, sizeof(sqlBuf));

        fschd_GetMoveSyncSql(syncTBName, histTableName, sqlBuf);
        fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in query, errmsg(%s)",
                           g_stMyCon->cpErrMsg );
        }

        WRITE_DEBUG(CATEGORY_DEBUG,"Try truncate tb table %s",syncTBName );

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf, "truncate table %s", syncTBName);
        fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in query, errmsg(%s)",g_stMyCon->cpErrMsg );
            WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                 "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        }

        /* ���� Parsing �� SYNC USER/GROUP Insert ó��.*/
        snprintf(szTemp, sizeof(szTemp), "%s/%s %d",
                 getenv("DAP_HOME"),
                 "/bin/dap_dir syncdir",
                 flag);

        WRITE_DEBUG(CATEGORY_INFO,"Execute Sync Process (%s) ",szTemp );
        nRet = system(szTemp);
    }
    return nRet;
}

int fschd_SetLocalSyncGroupLinkTb()
{
    int		rowCnt = 0;
    int		mergeCnt = 0;
    char    sqlBuf[512 +1] = {0x00,};
    unsigned long long pSq = 0;
    unsigned long long cSq = 0;

    WRITE_INFO(CATEGORY_DB, " start ");

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    strcpy	(sqlBuf,"select "
                      "B.UG_SQ as UG_SQ_P, B.UPDEPT_CODE, A.UG_SQ as UG_SQ_C, B.DEPT_CODE "
                      "from USER_GROUP_TB A, "
                      "("
                      "	select "
                      "	y.UG_SQ as UG_SQ, x.DEPT_CODE as DEPT_CODE, x.UPDEPT_CODE as UPDEPT_CODE "
                      "	from SYNC_GROUP_TB x "
                      "	left join USER_GROUP_TB y on x.UPDEPT_CODE=y.UG_CODE "
                      ") B "
                      "where A.UG_CODE = B.DEPT_CODE");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "rowCnt(%d) ", rowCnt );

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // higher-dept
            if(g_stMyCon->row[0] != NULL) 	pSq = atol(g_stMyCon->row[0]);
            else
            {
                // ibk������࿡�� �����, '9999'�� ibk������� �ֻ����ڵ�
                if (!strcmp(g_stMyCon->row[1], "9999")) pSq = 0;
                else
                {
                    WRITE_INFO(CATEGORY_DB, "Not found dept(%s) info ", g_stMyCon->row[1]);
                    continue;
                }
            }
            // sub-dept
            if(g_stMyCon->row[2] != NULL)
            {
                cSq = atol(g_stMyCon->row[2]);
            }
            else
            {
                cSq = 0;
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into USER_GROUP_LINK_TB "
                               "("
                               "UG_SQ_P,"
                               "UG_SQ_C,"
                               "UG_FLAG"
                               ") values( "
                               "%llu,"
                               "%llu,"
                               "'A'"
                               ") "
                               "on duplicate key update "
                               "UG_SQ_P=%llu,"
                               "UG_SQ_C=%llu,"
                               "UG_FLAG='M'",
                        pSq,
                        cSq,
                        pSq,
                        cSq
            );

            mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "Proc row(%d)merge(%d) ", rowCnt,mergeCnt);

    return mergeCnt;
}
