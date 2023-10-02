//
// Created by KimByoungGook on 2020-06-23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_checkdb.h"
#include "db/dap_trandb.h"

#include "com/dap_def.h"
#include "com/dap_com.h"


int fdb_InsertServerErrorlogHistory(_DAP_DB_SERVERLOG_INFO* p_dbLog, char *histTableName)
{
    int   insertCnt = 0;
    char  sqlBuf[1536 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,
                "insert into %s "
                "( "
                "IP, "
                "PID, "
                "PROCESS, "
                "LOGDATE, "
                "LOGLEVEL, "
                "LOGMSG "
                ") values ("
                "'%s',"
                "%d,"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s' "
                " ) ",
                histTableName,
                p_dbLog->logip,
                p_dbLog->pid,
                p_dbLog->process,
                p_dbLog->logdate,
                p_dbLog->loglevel,
                p_dbLog->logmsg);

    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Proc (%s) insertCnt(%d)", histTableName, insertCnt);

    return 0;
}

int fdb_GetGroupName(int p_grpSq, char *p_grpStr)
{
    char    sqlBuf[128] = {0x00,};
    int		rowCnt = 0;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf, "select UG_NAME from USER_GROUP_TB where UG_SQ=%d", p_grpSq);

    WRITE_INFO(CATEGORY_DB,"sqlBuf(%s)",sqlBuf);
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)	strcpy(p_grpStr, g_stMyCon->row[0]);
        }
    }
    else
    {
        strcpy(p_grpStr, "Group 미등록");
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"Proc rowCnt(%d)p_grpStr(%s)",
            rowCnt,
            p_grpStr );

    return rowCnt;
}



int fdb_GetSubGroupSq(unsigned long long p_grpSq, char* result)
{
    int 	rowCnt = 0;
    char	sqlBuf[256];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select UG_SQ from USER_GROUP_TB where UG_SQ in ("
                    "	select UG_SQ_C from USER_GROUP_LINK_TB where UG_SQ_P=%llu"
                    ")", p_grpSq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        int loop = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(loop == 0)
                    sprintf(result, "%llu", atoll(g_stMyCon->row[0]));
                else
                    sprintf(result+strlen(result), ",%llu", atoll(g_stMyCon->row[0]));
            }
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO,"Proc rowCnt(%d)",rowCnt);

    return rowCnt;
}


int fdb_InsertReportHistoryTb(char *gubun, char *p_histTableName)
{
    int   insertCnt;
    char  sqlBuf[2047 +1];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    if(!strcmp(gubun, "day"))
    {
        sprintf	(sqlBuf,
                    "insert into %s"
                    "("
                    "STD_SQ,"
                    "STD_RECORD_TIME,"
                    "STD_IPADDR,"
                    "STD_US_SQ,"
                    "STD_UG_SQ,"
                    "STD_TYPE,"
                    "STD_LEVEL,"
                    "STD_EXIST,"
                    "STD_COUNT,"
                    "STDH_RECORD_TIME"
                    ") "
                    "select "
                    "STD_SQ,"
                    "STD_RECORD_TIME,"
                    "STD_IPADDR,"
                    "STD_US_SQ,"
                    "STD_UG_SQ,"
                    "STD_TYPE,"
                    "STD_LEVEL,"
                    "STD_EXIST,"
                    "STD_COUNT,"
                    "sysdate() "
                    "from STD_EVENT_TB as T "
                    "where date_format(STD_RECORD_TIME, '%%Y-%%m-%%d') "
                    "= date_format(curdate() - interval 1 day, '%%Y-%%m-%%d') "
                    "on duplicate key update \n"
                    "STD_RECORD_TIME = T.STD_RECORD_TIME,\n"
                    "STD_IPADDR = T.STD_IPADDR,\n"
                    "STD_US_SQ = T.STD_US_SQ,\n"
                    "STD_UG_SQ = T.STD_UG_SQ,\n"
                    "STD_TYPE = T.STD_TYPE,\n"
                    "STD_LEVEL = T.STD_LEVEL,\n"
                    "STD_EXIST = T.STD_EXIST,\n"
                    "STD_COUNT = T.STD_COUNT",
                    p_histTableName
        );
    }
    else if(!strcmp(gubun, "week"))
    {
        sprintf	(sqlBuf,
             "insert into %s"
                    "("
                    "STW_SQ,"
                    "STW_RECORD_TIME,"
                    "STW_IPADDR,"
                    "STW_US_SQ,"
                    "STW_UG_SQ,"
                    "STW_TYPE,"
                    "STW_LEVEL,"
                    "STW_EXIST,"
                    "STW_COUNT,"
                    "STWH_RECORD_TIME"
                    ") "
                    "select "
                    "STW_SQ,"
                    "STW_RECORD_TIME,"
                    "STW_IPADDR,"
                    "STW_US_SQ,"
                    "STW_UG_SQ,"
                    "STW_TYPE,"
                    "STW_LEVEL,"
                    "STW_EXIST,"
                    "STW_COUNT,"
                    "sysdate() "
                    "from STW_EVENT_TB as T "
                    "where STW_RECORD_TIME = "
                    "CONCAT( date_format(CURDATE() - interval 7 day,'%%Y-%%m'),'-',"
                    "floor( (date_format(CURDATE() - interval 7 day,'%%d') +"
                    "(date_format(date_format(CURDATE() - interval 7 day,'%%Y%%m%%01'),'%%w')-1))/7)+1, 'W') "
                    "on duplicate key update \n"
                    "STW_RECORD_TIME = T.STW_RECORD_TIME,\n"
                    "STW_IPADDR = T.STW_IPADDR,\n"
                    "STW_US_SQ = T.STW_US_SQ,\n"
                    "STW_UG_SQ = T.STW_UG_SQ,\n"
                    "STW_TYPE = T.STW_TYPE,\n"
                    "STW_LEVEL = T.STW_LEVEL,\n"
                    "STW_EXIST = T.STW_EXIST,\n"
                    "STW_COUNT = T.STW_COUNT",
                    p_histTableName
        );
    }
    else if(!strcmp(gubun, "month"))
    {
        sprintf	(sqlBuf,
                    "insert into %s"
                    "("
                    "STM_SQ,"
                    "STM_RECORD_TIME,"
                    "STM_IPADDR,"
                    "STM_US_SQ,"
                    "STM_UG_SQ,"
                    "STM_TYPE,"
                    "STM_LEVEL,"
                    "STM_EXIST,"
                    "STM_COUNT,"
                    "STMH_RECORD_TIME"
                    ") "
                    "select "
                    "STM_SQ,"
                    "STM_RECORD_TIME,"
                    "STM_IPADDR,"
                    "STM_US_SQ,"
                    "STM_UG_SQ,"
                    "STM_TYPE,"
                    "STM_LEVEL,"
                    "STM_EXIST,"
                    "STM_COUNT,"
                    "sysdate() "
                    "from STM_EVENT_TB as T "
                    "where STM_RECORD_TIME "
                    "= date_format(CURDATE() - interval 1 month, '%%Y-%%m') "
                    "on duplicate key update \n"
                    "STM_RECORD_TIME = T.STM_RECORD_TIME,\n"
                    "STM_IPADDR = T.STM_IPADDR,\n"
                    "STM_US_SQ = T.STM_US_SQ,\n"
                    "STM_UG_SQ = T.STM_UG_SQ,\n"
                    "STM_TYPE = T.STM_TYPE,\n"
                    "STM_LEVEL = T.STM_LEVEL,\n"
                    "STM_EXIST = T.STM_EXIST,\n"
                    "STM_COUNT = T.STM_COUNT",
                    p_histTableName
        );
    }



    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Proc insertCnt(%d)",
            insertCnt);

    return 0;
}

int fdb_InsertHwBaseHistory(char *p_userKey, char *histTableName)
{
    int   insertCnt;
    char  sqlBuf[511 +1];

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,
                "insert into %s "
                "( "
                "US_SQ, "
                "HB_SQ, "
                "HB_UNQ, "
                "HB_MB_PN, "
                "HB_MB_MF, "
                "HB_MB_SN, "
                "HB_FIRST_TIME, "
                "HB_ACCESS_IP, "
                "HB_ACCESS_MAC, "
                "HB_AGENT_VER, "
                "HB_EXTERNAL, "
                "HB_ACCESS_TIME, "
                "HB_RECORD_TIME, "
                "HB_DEL, "
                "HBH_RECORD_TIME "
                ") "
                "select "
                "US_SQ, "
                "HB_SQ, "
                "HB_UNQ, "
                "HB_MB_PN, "
                "HB_MB_MF, "
                "HB_MB_SN, "
                "HB_FIRST_TIME, "
                "HB_ACCESS_IP, "
                "HB_ACCESS_MAC, "
                "HB_AGENT_VER, "
                "HB_EXTERNAL, "
                "HB_ACCESS_TIME, "
                "HB_RECORD_TIME, "
                "HB_DEL, "
                "sysdate() "
                "from HW_BASE_TB "
                "where HB_UNQ='%s'",
                histTableName,
                p_userKey
    );

    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)userKey(%s)",
            insertCnt,
            p_userKey);

    return 0;
}

int fdb_InsertUserHistory(unsigned long long p_sq, char *histTableName)
{
    int   insertCnt;
    char  sqlBuf[512];

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
                       "where US_SQ=%llu", histTableName,p_sq);

    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)sq(%llu)",
            insertCnt,
            p_sq);

    return 0;
}

int fdb_InsertHistory(unsigned long long p_tbSq, char *p_tableName, char *p_histTableName)
{
    int   insertCnt;
    char  sqlBuf[2047 +1];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    if(!strcasecmp(p_tableName, "SYSTEM_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "ST_SQ,"
                "ST_NAME,"
                "ST_DNS_HOST_NAME,"
                "ST_DOMAIN,"
                "ST_DOMAIN_ROLE,"
                "ST_TIME_ZONE,"
                "ST_WAKEUP_TYPE,"
                "ST_WORK_GROUP,"
                "ST_BOOTUP_STATE,"
                "ST_MEMORY,"
                "ST_VGA,"
                "ST_VM_NAME,"
                "ST_INSTALLED_VM,"
                "ST_DETECT_TIME,"
                "ST_PREV_HIST_SQ,"
                "ST_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "STH_RECORD_TIME "
                ") "
                "select "
                "ST_SQ,"
                "ST_NAME,"
                "ST_DNS_HOST_NAME,"
                "ST_DOMAIN,"
                "ST_DOMAIN_ROLE,"
                "ST_TIME_ZONE,"
                "ST_WAKEUP_TYPE,"
                "ST_WORK_GROUP,"
                "ST_BOOTUP_STATE,"
                "ST_MEMORY,"
                "ST_VGA,"
                "ST_VM_NAME,"
                "ST_INSTALLED_VM,"
                "ST_DETECT_TIME,"
                "ST_PREV_HIST_SQ,"
                "ST_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where ST_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "CONNECT_EXT_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "CE_SQ,"
                "CE_URL,"
                "CE_CONNECTED,"
                "CE_DETECT_TIME,"
                "CE_PREV_HIST_SQ,"
                "CE_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "CEH_RECORD_TIME"
                ") "
                "select "
                "CE_SQ,"
                "CE_URL,"
                "CE_CONNECTED,"
                "CE_DETECT_TIME,"
                "CE_PREV_HIST_SQ,"
                "CE_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where CE_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "OS_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "OS_SQ,"
                "OS_NAME,"
                "OS_VERSION,"
                "OS_ARCHITECTURE,"
                "OS_LANG,"
                "OS_TYPE,"
                "OS_PORTABLE,"
                "OS_SP_MAJOR_VER,"
                "OS_SP_MINOR_VER,"
                "OS_DETECT_TIME,"
                "OS_PREV_HIST_SQ,"
                "OS_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "OSH_RECORD_TIME "
                ") "
                "select "
                "OS_SQ,"
                "OS_NAME,"
                "OS_VERSION,"
                "OS_ARCHITECTURE,"
                "OS_LANG,"
                "OS_TYPE,"
                "OS_PORTABLE,"
                "OS_SP_MAJOR_VER,"
                "OS_SP_MINOR_VER,"
                "OS_DETECT_TIME,"
                "OS_PREV_HIST_SQ,"
                "OS_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where OS_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "CPU_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "CU_SQ,"
                "CU_NAME,"
                "CU_MF,"
                "CU_DESC,"
                "CU_PID,"
                "CU_DETECT_TIME,"
                "CU_PREV_HIST_SQ,"
                "CU_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "CUH_RECORD_TIME"
                ") "
                "select "
                "CU_SQ,"
                "CU_NAME,"
                "CU_MF,"
                "CU_DESC,"
                "CU_PID,"
                "CU_DETECT_TIME,"
                "CU_PREV_HIST_SQ,"
                "CU_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where CU_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "NET_ADAPTER_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "NA_SQ,"
                "NA_NAME,"
                "NA_PN,"
                "NA_DESC,"
                "NA_DEVICE_TYPE,"
                "NA_IPV4,"
                "NA_IPV6,"
                "NA_MAC,"
                "NA_SUBNET,"
                "NA_DEFAULT_GW,"
                "NA_DEFAULT_GW_MAC,"
                "NA_PREF_DNS,"
                "NA_ALTE_DNS,"
                "NA_NET_CONNECTION_ID,"
                "NA_NET_CONNECTION_STATUS,"
                "NA_NET_ENABLED,"
                "NA_PHYSICAL_ADAPTER,"
                "NA_PNP_DEVICE_ID,"
                "NA_SERVICE_NAME,"
                "NA_DETECT_TIME,"
                "NA_PREV_HIST_SQ,"
                "NA_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "NAH_RECORD_TIME "
                ") "
                "select "
                "NA_SQ,"
                "NA_NAME,"
                "NA_PN,"
                "NA_DESC,"
                "NA_DEVICE_TYPE,"
                "NA_IPV4,"
                "NA_IPV6,"
                "NA_MAC,"
                "NA_SUBNET,"
                "NA_DEFAULT_GW,"
                "NA_DEFAULT_GW_MAC,"
                "NA_PREF_DNS,"
                "NA_ALTE_DNS,"
                "NA_NET_CONNECTION_ID,"
                "NA_NET_CONNECTION_STATUS,"
                "NA_NET_ENABLED,"
                "NA_PHYSICAL_ADAPTER,"
                "NA_PNP_DEVICE_ID,"
                "NA_SERVICE_NAME,"
                "NA_DETECT_TIME,"
                "NA_PREV_HIST_SQ,"
                "NA_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where NA_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "WIFI_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "WF_SQ,"
                "WF_INTERFACE_STATUS,"
                "WF_INTERFACE_DESC,"
                "WF_CONNECTION_MODE,"
                "WF_PROFILE_NAME,"
                "WF_SSID,"
                "WF_BSS_NETWORK_TYPE,"
                "WF_MAC_ADDR,"
                "WF_PHY_NETWORK_TYPE,"
                "WF_SECURITY,"
                "WF_8021X,"
                "WF_AUTH_ALGO,"
                "WF_CIPHER_ALGO,"
                "WF_DETECT_TIME,"
                "WF_PREV_HIST_SQ,"
                "WF_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "WFH_RECORD_TIME "
                ") "
                "select "
                "WF_SQ,"
                "WF_INTERFACE_STATUS,"
                "WF_INTERFACE_DESC,"
                "WF_CONNECTION_MODE,"
                "WF_PROFILE_NAME,"
                "WF_SSID,"
                "WF_BSS_NETWORK_TYPE,"
                "WF_MAC_ADDR,"
                "WF_PHY_NETWORK_TYPE,"
                "WF_SECURITY,"
                "WF_8021X,"
                "WF_AUTH_ALGO,"
                "WF_CIPHER_ALGO,"
                "WF_DETECT_TIME,"
                "WF_PREV_HIST_SQ,"
                "WF_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where WF_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "BLUETOOTH_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "BT_SQ,"
                "BT_INSTANCE_NAME,"
                "BT_MAC_ADDR,BT_DEVICE,"
                "BT_MINOR_DEVICE,"
                "BT_DANGER,"
                "BT_CONNECTED,"
                "BT_AUTHENTICATED,"
                "BT_REMEMBERED,"
                "BT_DETECT_TIME,"
                "BT_PREV_HIST_SQ,"
                "BT_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "BTH_RECORD_TIME "
                ") "
                "select "
                "BT_SQ,"
                "BT_INSTANCE_NAME,"
                "BT_MAC_ADDR,BT_DEVICE,"
                "BT_MINOR_DEVICE,"
                "BT_DANGER,"
                "BT_CONNECTED,"
                "BT_AUTHENTICATED,"
                "BT_REMEMBERED,"
                "BT_DETECT_TIME,"
                "BT_PREV_HIST_SQ,"
                "BT_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where BT_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "NET_CONNECTION_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "NC_SQ,"
                "NC_LOCAL_NAME,"
                "NC_CONNECTION_STATE,"
                "NC_CONNECTION_TYPE,"
                "NC_DESC,"
                "NC_PROVIDER_NAME,"
                "NC_REMOTE_NAME,"
                "NC_REMOTE_PATH,"
                "NC_DISPLAY_TYPE,"
                "NC_RESOURCE_TYPE,"
                "NC_DETECT_TIME,"
                "NC_PREV_HIST_SQ,"
                "NC_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "NCH_RECORD_TIME "
                ") "
                "select "
                "NC_SQ,"
                "NC_LOCAL_NAME,"
                "NC_CONNECTION_STATE,"
                "NC_CONNECTION_TYPE,"
                "NC_DESC,"
                "NC_PROVIDER_NAME,"
                "NC_REMOTE_NAME,"
                "NC_REMOTE_PATH,"
                "NC_DISPLAY_TYPE,"
                "NC_RESOURCE_TYPE,"
                "NC_DETECT_TIME,"
                "NC_PREV_HIST_SQ,"
                "NC_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where NC_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "DISK_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "DK_SQ,"
                "DK_NAME,"
                "DK_DRIVE_TYPE,"
                "DK_FILE_SYSTEM,"
                "DK_ACCESS,"
                "DK_VOLUME_NAME,"
                "DK_VOLUME_SN,"
                "DK_DESC,"
                "DK_PHYSICAL_SN,"
                "DK_INTERFACE_TYPE,"
                "DK_MF,"
                "DK_MODEL,"
                "DK_DETECT_TIME,"
                "DK_PREV_HIST_SQ,"
                "DK_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "DKH_RECORD_TIME "
                ") "
                "select "
                "DK_SQ,"
                "DK_NAME,"
                "DK_DRIVE_TYPE,"
                "DK_FILE_SYSTEM,"
                "DK_ACCESS,"
                "DK_VOLUME_NAME,"
                "DK_VOLUME_SN,"
                "DK_DESC,"
                "DK_PHYSICAL_SN,"
                "DK_INTERFACE_TYPE,"
                "DK_MF,"
                "DK_MODEL,"
                "DK_DETECT_TIME,"
                "DK_PREV_HIST_SQ,"
                "DK_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where DK_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "NET_DRIVE_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "ND_SQ,"
                "ND_DRIVE_NAME,"
                "ND_USER_NAME,"
                "ND_CONNECTION_TYPE,"
                "ND_DEFER_FLAGS,"
                "ND_PROVIDER_NAME,"
                "ND_PROVIDER_TYPE,"
                "ND_REMOTE_PATH,"
                "ND_DETECT_TIME,"
                "ND_PREV_HIST_SQ,"
                "ND_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "NDH_RECORD_TIME"
                ") "
                "select "
                "ND_SQ,"
                "ND_DRIVE_NAME,"
                "ND_USER_NAME,"
                "ND_CONNECTION_TYPE,"
                "ND_DEFER_FLAGS,"
                "ND_PROVIDER_NAME,"
                "ND_PROVIDER_TYPE,"
                "ND_REMOTE_PATH,"
                "ND_DETECT_TIME,"
                "ND_PREV_HIST_SQ,"
                "ND_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where ND_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "OS_ACCOUNT_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "OA_SQ,"
                "OA_TYPE,"
                "OA_NAME,"
                "OA_LOCAL,"
                "OA_SID,"
                "OA_SID_TYPE,"
                "OA_CAPTION,"
                "OA_DESC,"
                "OA_DISABLED,"
                "OA_STATUS,"
                "OA_DETECT_TIME,"
                "OA_PREV_HIST_SQ,"
                "OA_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "OAH_RECORD_TIME "
                ") "
                "select "
                "OA_SQ,"
                "OA_TYPE,"
                "OA_NAME,"
                "OA_LOCAL,"
                "OA_SID,"
                "OA_SID_TYPE,"
                "OA_CAPTION,"
                "OA_DESC,"
                "OA_DISABLED,"
                "OA_STATUS,"
                "OA_DETECT_TIME,"
                "OA_PREV_HIST_SQ,"
                "OA_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where OA_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "SHARE_FOLDER_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "SF_SQ,"
                "SF_NAME,"
                "SF_TYPE,"
                "SF_PATH,"
                "SF_STATUS,"
                "SF_DETECT_TIME,"
                "SF_PREV_HIST_SQ,"
                "SF_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "SFH_RECORD_TIME "
                ") "
                "select "
                "SF_SQ,"
                "SF_NAME,"
                "SF_TYPE,"
                "SF_PATH,"
                "SF_STATUS,"
                "SF_DETECT_TIME,"
                "SF_PREV_HIST_SQ,"
                "SF_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where SF_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "INFRARED_DEVICE_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "ID_SQ,"
                "ID_NAME,"
                "ID_MF,"
                "ID_PROTOCOL_SUPPORTED,"
                "ID_STATUS,"
                "ID_STATUS_INFO,"
                "ID_DETECT_TIME,"
                "ID_PREV_HIST_SQ,"
                "ID_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "IDH_RECORD_TIME "
                ") "
                "select "
                "ID_SQ,"
                "ID_NAME,"
                "ID_MF,"
                "ID_PROTOCOL_SUPPORTED,"
                "ID_STATUS,"
                "ID_STATUS_INFO,"
                "ID_DETECT_TIME,"
                "ID_PREV_HIST_SQ,"
                "ID_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where ID_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "PROCESS_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "PS_SQ,"
                "PS_FILE_NAME,"
                "PS_TYPE,"
                "PS_FILE_PATH,"
                "PS_ORIGINAL_FILE_NAME,"
                "PS_COMPANY_NAME,"
                "PS_FILE_DESC,"
                "PS_FILE_VER,"
                "PS_COPY_RIGHT,"
                "PS_RUNNING,"
                "PS_CONNECTED_SVR_ADDR,"
                "PS_DETECT_TIME,"
                "PS_PREV_HIST_SQ,"
                "PS_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "PSH_RECORD_TIME "
                ") "
                "select "
                "PS_SQ,"
                "PS_FILE_NAME,"
                "PS_TYPE,"
                "PS_FILE_PATH,"
                "PS_ORIGINAL_FILE_NAME,"
                "PS_COMPANY_NAME,"
                "PS_FILE_DESC,"
                "PS_FILE_VER,"
                "PS_COPY_RIGHT,"
                "PS_RUNNING,"
                "PS_CONNECTED_SVR_ADDR,"
                "PS_DETECT_TIME,"
                "PS_PREV_HIST_SQ,"
                "PS_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where PS_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "ROUTER_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "RT_SQ,"
                "RT_DETECT_TYPE,"
                "RT_IPADDR,"
                "RT_MAC_ADDR,"
                "RT_WEB_TEXT,"
                "RT_CAPTION,"
                "RT_DETECT_TIME,"
                "RT_PREV_HIST_SQ,"
                "RT_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "RTH_RECORD_TIME "
                ") "
                "select "
                "RT_SQ,"
                "RT_DETECT_TYPE,"
                "RT_IPADDR,"
                "RT_MAC_ADDR,"
                "RT_WEB_TEXT,"
                "RT_CAPTION,"
                "RT_DETECT_TIME,"
                "RT_PREV_HIST_SQ,"
                "RT_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where RT_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else if(!strcasecmp(p_tableName, "NET_PRINTER_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "NP_SQ,"
                "NP_CONNECTED,"
                "NP_DISCORDANCE,"
                "NP_WEB_CONNECT,"
                "NP_HOST_NAME,"
                "NP_WSD_PRINTER_DEVICE,"
                "NP_WSD_LOCATION,"
                "NP_OPEN_PORT,"
                "NP_PRINTER_PORT,"
                "NP_WEB_TEXT,"
                "NP_DETECT_TIME,"
                "NP_PREV_HIST_SQ,"
                "NP_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "NPH_RECORD_TIME "
                ") "
                "select "
                "NP_SQ,"
                "NP_CONNECTED,"
                "NP_DISCORDANCE,"
                "NP_WEB_CONNECT,"
                "NP_HOST_NAME,"
                "NP_WSD_PRINTER_DEVICE,"
                "NP_WSD_LOCATION,"
                "NP_OPEN_PORT,"
                "NP_PRINTER_PORT,"
                "NP_WEB_TEXT,"
                "NP_DETECT_TIME,"
                "NP_PREV_HIST_SQ,"
                "NP_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where NP_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "NET_SCAN_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "NS_SQ,"
                "NS_DAP_AGENT,"
                "NS_IP,"
                "NS_MAC,"
                "NS_MAC_MATCH,"
                "NS_WEB_TEXT,"
                "NS_OPEN_PORT,"
                "NS_DETECT_TIME,"
                "NS_PREV_HIST_SQ,"
                "HB_SQ,"
                "US_SQ,"
                "NSH_RECORD_TIME "
                ") "
                "select "
                "NS_SQ,"
                "NS_DAP_AGENT,"
                "NS_IP,"
                "NS_MAC,"
                "NS_MAC_MATCH,"
                "NS_WEB_TEXT,"
                "NS_OPEN_PORT,"
                "NS_DETECT_TIME,"
                "NS_PREV_HIST_SQ,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where NS_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "ARP_TB"))
    {
    }
    else if(!strcasecmp(p_tableName, "WIN_DRV_TB"))
    {
        sprintf(sqlBuf,
                "insert into %s "
                "( "
                "DV_SQ,"
                "DV_CLASS,"
                "DV_CLASS_DESC,"
                "DV_DESC,"
                "DV_DRIVER,"
                "DV_ENUM,"
                "DV_FILE_COMPANY,"
                "DV_FILE_COPY_RIGHT,"
                "DV_FILE_DESC,"
                "DV_FILE_PATH,"
                "DV_FILE_PRODUCT,"
                "DV_FILE_VER,"
                "DV_LOCATION,"
                "DV_MFG,"
                "DV_NAME,"
                "DV_SERVICE,"
                "DV_START,"
                "DV_STATUS,"
                "DV_TYPE,"
                "DV_DATA_TYPE,"
                "DV_DETECT_TIME,"
                "DV_PREV_HIST_SQ,"
                "HB_SQ,"
                "US_SQ,"
                "DVH_RECORD_TIME "
                ") "
                "select "
                "DV_SQ,"
                "DV_CLASS,"
                "DV_CLASS_DESC,"
                "DV_DESC,"
                "DV_DRIVER,"
                "DV_ENUM,"
                "DV_FILE_COMPANY,"
                "DV_FILE_COPY_RIGHT,"
                "DV_FILE_DESC,"
                "DV_FILE_PATH,"
                "DV_FILE_PRODUCT,"
                "DV_FILE_VER,"
                "DV_LOCATION,"
                "DV_MFG,"
                "DV_NAME,"
                "DV_SERVICE,"
                "DV_START,"
                "DV_STATUS,"
                "DV_TYPE,"
                "DV_DATA_TYPE,"
                "DV_DETECT_TIME,"
                "DV_PREV_HIST_SQ,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "from %s "
                "where DV_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }
    else if(!strcasecmp(p_tableName, "RDP_SESSION_TB")) {
        sprintf(sqlBuf,
                "INSERT INTO %s "
                "( "
                "RDP_SQ,"
                "RDP_CLIENT_IP,"
                "RDP_CLIENT_NAME,"
                "RDP_CONNECT_TIME,"
                "RDP_USER_ID,"
                "RDP_DETECT_TIME,"
                "RDP_PREV_HIST_SQ,"
                "RDP_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "RDPH_RECORD_TIME"
                ")"
                "SELECT "
                "RDP_SQ,"
                "RDP_CLIENT_IP,"
                "RDP_CLIENT_NAME,"
                "RDP_CONNECT_TIME,"
                "RDP_USER_ID,"
                "RDP_DETECT_TIME,"
                "RDP_PREV_HIST_SQ,"
                "RDP_SUMMARY,"
                "HB_SQ,"
                "US_SQ,"
                "sysdate() "
                "FROM %s "
                "WHERE RDP_SQ=%llu", p_histTableName,p_tableName,p_tbSq);
    }

    else
    {
        WRITE_CRITICAL(CATEGORY_DB, "Unknown table name(%s)", p_tableName);
        return -1;
    }

    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "Proc tbSq(%llu)insertCnt(%d)", p_tbSq,insertCnt);

    return 0;
}

int fdb_InsertHistoryEventTb(unsigned long long p_evSq, char *p_histTableName, int p_evExist)
{
    int   insertCnt;
    char  sqlBuf[1023 +1];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf,
            "insert into %s "
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
            "EV_DUP_DETECT_TIME,"
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
            "%d,"
            "EV_EVENT_CONTEXT,"
            "EV_DETECT_TIME,"
            "EV_DUP_DETECT_TIME,"
            "EV_RECORD_TIME,"
            "sysdate() "
            "from EVENT_TB "
            "where EV_SQ=%llu", p_histTableName,p_evExist,p_evSq);


    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "Proc evSq(%llu)insertCnt(%d)", p_evSq,insertCnt);

    return 0;
}