#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/timeb.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "linuxke/dap_linux.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "data_define.h"

#define     MAX_EVENT_COUNT   100
_IBK_EXP_EVENT_INFO     g_stIBKExpEvent[MAX_EVENT_COUNT];
_IBK_EXP_EVENT_INFO*    g_ptrIBKExpEvent;

_IBK_EXP_MANAGER_LOG_INFO     g_stIBKExpManagerLog[MAX_EVENT_COUNT];
_IBK_EXP_MANAGER_LOG_INFO*    g_ptrIBKExpManagerLog;

int fIBK_GetInnerDataCount(char* szData)
{
    int nCount = 1;

    char *ptr1 = strstr(szData, "','");

    while (ptr1 != NULL)
    {
        nCount++;

        ptr1 = strstr(ptr1+3, "','");
    }

    return nCount;
}

char* fIBK_GetSubString(char *szData, char *szBegin, char *szEnd)
{
    static char szRtn[1024] = {};
    memset(szRtn, 0x00, 1024);

    char *ptr1 = strstr(szData, szBegin);
    int nA = ptr1-szData;

    // TEST
   // printf("fIBK_GetSubString - %s\n", ptr1);

    if (nA >= 0)
    {
        nA = nA + strlen(szBegin);

        ptr1 = strstr(szData+nA, szEnd);
        int nB = ptr1-szData;

        // TEST
        //printf("fIBK_GetSubString - %s\n", ptr1);

        // TEST
        //printf("%d, %d\n", nA, nB);

        if (nB > nA)
        {
            int nC = nB - nA;

            // TEST
            //printf("%d, %d, %d\n", nA, nB, nC);

            strncpy(szRtn, szData+nA, nC);
            //printf("%s\n", szValue2);

            // TEST
            //printf("fIBK_GetSubString - %s\n", ptr1);

            return szRtn;
        }
    }

    return NULL;
}

char* fIBK_GetInnerData(char* szData, int nIndex)
{
    char szValue[1024] = {};
    char szValue2[1024] = {};

    static char szRtn[1024] = {};
    memset(szRtn, 0x00, 1024);

    strcpy(szValue, szData);

    int nCount = 0;

    char *ptr1 = strtok(szValue, ",");

    while (ptr1 != NULL)
    {
        if (nCount == nIndex)
        {
            strcpy(szValue2, ptr1);

            ptr1 = fIBK_GetSubString(szValue2, "'", "'");

            strcpy(szRtn, ptr1);

            return szRtn;
        }

        ptr1 = strtok(NULL, ",");

        nCount++;
    }

    return NULL;
}

char* fIBK_GetAddInfo(char *szSummary)
{
    static char szRtn[1024] = {};
    memset(szRtn, 0x00, 1024);

    char *ptr1 = strstr(szSummary, "add(");
    int nA = ptr1-szSummary;

    if (nA >= 0)
    {
        nA = nA + 4;

        ptr1 = strstr(szSummary+nA, ")block(");
        int nB = ptr1-szSummary;

        if (nB > 4)
        {
            //printf("%d ~ %d\n", nA, nB);

            int nC = nB - nA;

            strncpy(szRtn, szSummary+nA, nC);
            //printf("%s\n", szValue2);

            return szRtn;
        }
    }

    return NULL;
}

char* fIBK_GetModifyInfo(char *szSummary)
{
    static char szRtn[1024] = {};
    memset(szRtn, 0x00, 1024);

    char *ptr1 = strstr(szSummary, "mod(");

    int nA = ptr1-szSummary;

    //printf("nA = %d\n", nA);

    if (nA >= 0)
    {
        nA = nA + 4;

        ptr1 = strstr(szSummary+nA, ")");

        int nB = 0;

        if (!ptr1)
        {
            return NULL;
        }

        nB = strlen(szSummary) - 1;

        if (nB > 4)
        {
            //printf("%d ~ %d\n", nA, nB);

            int nC = nB - nA;

            strncpy(szRtn, szSummary+nA, nC);
            //printf("%s\n", szValue2);

            return szRtn;
        }
    }

    return NULL;
}

int fIBK_GetLastEventNo(unsigned int *nLastEV_SQ)
{
    int		rowCnt = 0;
    char	sqlBuf[1024];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "SELECT EV_SQ FROM `DAP_DB`.`EVENT_TB` ORDER BY EV_SQ DESC LIMIT 1;");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nRtn = 0;

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                *nLastEV_SQ = atol(	g_stMyCon->row[0]);

            nRtn = 1;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    return nRtn;
}

int fIBK_GetLastEventNo_His(unsigned short sYear, unsigned short sMonth, unsigned int *nLastEV_SQ)
{
    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "EVENT_HISTORY_%02u", sMonth);

    sprintf(sqlBuf, "SELECT EV_SQ FROM `%s`.`%s` ORDER BY EV_SQ DESC LIMIT 1;",
            szHistoryDBName, szHistoryTableName);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nRtn = 0;

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                *nLastEV_SQ = atol(	g_stMyCon->row[0]);

            nRtn = 1;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    return nRtn;
}

int fIBK_Eventinfo(unsigned short sYear, unsigned short sMonth, char* szTime, unsigned int *nLastEV_SQ)
{
    register int	loop;

    int		rowCnt = 0;
    char	sqlBuf[1024];
    //char	szHistoryDBName[32];
    //char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    //memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    //memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    //sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    //sprintf(szHistoryTableName, "EVENT_HISTORY_%02u", sMonth);

    if (*nLastEV_SQ > 0)
    {
        sprintf(sqlBuf, "SELECT EV_SQ, A.US_SQ, B.US_SNO, A.HB_SQ, EV_TYPE, EV_IPADDR, EV_EVENT_CONTEXT, EV_DETECT_TIME, EV_DUP_DETECT_TIME FROM "
                        "`DAP_DB`.`EVENT_TB` AS A LEFT JOIN `DAP_DB`.`USER_TB` AS B ON A.US_SQ = B.US_SQ WHERE EV_LEVEL IN (4,5,6) AND EV_SQ > %u ORDER BY EV_SQ DESC;",
                *nLastEV_SQ);
    }
    else
    {
        sprintf(sqlBuf, "SELECT EV_SQ, A.US_SQ, B.US_SNO, A.HB_SQ, EV_TYPE, EV_IPADDR, EV_EVENT_CONTEXT, EV_DETECT_TIME, EV_DUP_DETECT_TIME FROM "
                        "`DAP_DB`.`EVENT_TB` AS A LEFT JOIN `DAP_DB`.`USER_TB` AS B ON A.US_SQ = B.US_SQ WHERE EV_LEVEL IN (4,5,6) AND EV_DETECT_TIME > '%s' ORDER BY EV_SQ DESC;",
                szTime);
    }

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    // TEST
    //printf("fIBK_Eventinfo\n");

    unsigned int nEventType = 0;
    unsigned char   ev_dup_detect_time[20];

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[4] != NULL)
                nEventType = atol(g_stMyCon->row[4]);
            else
                nEventType = 0;

            if (nEventType != EVENT_TYPE_ROUTER &&
                nEventType != EVENT_TYPE_DISK_REG &&
                nEventType != EVENT_TYPE_DISK_HIDDEN &&
                nEventType != EVENT_TYPE_DISK_NEW &&
                nEventType != EVENT_TYPE_DISK_MOBILE &&
                nEventType != EVENT_TYPE_DISK_MOBILE_READ &&
                nEventType != EVENT_TYPE_DISK_MOBILE_WRITE &&
                nEventType != EVENT_TYPE_NET_ADAPTER_OVER &&
                nEventType != EVENT_TYPE_WIFI &&
                nEventType != EVENT_TYPE_BLUETOOTH &&
                nEventType != EVENT_TYPE_INFRARED_DEVICE)
            {
                continue;
            }

            memset(ev_dup_detect_time, 0x00, sizeof(ev_dup_detect_time));

            // EVENT_TYPE_ROUTER
            // EVENT_TYPE_DISK_REG, EVENT_TYPE_DISK_HIDDEN, EVENT_TYPE_DISK_NEW,
            // EVENT_TYPE_DISK_MOBILE, EVENT_TYPE_DISK_MOBILE_READ, EVENT_TYPE_DISK_MOBILE_WRITE
            // EVENT_TYPE_NET_ADAPTER_OVER, EVENT_TYPE_WIFI, EVENT_TYPE_BLUETOOTH, EVENT_TYPE_INFRARED_DEVICE

            if (g_stMyCon->row[0] != NULL)
                g_stIBKExpEvent[loop].ev_sq = atol(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                g_stIBKExpEvent[loop].us_sq = atol(	g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(g_stIBKExpEvent[loop].us_sno,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                g_stIBKExpEvent[loop].hb_sq = atol(	g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                g_stIBKExpEvent[loop].ev_type = atol(	g_stMyCon->row[4]);
            if (g_stMyCon->row[5] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_ipaddr,		g_stMyCon->row[5]);
            if (g_stMyCon->row[6] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_context,		g_stMyCon->row[6]);
            if (g_stMyCon->row[7] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_detect_time,		g_stMyCon->row[7]);
            if (g_stMyCon->row[8] != NULL)
                strcpy(ev_dup_detect_time,		g_stMyCon->row[8]);

            if (strlen(ev_dup_detect_time) > 0)
            {
                strcpy(g_stIBKExpEvent[loop].ev_detect_time,		ev_dup_detect_time);
            }

            //WRITE_DEBUG(CATEGORY_DEBUG,"EVH_SQ : %d", g_stIBKExpEvent[loop].evh_sq);

            if (*nLastEV_SQ < g_stIBKExpEvent[loop].ev_sq)
            {
                *nLastEV_SQ = g_stIBKExpEvent[loop].ev_sq;
            }

            // TEST
            //printf("EV_SQ : %u\n", *nLastEV_SQ);

            loop++;

            if (loop >= 100)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return loop;
}

int fIBK_Eventinfo_His(unsigned short sYear, unsigned short sMonth, char* szTime, int nDataIndex, unsigned int *nLastEV_SQ)
{
    register int	loop = 0;

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "EVENT_HISTORY_%02u", sMonth);

    if (*nLastEV_SQ > 0)
    {
        sprintf(sqlBuf, "SELECT EV_SQ, A.US_SQ, B.US_SNO, A.HB_SQ, EV_TYPE, EV_IPADDR, EV_EVENT_CONTEXT, EV_DETECT_TIME, EV_DUP_DETECT_TIME FROM "
                        "`%s`.`%s` AS A LEFT JOIN `DAP_DB`.`USER_TB` AS B ON A.US_SQ = B.US_SQ WHERE EV_LEVEL IN (4,5,6) AND EV_SQ > %u ORDER BY EV_SQ DESC;",
                szHistoryDBName, szHistoryTableName, *nLastEV_SQ);
    }
    else
    {
        sprintf(sqlBuf, "SELECT EV_SQ, A.US_SQ, B.US_SNO, A.HB_SQ, EV_TYPE, EV_IPADDR, EV_EVENT_CONTEXT, EV_DETECT_TIME, EV_DUP_DETECT_TIME FROM "
                        "`%s`.`%s` AS A LEFT JOIN `DAP_DB`.`USER_TB` AS B ON A.US_SQ = B.US_SQ WHERE EV_LEVEL IN (4,5,6) AND EV_DETECT_TIME > '%s' ORDER BY EV_SQ DESC;",
                szHistoryDBName, szHistoryTableName, szTime);
    }

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    // TEST
    //printf("fIBK_Eventinfo_His\n");

    unsigned int nEventType = 0;
    unsigned char   ev_dup_detect_time[20];

    loop = nDataIndex;

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[4] != NULL)
                nEventType = atol(g_stMyCon->row[4]);
            else
                nEventType = 0;

            if (nEventType != EVENT_TYPE_ROUTER &&
                nEventType != EVENT_TYPE_DISK_REG &&
                nEventType != EVENT_TYPE_DISK_HIDDEN &&
                nEventType != EVENT_TYPE_DISK_NEW &&
                nEventType != EVENT_TYPE_DISK_MOBILE &&
                nEventType != EVENT_TYPE_DISK_MOBILE_READ &&
                nEventType != EVENT_TYPE_DISK_MOBILE_WRITE &&
                nEventType != EVENT_TYPE_NET_ADAPTER_OVER &&
                nEventType != EVENT_TYPE_WIFI &&
                nEventType != EVENT_TYPE_BLUETOOTH &&
                nEventType != EVENT_TYPE_INFRARED_DEVICE)
            {
                continue;
            }

            memset(ev_dup_detect_time, 0x00, sizeof(ev_dup_detect_time));

            // EVENT_TYPE_ROUTER
            // EVENT_TYPE_DISK_REG, EVENT_TYPE_DISK_HIDDEN, EVENT_TYPE_DISK_NEW,
            // EVENT_TYPE_DISK_MOBILE, EVENT_TYPE_DISK_MOBILE_READ, EVENT_TYPE_DISK_MOBILE_WRITE
            // EVENT_TYPE_NET_ADAPTER_OVER, EVENT_TYPE_WIFI, EVENT_TYPE_BLUETOOTH, EVENT_TYPE_INFRARED_DEVICE

            if (g_stMyCon->row[0] != NULL)
                g_stIBKExpEvent[loop].ev_sq = atol(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                g_stIBKExpEvent[loop].us_sq = atol(	g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(g_stIBKExpEvent[loop].us_sno,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                g_stIBKExpEvent[loop].hb_sq = atol(	g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                g_stIBKExpEvent[loop].ev_type = atol(	g_stMyCon->row[4]);
            if (g_stMyCon->row[5] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_ipaddr,		g_stMyCon->row[5]);
            if (g_stMyCon->row[6] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_context,		g_stMyCon->row[6]);
            if (g_stMyCon->row[7] != NULL)
                strcpy(g_stIBKExpEvent[loop].ev_detect_time,		g_stMyCon->row[7]);
            if (g_stMyCon->row[8] != NULL)
                strcpy(ev_dup_detect_time,		g_stMyCon->row[8]);

            if (strlen(ev_dup_detect_time) > 0)
            {
                strcpy(g_stIBKExpEvent[loop].ev_detect_time,		ev_dup_detect_time);
            }

            //WRITE_DEBUG(CATEGORY_DEBUG,"EVH_SQ : %d", g_stIBKExpEvent[loop].evh_sq);

            if (*nLastEV_SQ < g_stIBKExpEvent[loop].ev_sq)
            {
                *nLastEV_SQ = g_stIBKExpEvent[loop].ev_sq;
            }

            // TEST
            //printf("EV_SQ : %u\n", *nLastEV_SQ);

            loop++;

            if (loop >= 100)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return loop;
}

int fIBK_EventInfo_Router(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime, _ROUTER_EVENT_DATA *stData)
{
    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "ROUTER_HISTORY_%02u", sMonth);

    sprintf(sqlBuf, "SELECT RT_IPADDR, RT_WEB_TEXT, RT_DETECT_TIME FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND RT_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    // TEST
    //printf("fIBK_EventInfo_Router\n");

    int nRtn = 0;

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                strcpy((*stData).rt_ipaddr,		g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy((*stData).rt_info,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy((*stData).rt_detect_time,		g_stMyCon->row[2]);

            // TEST
            printf("rt_info : %s\n", (*stData).rt_info);

            nRtn = 1;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nRtn;
}

_DISK_DATA g_stDiskList[10];

void clear_DiskList()
{
    int nIndex = 0;
    for (; nIndex < 10; nIndex++)
    {
        memset(&g_stDiskList[nIndex], 0x00, sizeof(_DISK_DATA));
    }
}

int fIBK_EventInfo_Disk(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime)
{
    clear_DiskList();   // 기존 데이터 초기화

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "DISK_HISTORY_%02u", sMonth);

    // TEST
    //printf("fIBK_EventInfo_Disk1\n");

    sprintf(sqlBuf, "SELECT DK_DRIVE_TYPE, DK_VOLUME_SN, DK_PHYSICAL_SN, DK_INTERFACE_TYPE, DK_MODEL, DK_DETECT_TIME, DK_DESC, DK_SUMMARY FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND DK_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nloop = 0;

    if(rowCnt > 0)
    {
        _DISK_DATA stData;

        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(&stData, 0x00, sizeof(_DISK_DATA));

            if (g_stMyCon->row[0] != NULL)
                stData.dk_drive_type = atoi(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(stData.dk_volume_sn,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(stData.dk_physical_sn,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                strcpy(stData.dk_interface_type,		g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                strcpy(stData.dk_model,		g_stMyCon->row[4]);
            if (g_stMyCon->row[5] != NULL)
                strcpy(stData.dk_detect_time,		g_stMyCon->row[5]);
            if (g_stMyCon->row[6] != NULL)
                strcpy(stData.dk_desc,		g_stMyCon->row[6]);
            if (g_stMyCon->row[7] != NULL)
                strcpy(stData.dk_summary,		g_stMyCon->row[7]);

            // TEST
            //printf("rt_info : %s\n", (*stData).rt_info);

            memcpy(&g_stDiskList[nloop],  &stData, sizeof(_DISK_DATA));

            nloop++;

            if (nloop >= 10)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nloop;
}

_WIFI_EVENT_DATA g_stWifiList[10];

void clear_WifiList()
{
    int nIndex = 0;
    for (; nIndex < 10; nIndex++)
    {
        memset(&g_stWifiList[nIndex], 0x00, sizeof(_WIFI_EVENT_DATA));
    }
}

int fIBK_EventInfo_Wifi(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime)
{
    clear_WifiList();   // 기존 데이터 초기화

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "WIFI_HISTORY_%02u", sMonth);

    // TEST
    //printf("fIBK_EventInfo_Wifi\n");

    sprintf(sqlBuf, "SELECT WF_INTERFACE_DESC, WF_MAC_ADDR, WF_DETECT_TIME FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND WF_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nloop = 0;

    if(rowCnt > 0)
    {
        _WIFI_EVENT_DATA stData;

        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(&stData, 0x00, sizeof(_WIFI_EVENT_DATA));

            if (g_stMyCon->row[0] != NULL)
                strcpy(stData.wf_interface_desc,		g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(stData.wf_mac_addr,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(stData.wf_detect_time,		g_stMyCon->row[2]);

            memcpy(&g_stWifiList[nloop],  &stData, sizeof(_WIFI_EVENT_DATA));

            nloop++;

            if (nloop >= 10)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nloop;
}

_BLUETOOTH_EVENT_DATA g_stBluetoothList[10];

void clear_BluetoothList()
{
    int nIndex = 0;
    for (; nIndex < 10; nIndex++)
    {
        memset(&g_stBluetoothList[nIndex], 0x00, sizeof(_BLUETOOTH_EVENT_DATA));
    }
}

int fIBK_EventInfo_Bluetooth(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime)
{
    clear_BluetoothList();   // 기존 데이터 초기화

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "BLUETOOTH_HISTORY_%02u", sMonth);

    // TEST
    //printf("fIBK_EventInfo_Bluetooth\n");

    sprintf(sqlBuf, "SELECT BT_INSTANCE_NAME, BT_MAC_ADDR, BT_MINOR_DEVICE, BT_DETECT_TIME FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND BT_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nloop = 0;

    if(rowCnt > 0)
    {
        _BLUETOOTH_EVENT_DATA stData;

        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(&stData, 0x00, sizeof(_BLUETOOTH_EVENT_DATA));

            if (g_stMyCon->row[0] != NULL)
                strcpy(stData.bt_instance_name,		g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(stData.bt_mac_addr,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(stData.bt_minor_device,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                strcpy(stData.bt_detect_time,		g_stMyCon->row[3]);

            memcpy(&g_stBluetoothList[nloop],  &stData, sizeof(_BLUETOOTH_EVENT_DATA));

            nloop++;

            if (nloop >= 10)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nloop;
}

int fIBK_EventInfo_InfraredDevice(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime, _INFRARED_DEVICE_EVENT_DATA *pstData)
{
    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "INFRARED_DEVICE_HISTORY_%02u", sMonth);

    // TEST
    //printf("fIBK_EventInfo_InfraredDevice\n");

    sprintf(sqlBuf, "SELECT ID_NAME, ID_DETECT_TIME FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND ID_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nRtn = 0;

    if(rowCnt > 0)
    {
        if (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                strcpy(pstData->id_name,		g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(pstData->id_detect_time,		g_stMyCon->row[1]);

            nRtn = 1;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nRtn;
}

_NET_ADAPTER_OVER_EVENT_DATA g_stNetAdapterList[10];

void clear_NetAdapterList()
{
    int nIndex = 0;
    for (; nIndex < 10; nIndex++)
    {
        memset(&g_stNetAdapterList[nIndex], 0x00, sizeof(_NET_ADAPTER_OVER_EVENT_DATA));
    }
}

int fIBK_EventInfo_NetworkAdapter(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime)
{
    clear_NetAdapterList();   // 기존 데이터 초기화

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "NET_ADAPTER_HISTORY_%02u", sMonth);

    // TEST
    //printf("fIBK_EventInfo_NetworkAdapter\n");

    sprintf(sqlBuf, "SELECT NA_NAME, NA_IPV4, NA_MAC, NA_SUMMARY, NA_DETECT_TIME FROM "
                    "`%s`.`%s` WHERE HB_SQ = %d AND US_SQ = %d AND NA_DETECT_TIME = '%s';",
            szHistoryDBName, szHistoryTableName, nHB_SQ, nUS_SQ, szDetectTime);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nloop = 0;

    if(rowCnt > 0)
    {
        _NET_ADAPTER_OVER_EVENT_DATA stData;

        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(&stData, 0x00, sizeof(_NET_ADAPTER_OVER_EVENT_DATA));

            if (g_stMyCon->row[0] != NULL)
                strcpy(stData.na_name,		g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(stData.na_ip,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(stData.na_mac,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                strcpy(stData.na_summary,		g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                strcpy(stData.na_detect_time,		g_stMyCon->row[4]);

            memcpy(&g_stNetAdapterList[nloop],  &stData, sizeof(_NET_ADAPTER_OVER_EVENT_DATA));

            nloop++;

            if (nloop >= 10)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return nloop;
}

int fIBK_GetLastManagerLogNo_His(unsigned short sYear, unsigned short sMonth, unsigned int *nLastLog_SQ)
{
    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "MANAGER_EVENT_HISTORY_%02u", sMonth);

    sprintf(sqlBuf, "SELECT MNE_SQ FROM `%s`.`%s` ORDER BY MNE_SQ DESC LIMIT 1;",
            szHistoryDBName, szHistoryTableName);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    int nRtn = 0;

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                *nLastLog_SQ = atol(	g_stMyCon->row[0]);

            nRtn = 1;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    return nRtn;
}

int fIBK_ManagerLogInfo(unsigned short sYear, unsigned short sMonth, char* szTime, unsigned int *nLastLog_SQ)
{
    register int	loop;

    int		rowCnt = 0;
    char	sqlBuf[1024];
    char	szHistoryDBName[32];
    char	szHistoryTableName[32];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(szHistoryDBName, 0x00, sizeof(szHistoryDBName));
    memset(szHistoryTableName, 0x00, sizeof(szHistoryTableName));

    sprintf(szHistoryDBName, "DAP_HISTORY_%04u", sYear);
    sprintf(szHistoryTableName, "MANAGER_EVENT_HISTORY_%02u", sMonth);

    if (*nLastLog_SQ > 0)
    {
        sprintf(sqlBuf, "SELECT MNE_SQ, MNE_IPADDR, MN_ID, MNE_ACTION_TYPE, MNE_RECORD_TIME "
                        "FROM `%s`.`%s` as A LEFT JOIN `DAP_DB`.`MANAGER_TB` as B ON A.MN_SQ = B.MN_SQ WHERE MNE_SQ > %u ORDER BY MNE_SQ ASC;",
                szHistoryDBName, szHistoryTableName, *nLastLog_SQ);
    }
    else
    {
        sprintf(sqlBuf, "SELECT MNE_SQ, MNE_IPADDR, MN_ID, MNE_ACTION_TYPE, MNE_RECORD_TIME "
                        "FROM `%s`.`%s` as A LEFT JOIN `DAP_DB`.`MANAGER_TB` as B ON A.MN_SQ = B.MN_SQ WHERE MNE_RECORD_TIME > '%s' ORDER BY MNE_SQ ASC;",
                szHistoryDBName, szHistoryTableName, szTime);
    }

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    // TEST
    //printf("fIBK_Eventinfo\n");

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                g_stIBKExpManagerLog[loop].mne_sq = atol(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                strcpy(g_stIBKExpManagerLog[loop].mne_ipaddr,		g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(g_stIBKExpManagerLog[loop].mn_id,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                g_stIBKExpManagerLog[loop].mne_action_type = atol(	g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                strcpy(g_stIBKExpManagerLog[loop].mne_record_time,		g_stMyCon->row[4]);

            *nLastLog_SQ = g_stIBKExpManagerLog[loop].mne_sq;

            // TEST
            //printf("EV_SQ : %u\n", *nLastEV_SQ);

            loop++;

            if (loop >= 100)
            {
                break;
            }
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return loop;
}