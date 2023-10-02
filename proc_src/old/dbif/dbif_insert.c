//
// Created by KimByoungGook on 2020-10-30.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "com/dap_def.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "dbif.h"


/* INSERT EVENT_TB */
int	fdbif_MergeEventTb(_DAP_EventParam *p_EP, int p_stLen)
{
    int					rxt = 0;
    int					retryCnt = 0;
    int					rowCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	evSq = 0;
    char				sqlBuf[2048] = {0x00,};
    char				histTableName[50] = {0x00,};
    char				cmdSyslog[256] = {0x00,};
    char				sqlContext[512] = {0x00,};

    /*
   * 0:pass 1:drop 2:info 3:warning 4:critical 5:block
   * ev_level = 0,1,2는이벤트 기록을 안하는데,
   * 해지는 해야하므로, p_stLen이 '0'이 아닐경우와
   * 몇몇 장치의 예외처를 하여 리턴
   */
    if(p_EP->ev_level < 3)
    {
        if(	(p_EP->ev_type == NET_ADAPTER_OVER	 && p_stLen > 1) ||
               (p_EP->ev_type == NET_ADAPTER_DUPIP	 && p_stLen > 1) ||
               (p_EP->ev_type == NET_ADAPTER_DUPMAC && p_stLen > 1) ||
               (p_EP->ev_type == NET_ADAPTER_MULIP	 && p_stLen > 0) ||
               (p_EP->ev_type == DISK_NEW           && p_stLen > -1) )
        {
            WRITE_CRITICAL(CATEGORY_DB,	"Ignore evLevel(%d), evType(%d)stLen(%d)userKey(%s) ",
                           p_EP->ev_level,
                           p_EP->ev_type,
                           p_stLen,
                           p_EP->user_key);
            return 0;
        }
        else if(p_stLen != 0)
        {
            return 0;
        }
    }

    if (strcmp(p_EP->user_key, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s) ", p_EP->user_key);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_EP->user_key, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_EP->user_key);
        return -2;
    }

    WRITE_INFO(CATEGORY_DB, "Print HW_BASE_TB hbSq(%llu)", hbSq);

    rxt = fdbif_SelectEvsqEventTb(hbSq, p_EP->ev_type, &evSq);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    if(rxt > 0) //기존 데이터가 존재함
    {
        if(	p_stLen == 0
               || (p_EP->ev_type == NET_ADAPTER_OVER	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_DUPIP	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_DUPMAC	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_MULIP	&& p_stLen < 1)
               || (p_EP->ev_type == DISK_NEW            && p_stLen < 0)
               || (p_EP->ev_type == CPU_USAGE_ALARM     && p_stLen < 0) // Alarm Off
               || (p_EP->ev_type == CPU_USAGE_CONTROL   && p_stLen < 0) // Control Off
                )  //해지
//               || (p_EP->ev_type == PROCESS_BLACK      && p_stLen == -1) /* Process Black [off]????? Event ???? ??????? ??????? */

        {
            //history에 해지로 옮김
            memset(histTableName, 0x00, sizeof(histTableName));
            fdb_GetHistoryTableName("EVENT_TB", histTableName, p_EP->prefix, p_EP->postfix);
            rxt = fdb_InsertHistoryEventTb(evSq, histTableName, 0);
            if(rxt < 0) //fail in insert history
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in insert history, evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                               evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
                retryCnt = 0;
                while(retryCnt < 3)
                {
                    fcom_Msleep(500);
                    rxt = fdb_InsertHistoryEventTb(evSq, histTableName, 0);
                    if(rxt < 0)
                    {
                        retryCnt++;
                    }
                    else
                    {
                        break;
                    }
                }
                if(retryCnt == 3)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Retry fail in insert history, retry(%d/3)evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                                   retryCnt,evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
                }
                return -1;
            }

            //삭제
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf (sqlBuf, "delete from EVENT_TB where EV_SQ=%llu", evSq);
            rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode  != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode ,
                               g_stMyCon->cpErrMsg);
                retryCnt = 0;
                while(retryCnt < 3)
                {
                    fcom_Msleep(500);
                    rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
                    if(g_stMyCon->nErrCode  != 0)
                    {
                        retryCnt++;
                    }
                    else
                    {
                        break;
                    }
                }
                if(retryCnt == 3)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Retry fail in delete, retry(%d/3)evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                                   retryCnt, evSq, p_EP->user_key, p_EP->ev_type, p_EP->ev_level);
                }
                return -1;
            }
            WRITE_INFO(CATEGORY_DB, "[CANCEL] Delete Event record, evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                       evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);

            return 99;

        }
        else //해지가 아니라면 동일한 데이터가 또 들어옴
        {
            memset		(sqlContext, 0x00, sizeof(sqlContext));
            fcom_ReplaceAll(p_EP->ev_context, "\'", "\\'", sqlContext);
            snprintf(sqlBuf,
                    sizeof(sqlBuf),
                    "update EVENT_TB set "
                    "US_SQ=%llu,"
                    "RU_SQ=%llu,"
                    "EV_IPADDR='%s',"
                    "EV_LEVEL=%d,"
                    "EV_EVENT_CONTEXT='%s',"
                    "EV_DUP_DETECT_TIME='%s',"
                    "EV_RECORD_TIME=sysdate() "
                    "where EV_SQ=%llu",
                    p_EP->user_seq,
                    p_EP->ru_sq,
                    p_EP->user_ip,
                    p_EP->ev_level,
                    sqlContext,
                    p_EP->detect_time,
                    evSq );

            rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode  != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                               g_stMyCon->nErrCode ,
                               g_stMyCon->cpErrMsg);

                retryCnt = 0;
                while(retryCnt < 3)
                {
                    fcom_Msleep(500);
                    rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
                    if(g_stMyCon->nErrCode  != 0)
                    {
                        retryCnt++;
                    }
                    else
                    {
                        break;
                    }
                }
                if(retryCnt == 3)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Retry fail update, "
                                                "retry(%d/3)evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                                   retryCnt,evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
                }
                return -1;
            }
            WRITE_INFO(CATEGORY_DB,"Update EVENT_TB us_sq (%llu) rule_sq (%llu) ev_sq (%llu) ",
                       p_EP->user_seq,
                       p_EP->ru_sq,
                       evSq);

            //history에 중복으로 기록
            memset(histTableName, 0x00, sizeof(histTableName));
            fdb_GetHistoryTableName("EVENT_TB", histTableName, p_EP->prefix, p_EP->postfix);
            rxt = fdb_InsertHistoryEventTb(evSq, histTableName, 2);
            if(rxt < 0) //fail in insert history
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in insert history, evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                               evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
                retryCnt = 0;
                while(retryCnt < 3)
                {
                    fcom_Msleep(500);
                    rxt = fdb_InsertHistoryEventTb(evSq, histTableName, 2);
                    if(rxt < 0)
                    {
                        retryCnt++;
                    }
                    else
                    {
                        break;
                    }
                }
                if(retryCnt == 3)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Retry fail in insert history, retry(%d/3)evSq(%llu)userKey(%s)evType(%d)evLevel(%d) ",
                                   retryCnt,evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
                }
                return -1;
            }
            WRITE_INFO(CATEGORY_DB, "[DUPL] Event record, evSq(%llu)userKey(%s)evType(%d)evLevel(%d)",
                       evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
        }
    }
    else
    {
        //기존 데이터가 없음
        if(	p_stLen == 0
               || (p_EP->ev_type == NET_ADAPTER_OVER	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_DUPIP	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_DUPMAC	&& p_stLen < 2)
               || (p_EP->ev_type == NET_ADAPTER_MULIP	&& p_stLen < 1)
               || (p_EP->ev_type == DISK_NEW            && p_stLen < 0)
               || (p_EP->ev_type == CPU_USAGE_CONTROL   && p_stLen == (-1)) //Control Off
               || (p_EP->ev_type == CPU_USAGE_ALARM     && p_stLen == (-1)) //Alarm Off
//               || (p_EP->ev_type == PROCESS_BLACK      && p_stLen == -1) /* Process Black [off]일때는 Event 삭제 안하도록 예외처리 */
                )
        {

            WRITE_INFO(CATEGORY_DB, "Not exist event record, but stLen(%d), evSq(%llu)userKey(%s)evType(%d)evLevel(%d) ",
                           p_stLen, evSq, p_EP->user_key, p_EP->ev_type, p_EP->ev_level);
            return 0;
        }

        memset(sqlContext, 0x00, sizeof(sqlContext));
        fcom_ReplaceAll(p_EP->ev_context, "\'", "\\'", sqlContext);
        snprintf(sqlBuf,
                sizeof(sqlBuf),
                "insert into EVENT_TB ("
                "US_SQ,"
                "HB_SQ,"
                "RU_SQ,"
                "EV_IPADDR,"
                "EV_LEVEL,"
                "EV_TYPE,"
                "EV_EVENT_CONTEXT,"
                "EV_DETECT_TIME,"
                "EV_RECORD_TIME,"
                "EV_EXIST"
                ") values ("
                "%llu,"
                "%llu,"
                "%llu,"
                "'%s',"
                "%d,"
                "%d,"
                "'%s',"
                "'%s',"
                "sysdate(),"
                "1"
                ")",
                p_EP->user_seq,
                hbSq,
                p_EP->ru_sq,
                p_EP->user_ip,
                p_EP->ev_level,
                p_EP->ev_type,
                sqlContext,
                p_EP->detect_time
        );

        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            retryCnt = 0;
            while(retryCnt < 3)
            {
                fcom_Msleep(500);
                rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode  != 0)
                {
                    retryCnt++;
                }
                else
                {
                    break;
                }
            }
            if(retryCnt == 3)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Retry fail insert, retry(%d/3)evSq(%llu)userKey(%s)evType(%d)evLevel(%d) ",
                               retryCnt,evSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);
            }
            return -1;
        }
    }

    WRITE_INFO(CATEGORY_DB, "[MERGE EVENT] Event record(%d), evSq(%llu)hbSq(%llu)userKey(%s)evType(%d)evLevel(%d) ",
               rowCnt,evSq,hbSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);

    memset	(cmdSyslog, 0x00, sizeof(cmdSyslog));
    snprintf	(cmdSyslog, sizeof(cmdSyslog),
              "[DAP-EVENT]evsq[%llu]hbsq[%llu]key[%s]type[%d]level[%d]\n",
                evSq,hbSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);

    /* sys/syslog.h LOG_INFO is 6 */
    syslog(6|LOG_LOCAL0, cmdSyslog);
    WRITE_INFO(CATEGORY_DB, "[SYSLOG]evsq(%llu)hbSq(%llu)userKey(%s)evType(%d)evLevel(%d) ",
               evSq,hbSq,p_EP->user_key,p_EP->ev_type,p_EP->ev_level);

    return rowCnt;
}

/* INSERT HW_BASE_TB */
int fdbif_MergeHwBaseTb(_DAP_AGENT_INFO *p_AgentInfo,
                        char *p_detectTime,
                        _DAP_MAINBOARD *p_MainBoard,
                        char *p_prefix,
                        char *p_postfix)
{
    int					mergeCnt = 0;
    int					updateCnt = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8+1] = {0x00,};
    char				sqlBuf[2048] = {0x00,};
    char				histTableName[50] = {0x00,};


    if (strcmp(p_AgentInfo->user_key, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_AgentInfo->user_key);
        return 0;
    }

    // If existing data exists
    // 1. get curr_hist_sq from hw_base_tb
    strPrevHistSq = fdbif_GetPrevHistSqByBase(	"HW_BASE_TB",
                                                  "HB_CURR_HIST_SQ,HB_CURR_HIST_DATE",
                                                  p_AgentInfo->user_key );

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    snprintf (strCurrHistDate, sizeof(strCurrHistDate),"%s%s", p_prefix, p_postfix);

    //LogDRet(5, "- usSq(%llu)\n", STR_DEBUG,usSq);

    mergeCnt = 0;
    updateCnt = 0;
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf,sizeof(sqlBuf),
            "insert into HW_BASE_TB (" //HB_ACCESS_TIME은 정책요청시에 넣는다
            "US_SQ,"
            "HB_UNQ,"
            "HB_MB_PN,"
            "HB_MB_MF,"
            "HB_MB_SN,"
            "HB_ACCESS_IP,"
            "HB_ACCESS_MAC,"
            "HB_AGENT_VER,"
            "HB_FIRST_TIME,"
            "HB_RECORD_TIME,"
            "HB_PREV_HIST_SQ,"
            "HB_CURR_HIST_DATE"
            ") values ("
            "%llu,"
            "'%s',"
            "'%s',"
            "'%s',"
            "'%s',"
            "'%s',"
            "'%s',"
            "'%s',"
            "'%s',"
            "sysdate(),"
            "'%s',"
            "'%s') "
            "on duplicate key update "
            "US_SQ=%llu,"
            "HB_UNQ='%s',"
            "HB_MB_PN='%s',"
            "HB_MB_MF='%s',"
            "HB_MB_SN='%s',"
            "HB_ACCESS_IP='%s',"
            "HB_ACCESS_MAC='%s',"
            "HB_AGENT_VER='%s',"
            "HB_FIRST_TIME='%s',"
            "HB_RECORD_TIME=sysdate(),"
            "HB_PREV_HIST_SQ='%s',"
            "HB_CURR_HIST_DATE='%s'",
            p_AgentInfo->user_seq,
            p_AgentInfo->user_key,
            p_MainBoard->hb_mb_pn,
            p_MainBoard->hb_mb_mf,
            p_MainBoard->hb_mb_sn,
            p_AgentInfo->user_ip,
            p_AgentInfo->user_mac,
            p_AgentInfo->agent_ver,
            p_detectTime,
            strPrevHistSq,
            strCurrHistDate,
            p_AgentInfo->user_seq,
            p_AgentInfo->user_key,
            p_MainBoard->hb_mb_pn,
            p_MainBoard->hb_mb_mf,
            p_MainBoard->hb_mb_sn,
            p_AgentInfo->user_ip,
            p_AgentInfo->user_mac,
            p_AgentInfo->agent_ver,
            p_detectTime,
            strPrevHistSq,
            strCurrHistDate
    );

    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        fcom_MallocFree((void**)&strPrevHistSq);
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    tbSq = fdb_SqlInsertId(g_stMyCon);

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("HW_BASE_TB", histTableName, p_prefix, p_postfix);

    // insert history
    fdb_InsertHwBaseHistory(p_AgentInfo->user_key, histTableName);
    currHistSq = fdb_SqlInsertId(g_stMyCon);

    // update currHistSq to tb
    memset  (sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf (sqlBuf,sizeof(sqlBuf),
             "update HW_BASE_TB set HB_CURR_HIST_SQ=%llu where HB_SQ=%llu",
             currHistSq,tbSq);

    updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);

        return (-1);
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB,	"Proc mergeCnt(%d)updateCnt(%d) ",
               mergeCnt, updateCnt);

    return 0;
}

/* INSERT SYSTEM_TB */
int fdbif_MergeSystemTb(char				*p_userKey,
                        unsigned long long 	p_userSeq,
                        char 				*p_detectTime,
                        _DAP_SYSTEM 			*p_ST,
                        char 				*p_prefix,
                        char 				*p_postfix)
{
    int					i = 0, rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				arrInstalledVm = NULL;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "SYSTEM_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("SYSTEM_TB", "ST_CURR_HIST_SQ,ST_CURR_HIST_DATE", hbSq);

        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                 "delete from SYSTEM_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            // free strPrevHistSq
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);

            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    if(fcom_malloc((void**)&arrInstalledVm, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&strPrevHistSq);
        return (-1);
    }

    for(i=0; i<p_ST->st_installed_vm_size; i++)
    {
        arrInstalledVm = (char *)realloc(arrInstalledVm, sizeof(char) * (VM_LEN * p_ST->st_installed_vm_size));
        memset(arrInstalledVm, 0x00, sizeof(char) * (VM_LEN * p_ST->st_installed_vm_size));
        if(i == p_ST->st_installed_vm_size-1)
        {
            sprintf(arrInstalledVm+strlen(arrInstalledVm), "%s", p_ST->st_installed_vm[i]);
        }
        else
        {
            sprintf(arrInstalledVm+strlen(arrInstalledVm), "%s;", p_ST->st_installed_vm[i]);
        }
    }

    WRITE_INFO(CATEGORY_DB, "arrInstalledVm(%s) ", arrInstalledVm);

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("SYSTEM_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    snprintf (strCurrHistDate, sizeof(strCurrHistDate),"%s%s", p_prefix,p_postfix);

    memset	(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_ST->st_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf	(sqlBuf, sizeof(sqlBuf),
                "insert into SYSTEM_TB ( "
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
                "ST_CURR_HIST_DATE,"
                "ST_SUMMARY,"
                "HB_SQ,"
                "US_SQ"
                ") values ("
                "'%s',"
                "'%s',"
                "'%s',"
                "%d,"
                "%d,"
                "%d,"
                "'%s',"
                "'%s',"
                "%d,"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "%llu,"
                "%llu)",
                p_ST->st_name,
                p_ST->st_dns_host_name,
                p_ST->st_domain,
                p_ST->st_domain_role,
                p_ST->st_time_zone,
                p_ST->st_wakeup_type,
                p_ST->st_work_group,
                p_ST->st_bootup_state,
                p_ST->st_memory,
                p_ST->st_vga,
                p_ST->st_vm_name,
                arrInstalledVm,
                p_detectTime,
                strPrevHistSq,
                strCurrHistDate,
                sqlSummary,
                hbSq,
                p_userSeq
    );

    fcom_MallocFree((void**)&arrInstalledVm);


    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        fcom_MallocFree((void**)&strPrevHistSq);
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);

        return -1;
    }
    tbSq = fdb_SqlInsertId(g_stMyCon);

    // insert history
    rxt = fdb_InsertHistory(tbSq, "SYSTEM_TB", histTableName);
    currHistSq = fdb_SqlInsertId(g_stMyCon);

    // update currHistSq to tb
    memset  (sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf (sqlBuf, sizeof(sqlBuf),
             "update SYSTEM_TB set ST_CURR_HIST_SQ=%llu where ST_SQ=%llu",
             currHistSq,tbSq);
    updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT CONNECT_EXT_TB */
int fdbif_MergeConnectExtTb(
        char 				*p_userKey,
        unsigned long long 	p_userSeq,
        char 				*p_detectTime,
        _DAP_CONNECT_EXT 	*p_CE,
        char 				*p_prefix,
        char 				*p_postfix
)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "CONNECT_EXT_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("CONNECT_EXT_TB", "CE_CURR_HIST_SQ,CE_CURR_HIST_DATE", hbSq);

        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from CONNECT_EXT_TB where HB_SQ=%llu", hbSq);

        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("CONNECT_EXT_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset(strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_CE->ce_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_CE->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into CONNECT_EXT_TB ("
                    "CE_URL,"
                    "CE_CONNECTED,"
                    "CE_DETECT_TIME,"
                    "CE_PREV_HIST_SQ,"
                    "CE_CURR_HIST_DATE,"
                    "CE_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_CE->ConnectExtValue[idx].ce_url,
                    p_CE->ConnectExtValue[idx].ce_connected,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);
        //LogDRet(5, "- tbSq(%llu)\n", STR_DEBUG,tbSq);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "CONNECT_EXT_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf (sqlBuf,
                 "update CONNECT_EXT_TB set CE_CURR_HIST_SQ=%llu where CE_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);
        }
    }

    // free strPrevHistSq
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT OS_TB */
int fdbif_MergeOsTb(
        char 				 *p_userKey,
        unsigned long long 	 p_userSeq,
        char 				 *p_detectTime,
        _DAP_OPERATING_SYSTEM *p_OS,
        char 				  *p_prefix,
        char 			      *p_postfix)
{
    int					rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "OS_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("OS_TB", "OS_CURR_HIST_SQ,OS_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from OS_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("OS_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_OS->os_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf	(sqlBuf, sizeof(sqlBuf),
                "insert into OS_TB ("
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
                "OS_CURR_HIST_DATE,"
                "OS_SUMMARY,"
                "HB_SQ,"
                "US_SQ"
                ") values ("
                "'%s',"
                "'%s',"
                "'%s',"
                "%d,"
                "%d,"
                "%d,"
                "%d,"
                "%d,"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "%llu,"
                "%llu)",
                p_OS->os_name,
                p_OS->os_version,
                p_OS->os_architecture,
                p_OS->os_lang,
                p_OS->os_type,
                p_OS->os_portable,
                p_OS->os_sp_major_ver,
                p_OS->os_sp_minor_ver,
                p_detectTime,
                strPrevHistSq,
                strCurrHistDate,
                sqlSummary,
                hbSq,
                p_userSeq
    );


    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        fcom_MallocFree((void**)&strPrevHistSq);
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    tbSq = fdb_SqlInsertId(g_stMyCon);

    // insert history
    rxt = fdb_InsertHistory(tbSq, "OS_TB", histTableName);
    currHistSq = fdb_SqlInsertId(g_stMyCon);

    // update currHistSq to tb
    memset  (sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf (sqlBuf, sizeof(sqlBuf),
             "update OS_TB set OS_CURR_HIST_SQ=%llu where OS_SQ=%llu",
             currHistSq,tbSq);
    updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
    }

    // free strPrevHistSq
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, 	"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT CPU_TB */
int fdbif_MergeCpuTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char* 				p_detectTime,
        _DAP_CPU * 				p_CU,
        char* 				p_prefix,
        char* 				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "CPU_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("CPU_TB", "CU_CURR_HIST_SQ,CU_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from CPU_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode ,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)", rowCnt, hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("CPU_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate) );
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset	(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_CU->cu_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_CU->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into CPU_TB ("
                    "CU_NAME,"
                    "CU_MF,"
                    "CU_DESC,"
                    "CU_PID,"
                    "CU_DETECT_TIME,"
                    "CU_PREV_HIST_SQ,"
                    "CU_CURR_HIST_DATE,"
                    "CU_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_CU->CpuValue[idx].cu_name,
                    p_CU->CpuValue[idx].cu_mf,
                    p_CU->CpuValue[idx].cu_desc,
                    p_CU->CpuValue[idx].cu_pid,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "CPU_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update CPU_TB set CU_CURR_HIST_SQ=%llu where CU_SQ=%llu",
                 currHistSq,tbSq);
        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    // free strPrevHistSq
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT NET_ADAPTER_TB */
int fdbif_MergeNetAdapterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETADAPTER*	p_NA,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlPnpDeviceId[200 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "NET_ADAPTER_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("NET_ADAPTER_TB", "NA_CURR_HIST_SQ,NA_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf), "delete from NET_ADAPTER_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt, hbSq);
//        fdb_SqlCommit(g_stMyCon);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("NET_ADAPTER_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_NA->na_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_NA->size; idx++)
    {
        memset		(sqlPnpDeviceId, 0x00, sizeof(sqlPnpDeviceId));
        fcom_ReplaceAll(p_NA->NetAdapterValue[idx].na_pnp_device_id, "\'", "\\'", sqlPnpDeviceId);
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into NET_ADAPTER_TB ("
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
                    "NA_CURR_HIST_DATE,"
                    "NA_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_NA->NetAdapterValue[idx].na_name,
                    p_NA->NetAdapterValue[idx].na_pn,
                    p_NA->NetAdapterValue[idx].na_desc,
                    p_NA->NetAdapterValue[idx].na_device_type,
                    p_NA->NetAdapterValue[idx].na_ipv4,
                    p_NA->NetAdapterValue[idx].na_ipv6,
                    p_NA->NetAdapterValue[idx].na_mac,
                    p_NA->NetAdapterValue[idx].na_subnet,
                    p_NA->NetAdapterValue[idx].na_default_gw,
                    p_NA->NetAdapterValue[idx].na_default_gw_mac,
                    p_NA->NetAdapterValue[idx].na_pref_dns,
                    p_NA->NetAdapterValue[idx].na_alte_dns,
                    p_NA->NetAdapterValue[idx].na_net_connection_id,
                    p_NA->NetAdapterValue[idx].na_net_connection_status,
                    p_NA->NetAdapterValue[idx].na_net_enabled,
                    p_NA->NetAdapterValue[idx].na_physical_adapter,
                    sqlPnpDeviceId,
                    p_NA->NetAdapterValue[idx].na_service_name,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "NET_ADAPTER_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update NET_ADAPTER_TB set NA_CURR_HIST_SQ=%llu where NA_SQ=%llu",
                 currHistSq,tbSq);
        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    // free strPrevHistSq
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT WIFI */
int fdbif_MergeWifiTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WIFI*				p_WF,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s)",
                       p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "WIFI_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("WIFI_TB", "WF_CURR_HIST_SQ,WF_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf), "delete from WIFI_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("WIFI_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset (sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_WF->wf_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_WF->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf,sizeof(sqlBuf),
                    "INSERT INTO WIFI_TB ("
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
                    "WF_CURR_HIST_DATE,"
                    "WF_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "%d,"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_WF->WifiValue[idx].wf_interface_status,
                    p_WF->WifiValue[idx].wf_interface_desc,
                    p_WF->WifiValue[idx].wf_connection_mode,
                    p_WF->WifiValue[idx].wf_profile_name,
                    p_WF->WifiValue[idx].wf_ssid,
                    p_WF->WifiValue[idx].wf_bss_network_type,
                    p_WF->WifiValue[idx].wf_mac_addr,
                    p_WF->WifiValue[idx].wf_phy_network_type,
                    p_WF->WifiValue[idx].wf_security,
                    p_WF->WifiValue[idx].wf_8021x,
                    p_WF->WifiValue[idx].wf_auth_algo,
                    p_WF->WifiValue[idx].wf_cipher_algo,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "WIFI_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update WIFI_TB set WF_CURR_HIST_SQ=%llu where WF_SQ=%llu",
                 currHistSq,tbSq);
        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    // free strPrevHistSq
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT BLUETOOTH_TB */
int fdbif_MergeBluetoothTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_BLUETOOTH*	p_BT,
        char*				p_prefix,
        char*				p_postfix
)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "BLUETOOTH_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("BLUETOOTH_TB", "BT_CURR_HIST_SQ,BT_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from BLUETOOTH_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",rowCnt, hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("BLUETOOTH_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset		(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_BT->bt_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_BT->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into BLUETOOTH_TB ("
                    "BT_INSTANCE_NAME,"
                    "BT_MAC_ADDR,"
                    "BT_DEVICE,"
                    "BT_MINOR_DEVICE,"
                    "BT_DANGER,"
                    "BT_CONNECTED,"
                    "BT_AUTHENTICATED,"
                    "BT_REMEMBERED,"
                    "BT_DETECT_TIME,"
                    "BT_PREV_HIST_SQ,"
                    "BT_CURR_HIST_DATE,"
                    "BT_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_BT->BluetoothValue[idx].bt_instance_name,
                    p_BT->BluetoothValue[idx].bt_mac_addr,
                    p_BT->BluetoothValue[idx].bt_device,
                    p_BT->BluetoothValue[idx].bt_minor_device,
                    p_BT->BluetoothValue[idx].bt_danger,
                    p_BT->BluetoothValue[idx].bt_connected,
                    p_BT->BluetoothValue[idx].bt_authenticated,
                    p_BT->BluetoothValue[idx].bt_remembered,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "BLUETOOTH_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update BLUETOOTH_TB set BT_CURR_HIST_SQ=%llu where BT_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, 	"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT NET_CONNECTION_TB */
int fdbif_MergeNetConnectionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETCONNECTION *		p_NC,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlPath[256 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "NET_CONNECTION_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("NET_CONNECTION_TB", "NC_CURR_HIST_SQ,NC_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf), "delete from NET_CONNECTION_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("NET_CONNECTION_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_NC->nc_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_NC->size; idx++)
    {
        memset(sqlPath, 0x00, sizeof(sqlPath));
        fcom_ReplaceAll(p_NC->NetConnectionValue[idx].nc_remote_path, "\\", "\\\\", sqlPath);

        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into NET_CONNECTION_TB ("
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
                    "NC_CURR_HIST_DATE,"
                    "NC_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_NC->NetConnectionValue[idx].nc_local_name,
                    p_NC->NetConnectionValue[idx].nc_connection_state,
                    p_NC->NetConnectionValue[idx].nc_connection_type,
                    p_NC->NetConnectionValue[idx].nc_desc,
                    p_NC->NetConnectionValue[idx].nc_provider_name,
                    p_NC->NetConnectionValue[idx].nc_remote_name,
                    sqlPath,
                    p_NC->NetConnectionValue[idx].nc_display_type,
                    p_NC->NetConnectionValue[idx].nc_resource_type,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "NET_CONNECTION_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update NET_CONNECTION_TB set NC_CURR_HIST_SQ=%llu where NC_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT DISK_TB */
int fdbif_MergeDiskTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_DISK *				p_DK,
        char*				p_prefix,
        char*				p_postfix,
        int	*				p_dkNewCnt,
        char*				p_resVal)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};
    char				sqlPath[128 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    //DISK_TB에 기록하기전 추가된디스크인지 확인한다.
    *p_dkNewCnt = fdbif_SelectCountNewByDisk(hbSq, p_DK, p_resVal);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "DISK_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("DISK_TB", "DK_CURR_HIST_SQ,DK_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from DISK_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ", rowCnt, hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("DISK_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_DK->dk_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_DK->size; idx++)
    {
        memset(sqlPath, 0x00, sizeof(sqlPath));
        fcom_ReplaceAll( p_DK->DiskValue[idx].dk_name, "\\", "", sqlPath);
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into DISK_TB ("
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
                    "DK_CURR_HIST_DATE,"
                    "DK_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    sqlPath,
                    p_DK->DiskValue[idx].dk_drive_type,
                    p_DK->DiskValue[idx].dk_file_system,
                    p_DK->DiskValue[idx].dk_access,
                    p_DK->DiskValue[idx].dk_volume_name,
                    p_DK->DiskValue[idx].dk_volume_sn,
                    p_DK->DiskValue[idx].dk_desc,
                    p_DK->DiskValue[idx].dk_physical_sn,
                    p_DK->DiskValue[idx].dk_interface_type,
                    p_DK->DiskValue[idx].dk_mf,
                    p_DK->DiskValue[idx].dk_model,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "DISK_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update DISK_TB set DK_CURR_HIST_SQ=%llu where DK_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT NET_DRIVE_TB */
int fdbif_MergeNetDriveTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETDRIVE*			p_ND,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlPath[256 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "NET_DRIVE_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("NET_DRIVE_TB", "ND_CURR_HIST_SQ,ND_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf), "delete from NET_DRIVE_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("NET_DRIVE_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_ND->nd_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_ND->size; idx++)
    {
        memset(sqlPath, 0x00, sizeof(sqlPath));
        fcom_ReplaceAll(p_ND->NetDriveValue[idx].nd_remote_path, "\\", "\\\\", sqlPath);

        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into NET_DRIVE_TB ("
                    "ND_DRIVE_NAME,"
                    "ND_USER_NAME,"
                    "ND_CONNECTION_TYPE,"
                    "ND_DEFER_FLAGS,"
                    "ND_PROVIDER_NAME,"
                    "ND_PROVIDER_TYPE,"
                    "ND_REMOTE_PATH,"
                    "ND_DETECT_TIME,"
                    "ND_PREV_HIST_SQ,"
                    "ND_CURR_HIST_DATE,"
                    "ND_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_ND->NetDriveValue[idx].nd_drive_name,
                    p_ND->NetDriveValue[idx].nd_user_name,
                    p_ND->NetDriveValue[idx].nd_connection_type,
                    p_ND->NetDriveValue[idx].nd_defer_flags,
                    p_ND->NetDriveValue[idx].nd_provider_name,
                    p_ND->NetDriveValue[idx].nd_provider_type,
                    sqlPath,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "NET_DRIVE_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update NET_DRIVE_TB set ND_CURR_HIST_SQ=%llu where ND_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT NET_PRINTER_TB */
int fdbif_MergeNetPrinterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETPRINTER*		p_NP,
        char*				p_prefix,
        char*				p_postfix)
{
    int					i = 0,idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				*arrOpenPort = NULL;
    char				*arrPrinterPort = NULL;
    char				*strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "NET_PRINTER_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("NET_PRINTER_TB", "NP_CURR_HIST_SQ,NP_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from NET_PRINTER_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            // free strPrevHistSq
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("NET_PRINTER_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset		(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_NP->np_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;

    // init for realloc
    if(fcom_malloc((void**)&arrOpenPort, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&strPrevHistSq);
        return (-1);
    }

    if(fcom_malloc((void**)&arrPrinterPort, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&strPrevHistSq);
        return (-1);
    }

    for(idx=0; idx<p_NP->size; idx++)
    {
        if( p_NP->NetPrinterValue[idx].np_open_port_size > 0 )
        {
            arrOpenPort = (char *)realloc(	arrOpenPort,
                                              sizeof(char)*(PORT_LEN*p_NP->NetPrinterValue[idx].np_open_port_size));
            memset(arrOpenPort, 0x00, sizeof(char)*(PORT_LEN*p_NP->NetPrinterValue[idx].np_open_port_size));
        }
        else
        {
            arrOpenPort = (char *)realloc(arrOpenPort, sizeof(char));
            memset(arrOpenPort, 0x00, sizeof(char));
        }

        for(i=0; i<p_NP->NetPrinterValue[idx].np_open_port_size; i++)
        {
            if(i == p_NP->NetPrinterValue[idx].np_open_port_size-1)
            {
                sprintf(arrOpenPort+strlen(arrOpenPort), "%d", p_NP->NetPrinterValue[idx].np_open_port[i]);
            }
            else
            {
                sprintf(arrOpenPort+strlen(arrOpenPort), "%d;", p_NP->NetPrinterValue[idx].np_open_port[i]);
            }
        }

        if( p_NP->NetPrinterValue[idx].np_printer_port_size > 0 )
        {
            arrPrinterPort = (char *)realloc(arrPrinterPort,
                                             sizeof(char)*(PORT_LEN*p_NP->NetPrinterValue[idx].np_printer_port_size));
            memset(arrPrinterPort, 0x00, sizeof(char)*(PORT_LEN*p_NP->NetPrinterValue[idx].np_printer_port_size));
        }
        else
        {
            arrPrinterPort = (char *)realloc(arrPrinterPort, sizeof(char));
            memset(arrPrinterPort, 0x00, sizeof(char));
        }

        for(i=0; i<p_NP->NetPrinterValue[idx].np_printer_port_size; i++)
        {
            if(i == p_NP->NetPrinterValue[idx].np_printer_port_size-1)
            {
                sprintf(arrPrinterPort+strlen(arrPrinterPort), "%d", p_NP->NetPrinterValue[idx].np_printer_port[i]);
            }
            else
            {
                sprintf(arrPrinterPort+strlen(arrPrinterPort), "%d;", p_NP->NetPrinterValue[idx].np_printer_port[i]);
            }
        }

        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into NET_PRINTER_TB ("
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
                    "NP_CURR_HIST_DATE,"
                    "NP_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "%d,"
                    "%d,"
                    "%d,"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_NP->NetPrinterValue[idx].np_connected,
                    p_NP->NetPrinterValue[idx].np_discordance,
                    p_NP->NetPrinterValue[idx].np_web_connect,
                    p_NP->NetPrinterValue[idx].np_host_name,
                    p_NP->NetPrinterValue[idx].np_wsd_printer_device,
                    p_NP->NetPrinterValue[idx].np_wsd_location,
                    arrOpenPort,
                    arrPrinterPort,
                    p_NP->NetPrinterValue[idx].np_web_text,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&arrOpenPort);
            fcom_MallocFree((void**)&arrPrinterPort);
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "NET_PRINTER_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf (sqlBuf,
                 "update NET_PRINTER_TB set NP_CURR_HIST_SQ=%llu where NP_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&arrOpenPort);
    fcom_MallocFree((void**)&arrPrinterPort);
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}
/* INSERT NET_SCAN_TB */
int fdbif_MergeNetScanTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETSCAN*			p_NS,
        char*				p_prefix,
        char*				p_postfix)
{
    int					i = 0,idx = 0,rxt = 0;
    int					mergeCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				*arrOpenPort = NULL;
    char				*strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "NET_SCAN_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("NET_SCAN_TB", "NS_CURR_HIST_SQ,NS_CURR_HIST_DATE", hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("NET_SCAN_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    mergeCnt = 0;
    updateCnt = 0;

    // init for realloc
    if(fcom_malloc((void**)&arrOpenPort, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&strPrevHistSq);
        return (-1);
    }


    for(idx=0; idx<p_NS->size; idx++)
    {
        if( p_NS->NetScanValue[idx].ns_open_port_size > 0 )
        {
            arrOpenPort = (char *)realloc(	arrOpenPort,
                                              sizeof(char)*(PORT_LEN*p_NS->NetScanValue[idx].ns_open_port_size));
            memset(arrOpenPort, 0x00, sizeof(char)*(PORT_LEN*p_NS->NetScanValue[idx].ns_open_port_size));
        }
        else
        {
            arrOpenPort = (char *)realloc(arrOpenPort, sizeof(char));
            memset(arrOpenPort, 0x00, sizeof(char));
        }
        for(i=0; i<p_NS->NetScanValue[idx].ns_open_port_size; i++)
        {
            if(i == p_NS->NetScanValue[idx].ns_open_port_size-1)
            {
                sprintf(arrOpenPort+strlen(arrOpenPort), "%d", p_NS->NetScanValue[idx].ns_open_port[i]);
            }
            else
            {
                sprintf(arrOpenPort+strlen(arrOpenPort), "%d;", p_NS->NetScanValue[idx].ns_open_port[i]);
            }
        }
        WRITE_INFO(CATEGORY_DB,	"idx(%d) arrOpenPort(%s) ", idx,arrOpenPort);

        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into NET_SCAN_TB ("
                    "NS_DAP_AGENT,"
                    "NS_IP,"
                    "NS_MAC,"
                    "NS_MAC_MATCH,"
                    "NS_WEB_TEXT,"
                    "NS_OPEN_PORT,"
                    "NS_DETECT_TIME,"
                    "NS_PREV_HIST_SQ,"
                    "NS_CURR_HIST_DATE,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu) "
                    "on duplicate key update "
                    "NS_DAP_AGENT = %d,"
                    "NS_IP = '%s',"
                    "NS_MAC = '%s',"
                    "NS_MAC_MATCH = %d,"
                    "NS_WEB_TEXT = '%s',"
                    "NS_OPEN_PORT = '%s',"
                    "NS_DETECT_TIME = '%s',"
                    "NS_PREV_HIST_SQ = '%s',"
                    "NS_CURR_HIST_DATE = '%s',"
                    "HB_SQ = %llu,"
                    "US_SQ = %llu",
                    p_NS->NetScanValue[idx].ns_dap_agent,
                    p_NS->NetScanValue[idx].ns_ip,
                    p_NS->NetScanValue[idx].ns_mac,
                    p_NS->NetScanValue[idx].ns_mac_match,
                    p_NS->NetScanValue[idx].ns_web_text,
                    arrOpenPort,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    hbSq,
                    p_userSeq,
                    p_NS->NetScanValue[idx].ns_dap_agent,
                    p_NS->NetScanValue[idx].ns_ip,
                    p_NS->NetScanValue[idx].ns_mac,
                    p_NS->NetScanValue[idx].ns_mac_match,
                    p_NS->NetScanValue[idx].ns_web_text,
                    arrOpenPort,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    hbSq,
                    p_userSeq
        );

        mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&arrOpenPort);
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        if(p_NS->unchanged != 1) //바뀐것이있으면
        {
            tbSq = fdb_SqlInsertId(g_stMyCon);

            // insert history
            rxt = fdb_InsertHistory(tbSq, "NET_SCAN_TB", histTableName);
            currHistSq = fdb_SqlInsertId(g_stMyCon);

            // update currHistSq to tb
            memset  (sqlBuf, 0x00, sizeof(sqlBuf));
            snprintf (sqlBuf, sizeof(sqlBuf),
                     "update NET_SCAN_TB set NS_CURR_HIST_SQ=%llu where NS_SQ=%llu",
                     currHistSq,tbSq);

            updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
            }
        }
    } //for(idx=0; idx<p_NS->size; idx++)

    fcom_MallocFree((void**)&arrOpenPort);
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc mergeCnt(%d)updateCnt(%d)unchanged(%d)hbSq(%llu) ",
               mergeCnt,updateCnt,p_NS->unchanged,hbSq);

    return 0;
}
/* INSERT WIN_DRV_TB */
int fdbif_MergeWinDrvTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WINDRV *			p_WD,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					deleteCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				sqlPath[128 +1] = {0x00,};
    char				sqlClassDesc[128 +1] = {0x00,};
    char				sqlDesc[128 +1] = {0x00,};
    char				sqlDriver[128 +1] = {0x00,};
    char				sqlName[128 +1] = {0x00,};
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userkey(%s) ", p_userKey);
        return -2;
    }

    // get history table name
    memset	(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("WIN_DRV_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    // 루프를 돌며
    // dv_data_type이 1이면 tb에 추가하고 history 기록
    // dv_data_type이 2이면 tb에서 삭제하고 history에 기록
    for(idx=0; idx<p_WD->size; idx++)
    {
        memset(sqlDriver, 0x00, sizeof(sqlDriver));
        fcom_ReplaceAll(p_WD->WinDrvValue[idx].dv_driver, "\\", "\\\\", sqlDriver);
        memset	(sqlName, 0x00, sizeof(sqlName));
        fcom_ReplaceAll(p_WD->WinDrvValue[idx].dv_name, "\\", "\\\\", sqlName);
        memset	(sqlPath, 0x00, sizeof(sqlPath));
        fcom_ReplaceAll(p_WD->WinDrvValue[idx].dv_file_path, "\\", "\\\\", sqlPath);
        memset	(sqlClassDesc, 0x00, sizeof(sqlClassDesc));
        fcom_ReplaceAll(p_WD->WinDrvValue[idx].dv_class_desc, "\'", "\\'", sqlClassDesc);
        memset	(sqlDesc, 0x00, sizeof(sqlDesc));
        fcom_ReplaceAll(p_WD->WinDrvValue[idx].dv_desc, "\'", "\\'", sqlDesc);

        rowCnt = fdb_SelectSqWinDrv( hbSq,
                                     sqlDriver,
                                     sqlName,
                                     p_WD->WinDrvValue[idx].dv_service,
                                     &tbSq);


        if(rowCnt > 0) // if exists
        {
            char *strPrevHistSq = fdbif_GetPrevHistSqWinDrv(	"WIN_DRV_TB",
                                                                "DV_CURR_HIST_SQ,DV_CURR_HIST_DATE",
                                                                tbSq);
            // insert history
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            snprintf	(sqlBuf, sizeof (sqlBuf),
                        "insert into %s ("
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
                        "US_SQ"
                        ") values ("
                        "%llu,"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "'%s',"
                        "%d,"
                        "'%s',"
                        "'%s',"
                        "%llu,"
                        "%llu)",
                        histTableName,
                        tbSq,
                        p_WD->WinDrvValue[idx].dv_class,
                        sqlClassDesc,
                        sqlDesc,
                        sqlDriver,
                        p_WD->WinDrvValue[idx].dv_enum,
                        p_WD->WinDrvValue[idx].dv_file_company,
                        p_WD->WinDrvValue[idx].dv_file_copy_right,
                        p_WD->WinDrvValue[idx].dv_file_desc,
                        sqlPath,
                        p_WD->WinDrvValue[idx].dv_file_product,
                        p_WD->WinDrvValue[idx].dv_file_ver,
                        p_WD->WinDrvValue[idx].dv_location,
                        p_WD->WinDrvValue[idx].dv_mfg,
                        sqlName,
                        p_WD->WinDrvValue[idx].dv_service,
                        p_WD->WinDrvValue[idx].dv_start,
                        p_WD->WinDrvValue[idx].dv_status,
                        p_WD->WinDrvValue[idx].dv_type,
                        p_WD->WinDrvValue[idx].dv_data_type,
                        p_detectTime,
                        strPrevHistSq,
                        hbSq,
                        p_userSeq
            );

            insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
            }

            if(p_WD->WinDrvValue[idx].dv_data_type == 2) //삭제
            {
                memset	(sqlBuf, 0x00, sizeof(sqlBuf));
                snprintf	(sqlBuf, sizeof(sqlBuf),
                            "delete from WIN_DRV_TB where DV_SQ=%llu", tbSq);

                deleteCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                }

                WRITE_INFO(CATEGORY_DB, 	"Proc deleteCnt(%d)hbSq(%llu)tbSq(%llu)",
                           deleteCnt,hbSq,tbSq);
            }
            else // 추가
            {
                currHistSq 		= fdb_SqlInsertId(g_stMyCon);
                memset	(sqlBuf, 0x00, sizeof(sqlBuf));
                snprintf	(sqlBuf, sizeof(sqlBuf),
                            "update WIN_DRV_TB set "
                            "DV_CLASS='%s',"
                            "DV_CLASS_DESC='%s',"
                            "DV_DESC='%s',"
                            "DV_DRIVER='%s',"
                            "DV_ENUM='%s',"
                            "DV_FILE_COMPANY='%s',"
                            "DV_FILE_COPY_RIGHT='%s',"
                            "DV_FILE_DESC='%s',"
                            "DV_FILE_PATH='%s',"
                            "DV_FILE_PRODUCT='%s',"
                            "DV_FILE_VER='%s',"
                            "DV_LOCATION='%s',"
                            "DV_MFG='%s',"
                            "DV_NAME='%s',"
                            "DV_SERVICE='%s',"
                            "DV_START='%s',"
                            "DV_STATUS='%s',"
                            "DV_TYPE='%s',"
                            "DV_DATA_TYPE=%d,"
                            "DV_DETECT_TIME='%s',"
                            "DV_PREV_HIST_SQ='%s',"
                            "DV_CURR_HIST_SQ=%llu,"
                            "DV_CURR_HIST_DATE='%s',"
                            "HB_SQ=%llu,"
                            "US_SQ=%llu "
                            "where DV_SQ=%llu",
                            p_WD->WinDrvValue[idx].dv_class,
                            sqlClassDesc,
                            sqlDesc,
                            sqlDriver,
                            p_WD->WinDrvValue[idx].dv_enum,
                            p_WD->WinDrvValue[idx].dv_file_company,
                            p_WD->WinDrvValue[idx].dv_file_copy_right,
                            p_WD->WinDrvValue[idx].dv_file_desc,
                            sqlPath,
                            p_WD->WinDrvValue[idx].dv_file_product,
                            p_WD->WinDrvValue[idx].dv_file_ver,
                            p_WD->WinDrvValue[idx].dv_location,
                            p_WD->WinDrvValue[idx].dv_mfg,
                            sqlName,
                            p_WD->WinDrvValue[idx].dv_service,
                            p_WD->WinDrvValue[idx].dv_start,
                            p_WD->WinDrvValue[idx].dv_status,
                            p_WD->WinDrvValue[idx].dv_type,
                            p_WD->WinDrvValue[idx].dv_data_type,
                            p_detectTime,
                            strPrevHistSq,
                            currHistSq,
                            strCurrHistDate,
                            hbSq,
                            p_userSeq,
                            tbSq
                );

                updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                }

                WRITE_INFO(CATEGORY_DB, "Proc updateCnt(%d)hbSq(%llu)tbSq(%llu)",
                           updateCnt,hbSq,tbSq);
            }
            fcom_MallocFree((void**)&strPrevHistSq);
        }
        else // if not exists
        {
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            if(p_WD->WinDrvValue[idx].dv_data_type == 2) //삭제
            {
                WRITE_INFO(CATEGORY_DEBUG, "Not found data, hbSq(%llu)driver(%s)name(%s)service(%s)",
                           hbSq,sqlDriver,sqlName,p_WD->WinDrvValue[idx].dv_service);
            }
            else //추가
            {
                snprintf	(sqlBuf, sizeof(sqlBuf),
                            "insert into WIN_DRV_TB ("
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
                            "DV_CURR_HIST_DATE,"
                            "HB_SQ,"
                            "US_SQ"
                            ") values ("
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "%d,"
                            "'%s',"
                            "'%s',"
                            "%llu,"
                            "%llu)",
                            p_WD->WinDrvValue[idx].dv_class,
                            sqlClassDesc,
                            sqlDesc,
                            sqlDriver,
                            p_WD->WinDrvValue[idx].dv_enum,
                            p_WD->WinDrvValue[idx].dv_file_company,
                            p_WD->WinDrvValue[idx].dv_file_copy_right,
                            p_WD->WinDrvValue[idx].dv_file_desc,
                            sqlPath,
                            p_WD->WinDrvValue[idx].dv_file_product,
                            p_WD->WinDrvValue[idx].dv_file_ver,
                            p_WD->WinDrvValue[idx].dv_location,
                            p_WD->WinDrvValue[idx].dv_mfg,
                            sqlName,
                            p_WD->WinDrvValue[idx].dv_service,
                            p_WD->WinDrvValue[idx].dv_start,
                            p_WD->WinDrvValue[idx].dv_status,
                            p_WD->WinDrvValue[idx].dv_type,
                            p_WD->WinDrvValue[idx].dv_data_type,
                            p_detectTime,
                            strCurrHistDate,
                            hbSq,
                            p_userSeq
                );

                insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                }

                tbSq = fdb_SqlInsertId(g_stMyCon);

                // insert history
                rxt = fdb_InsertHistory(tbSq, "WIN_DRV_TB", histTableName);
                currHistSq = fdb_SqlInsertId(g_stMyCon);

                memset	(sqlBuf, 0x00, sizeof(sqlBuf));
                sprintf	(sqlBuf,
                            "update WIN_DRV_TB set DV_CURR_HIST_SQ=%llu where DV_SQ=%llu",
                            currHistSq,tbSq);

                //LogDRet(5, "- sqlBuf(%s)\n", STR_DEBUG,sqlBuf);
                updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                }
            }
        }
    } // for

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)deleteCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,deleteCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT RDP_SESSION_TB */
int fdbif_MergeRdpSessionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_RDP_SESSION *		p_RDP,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlName[128 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s)", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "RDP_SESSION_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("RDP_SESSION_TB", "RDP_CURR_HIST_SQ,RDP_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from RDP_SESSION_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("RDP_SESSION_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset  (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset		(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_RDP->rdp_summary, "\'", "\\'", sqlSummary);

    for(idx=0; idx<p_RDP->size; idx++)
    {
        memset	(sqlName, 0x00, sizeof(sqlName));
        fcom_ReplaceAll(p_RDP->RdpSessionValue[idx].rdp_client_name, "\\", "\\\\", sqlName);
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into RDP_SESSION_TB ("
                    "RDP_CLIENT_IP,"
                    "RDP_CLIENT_NAME,"
                    "RDP_CONNECT_TIME,"
                    "RDP_USER_ID,"
                    "RDP_DETECT_TIME,"
                    "RDP_PREV_HIST_SQ,"
                    "RDP_CURR_HIST_DATE,"
                    "RDP_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_RDP->RdpSessionValue[idx].rdp_client_ip,
                    sqlName,
                    p_RDP->RdpSessionValue[idx].rdp_connect_time,
                    p_RDP->RdpSessionValue[idx].rdp_user_id,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "RDP_SESSION_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update RDP_SESSION_TB set RDP_CURR_HIST_SQ=%llu where RDP_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT OS_ACCOUNT_TB */
int fdbif_MergeOsAccountTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_OSACCOUNT *	p_OA,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    rxt = fdb_SelectCountHbSq(hbSq, "OS_ACCOUNT_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("OS_ACCOUNT_TB", "OA_CURR_HIST_SQ,OA_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf, "delete from OS_ACCOUNT_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("OS_ACCOUNT_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_OA->oa_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_OA->size; idx++)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf, sizeof(sqlBuf),
                "insert into OS_ACCOUNT_TB ("
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
                "OA_CURR_HIST_DATE,"
                "OA_SUMMARY,"
                "HB_SQ,"
                "US_SQ"
                ") values ("
                "%d,"
                "'%s',"
                "%d,"
                "'%s',"
                "%d,"
                "'%s',"
                "'%s',"
                "%d,"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "'%s',"
                "%llu,"
                "%llu)",
                p_OA->OSAccountValue[idx].oa_type,
                p_OA->OSAccountValue[idx].oa_name,
                p_OA->OSAccountValue[idx].oa_local,
                p_OA->OSAccountValue[idx].oa_sid,
                p_OA->OSAccountValue[idx].oa_sid_type,
                p_OA->OSAccountValue[idx].oa_caption,
                p_OA->OSAccountValue[idx].oa_desc,
                p_OA->OSAccountValue[idx].oa_disabled,
                p_OA->OSAccountValue[idx].oa_status,
                p_detectTime,
                strPrevHistSq,
                strCurrHistDate,
                sqlSummary,
                hbSq,
                p_userSeq);

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "OS_ACCOUNT_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update OS_ACCOUNT_TB set OA_CURR_HIST_SQ=%llu where OA_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0) {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT SHARE_FOLDER_TB */
int fdbif_MergeShareFolderTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_SHARE_FOLDER*		p_SF,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*				strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlPath[256 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "SHARE_FOLDER_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("SHARE_FOLDER_TB", "SF_CURR_HIST_SQ,SF_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf, "delete from SHARE_FOLDER_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("SHARE_FOLDER_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_SF->sf_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_SF->size; idx++)
    {
        memset(sqlPath, 0x00, sizeof(sqlPath));
        fcom_ReplaceAll(p_SF->ShareFolderValue[idx].sf_path, "\\", "\\\\", sqlPath);
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into SHARE_FOLDER_TB ("
                    "SF_NAME,"
                    "SF_TYPE,"
                    "SF_PATH,"
                    "SF_STATUS,"
                    "SF_DETECT_TIME,"
                    "SF_PREV_HIST_SQ,"
                    "SF_CURR_HIST_DATE,"
                    "SF_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_SF->ShareFolderValue[idx].sf_name,
                    p_SF->ShareFolderValue[idx].sf_type,
                    sqlPath,
                    p_SF->ShareFolderValue[idx].sf_status,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "SHARE_FOLDER_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update SHARE_FOLDER_TB set SF_CURR_HIST_SQ=%llu where SF_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB,"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT INFRARED_DEVICE_TB */
int fdbif_MergeInfraredDeviceTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_INFRARED_DEVICE*	p_ID,
        char*				p_prefix,
        char*				p_postfix
)
{
    int					idx = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				*strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "INFRARED_DEVICE_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("INFRARED_DEVICE_TB", "ID_CURR_HIST_SQ,ID_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from INFRARED_DEVICE_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);

        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("INFRARED_DEVICE_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset (strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf (strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_ID->id_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;
    for(idx=0; idx<p_ID->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into INFRARED_DEVICE_TB ("
                    "ID_NAME,"
                    "ID_MF,"
                    "ID_PROTOCOL_SUPPORTED,"
                    "ID_STATUS,"
                    "ID_STATUS_INFO,"
                    "ID_DETECT_TIME,"
                    "ID_PREV_HIST_SQ,"
                    "ID_CURR_HIST_DATE,"
                    "ID_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_ID->InfraredDeviceValue[idx].id_name,
                    p_ID->InfraredDeviceValue[idx].id_mf,
                    p_ID->InfraredDeviceValue[idx].id_protocol_supported,
                    p_ID->InfraredDeviceValue[idx].id_status,
                    p_ID->InfraredDeviceValue[idx].id_status_info,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "INFRARED_DEVICE_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf (sqlBuf, sizeof(sqlBuf),
                 "update INFRARED_DEVICE_TB set ID_CURR_HIST_SQ=%llu where ID_SQ=%llu",
                 currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode  != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d)updateCnt(%d)hpSq(%llu)",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT PROCESS_TB */
int fdbif_MergeProcessTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_PROCESS *			p_PS,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0,i = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				*arrSvrAddr = NULL;
    char				*strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlPath[256 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "PROCESS_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("PROCESS_TB", "PS_CURR_HIST_SQ,PS_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from PROCESS_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            // free strPrevHistSq
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);

            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("PROCESS_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset(strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf	(strCurrHistDate, "%s%s", p_prefix,p_postfix);


    memset		(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_PS->ps_summary, "\'", "\\'", sqlSummary);

    insertCnt = 0;
    updateCnt = 0;

    // init for realloc
    if(fcom_malloc((void**)&arrSvrAddr, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&strPrevHistSq);
        return (-1);
    }

    for(idx=0; idx<p_PS->size; idx++)
    {
        if( p_PS->ProcessValue[idx].ps_connected_svr_addr_size > 0 )
        {
            arrSvrAddr= (char *)realloc(arrSvrAddr,
                                        sizeof(char)*(ADDR_LEN*p_PS->ProcessValue[idx].ps_connected_svr_addr_size));
            memset(arrSvrAddr, 0x00, sizeof(char)*(ADDR_LEN*p_PS->ProcessValue[idx].ps_connected_svr_addr_size));
        }
        else
        {
            arrSvrAddr = (char *)realloc(arrSvrAddr, sizeof(char));
            memset(arrSvrAddr, 0x00, sizeof(char));
        }

        for(i=0; i<p_PS->ProcessValue[idx].ps_connected_svr_addr_size; i++)
        {
            if(i == p_PS->ProcessValue[idx].ps_connected_svr_addr_size-1)
            {
                sprintf(arrSvrAddr+strlen(arrSvrAddr), "%s", p_PS->ProcessValue[idx].ps_connected_svr_addr[i]);
            }
            else
            {
                sprintf(arrSvrAddr+strlen(arrSvrAddr), "%s;", p_PS->ProcessValue[idx].ps_connected_svr_addr[i]);
            }
        }
        memset	(sqlPath, 0x00, sizeof(sqlPath));

        fcom_ReplaceAll(p_PS->ProcessValue[idx].ps_file_path, "\\", "\\\\", sqlPath);
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into PROCESS_TB ("
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
                    "PS_CURR_HIST_DATE,"
                    "PS_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_PS->ProcessValue[idx].ps_file_name,
                    p_PS->ProcessValue[idx].ps_type,
                    sqlPath,
                    p_PS->ProcessValue[idx].ps_original_file_name,
                    p_PS->ProcessValue[idx].ps_company_name,
                    p_PS->ProcessValue[idx].ps_file_desc,
                    p_PS->ProcessValue[idx].ps_file_ver,
                    p_PS->ProcessValue[idx].ps_copy_right,
                    p_PS->ProcessValue[idx].ps_running,
                    arrSvrAddr,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );


        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&arrSvrAddr);
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);

            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "PROCESS_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "update PROCESS_TB set PS_CURR_HIST_SQ=%llu where PS_SQ=%llu",
                    currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);

        }
    }

    fcom_MallocFree((void**)&arrSvrAddr);
    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB,	"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT ROUTER_TB */
int fdbif_MergeRouterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_ROUTER *			p_RT,
        char*				p_prefix,
        char*				p_postfix)
{
    int					idx = 0;
    int					rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char				*strPrevHistSq = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s) ", p_userKey);
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "ROUTER_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq fromtb
        strPrevHistSq = fdbif_GetPrevHistSq("ROUTER_TB", "RT_CURR_HIST_SQ,RT_CURR_HIST_DATE", hbSq);
        // 2. delete tb
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),"delete from ROUTER_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            // free strPrevHistSq
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }
        WRITE_INFO(CATEGORY_DB, "Succeed in delete, rowCnt(%d)hbSq(%llu) ",
                   rowCnt,hbSq);
//        fdb_SqlCommit(g_stMyCon);
    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("ROUTER_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset	(strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf	(strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset		(sqlSummary, 0x00, sizeof(sqlSummary));
    fcom_ReplaceAll(p_RT->rt_summary, "\'", "\\'", sqlSummary);

    for(idx=0; idx<p_RT->size; idx++)
    {
        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "insert into ROUTER_TB ("
                    "RT_DETECT_TYPE,"
                    "RT_IPADDR,"
                    "RT_MAC_ADDR,"
                    "RT_WEB_TEXT,"
                    "RT_CAPTION,"
                    "RT_DETECT_TIME,"
                    "RT_PREV_HIST_SQ,"
                    "RT_CURR_HIST_DATE,"
                    "RT_SUMMARY,"
                    "HB_SQ,"
                    "US_SQ"
                    ") values ("
                    "%d,"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "'%s',"
                    "%llu,"
                    "%llu)",
                    p_RT->RouterValue[idx].rt_detect_type,
                    p_RT->RouterValue[idx].rt_ipaddr,
                    p_RT->RouterValue[idx].rt_mac_addr,
                    p_RT->RouterValue[idx].rt_web_text,
                    p_RT->RouterValue[idx].rt_caption,
                    p_detectTime,
                    strPrevHistSq,
                    strCurrHistDate,
                    sqlSummary,
                    hbSq,
                    p_userSeq
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        // insert history
        rxt = fdb_InsertHistory(tbSq, "ROUTER_TB", histTableName);
        currHistSq = fdb_SqlInsertId(g_stMyCon);

        memset	(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf,
                    "update ROUTER_TB set RT_CURR_HIST_SQ=%llu where RT_SQ=%llu",
                    currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);

    WRITE_INFO(CATEGORY_DB,	"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);

    return 0;
}

/* INSERT CTRL_PROCESS_CPU_TB */
int fdbif_MergeCpuUsageTb(char*			    	p_userKey,
                          unsigned long long 	p_userSeq,
                          char*				    p_detectTime,
                          _DAP_CPU_USAGE*		p_CpuUsage,
                          char*				    p_prefix,
                          char*				    p_postfix)
{
    int					idx = 0,i = 0,rxt = 0;
    int					rowCnt = 0;
    int					insertCnt = 0;
    int					updateCnt = 0;
    int                 nSkipFlag = 0;
    unsigned long long	hbSq = 0;
    unsigned long long	tbSq = 0;
    unsigned long long	currHistSq = 0;
    char*               strPrevHistSq = NULL;
    int*                ptrProcessId = NULL;
    char				strCurrHistDate[8 +1] = {0x00,};
    char				sqlBuf[4096 +1] = {0x00,};
    char				sqlSummary[1280 +1] = {0x00,};
    char				histTableName[50 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_WARNING(CATEGORY_DEBUG,"Fail in user_key(%s)",p_userKey );
        return 0;
    }

    rxt = fdbif_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_WARNING(CATEGORY_DEBUG,"Not found hb_sq, userKey(%s)",p_userKey );
        return -2;
    }
    if(p_CpuUsage->historysize > 0)
    {
        if(fcom_malloc((void**)&ptrProcessId, sizeof(int) * p_CpuUsage->historysize) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        for(idx=0; idx < p_CpuUsage->historysize; idx++)
        {
            ptrProcessId[idx] = atoi(p_CpuUsage->CpuHistoryValue[idx].cpu_usage_process_id);
        }

    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    rxt = fdb_SelectCountHbSq(hbSq, "CTRL_PROCESS_CPU_TB");
    if(rxt > 0)
    {
        // If existing data exists
        // 1. get curr_hist_sq from tb
        strPrevHistSq = fdbif_GetPrevHistSq("CTRL_PROCESS_CPU_TB", "CP_CURR_HIST_SQ,CP_CURR_HIST_DATE", hbSq);

        // 2. delete tb
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf, "delete from CTRL_PROCESS_CPU_TB where HB_SQ=%llu", hbSq);
        rowCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            // free strPrevHistSq
            fcom_MallocFree((void**)&strPrevHistSq);
            fcom_MallocFree((void**)&ptrProcessId);

            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);

            return -1;
        }
        WRITE_INFO(CATEGORY_INFO, "Succeed in delete, rowCnt(%d)hbSq(%llu)",
                   rowCnt,hbSq);

    }

    // get history table name
    memset(histTableName, 0x00, sizeof(histTableName));
    fdb_GetHistoryTableName("CTRL_PROCESS_CPU_TB", histTableName, p_prefix, p_postfix);

    // put curr hist date
    memset(strCurrHistDate, 0x00, sizeof(strCurrHistDate));
    sprintf	(strCurrHistDate, "%s%s", p_prefix,p_postfix);

    memset(sqlSummary, 0x00, sizeof(sqlSummary));

    insertCnt = 0;
    updateCnt = 0;

    /* CPU Usage */
    for(idx=0; idx < p_CpuUsage->size; idx++)
    {
        nSkipFlag = 0;
        for(i = 0; i < p_CpuUsage->historysize; i++)
        {
            /* Body에도있고 Summary에도 있는 Process정보 History만 처리되도록 Skip. */
            if(atoi(p_CpuUsage->CpuUsageValue[idx].cpu_usage_process_id) == ptrProcessId[i])
            {
                nSkipFlag = 1;
                break;
            }
        }
        if(nSkipFlag == 1)
            continue;

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf, sizeof(sqlBuf),
                "INSERT INTO CTRL_PROCESS_CPU_TB( "
                "CP_ALARM_TYPE,"
                "CP_PNAME,"
                "CP_VALUE,"
                "CP_DETECT_TIME,"
                "CP_CURR_HIST_DATE,"

                "CP_PREV_HIST_SQ,"
                "HB_SQ,"
                "US_SQ,"
                "CP_START_TIME,"
                "CP_PROCESS_ID,"

                "CP_VALUE_CONDITION,"
                "CP_VALUE_LIMIT,"
                "CP_DURATION_TIME,"
                "CP_DURATION_TIME_CONDITION,"
                "CP_NEW_DATA_FLAG,"

                "CP_STATUS,"
                "CP_STATUS_NAME,"
                "CP_IS_DAP_FLAG"
                ")"
                "VALUES ( "
                "'%s'," // CP_ALARM_TYPE
                "'%s'," //CP_PNAME
                "'%s'," //CP_VALUE
                "'%s'," //CP_DETECT_TIME
                "'%s'," //CP_CURR_HIST_DATE

                "'%s'," //CP_PREV_HIST_SQ
                " %llu ," //HQ_SQ
                " %llu ," //US_SQ
                " '%s' ," //CP_START_TIME
                " '%s' ," //CP_PROCESS_ID

                " '%s' ," //CP_VALUE_CONDITION
                " '%s' ," //CP_VALUE_LIMIT
                " '%s' ," //CP_DURATION_TIME
                " '%s' ," //CP_DURATION_TIME_CONDITION
                " %d   ," //CP_NEW_DATA_FLAG

                " %s   ," //CP_STATUS
                " '%s' ," //CP_STATUS_NAME
                " %d " //CP_IS_DAP_FLAG
                ") ",
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_type, //CP_ALARM_TYPE
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_process_name, //CP_PNAME
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate, //CP_VALUE
                p_detectTime, //CP_DETECT_TIME , json 받은시간의 detect time
                strCurrHistDate,//CP_CURR_HIST_DATE

                strPrevHistSq,//CP_PREV_HIST_SQ
                hbSq,//HQ_SQ
                p_userSeq,//US_SQ
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_detect_time, //CP_START_TIME
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_process_id, //CP_PROCESS_ID

                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate_condition, //CP_VALUE_CONDITION
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate_limit,//CP_VALUE_LIMIT
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_duration_time,//CP_DURATION_TIME
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_duration_time_condition,//CP_DURATION_TIME_CONDITION
                0,

                p_CpuUsage->CpuUsageValue[idx].cpu_usage_status,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_status_name,
                p_CpuUsage->CpuUsageValue[idx].is_dap_agent
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if (g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            fcom_MallocFree((void**)&ptrProcessId);

            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        /* 추가 history 테이블에 summary 변경이력 데이터 추가 */
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf, sizeof(sqlBuf),
                "INSERT INTO %s "
                "( "
                "CP_SQ,"
                "CP_ALARM_TYPE,"
                "CP_PNAME,"
                "CP_VALUE,"
                "CP_DETECT_TIME,"

                "HB_SQ,"
                "US_SQ,"
                "CPH_RECORD_TIME,"
                "CP_START_TIME,"
                "CP_PROCESS_ID,"

                "CP_VALUE_CONDITION,"
                "CP_VALUE_LIMIT,"
                "CP_DURATION_TIME,"
                "CP_DURATION_TIME_CONDITION,"
                "CP_NEW_DATA_FLAG,"

                "CP_PREV_HIST_SQ,"
                "CP_STATUS,"
                "CP_STATUS_NAME,"
                "CP_IS_DAP_FLAG"
                ")"
                "VALUES("
                "%llu," //CP_SQ
                "'%s'," //CP_ALARM_TYPE
                "'%s'," //CP_PNAME
                "'%s'," //CP_VALUE
                "'%s'," //CP_DETECT_TIME

                " %llu ," //HB_SQ
                " %llu ," //US_SQ
                " sysdate() ," //CPH_RECORD_TIME
                " '%s', " //CP_START_TIME
                " '%s'," //CP_PROCESS_ID

                " '%s'," //CP_VALUE_CONDITION
                " '%s'," //CP_VALUE_LIMIT
                " '%s'," //CP_DURATION_TIME
                " '%s'," //CP_DURATION_TIME_CONDITION
                "  %d ," //CP_NEW_DATA_FLAG

                " '%s'," //CP_PREV_HIST_SQ
                "  %s ," //CP_STATUS
                " '%s',"  //CP_STATUS_NAME
                " %d " //CP_IS_DAP_FLAG
                ") ",
                histTableName,
                tbSq,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_type,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_process_name,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate,

                p_detectTime,
                hbSq,
                p_userSeq,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_detect_time,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_process_id,

                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate_condition,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_rate_limit,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_duration_time,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_duration_time_condition,
                0,

                strPrevHistSq,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_status,
                p_CpuUsage->CpuUsageValue[idx].cpu_usage_status_name,
                p_CpuUsage->CpuUsageValue[idx].is_dap_agent
        );

        fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if (g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            if(p_CpuUsage->historysize > 0)
            {
                fcom_MallocFree((void**)&ptrProcessId);
            }
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        if(p_CpuUsage->size <= 0)
            currHistSq = 0;
        else
            currHistSq = fdb_SqlInsertId(g_stMyCon);


        // update currHistSq to tb
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "update CTRL_PROCESS_CPU_TB set CP_CURR_HIST_SQ=%llu where CP_SQ=%llu",
                    currHistSq,tbSq);

        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    /* History */
    for(idx=0; idx < p_CpuUsage->historysize; idx++)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf, sizeof(sqlBuf),
                "INSERT INTO CTRL_PROCESS_CPU_TB( "
                "CP_ALARM_TYPE,"
                "CP_PNAME,"
                "CP_VALUE,"
                "CP_DETECT_TIME,"
                "CP_CURR_HIST_DATE,"

                "HB_SQ,"
                "US_SQ,"
                "CP_START_TIME,"
                "CP_PROCESS_ID,"
                "CP_VALUE_CONDITION,"

                "CP_VALUE_LIMIT,"
                "CP_DURATION_TIME,"
                "CP_DURATION_TIME_CONDITION,"
                "CP_NEW_DATA_FLAG,"
                "CP_PREV_HIST_SQ,"

                "CP_STATUS,"
                "CP_STATUS_NAME,"
                "CP_IS_DAP_FLAG"
                ")"
                "VALUES ( "
                "'%s',"  //CP_ALARM_TYPE
                "'%s',"  //CP_PNAME
                "'%s',"  //CP_VALUE
                "'%s',"  //CP_DETECT_TIME
                "'%s',"  //CP_CURR_HIST_DATE

                " %llu ," //HQ_SQ
                " %llu ," //US_SQ
                " '%s' ," //CP_START_TIME
                " '%s' ," //CP_PROCESS_ID
                " '%s' ," //CP_VALUE_CONDITION

                " '%s' ," //CP_VALUE_LIMIT
                " '%s' ," //CP_DURATION_TIME
                " '%s' ," //CP_DURATION_TIME_CONDITION
                "  %d  ,"    //CP_NEW_DATA_FLAG
                " '%s' ," //CP_PREV_HIST_SQ

                " %s   ," //CP_STATUS
                "'%s'  ," //CP_STATUS_NAME
                " %d   " //CP_IS_DAP_FLAG
                ") ",
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_type, //CP_ALARM_TYPE
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_process_name, //CP_PNAME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate, //CP_VALUE
                p_detectTime, //CP_DETECT_TIME , json 받은시간의 detect time
                strCurrHistDate,//CP_CURR_HIST_DATE

                hbSq,//HQ_SQ
                p_userSeq,//US_SQ
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_detect_time, //CP_START_TIME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_process_id, //CP_PROCESS_ID
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate_condition, //CP_VALUE_CONDITION

                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate_limit,//CP_VALUE_LIMIT
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_duration_time,//CP_DURATION_TIME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_duration_time_condition,//CP_DURATION_TIME_CONDITION
                1,
                strPrevHistSq,

                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_status,
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_status_name,
                p_CpuUsage->CpuHistoryValue[idx].is_dap_agent
        );

        insertCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if (g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            if(p_CpuUsage->historysize > 0)
            {
                fcom_MallocFree((void**)&ptrProcessId);
            }
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }

        tbSq = fdb_SqlInsertId(g_stMyCon);

        /* 추가 history 테이블에 summary 변경이력 데이터 추가 */
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf, sizeof(sqlBuf),
                "INSERT INTO %s "
                "( "
                "CP_SQ,"
                "CP_ALARM_TYPE,"
                "CP_PNAME,"
                "CP_VALUE,"
                "CP_DETECT_TIME,"

                "HB_SQ,"
                "US_SQ,"
                "CPH_RECORD_TIME,"
                "CP_START_TIME,"
                "CP_PROCESS_ID,"

                "CP_VALUE_CONDITION,"
                "CP_VALUE_LIMIT,"
                "CP_DURATION_TIME,"
                "CP_DURATION_TIME_CONDITION,"
                "CP_NEW_DATA_FLAG,"

                "CP_PREV_HIST_SQ,"
                "CP_STATUS,"
                "CP_STATUS_NAME,"
                "CP_IS_DAP_FLAG"
                ")"
                "VALUES("
                "%llu," //CP_SQ
                "'%s'," // CP_ALARM_TYPE
                "'%s'," //CP_PNAME
                "'%s'," //CP_VALUE
                "'%s'," //CP_DETECT_TIME

                " %llu ," //HB_SQ
                " %llu ," //US_SQ
                " sysdate() ," //CPH_RECORD_TIME
                " '%s', " //CPH_START_TIME
                "'%s'," //CP_PROCESS_ID

                "'%s'," //CP_VALUE_CONDITION
                "'%s'," //CP_VALUE_LIMIT
                "'%s'," //CP_DURATION_TIME
                "'%s'," //CP_DURATION_TIME_CONDITION
                " %d ," //CP_NEW_DATA_FLAG

                "'%s'," //CP_PREV_HIST_SQ
                "%s  ," //CP_STATUS
                "'%s',"  //CP_STATUS_NAME
                " %d " //CP_IS_DAP_FLAG
                ") ",
                histTableName,
                tbSq, //CP_SQ
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_type, //CP_ALARM_TYPE
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_process_name, //CP_PNAME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate, //CP_VALUE

                p_detectTime, //CPH_DETECT_TIME
                hbSq, //HB_SQ
                p_userSeq, //US_SQ
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_detect_time, //CP_START_TIME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_process_id, //CP_PROCESS_ID

                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate_condition, //CP_VALUE_CONDITION
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_rate_limit,//CP_VALUE_LIMIT
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_duration_time,//CP_DURATION_TIME
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_duration_time_condition, //CP_DURATION_TIME_CONDITION
                1, //CP_NEW_DATA_FLAG
                strPrevHistSq,

                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_status,
                p_CpuUsage->CpuHistoryValue[idx].cpu_usage_status_name,
                p_CpuUsage->CpuHistoryValue[idx].is_dap_agent
        );

        fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if (g_stMyCon->nErrCode != 0)
        {
            fcom_MallocFree((void**)&strPrevHistSq);
            if(p_CpuUsage->historysize > 0)
            {
                fcom_MallocFree((void**)&ptrProcessId);
            }

            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            return -1;
        }


        if(p_CpuUsage->historysize <= 0)
            currHistSq = 0;
        else
            currHistSq = fdb_SqlInsertId(g_stMyCon);

        // update currHistSq to tb
        memset  (sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf	(sqlBuf, sizeof(sqlBuf),
                    "update CTRL_PROCESS_CPU_TB set CP_CURR_HIST_SQ=%llu where CP_SQ=%llu",
                    currHistSq,tbSq);
        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    fcom_MallocFree((void**)&strPrevHistSq);
    if(p_CpuUsage->historysize > 0)
    {
        fcom_MallocFree((void**)&ptrProcessId);

    }
    WRITE_INFO(CATEGORY_DB,	"Proc insertCnt(%d)updateCnt(%d)hbSq(%llu) ",
               insertCnt,updateCnt,hbSq);
    return 0;
}
int fdbif_InsertEmptyBase(_DAP_AGENT_INFO* p_AgentInfo )
{
    int     insertCnt = 0;
    char    sqlBuf[512 +1] = {0x00,};


    if (strcmp(p_AgentInfo->user_key, "unknown") == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in user_key(%s)", p_AgentInfo->user_key);
        return 0;
    }
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf),
            "insert into HW_BASE_TB (" //HB_ACCESS_TIME은 정책요청시에 넣는다
            "US_SQ,"
            "HB_UNQ,"
            "HB_ACCESS_IP,"
            "HB_AGENT_VER,"
            "HB_RECORD_TIME"
            ") values ("
            "%llu,"
            "'%s',"
            "'%s',"
            "'%s',"
            "sysdate()) ",
            p_AgentInfo->user_seq,
            p_AgentInfo->user_key,
            p_AgentInfo->user_ip,
            p_AgentInfo->agent_ver
    );

    insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB, "Proc insertCnt(%d) ", insertCnt);

    return 0;
}
