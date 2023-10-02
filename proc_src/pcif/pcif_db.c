//
// Created by KimByoungGook on 2020-06-30.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "com/dap_com.h"
#include  "sock/dap_sock.h"
#include "secure/dap_secure.h"
#include "secure/bcrypt.h"

#include "pcif.h"

int fpcif_LoadDpPrintPort(dbRule *rule)
{
    int		rowCnt = 0;
    int		loop = 0;
    char	sqlBuf[128] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf, "select PP_PORT from DETECT_PRINTER_PORT_TB where PP_USE=1");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    if(rule->dp_print_port != NULL)
        fcom_MallocFree((void**)&(rule->dp_print_port));

    loop = 0;
    if(rowCnt > 0)
    {
        if(fcom_malloc((void**)&(rule->dp_print_port),sizeof(char)*(MAX_PORT_LENGTH*rowCnt)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(loop == 0)	sprintf(rule->dp_print_port,					"%d",  atoi(g_stMyCon->row[0]));
            else			sprintf(rule->dp_print_port+strlen(rule->dp_print_port),";%d", atoi(g_stMyCon->row[0]));
            loop++;
        }//while
    }//if
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO,"PID[%d] Reload DETECT_PRINTER_PORT_TB loop(%d)", getpid(),loop);

    return loop;
}
int fpcif_LoadScheduleSt(dbSchd *pSchdInfo)
{
    int				loop = 0;
    int				rowCnt = 0;
    char			sqlBuf[256] = {0x00,};
    register int	preIndex = -1;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RU_SQ,RS_NAME,RS_TYPE,RS_WEEKOFDAY,RS_DAYOFWEEK,"
                       "RS_DAY,RS_DATE,RS_TIME,RS_EXCEPTION,RS_USE "
                       "from RULE_SCHEDULE_TB where RS_USE=1");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            preIndex = fpcif_ScheduleInit(pSchdInfo, preIndex, loop);
            if (g_stMyCon->row[0] != NULL)	pSchdInfo->ru_sq[loop] = atol( g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
            {
                if(strlen(g_stMyCon->row[1]) > 32)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[1] );
                }
                snprintf(pSchdInfo->rs_name[loop], sizeof(pSchdInfo->rs_name[loop]),"%s",g_stMyCon->row[1]);
            }

            if (g_stMyCon->row[2] != NULL)	pSchdInfo->rs_type[loop] = *g_stMyCon->row[2];
            if (g_stMyCon->row[3] != NULL)
            {
                if(strlen(g_stMyCon->row[3]) > 20)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[3] );
                }
                snprintf(pSchdInfo->rs_weekofday[loop], sizeof(pSchdInfo->rs_weekofday[loop]),"%s",g_stMyCon->row[3]);

            }
            if (g_stMyCon->row[4] != NULL)
            {
                if(strlen(g_stMyCon->row[4]) > 30)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[4]);
                }
                snprintf(pSchdInfo->rs_dayofweek[loop], sizeof(pSchdInfo->rs_dayofweek[loop]),"%s",g_stMyCon->row[4]);

            }
            if (g_stMyCon->row[5] != NULL)
            {
                if(strlen(g_stMyCon->row[5]) > 100)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[5]);
                }
                snprintf(pSchdInfo->rs_day[loop], sizeof(pSchdInfo->rs_day[loop]),"%s",g_stMyCon->row[5]);
            }
            if (g_stMyCon->row[6] != NULL)
            {
                if(strlen(g_stMyCon->row[6]) > 17)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[6]);
                }
                snprintf(pSchdInfo->rs_date[loop], sizeof(pSchdInfo->rs_date[loop]),"%s",g_stMyCon->row[6]);
//                strcpy(pSchdInfo->rs_date[loop], g_stMyCon->row[6]);
            }
            if (g_stMyCon->row[7] != NULL)
            {
                if(strlen(g_stMyCon->row[7]) > 13)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"RULE_SCHEDULE_TB Data Is Not Length : [%s] ",g_stMyCon->row[7] );
                }
                snprintf(pSchdInfo->rs_time[loop], sizeof(pSchdInfo->rs_time[loop]),"%s",g_stMyCon->row[7]);
            }
            if (g_stMyCon->row[8] != NULL)	pSchdInfo->rs_exception[loop] = *g_stMyCon->row[8];

            loop++;
        }//while
        pSchdInfo->tot_cnt = loop; //count�̱⶧����
    }//if
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO,"PID[%d] Reload RULE_SCHEDULE_TB loop(%d)", getpid(),loop);

    return loop;
}

int fpcif_LoadRule(dbRule *pRuleInfo)
{
    register unsigned int	loop = 0;
    register int			preIndex = -1;

    int						rowCnt = 0;
    char					sqlBuf[4096] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select RU_ORDER,"
                    "RU_SQ,"
                    "RU_MODIFY_CNT,"
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
                    "RU_FLAG, "
                    "RU_CPU_USAGE_ALARM,"
                    "RU_CPU_USAGE_CONTROL,"
                    "RU_CPU_ALARM_RATE," // 전체 CPU 알람 기준 사용률 (1 ~ 100 %)
                    "RU_CPU_ALARM_SUSTAINED_TIME, " // 전체 CPU 알람 기준 지속시간 (1 ~ 3600 초)
                    "RU_CPU_CTRL_RATE, " // 전체 CPU 통제 기준 사용률 (1 ~ 100 %)
                    "RU_CPU_CTRL_SUSTAINED_TIME, " // 전체 CPU 통제 기준 지속시간 (1 ~ 3600 초)
                    "RU_CPU_CTRL_LIMIT_RATE, " // 전체 CPU 통제 목표 사용률 (1 ~ 100 %)
                    "RU_AGENT_CPU_LIMIT ,"  //DAP AGENT CPU 제한
                    "RU_AGENT_SELF_PROTECT " //AGENT 자기보호 기능
                    "FROM RULE_TB "
                    " WHERE	RU_USE = 1 "
                    " AND RU_FLAG != 'D' "
                    "order by RU_ORDER");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            loop = atoi(g_stMyCon->row[0]);
            preIndex = fpcif_RuleInit(pRuleInfo, preIndex, loop);
            if (g_stMyCon->row[0] != NULL)
                pRuleInfo->ru_order[loop]				= atoi( g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                pRuleInfo->ru_sq[loop]					= atol( g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                pRuleInfo->ru_modify_cnt[loop]			= atol( g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                pRuleInfo->mn_sq[loop]					= atol( g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                pRuleInfo->ru_target_type[loop]			=		*g_stMyCon->row[4];
            if (g_stMyCon->row[5] != NULL)
                strcpy(pRuleInfo->ru_target_value[loop],		g_stMyCon->row[5]);
            if (g_stMyCon->row[6] != NULL)
                pRuleInfo->ru_net_adapter[loop]			=		*g_stMyCon->row[6];
            if (g_stMyCon->row[7] != NULL)
                pRuleInfo->ru_net_adapter_over[loop]	=		*g_stMyCon->row[7];
            if (g_stMyCon->row[8] != NULL)
                pRuleInfo->ru_net_adapter_dupip[loop]	=		*g_stMyCon->row[8];
            if (g_stMyCon->row[9] != NULL)
                pRuleInfo->ru_net_adapter_dupmac[loop]	=		*g_stMyCon->row[9];
            if (g_stMyCon->row[10] != NULL)
                pRuleInfo->ru_net_adapter_mulip[loop]	=		*g_stMyCon->row[10];
            if (g_stMyCon->row[11] != NULL)
                pRuleInfo->ru_wifi[loop]				=		*g_stMyCon->row[11];
            if (g_stMyCon->row[12] != NULL)
                pRuleInfo->ru_bluetooth[loop]			=		*g_stMyCon->row[12];
            if (g_stMyCon->row[13] != NULL)
                pRuleInfo->ru_router[loop]				=		*g_stMyCon->row[13];
            if (g_stMyCon->row[14] != NULL)
                pRuleInfo->ru_printer[loop]				=		*g_stMyCon->row[14];
            if (g_stMyCon->row[15] != NULL)
                pRuleInfo->ru_disk[loop]				=		*g_stMyCon->row[15];
            if (g_stMyCon->row[16] != NULL)
                pRuleInfo->ru_disk_reg[loop]			=		*g_stMyCon->row[16];
            if (g_stMyCon->row[17] != NULL)
                pRuleInfo->ru_disk_hidden[loop]			=		*g_stMyCon->row[17];
            if (g_stMyCon->row[18] != NULL)
                pRuleInfo->ru_disk_new[loop]			=		*g_stMyCon->row[18];
            if (g_stMyCon->row[19] != NULL)
                pRuleInfo->ru_disk_mobile[loop]			=		*g_stMyCon->row[19];
            if (g_stMyCon->row[20] != NULL)
                pRuleInfo->ru_disk_mobile_read[loop]	=		*g_stMyCon->row[20];
            if (g_stMyCon->row[21] != NULL)
                pRuleInfo->ru_disk_mobile_write[loop]	=		*g_stMyCon->row[21];
            if (g_stMyCon->row[22] != NULL)
                pRuleInfo->ru_net_drive[loop]			=		*g_stMyCon->row[22];
            if (g_stMyCon->row[23] != NULL)
                pRuleInfo->ru_net_connection[loop]		=		*g_stMyCon->row[23];
            if (g_stMyCon->row[24] != NULL)
                pRuleInfo->ru_share_folder[loop]		=		*g_stMyCon->row[24];
            if (g_stMyCon->row[25] != NULL)
                pRuleInfo->ru_infrared_device[loop]		=		*g_stMyCon->row[25];
            if (g_stMyCon->row[26] != NULL)
                pRuleInfo->ru_virtual_machine[loop]		=		*g_stMyCon->row[26];
            if (g_stMyCon->row[27] != NULL)
                pRuleInfo->ru_process_white[loop]		=		*g_stMyCon->row[27];
            if (g_stMyCon->row[28] != NULL)
                pRuleInfo->ru_process_black[loop]		=		*g_stMyCon->row[28];
            if (g_stMyCon->row[29] != NULL)
                pRuleInfo->ru_process_accessmon[loop]	=		*g_stMyCon->row[29];
            if (g_stMyCon->row[30] != NULL)
                pRuleInfo->ru_process_accessmon_exp[loop]=		*g_stMyCon->row[30];
            if (g_stMyCon->row[31] != NULL)
                pRuleInfo->ru_process_detailinfo[loop]	=		*g_stMyCon->row[31];
            if (g_stMyCon->row[32] != NULL)
                pRuleInfo->ru_process_forcekill[loop]	=		*g_stMyCon->row[32];
            if (g_stMyCon->row[33] != NULL)
                pRuleInfo->ru_connect_ext_svr[loop]		=		*g_stMyCon->row[33];
            if (g_stMyCon->row[34] != NULL)
                pRuleInfo->ru_ext_net_detect_type[loop]	=		*g_stMyCon->row[34];
            if (g_stMyCon->row[35] != NULL)
                pRuleInfo->ru_sso_cert[loop]			=		*g_stMyCon->row[35];
            if (g_stMyCon->row[36] != NULL)
                pRuleInfo->ru_win_drv[loop]				=		atoi(g_stMyCon->row[36]);
            if (g_stMyCon->row[37] != NULL)
                pRuleInfo->ru_rdp_session[loop]			=		*g_stMyCon->row[37];
            if (g_stMyCon->row[38] != NULL)
                pRuleInfo->ru_rdp_block_copy[loop]		=		*g_stMyCon->row[38];
            if (g_stMyCon->row[39] != NULL)
                pRuleInfo->ru_agent_cycle_process[loop]			= atoi(g_stMyCon->row[39]);
            if (g_stMyCon->row[40] != NULL)
                pRuleInfo->ru_agent_cycle_process_access[loop]	= atoi(g_stMyCon->row[40]);
            if (g_stMyCon->row[41] != NULL)
                pRuleInfo->ru_agent_cycle_net_printer[loop]		= atoi(g_stMyCon->row[41]);
            if (g_stMyCon->row[42] != NULL)
                pRuleInfo->ru_agent_cycle_net_scan[loop]		= atoi(g_stMyCon->row[42]);
            if (g_stMyCon->row[43] != NULL)
                pRuleInfo->ru_agent_cycle_router[loop]			= atoi(g_stMyCon->row[43]);
            if (g_stMyCon->row[44] != NULL)
                pRuleInfo->ru_agent_cycle_ext_access[loop]		= atoi(g_stMyCon->row[44]);
            if (g_stMyCon->row[45] != NULL)
                pRuleInfo->ru_agent_sso_check_cycle[loop]		= atoi(g_stMyCon->row[45]);
            if (g_stMyCon->row[46] != NULL)
                pRuleInfo->ru_agent_sso_keep_time[loop]			= atoi(g_stMyCon->row[46]);
            if (g_stMyCon->row[47] != NULL)
                pRuleInfo->ru_alarm_type[loop]			        = *g_stMyCon->row[47];
            if (g_stMyCon->row[48] != NULL)
                pRuleInfo->ru_alarm_mn_sq[loop]			        = atoi(g_stMyCon->row[48]);
            if (g_stMyCon->row[49] != NULL)
                pRuleInfo->ru_flag[loop]				        = *g_stMyCon->row[49];
            if (g_stMyCon->row[50] != NULL)
                pRuleInfo->ru_cpu_alarm[loop]				    = *g_stMyCon->row[50]; // CPU 알람 레벨
            if (g_stMyCon->row[51] != NULL)
                pRuleInfo->ru_cpu_ctrl[loop]				    = *g_stMyCon->row[51]; //CPU 통제 레벨
            if (g_stMyCon->row[52] != NULL)
                pRuleInfo->ru_cpu_alarm_rate[loop]              = atoi(g_stMyCon->row[52]); // 전체 CPU 알람 기준 사용률 (1 ~ 100 %)
            if (g_stMyCon->row[53] != NULL)
                pRuleInfo->ru_cpu_alarm_sustained_time[loop]    = atoi(g_stMyCon->row[53]); // 전체 CPU 알람 기준 지속시간 (1 ~ 3600 초)
            if (g_stMyCon->row[54] != NULL)
                pRuleInfo->ru_cpu_ctrl_rate[loop]               = atoi(g_stMyCon->row[54]); //전체 CPU 통제 기준 사용률 (1 ~ 100 %)
            if (g_stMyCon->row[55] != NULL)
                pRuleInfo->ru_cpu_ctrl_sustained_time[loop]     = atoi(g_stMyCon->row[55]); //전체 CPU 통제 기준 지속시간 (1 ~ 3600 초)
            if (g_stMyCon->row[56] != NULL)
                pRuleInfo->ru_cpu_ctrl_limit_rate[loop]         = atoi(g_stMyCon->row[56]); //전체 CPU 통제 목표 사용률 (1 ~ 100 %)
            if (g_stMyCon->row[57] != NULL)
                sprintf(pRuleInfo->ru_str_agent_cpu[loop],"%s",g_stMyCon->row[57]);
            if (g_stMyCon->row[58] != NULL)
                pRuleInfo->ru_agent_self_protect[loop]          = atoi(g_stMyCon->row[58]);     //Agent 자기보호기능 활성화 여부

        }
        pRuleInfo->tot_cnt = loop; //count이기때문에
    }
    fdb_SqlFreeResult(g_stMyCon);

    //dp_print_port
    fpcif_LoadDpPrintPort(pRuleInfo);

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload RULE_TB loop(%d)",getpid(),loop );

    return loop;
}


int fpcif_LoadDetectProcess(int* param_RuleCnt,
                            dbDetect *pDetectInfo)
{
    int   local_nLoop = 0;
    int   local_nRowCnt = 0;
    int   local_nMaxRuleSq = 0;
    int   local_nRuleCnt = 0, local_nRdTypeCnt = 0, local_nFetchLoop = 0;
    int   local_nRuleDetectCnt = 0;
    char* SqlBuf = NULL;
    char*    local_rdDetectBuffer[MAX_DETECT_TYPE] = {NULL,};
    int      local_nDetectBufferIndex[MAX_DETECT_TYPE] = {0,};
    struct local_DetectSt
    {
        int  RuleSq;
        int  targetType;
        int  DetailFlag; // 0: 프로세스 세부정보 미수집, 1: 프로세스 세부정보 수집
        int  ProtectFlag; // 0:프로세스보호 비활성화, 1: 프로세스보호 활성화
        int  DetectData1;
        int  DetectData2;
        int  DetectData3;
        char szDetectName[128 +1];
        char szDetectHash[256 +1];
    };
    struct local_DetectSt* local_ptrDetectSt = NULL;


    fcom_malloc((void**)&SqlBuf, sizeof(char) * 2048 +1);

    //    RULE_DETECT_TB PROCESS 정보 조회
    snprintf    (SqlBuf, sizeof(char) * 2048, "%s",
                 " SELECT "
                 " * "
                 " FROM "
                 " ( "
                 " SELECT "
                 " G_DETECT_TB.RU_SQ, "
                 " DETECT_PROCESS_TB.DP_PROCESS_NAME, "
                 " DETECT_PROCESS_TB.DP_HASH, "
                 " G_DETECT_TB.TARGET_TYPE, "
                 " DETECT_PROCESS_TB.DP_DETAIL_INFO, "
                 " DETECT_PROCESS_TB.DP_PROTECTION, "
                 " G_DETECT_TB.RDL_INT_DATA1, "
                 " G_DETECT_TB.RDL_INT_DATA2, "
                 " G_DETECT_TB.RDL_INT_DATA3 "
                 " FROM "
                 " DETECT_PROCESS_TB AS DETECT_PROCESS_TB, "
                 " ( "
                 " SELECT "
                 " RULE_DETECT_LINK_TB.RU_SQ, "
                 " DETECT_LINK_TB.OBJECT_SQ, "
                 " DETECT_LINK_TB.OBJECT_TYPE, "
                 " RULE_DETECT_LINK_TB.TARGET_TYPE, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA1, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA2, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA3 "
                 " FROM "
                 " ( "
                 " SELECT "
                 " * "
                 " FROM "
                 " DETECT_LINK_TB "
                 " WHERE "
                 " DL_FLAG != 'D'  " // 삭제예정 RULE_DETECT 제외
                 ") as DETECT_LINK_TB, "
                 "( "
                 " SELECT "
                 " * "
                 " FROM "
                 " RULE_DETECT_LINK_TB "
                 " WHERE "
                 " TARGET_IS_GROUP = 1 " // 그룹
                 " AND RDL_FLAG != 'D'  " //삭제예정 RULE_DETECT 제외
//                 " AND TARGET_TYPE NOT IN(3,4)  " // 3 : 외부 URL, 4 : 가상머신 프로세스 제외
                 " AND TARGET_TYPE NOT IN(2,3,4)  " // 2 : 접속감시대상, 3 : 외부 URL, 4 : 가상머신 프로세스 제외
                 " ) AS RULE_DETECT_LINK_TB "
                 " WHERE "
                 " DETECT_LINK_TB.DG_SQ = RULE_DETECT_LINK_TB.TARGET_SQ "
                 " ) AS G_DETECT_TB "
                 " WHERE "
                 " DETECT_PROCESS_TB.DP_SQ = G_DETECT_TB.OBJECT_SQ "
                 " AND DETECT_PROCESS_TB.DP_USE = 1 "
                 " AND DETECT_PROCESS_TB.DP_FLAG != 'D'  " // 삭제예정 RULE_DETECT 제외
                 " UNION ALL "
                 " SELECT "
                 " RULE_DETECT_LINK_TB.RU_SQ, "
                 " DETECT_PROCESS_TB.DP_PROCESS_NAME, "
                 " DETECT_PROCESS_TB.DP_HASH, "
                 " RULE_DETECT_LINK_TB.TARGET_TYPE, "
                 " DETECT_PROCESS_TB.DP_DETAIL_INFO, "
                 " DETECT_PROCESS_TB.DP_PROTECTION, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA1, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA2, "
                 " RULE_DETECT_LINK_TB.RDL_INT_DATA3 "
                 " FROM "
                 " ( "
                 "   SELECT "
                 "   * "
                 "   FROM "
                 " DETECT_PROCESS_TB "
                 " WHERE "
                 " DP_USE = 1 "
                 " AND DP_FLAG != 'D'  " // 삭제예정 RULE_DETECT 제외
                 " ) AS DETECT_PROCESS_TB, "
                 "               ( "
                 " SELECT "
                 " * "
                 " FROM "
                 " RULE_DETECT_LINK_TB "
                 " WHERE "
                 " TARGET_IS_GROUP = 0  " //  객체
                 " AND RDL_FLAG != 'D'  " // 삭제예정 RULE_DETECT 제외
//                 " AND TARGET_TYPE NOT IN(3,4)  " // 3 : 외부 URL, 4 : 가상머신 프로세스 제외
                 " AND TARGET_TYPE NOT IN(2,3,4)  " //  2 : 접속감시대상, 3 : 외부 URL, 4 : 가상머신 프로세스 제외
                 " ) AS RULE_DETECT_LINK_TB "
                 "WHERE "
                 "DETECT_PROCESS_TB.DP_SQ = RULE_DETECT_LINK_TB.TARGET_SQ "
                 ") G "
                 "ORDER BY "
                 "G.RU_SQ, "
                 "G.TARGET_TYPE, G.DP_PROCESS_NAME "    );

    local_nRowCnt = fdb_SqlQuery(g_stMyCon, SqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        fcom_MallocFree((void**)&SqlBuf);
        return -1;
    }


    for(local_nLoop = 0; local_nLoop < MAX_DETECT_TYPE; local_nLoop++)
    {
//        fcom_malloc((void**)&local_rdDetectBuffer[local_nLoop], sizeof(char) * 1024);
        fcom_malloc((void**)&local_rdDetectBuffer[local_nLoop], sizeof(char) * 4096);
    }


    /* 쿼리 Fetch결과를 local struct에 저장 */
    local_nLoop = 0;
    if(local_nRowCnt > 0)
    {
        fcom_malloc((void**)&local_ptrDetectSt, sizeof(struct local_DetectSt) * local_nRowCnt);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                local_ptrDetectSt[local_nLoop].RuleSq = atoi(g_stMyCon->row[0]);

                /* 쿼리 Fetch하면서 최대 Rule Sequence값을 저장 */
                if(local_ptrDetectSt[local_nLoop].RuleSq > local_nMaxRuleSq)
                {
                    local_nMaxRuleSq = local_ptrDetectSt[local_nLoop].RuleSq;
                }
            }

            if(g_stMyCon->row[1] != NULL)
            {
                snprintf(local_ptrDetectSt[local_nLoop].szDetectName,
                         sizeof(local_ptrDetectSt[local_nLoop].szDetectName),
                         "%s",
                         g_stMyCon->row[1]);
            }
            if(g_stMyCon->row[2] != NULL)
            {
                snprintf(local_ptrDetectSt[local_nLoop].szDetectHash,
                         sizeof(local_ptrDetectSt[local_nLoop].szDetectHash),
                         "%s",
                         g_stMyCon->row[2]);
            }

            if(g_stMyCon->row[3] != NULL)
            {
                local_ptrDetectSt[local_nLoop].targetType = atoi(g_stMyCon->row[3]);
            }

            if(g_stMyCon->row[4] != NULL)
            {
                local_ptrDetectSt[local_nLoop].DetailFlag = atoi(g_stMyCon->row[4]);
            }

            if(g_stMyCon->row[5] != NULL)
            {
                local_ptrDetectSt[local_nLoop].ProtectFlag = atoi(g_stMyCon->row[5]);
            }

            if(g_stMyCon->row[6] != NULL)
            {
                local_ptrDetectSt[local_nLoop].DetectData1 = atoi(g_stMyCon->row[6]);
            }

            if(g_stMyCon->row[7] != NULL)
            {
                local_ptrDetectSt[local_nLoop].DetectData2 = atoi(g_stMyCon->row[7]);
            }

            if(g_stMyCon->row[8] != NULL)
            {
                local_ptrDetectSt[local_nLoop].DetectData3 = atoi(g_stMyCon->row[8]);
            }

            local_nLoop++;
        }
    }

    /* RULE Count만큼 돌면서 */
    for(local_nRuleCnt = 1; local_nRuleCnt < local_nMaxRuleSq + 1; local_nRuleCnt++)
    {
        /* RD_TYPE 버퍼 및 인덱스 초기화 */
        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
        {
            memset(local_rdDetectBuffer[local_nRdTypeCnt], 0x00, sizeof(char) * 1024);
            local_nDetectBufferIndex[local_nRdTypeCnt] = 0;
        }

        for(local_nFetchLoop = 0; local_nFetchLoop < local_nRowCnt; local_nFetchLoop++)
        {
            /* 룰이 같으면 해당 룰의 RD_TYPE별로 버퍼에 저장 */
            if(local_ptrDetectSt[local_nFetchLoop].RuleSq == local_nRuleCnt )
            {
                if ( local_nDetectBufferIndex[local_ptrDetectSt[local_nFetchLoop].targetType] >= sizeof(char) * 4096)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"Detect Data Is Bigger Than Buffer (%d > %d)",
                                  local_nDetectBufferIndex[local_ptrDetectSt[local_nFetchLoop].targetType], sizeof(char) * 4096);
                    break;
                }

                switch(local_ptrDetectSt[local_nFetchLoop].targetType)
                {
                    // RD_TYPE = 0 , BLACK PROCESS
                    case BLACK_PROCESS:
                    {
                        /* DP_PROCESS_NAME */
                        local_nDetectBufferIndex[BLACK_PROCESS] += snprintf( &(local_rdDetectBuffer[BLACK_PROCESS][local_nDetectBufferIndex[BLACK_PROCESS]] ),
                                                                 (sizeof(char) * 4096) - local_nDetectBufferIndex[BLACK_PROCESS],
                                                                 "%s",
                                                                 local_ptrDetectSt[local_nFetchLoop].szDetectName);
                        /* (DP_HASH:DP_DETAIL_INFO:DP_PROTACTION) */
                        local_nDetectBufferIndex[BLACK_PROCESS] += snprintf(&(local_rdDetectBuffer[BLACK_PROCESS][local_nDetectBufferIndex[BLACK_PROCESS]]),
                                                                (sizeof(char) * 4096) - local_nDetectBufferIndex[BLACK_PROCESS],
                                                                "(%s:%d:%d);",
                                                                local_ptrDetectSt[local_nFetchLoop].szDetectHash,
                                                                local_ptrDetectSt[local_nFetchLoop].DetailFlag,
                                                                local_ptrDetectSt[local_nFetchLoop].ProtectFlag);
                        break;
                    }
                    // RD_TYPE = 1, WHITE PROCESS
                    case WHITE_PROCESS:
                    {
                        /* DP_PROCESS_NAME */
                        local_nDetectBufferIndex[WHITE_PROCESS] += snprintf( &(local_rdDetectBuffer[WHITE_PROCESS][local_nDetectBufferIndex[WHITE_PROCESS]] ),
                                                                 (sizeof(char) * 4096) - local_nDetectBufferIndex[WHITE_PROCESS],
                                                                 "%s",
                                                                 local_ptrDetectSt[local_nFetchLoop].szDetectName);

                        /* (DP_HASH:DP_DETAIL_INFO:DP_PROTACTION) */
                        local_nDetectBufferIndex[WHITE_PROCESS] += snprintf(&(local_rdDetectBuffer[WHITE_PROCESS][local_nDetectBufferIndex[WHITE_PROCESS]]),
                                                                            (sizeof(char) * 4096) - local_nDetectBufferIndex[WHITE_PROCESS],
                                                                            "(%s:%d:%d);",
                                                                            local_ptrDetectSt[local_nFetchLoop].szDetectHash,
                                                                            local_ptrDetectSt[local_nFetchLoop].DetailFlag,
                                                                            local_ptrDetectSt[local_nFetchLoop].ProtectFlag);

                        break;
                    }
                    // RD_TYPE = 2, ACCESS PROCESS
//                    case ACCESS_PROCESS:
//                    {
//                        local_nDetectBufferIndex[ACCESS_PROCESS] += snprintf( &(local_rdDetectBuffer[ACCESS_PROCESS][local_nDetectBufferIndex[ACCESS_PROCESS]] ),
//                                                                 (sizeof(char) * 4096) - local_nDetectBufferIndex[ACCESS_PROCESS],
//                                                                 "%s",
//                                                                 local_ptrDetectSt[local_nFetchLoop].szDetectName);
//
//                        local_nDetectBufferIndex[ACCESS_PROCESS] += snprintf(&(local_rdDetectBuffer[ACCESS_PROCESS][local_nDetectBufferIndex[ACCESS_PROCESS]]),
//                                                                            (sizeof(char) * 4096) - local_nDetectBufferIndex[ACCESS_PROCESS],
//                                                                            "(%d);",
//                                                                            local_ptrDetectSt[local_nFetchLoop].DetailFlag );
//
//                        break;
//                    }

                    //RD_TYPE = 5, TOTAL ALARM EXCEPTION
                    case TOTAL_ALARM_CPU_EXCEPTION:
                    {
                        local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION] +=
                                            snprintf( &(local_rdDetectBuffer[TOTAL_ALARM_CPU_EXCEPTION][local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION]] ),
                                                                          (sizeof(char) * 4096) - local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION],
                                                                          "%s",
                                                                          local_ptrDetectSt[local_nFetchLoop].szDetectName);

                        local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION] +=
                                            snprintf(&(local_rdDetectBuffer[TOTAL_ALARM_CPU_EXCEPTION][local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION]]),
                                                                          (sizeof(char) * 4096) - local_nDetectBufferIndex[TOTAL_ALARM_CPU_EXCEPTION],
                                                                          "<%d:%d>;",
                                                                          local_ptrDetectSt[local_nFetchLoop].DetectData1,
                                                                          local_ptrDetectSt[local_nFetchLoop].DetectData2);

                        break;

                    }
                    // RD_TYPE = 6, ALARM CPU EXCEPTION
                    case ALARM_CPU_EXCEPTION:
                    {
                        local_nDetectBufferIndex[ALARM_CPU_EXCEPTION] += snprintf( &(local_rdDetectBuffer[ALARM_CPU_EXCEPTION][local_nDetectBufferIndex[ALARM_CPU_EXCEPTION]] ),
                                                     (sizeof(char) * 4096) - local_nDetectBufferIndex[ALARM_CPU_EXCEPTION],
                                                     "%s",
                                                     local_ptrDetectSt[local_nFetchLoop].szDetectName);

                        local_nDetectBufferIndex[ALARM_CPU_EXCEPTION] += snprintf(&(local_rdDetectBuffer[ALARM_CPU_EXCEPTION][local_nDetectBufferIndex[ALARM_CPU_EXCEPTION]]),
                                                    (sizeof(char) * 4096) - local_nDetectBufferIndex[ALARM_CPU_EXCEPTION],
                                                    "<%d:%d>;",
                                                    local_ptrDetectSt[local_nFetchLoop].DetectData1,
                                                    local_ptrDetectSt[local_nFetchLoop].DetectData2);


                        break;
                    }
                    //RD_TYPE = 7, TOTAL CONTROL CPU EXCEPTION
                    case TOTAL_CONTROL_CPU_EXCEPTION:
                    {
                        local_nDetectBufferIndex[TOTAL_CONTROL_CPU_EXCEPTION] += snprintf( &(local_rdDetectBuffer[TOTAL_CONTROL_CPU_EXCEPTION][local_nDetectBufferIndex[TOTAL_CONTROL_CPU_EXCEPTION]] ),
                                                                 sizeof(char) * 4096,
                                                                 "%s",
                                                                 local_ptrDetectSt[local_nFetchLoop].szDetectName);

                        break;
                    }
                    // RD_TYPE = 8, CONTROL CPU EXCEPTION
                    case CONTROL_CPU_EXCEPTION:
                    {
                        local_nDetectBufferIndex[CONTROL_CPU_EXCEPTION] += snprintf( &(local_rdDetectBuffer[CONTROL_CPU_EXCEPTION][local_nDetectBufferIndex[CONTROL_CPU_EXCEPTION]] ),
                                                                                     sizeof(char) * 4096,
                                                                                    "%s",
                                                                                    local_ptrDetectSt[local_nFetchLoop].szDetectName);
                        local_nDetectBufferIndex[CONTROL_CPU_EXCEPTION] += snprintf(&(local_rdDetectBuffer[CONTROL_CPU_EXCEPTION][local_nDetectBufferIndex[CONTROL_CPU_EXCEPTION]]),
                                                                                          (sizeof(char) * 4096) - local_nDetectBufferIndex[CONTROL_CPU_EXCEPTION],
                                                                                          "<%d:%d:%d>;",
                                                                                          local_ptrDetectSt[local_nFetchLoop].DetectData1,
                                                                                          local_ptrDetectSt[local_nFetchLoop].DetectData2,
                                                                                          local_ptrDetectSt[local_nFetchLoop].DetectData3);
                        break;
                    }
                    default:
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Unknown Target Type ");
                        break;
                    }
                }
            }
        }

        /* 전역 RULE DETECT STRUCT에 RD_TYPE버퍼와 TYPE 저장 */
        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++ )
        {
            if(strlen(local_rdDetectBuffer[local_nRdTypeCnt]) > 0)
            {
                fcom_malloc((void**)&pDetectInfo->rd_value[local_nRuleDetectCnt], sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt]) + 1);

                /* RD_VALUE 저장 */
                snprintf(pDetectInfo->rd_value[local_nRuleDetectCnt],
                         (sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt])) +1,
                         "%s", local_rdDetectBuffer[local_nRdTypeCnt]);
                /* RU_SQ 저장 */
                pDetectInfo->ru_sq[local_nRuleDetectCnt] = local_nRuleCnt;
                /* RD_TYPE 저장 */
                pDetectInfo->rd_type[local_nRuleDetectCnt] = local_nRdTypeCnt;

                local_nRuleDetectCnt++;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);
    fcom_MallocFree((void**)&SqlBuf);

    /* RD_TYPE Buffer Free */
    for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
    {
        fcom_MallocFree((void**)&local_rdDetectBuffer[local_nRdTypeCnt]);
    }
    fcom_MallocFree((void**)&local_ptrDetectSt);

    *param_RuleCnt += local_nRuleDetectCnt;

    return 0;


}
int fpcif_LoadDetectUrl( int* param_RuleCnt,
                         dbDetect *pDetectInfo)
{

    int   local_nLoop = 0;
    int   local_nRowCnt = 0;
    int   local_nMaxRuleSq = 0;
    int   local_nRuleCnt = 0, local_nRdTypeCnt = 0, local_nFetchLoop = 0;
    int   local_nRuleDetectCnt = 0;
    int   local_nDetectBufferIndex[MAX_DETECT_TYPE] = {0,};
    struct local_DetectSt
    {
        int  RuleSq;
        int  targetType;
        char szDetectName[256 +1];
    };
    char* SqlBuf = NULL;
    char* local_rdDetectBuffer[MAX_DETECT_TYPE] = {NULL,};
    struct local_DetectSt* local_ptrDetectSt    = NULL;

    fcom_malloc((void**)&SqlBuf, sizeof(char) * 2048 +1);

    //    RULE_DETECT_TB PROCESS 정보 조회
    snprintf    (SqlBuf, sizeof(char) * 2048, "%s",
                 " SELECT "
                 " distinct "
                 " G_DETECT_TB.RU_SQ, "
                 " WATCH_SERVER.WS_SVR, "
                 " G_DETECT_TB.TARGET_TYPE "
                 " FROM "
                 " WATCH_SERVER_TB AS WATCH_SERVER, "
                 " ( "
                 " SELECT "
                 " * "
                 " FROM"
                 " ( "
                 "      SELECT "
                 "           * "
                 "           FROM "
                 " DETECT_LINK_TB "
                 " WHERE DL_FLAG != 'D' " //삭제예정 RULE_DETECT 제외
                 " ) AS DETECT_LINK, "
                 " ( "
                 " SELECT "
                 " * "
                 " FROM "
                 " RULE_DETECT_LINK_TB "
                 " WHERE "
                 " TARGET_IS_GROUP = 0 " // 객체
                 " AND TARGET_TYPE = 3  " // 외부 URL Type
                 " AND RDL_FLAG != 'D' " // 삭제예정 RULE_DETECT 제외
                 " ) AS RULE_DETECT_LINK "
                 " WHERE "
                 " DETECT_LINK.OBJECT_SQ = RULE_DETECT_LINK.TARGET_SQ "
                 " ) AS G_DETECT_TB "
                 " WHERE "
                 " WATCH_SERVER.WS_SQ = G_DETECT_TB.OBJECT_SQ "
                 " AND WATCH_SERVER.WS_FLAG != 'D' "//삭제예정 RULE_DETECT 제외
                 " UNION ALL "
                 " SELECT "
                 " G_DETECT_TB.RU_SQ, "
                 " WATCH_SERVER.WS_SVR, "
                 " G_DETECT_TB.TARGET_TYPE "
                 " FROM "
                 " WATCH_SERVER_TB AS WATCH_SERVER, "
                 " ( "
                 " SELECT "
                 " RULE_DETECT_LINK.RU_SQ, "
                 " DETECT_LINK.OBJECT_SQ, "
                 " DETECT_LINK.OBJECT_TYPE, "
                 " RULE_DETECT_LINK.TARGET_TYPE, "
                 " RULE_DETECT_LINK.RDL_INT_DATA1, "
                 " RULE_DETECT_LINK.RDL_INT_DATA2, "
                 " RULE_DETECT_LINK.RDL_INT_DATA3 "
                 " FROM "
                 " ( "
                 "   SELECT "
                 "   * "
                 "   FROM "
                 " DETECT_LINK_TB "
                 " ) AS DETECT_LINK, "
                 " ( "
                 " SELECT "
                 " * "
                 " FROM "
                 " RULE_DETECT_LINK_TB "
                 " WHERE "
                 " TARGET_IS_GROUP = 1 " // 그룹
                 " and TARGET_TYPE = 3 " //  외부 URL Type
                 " ) AS RULE_DETECT_LINK "
                 " WHERE "
                 " DETECT_LINK.DG_SQ = RULE_DETECT_LINK.TARGET_SQ "
                 " ) G_DETECT_TB "
                 "         WHERE "
                 " WATCH_SERVER.WS_SQ = G_DETECT_TB.OBJECT_SQ "
                 " AND WATCH_SERVER.WS_FLAG != 'D'  ORDER BY RU_SQ,WS_SVR ASC ");

    local_nRowCnt = fdb_SqlQuery(g_stMyCon, SqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        fcom_MallocFree((void**)&SqlBuf);

        return -1;
    }


    for(local_nLoop = 0; local_nLoop < MAX_DETECT_TYPE; local_nLoop++)
    {
        fcom_malloc((void**)&local_rdDetectBuffer[local_nLoop], sizeof(char) * 1024);
    }

    local_nLoop = 0;
    if(local_nRowCnt > 0)
    {
        fcom_malloc((void**)&local_ptrDetectSt, sizeof(struct local_DetectSt) * local_nRowCnt);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                local_ptrDetectSt[local_nLoop].RuleSq = atoi(g_stMyCon->row[0]);

                /* 쿼리 Fetch하면서 최대 Rule Sequence값을 저장 */
                if(local_ptrDetectSt[local_nLoop].RuleSq > local_nMaxRuleSq)
                {
                    local_nMaxRuleSq = local_ptrDetectSt[local_nLoop].RuleSq;
                }
            }

            if(g_stMyCon->row[1] != NULL)
            {
                snprintf(local_ptrDetectSt[local_nLoop].szDetectName,
                         sizeof(local_ptrDetectSt[local_nLoop].szDetectName),
                         "%s",
                         g_stMyCon->row[1]);
            }

            if(g_stMyCon->row[2] != NULL)
            {
                local_ptrDetectSt[local_nLoop].targetType = atoi(g_stMyCon->row[2]);
            }

            local_nLoop++;
        }
    }

    /* RULE Count */
    local_nRuleDetectCnt = *param_RuleCnt;
    for(local_nRuleCnt = 1; local_nRuleCnt < local_nMaxRuleSq + 1; local_nRuleCnt++)
    {
        /* TYPE Count */
        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
        {
            memset(local_rdDetectBuffer[local_nRdTypeCnt], 0x00, sizeof(char) * 1024);
            local_nDetectBufferIndex[local_nRdTypeCnt] = 0;
        }

        for(local_nFetchLoop = 0; local_nFetchLoop < local_nRowCnt; local_nFetchLoop++)
        {
            /* 룰이 같으면 */
            if(local_ptrDetectSt[local_nFetchLoop].RuleSq == local_nRuleCnt )
            {
                switch(local_ptrDetectSt[local_nFetchLoop].targetType)
                {

                    // RD_TYPE = 3, CONNEXT
                    case CONEXT_URL:
                    {
                        local_nDetectBufferIndex[CONEXT_URL] += snprintf( &(local_rdDetectBuffer[CONEXT_URL][local_nDetectBufferIndex[CONEXT_URL]] ),
                                                                          (sizeof(char) * 1024) - local_nDetectBufferIndex[CONEXT_URL],
                                                                          "%s",
                                                                          local_ptrDetectSt[local_nFetchLoop].szDetectName);
                        local_nDetectBufferIndex[CONEXT_URL] += snprintf(&(local_rdDetectBuffer[CONEXT_URL][local_nDetectBufferIndex[CONEXT_URL]]),
                                                                            (sizeof(char) * 1024) - local_nDetectBufferIndex[CONEXT_URL],
                                                                            ";");

                        break;
                    }
                    default:
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Unknown Target Type ");
                        break;
                    }
                }
            }
        }

        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++ )
        {
            if(strlen(local_rdDetectBuffer[local_nRdTypeCnt]) > 0)
            {
                fcom_malloc((void**)&pDetectInfo->rd_value[local_nRuleDetectCnt], sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt]) + 1);

                /* RD_VALUE 저장 */
                snprintf(pDetectInfo->rd_value[local_nRuleDetectCnt],
                         (sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt])) +1,
                         "%s", local_rdDetectBuffer[local_nRdTypeCnt]);
                /* RU_SQ 저장 */
                pDetectInfo->ru_sq[local_nRuleDetectCnt] = local_nRuleCnt;
                /* RD_TYPE 저장 */
                pDetectInfo->rd_type[local_nRuleDetectCnt] = local_nRdTypeCnt;

                local_nRuleDetectCnt++;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);
    fcom_MallocFree((void**)&SqlBuf);
    /* RD_TYPE Buffer Free */
    for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
    {
        fcom_MallocFree((void**)&local_rdDetectBuffer[local_nRdTypeCnt]);
    }
    fcom_MallocFree((void**)&local_ptrDetectSt);

    *param_RuleCnt = local_nRuleDetectCnt;

    return 0;


}

int fpcif_LoadDetectAccessMon(int* param_RuleCnt, dbDetect* pDetectInfo)
{
    int   local_nRuleCnt    = 0;
    char local_szQuery[1024] = {0x00,};


    snprintf    (local_szQuery, sizeof(local_szQuery), "%s",
                 " select RD_VALUE, RU_SQ, RD_TYPE from RULE_DETECT_TB where RD_TYPE = 2 " );

    fdb_SqlQuery(g_stMyCon, local_szQuery);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        return -1;
    }


    local_nRuleCnt = *param_RuleCnt;
    while(fdb_SqlFetchRow(g_stMyCon) == 0)
    {
        fcom_malloc((void**)&pDetectInfo->rd_value[local_nRuleCnt],
                    sizeof(char) * strlen(g_stMyCon->row[0] ) + 1);

        if ( g_stMyCon->row[0] != NULL )
        {
            /* RD_VALUE 저장 */
            snprintf(pDetectInfo->rd_value[local_nRuleCnt],
                     sizeof(char) * strlen(g_stMyCon->row[0] ) + 1,
                     "%s", g_stMyCon->row[0]);
        }
        else
        {
            snprintf(pDetectInfo->rd_value[local_nRuleCnt],
                     sizeof(char) * strlen(g_stMyCon->row[0] ) + 1,
                     "%s", "");
        }

        if ( g_stMyCon->row[1] != NULL )
        {
            /* RU_SQ 저장 */
            pDetectInfo->ru_sq[local_nRuleCnt] = atoi(g_stMyCon->row[1]);
        }
        else
        {
            /* RU_SQ 저장 */
            pDetectInfo->ru_sq[local_nRuleCnt] = -1;
        }

        if ( g_stMyCon->row[2] != NULL )
        {
            /* RD_TYPE 저장 */
            pDetectInfo->rd_type[local_nRuleCnt] = atoi(g_stMyCon->row[2]);

        }
        else
        {
            pDetectInfo->rd_type[local_nRuleCnt] = -1;
        }

        local_nRuleCnt++;
    }

    fdb_SqlFreeResult(g_stMyCon);

    *param_RuleCnt = local_nRuleCnt;

    return 0;

}
int fpcif_LoadDetectVirtualMachine(
        int* param_RuleCnt,
        dbDetect *pDetectInfo)
{

    int   local_nLoop = 0;
    int   local_nRowCnt = 0;
    int   local_nMaxRuleSq = 0;
    int   local_nRuleCnt = 0, local_nRdTypeCnt = 0, local_nFetchLoop = 0;
    int   local_nRuleDetectCnt = 0;
    int      local_nDetectBufferIndex[MAX_DETECT_TYPE] = {0,};
    struct local_DetectSt
    {
        int  RuleSq;
        int  targetType;
        char szDetectName[256];
        char szDetectHash[512];
    };
    struct local_DetectSt* local_ptrDetectSt    = NULL;
    char* SqlBuf                                = NULL;
    char* local_rdDetectBuffer[MAX_DETECT_TYPE] = {NULL,};


    fcom_malloc((void**)&SqlBuf, sizeof(char) * 1024 +1);

    //    RULE_DETECT_TB PROCESS 정보 조회
    snprintf    (SqlBuf, sizeof(char) * 1024, "%s",
                 " SELECT "
                 " *, "
                 " 4 as RD_TYPE "
                 " FROM "
                 " ( "
                 " SELECT "
                 " RU_SQ "
                 " FROM "
                 " RULE_TB "
                 " WHERE "
                 " RU_VIRTUAL_MACHINE = 5 "
                 " ) AS RU_SQ, "
                 " ( "
                 " SELECT "
                 " DP_PROCESS_NAME, "
                 " DP_HASH "
                 " FROM "
                 " DETECT_PROCESS_TB "
                 " WHERE "
                 " DP_DETAIL_INFO = 2 "
                 " AND DP_USE = 1 "
                 " AND DP_FLAG = 'S' "
                 " ) AS DETECT_PROCESS  ORDER BY RU_SQ,DP_PROCESS_NAME" );

    local_nRowCnt = fdb_SqlQuery(g_stMyCon, SqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        fcom_MallocFree((void**)&SqlBuf);

        return -1;
    }


    for(local_nLoop = 0; local_nLoop < MAX_DETECT_TYPE; local_nLoop++)
    {
        fcom_malloc((void**)&local_rdDetectBuffer[local_nLoop], sizeof(char) * 1024);
    }

    local_nLoop = 0;
    if(local_nRowCnt > 0)
    {
        fcom_malloc((void**)&local_ptrDetectSt, sizeof(struct local_DetectSt) * local_nRowCnt);

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                local_ptrDetectSt[local_nLoop].RuleSq = atoi(g_stMyCon->row[0]);

                /* 쿼리 Fetch하면서 최대 Rule Sequence값을 저장 */
                if(local_ptrDetectSt[local_nLoop].RuleSq > local_nMaxRuleSq)
                {
                    local_nMaxRuleSq = local_ptrDetectSt[local_nLoop].RuleSq;
                }
            }

            if(g_stMyCon->row[1] != NULL)
            {
                snprintf(local_ptrDetectSt[local_nLoop].szDetectName,
                         sizeof(local_ptrDetectSt[local_nLoop].szDetectName),
                         "%s",
                         g_stMyCon->row[1]);
            }
            if(g_stMyCon->row[2] != NULL)
            {
                snprintf(local_ptrDetectSt[local_nLoop].szDetectHash,
                         sizeof(local_ptrDetectSt[local_nLoop].szDetectHash),
                         "%s",
                         g_stMyCon->row[2]);
            }

            if(g_stMyCon->row[3] != NULL)
            {
                local_ptrDetectSt[local_nLoop].targetType = atoi(g_stMyCon->row[3]);
            }

            local_nLoop++;
        }
    }

    /* RULE Count */
    local_nRuleDetectCnt = *param_RuleCnt;
    for(local_nRuleCnt = 1; local_nRuleCnt < local_nMaxRuleSq + 1; local_nRuleCnt++)
    {
        /* TYPE Count */
        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
        {
            memset(local_rdDetectBuffer[local_nRdTypeCnt], 0x00, sizeof(char) * 1024);
            local_nDetectBufferIndex[local_nRdTypeCnt] = 0;
        }

        for(local_nFetchLoop = 0; local_nFetchLoop < local_nRowCnt; local_nFetchLoop++)
        {
            /* 룰이 같으면 */
            if(local_ptrDetectSt[local_nFetchLoop].RuleSq == local_nRuleCnt )
            {
                switch(local_ptrDetectSt[local_nFetchLoop].targetType)
                {

                    // RD_TYPE = 4, Virtual Machine Process
                    case KILL_PROCESS:
                    {
                        local_nDetectBufferIndex[KILL_PROCESS] += snprintf( &(local_rdDetectBuffer[KILL_PROCESS][local_nDetectBufferIndex[KILL_PROCESS]] ),
                                                                          (sizeof(char) * 1024) - local_nDetectBufferIndex[KILL_PROCESS],
                                                                          "%s",
                                                                          local_ptrDetectSt[local_nFetchLoop].szDetectName);
                        local_nDetectBufferIndex[KILL_PROCESS] += snprintf(&(local_rdDetectBuffer[KILL_PROCESS][local_nDetectBufferIndex[KILL_PROCESS]]),
                                                                         (sizeof(char) * 1024) - local_nDetectBufferIndex[KILL_PROCESS],
                                                                         "(%s);",local_ptrDetectSt[local_nFetchLoop].szDetectHash);
                        break;
                    }
                    default:
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG, "Unknown Target Type ");
                        break;
                    }
                }
            }
        }

        for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++ )
        {
            if(strlen(local_rdDetectBuffer[local_nRdTypeCnt]) > 0)
            {
                fcom_malloc((void**)&pDetectInfo->rd_value[local_nRuleDetectCnt], sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt]) + 1);

                /* RD_VALUE 저장 */
                snprintf(pDetectInfo->rd_value[local_nRuleDetectCnt],
                         (sizeof(char) * strlen(local_rdDetectBuffer[local_nRdTypeCnt])) +1,
                         "%s", local_rdDetectBuffer[local_nRdTypeCnt]);
                /* RU_SQ 저장 */
                pDetectInfo->ru_sq[local_nRuleDetectCnt] = local_nRuleCnt;
                /* RD_TYPE 저장 */
                pDetectInfo->rd_type[local_nRuleDetectCnt] = local_nRdTypeCnt;

                local_nRuleDetectCnt++;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);
    fcom_MallocFree((void**)&SqlBuf);

    /* RD_TYPE Buffer Free */
    for(local_nRdTypeCnt = 0; local_nRdTypeCnt < MAX_DETECT_TYPE; local_nRdTypeCnt++)
    {
        fcom_MallocFree((void**)&local_rdDetectBuffer[local_nRdTypeCnt]);
    }
    fcom_MallocFree((void**)&local_ptrDetectSt);

    *param_RuleCnt = local_nRuleDetectCnt;

    return 0;


}
int fpcif_LoadDetectSt(dbDetect *pDetectInfo)
{
    int				ruleCnt = 0;
    int             local_ruleDetectCnt = 0;
    int             local_nIdx = 0;
//    register int	preIndex = -1;

    /* 기존 Detect RD_VALUE Buffer 해제 */
    for(ruleCnt=0; ruleCnt < MAX_RULE_COUNT; ruleCnt++)
    {
        fcom_BufferFree(pDetectInfo->rd_value[ruleCnt]);
    }

    /* RULE_DETECT_ST 초기화 , 필요없어보여서 주석처리 */
//    fpcif_DetectInit(pDetectInfo, -1, MAX_RULE_COUNT);

    /* RULE_DETECT Process 정보 Load */
    if(fpcif_LoadDetectProcess(&local_ruleDetectCnt, pDetectInfo) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"fpcif_LoadDetectProcess is failed ");
        return -1;
    }

    /* RULE_DETECT URL 정보 Load */
    if(fpcif_LoadDetectUrl(&local_ruleDetectCnt, pDetectInfo) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"fpcif_LoadDetectProcess is failed ");
        return -1;
    }

    /* RULE_DETECT Virtual Machine 프로세스 정보 Load */
    if(fpcif_LoadDetectVirtualMachine(&local_ruleDetectCnt, pDetectInfo) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"fpcif_LoadDetectVirtualMachine is failed ");
        return -1;
    }

    for(ruleCnt = 0; ruleCnt < local_ruleDetectCnt; ruleCnt++)
    {
        /* 마지막 ;(세미콜론) 제거 */
        for(local_nIdx = strlen(pDetectInfo->rd_value[ruleCnt]); local_nIdx > 0; local_nIdx--)
        {
            if(pDetectInfo->rd_value[ruleCnt][local_nIdx] == ';')
            {
                pDetectInfo->rd_value[ruleCnt][local_nIdx] = 0x00;
                break;
            }
        }
    }

    /* RULE_DETECT ACCESS MON 정보 Load */
    if ( fpcif_LoadDetectAccessMon(&local_ruleDetectCnt, pDetectInfo) != 0 )
    {
        WRITE_CRITICAL(CATEGORY_DB,"fpcif_LoadDetectAccessMon is failed ");
        return -1;
    }

    pDetectInfo->tot_cnt = local_ruleDetectCnt;
    for(ruleCnt = 0; ruleCnt < pDetectInfo->tot_cnt; ruleCnt++)
    {
        WRITE_DEBUG(CATEGORY_DEBUG, "Load RULE_DETECT_TB -> rulesq : %d value : %s type : %d ",
                    pDetectInfo->ru_sq[ruleCnt],
                    pDetectInfo->rd_value[ruleCnt],
                    pDetectInfo->rd_type[ruleCnt]);
    }

    WRITE_DEBUG(CATEGORY_DB, "PID[%d] Reload RULE_DETECT_TB loop(%d)", getpid(), pDetectInfo->tot_cnt);

    return pDetectInfo->tot_cnt;
}


int fpcif_LoadUpgradeSt(dbUgd *pUgdInfo)
{

    int				loop    = 0;
    int				rowCnt  = 0;
    char			sqlBuf[255 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf	(sqlBuf,    "select UA_SQ,UA_NAME,UA_VERSION,"
                        "UA_TARGET_TYPE,UA_TARGET_VALUE,"
                        "UA_DATE,UA_TIME "
                        "from UPGRADE_AGENT_TB where UA_USE=1 AND UA_FLAG != 'D' ORDER BY UA_ORDER");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    /* Update Struct Memory Reset*/
    fpcif_UpgradeInit(pUgdInfo, 0, MAX_UPGRADE_COUNT);
    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)	pUgdInfo->ua_sq[loop] = atol( g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
            {
                if(strlen(g_stMyCon->row[1]) > 32)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"UPGRADE_AGENT_TB Data Is Not Length : [%s] ",g_stMyCon->row[1] );
                }
                snprintf(pUgdInfo->ua_name[loop], sizeof(pUgdInfo->ua_name[loop]),"%s",g_stMyCon->row[1]);
            }
            if (g_stMyCon->row[2] != NULL)
            {
                if(strlen(g_stMyCon->row[2]) > 15)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"UPGRADE_AGENT_TB Data Is Not Length : [%s] ",g_stMyCon->row[2] );
                }
                snprintf(pUgdInfo->ua_version[loop], sizeof(pUgdInfo->ua_version[loop]),"%s",g_stMyCon->row[2]);
            }
            if (g_stMyCon->row[3] != NULL)	pUgdInfo->ua_target_type[loop] = *g_stMyCon->row[3];
            if (g_stMyCon->row[4] != NULL)
            {
                if(strlen(g_stMyCon->row[4]) > 31)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"UPGRADE_AGENT_TB Data Is Not Length : [%s] ",g_stMyCon->row[4] );
                }
                snprintf(pUgdInfo->ua_target_value[loop], sizeof(pUgdInfo->ua_target_value[loop]),"%s",g_stMyCon->row[4]);
            }
            if (g_stMyCon->row[5] != NULL)
            {
                if(strlen(g_stMyCon->row[5]) > 8)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"UPGRADE_AGENT_TB Data Is Not Length : [%s] ",g_stMyCon->row[5] );
                }
                snprintf(pUgdInfo->ua_date[loop], sizeof(pUgdInfo->ua_date[loop]),"%s",g_stMyCon->row[5]);
            }
            if (g_stMyCon->row[6] != NULL)
            {
                if(strlen(g_stMyCon->row[6]) > 6)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"UPGRADE_AGENT_TB Data Is Not Length : [%s] ",g_stMyCon->row[6] );
                }
                snprintf(pUgdInfo->ua_time[loop], sizeof(pUgdInfo->ua_time[loop]),"%s",g_stMyCon->row[6]);
            }


            WRITE_INFO(CATEGORY_DB, "      %s[%d]: %llu",   "ua_sq",loop, pUgdInfo->ua_sq[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%s'",   "ua_name",loop, pUgdInfo->ua_name[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%s'",	"ua_version",loop, pUgdInfo->ua_version[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%c'",	"ua_target_type",loop, pUgdInfo->ua_target_type[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%s'",	"ua_target_value",loop, pUgdInfo->ua_target_value[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%s'",	"ua_date",loop, pUgdInfo->ua_date[loop]);
            WRITE_INFO(CATEGORY_DB, "      %s[%d]: '%s'",	"ua_time",loop, pUgdInfo->ua_time[loop]);

            loop++;
        }//while
        pUgdInfo->tot_cnt = loop; //count이기때문에
    }//if

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload UPGRADE_AGENT_TB loop(%d)", getpid(),loop);

    return loop;
}

int fpcif_LoadGwSt(dbGw *pGwInfo)
{
    register int    loop    = 0;
    int             rowCnt  = 0;
    char            sqlBuf[127 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select SG_CLASS_IP,SG_DEFAULT_MAC from STATS_GW_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
            {
                if(strlen(g_stMyCon->row[0]) > 11)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"STATS_GW_TB Data Is Not Length : [%s] ",g_stMyCon->row[0] );
                }
                memset(pGwInfo[loop].sg_class_ip, 0x00, sizeof(pGwInfo[loop].sg_class_ip));
                snprintf(pGwInfo[loop].sg_class_ip, sizeof(pGwInfo[loop].sg_class_ip), "%s",g_stMyCon->row[0]);
            }
            if (g_stMyCon->row[1] != NULL)
            {
                if(strlen(g_stMyCon->row[1]) > 128)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"STATS_GW_TB Data Is Not Length : [%s] ",g_stMyCon->row[1] );
                }
                memset(pGwInfo[loop].sg_default_mac, 0x00, sizeof(pGwInfo[loop].sg_default_mac));
                snprintf(pGwInfo[loop].sg_default_mac, sizeof(pGwInfo[loop].sg_default_mac), "%s",g_stMyCon->row[1]);
            }
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload STATS_GW_TB loop(%d)", getpid(),loop);

    return loop;
}
int fpcif_LoadManager(dbManager *pMI)
{
    register int	loop    = 0;
    int				rowCnt  = 0;
    char			sqlBuf[255 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select	MN_SQ,"
                    "MN_LEVEL,"
                    "MN_ID,"
                    "MN_PW,"
                    "MN_IPADDR,"
                    "MN_LOGIN_STATUS,"
                    "MN_FLAG,"
                    "MN_FAIL_COUNT,"
                    "MN_EVENT_NOTI,"
                    "MN_CONN_PID,"
                    "MN_CONN_FQ "
                    "from	MANAGER_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)	pMI[loop].mn_sq				= atol(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)	pMI[loop].mn_level			= atoi(	g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
            {
                if(strlen(g_stMyCon->row[2]) > 32)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"MANAGER_TB Data Is Not Length : [%s] ",g_stMyCon->row[2] );
                }
                snprintf(pMI[loop].mn_id, sizeof(pMI[loop].mn_id),"%s",g_stMyCon->row[2]);
            }
            if (g_stMyCon->row[3] != NULL)
            {
                if(strlen(g_stMyCon->row[3]) > 255)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"MANAGER_TB Data Is Not Length : [%s] ",g_stMyCon->row[3] );
                }
                snprintf(pMI[loop].mn_pw, sizeof(pMI[loop].mn_pw),"%s",g_stMyCon->row[3]);
            }
            if (g_stMyCon->row[4] != NULL)
            {
                if(strlen(g_stMyCon->row[4]) > 50)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"MANAGER_TB Data Is Not Length : [%s] ",g_stMyCon->row[4] );
                }
                snprintf(pMI[loop].mn_ipaddr, sizeof(pMI[loop].mn_ipaddr),"%s",g_stMyCon->row[4]);
            }
            if (g_stMyCon->row[5] != NULL)	pMI[loop].mn_login_status	=		*g_stMyCon->row[5];
            if (g_stMyCon->row[6] != NULL)	pMI[loop].mn_flag			=		*g_stMyCon->row[6];
            if (g_stMyCon->row[7] != NULL)	pMI[loop].mn_fail_count		= atoi(	g_stMyCon->row[7]);
            if (g_stMyCon->row[8] != NULL)	pMI[loop].mn_event_noti		=		*g_stMyCon->row[8];
            if (g_stMyCon->row[9] != NULL)	pMI[loop].mn_conn_pid		= atoi(	g_stMyCon->row[9]);
            if (g_stMyCon->row[10] != NULL)	pMI[loop].mn_conn_fq		= atoi(	g_stMyCon->row[10]);

            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);
    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload MANAGER_TB loop(%d)", getpid(),loop);

    return loop;
}



int fpcif_LoadUser(dbUser *pUI)
{
    register int loop;

    int rowCnt = 0;
    char sqlBuf[256] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    /* 2020.10.12 Query 변경 */
    /*sprintf(sqlBuf, "select	A.US_SQ,A.US_IP,A.US_SNO,A.US_DB_ROUTER,"
                    "A.US_LOG_LEVEL,A.US_AUTO_SYNC,A.US_FLAG,B.UG_SQ,B.UG_NAME "
                    "from USER_TB A, USER_GROUP_TB B, USER_LINK_TB C "
                    "where B.UG_SQ = C.UG_SQ and A.US_SQ = C.US_SQ ");*/
//                    "order by A.US_SQ");


    sprintf(sqlBuf,"SELECT A.US_SQ,A.US_IP,A.US_SNO,A.US_DB_ROUTER,A.US_LOG_LEVEL,A.US_AUTO_SYNC,A.US_FLAG,B.UG_SQ,B.UG_NAME "
                   "FROM "
                   "USER_TB AS A , "
                   "(SELECT UG_SQ , UG_NAME from USER_GROUP_TB) AS B , "
                   "(SELECT US_SQ,UG_SQ from USER_LINK_TB) AS C "
                   "WHERE B.UG_SQ = C.UG_SQ and A.US_SQ = C.US_SQ ");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if (g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                       g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }


    loop = 0;
    if (rowCnt > 0)
    {
        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL) pUI[loop].us_sq = atol(g_stMyCon->row[0]);

            if (g_stMyCon->row[1] != NULL)
            {
                if(strlen(g_stMyCon->row[1]) > 128)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"USER_TB  Data Is Not Length : [%s] ",g_stMyCon->row[1] );
                }
                snprintf(pUI[loop].us_ip, sizeof(pUI[loop].us_ip),"%s",g_stMyCon->row[1]);
            }
            if (g_stMyCon->row[2] != NULL)
            {
                if(strlen(g_stMyCon->row[2]) > 32)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"USER_TB Data Is Not Length : [%s]",g_stMyCon->row[2]);
                }
                snprintf(pUI[loop].us_sno, sizeof(pUI[loop].us_sno),"%s",g_stMyCon->row[2]);
            }
            if (g_stMyCon->row[3] != NULL) pUI[loop].us_db_router = *g_stMyCon->row[3];
            if (g_stMyCon->row[4] != NULL) pUI[loop].us_log_level = *g_stMyCon->row[4];
            if (g_stMyCon->row[5] != NULL) pUI[loop].us_auto_sync = *g_stMyCon->row[5];
            if (g_stMyCon->row[6] != NULL) pUI[loop].us_flag = *g_stMyCon->row[6];
            if (g_stMyCon->row[7] != NULL) pUI[loop].ug_sq = atol(g_stMyCon->row[7]);
            if (g_stMyCon->row[8] != NULL)
            {
                if(strlen(g_stMyCon->row[8]) > 100)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"USER_TB Data Is Not Length : [%s] ",g_stMyCon->row[8] );
                }
                snprintf(pUI[loop].ug_name, sizeof(pUI[loop].ug_name),"%s",g_stMyCon->row[8]);
            }

/*
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: %llu",	"us_sq",loop,pUI[loop].us_sq);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"us_ip",loop,pUI[loop].us_ip);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"us_sno",loop,pUI[loop].us_sno);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%c'",	"us_db_router",loop,pUI[loop].us_db_router);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%c'",	"us_log_level",loop,pUI[loop].us_log_level);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%c'",	"us_auto_sync",loop,pUI[loop].us_auto_sync);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%c'",	"us_flag",loop,pUI[loop].us_flag);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: %llu",	"ug_sq",loop,pUI[loop].ug_sq);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"ug_name",loop,pUI[loop].ug_name);
*/
            loop++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "PID[%d] Reload USER_TB/USER_GROUP_TB/USER_LINK_TB loop(%d)", getpid(), loop);

    return loop;

}

int fpcif_LoadBase(void)
{
    register int	loop = 0;
    int             local_loop_count = 0;
    int				rowCnt = 0;
    char			sqlBuf[255 +1]  = {0x00,};
    dbBase*			pBaseInfoTemp   = NULL;
    dbBase*         local_pBaseInfo = NULL;

    fcom_malloc((void**)&local_pBaseInfo, sizeof(dbBase) * cfgMaxBase);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select	HB_SQ,US_SQ,HB_UNQ,"
                    "HB_ACCESS_IP,HB_ACCESS_MAC,HB_SOCKET_IP,"
                    "HB_AGENT_VER,HB_FIRST_TIME,"
                    "HB_ACCESS_TIME,HB_DEL,HB_EXTERNAL "
                    "from HW_BASE_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        fcom_MallocFree((void**)&local_pBaseInfo);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
            {
                local_pBaseInfo[loop].hb_sq	= 		atol( g_stMyCon->row[0]);
            }

            if (g_stMyCon->row[1] != NULL)
            {
                local_pBaseInfo[loop].us_sq	= 		atol( g_stMyCon->row[1]);
            }

            if (g_stMyCon->row[2] != NULL)
            {
                if(strlen(g_stMyCon->row[2]) > 20)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[2] );
                }
                snprintf(local_pBaseInfo[loop].hb_unq, sizeof(local_pBaseInfo[loop].hb_unq),"%s",g_stMyCon->row[2]);
            }

            if (g_stMyCon->row[3] != NULL)
            {
                if(strlen(g_stMyCon->row[3]) > 15)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[3] );
                }
                snprintf(local_pBaseInfo[loop].hb_access_ip, sizeof(local_pBaseInfo[loop].hb_access_ip),"%s",g_stMyCon->row[3]);
            }

            if (g_stMyCon->row[4] != NULL)
            {
                if(strlen(g_stMyCon->row[4]) > 17)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[4] );
                }
                snprintf(local_pBaseInfo[loop].hb_access_mac, sizeof(local_pBaseInfo[loop].hb_access_mac),"%s",g_stMyCon->row[4]);
            }
            if (g_stMyCon->row[5] != NULL)
            {
                if(strlen(g_stMyCon->row[5]) > 15)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[5] );
                }
                snprintf(local_pBaseInfo[loop].hb_sock_ip, sizeof(local_pBaseInfo[loop].hb_sock_ip),"%s",g_stMyCon->row[5]);
            }

            if (g_stMyCon->row[6] != NULL)
            {
                if(strlen(g_stMyCon->row[6]) > 16)
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[6]);
                }
                snprintf(local_pBaseInfo[loop].hb_agent_ver, sizeof(local_pBaseInfo[loop].hb_agent_ver),"%s",g_stMyCon->row[6]);
            }

            if (g_stMyCon->row[7] != NULL)
            {
                if(strlen(g_stMyCon->row[7]) > 19)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[7]);
                }
                snprintf(local_pBaseInfo[loop].hb_first_time, sizeof(local_pBaseInfo[loop].hb_first_time),"%s",g_stMyCon->row[7]);
            }
            if (g_stMyCon->row[8] != NULL)
            {
                if(strlen(g_stMyCon->row[8]) > 19)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"HW_BASE_TB Data Is Not Length : [%s] ",g_stMyCon->row[8]);
                }
                snprintf(local_pBaseInfo[loop].hb_access_time, sizeof(local_pBaseInfo[loop].hb_access_time),"%s",g_stMyCon->row[8]);
            }
            if (g_stMyCon->row[9] != NULL)
            {
                local_pBaseInfo[loop].hb_del =				*g_stMyCon->row[9];
            }
            if (g_stMyCon->row[10] != NULL)
            {
                local_pBaseInfo[loop].hb_external =         *g_stMyCon->row[10];
            }

/*
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: %llu", "hb_sq",loop,pBI[loop].hb_sq);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: %llu", "us_sq",loop,pBI[loop].us_sq);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_unq",loop,pBI[loop].hb_unq);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_access_ip",loop,pBI[loop].hb_access_ip);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_access_mac",loop,pBI[loop].hb_access_mac);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_sock_ip",loop,pBI[loop].hb_sock_ip);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_agent_ver",loop,pBI[loop].hb_agent_ver);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_first_time",loop,pBI[loop].hb_first_time);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%s'",	"hb_access_time",loop,pBI[loop].hb_access_time);
			WRITE_INFO(CATEGORY_DB,"          %s[%d]: '%c'",	"hb_del",loop,pBI[loop].hb_del);
*/
            loop++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    /** Write Lock 진입을 빠르게위해 Read Lock 진입을 못하게 Flag 막아준다. **/
    fpcif_PthreadMemoryWriteLockBegin();

    while(1)
    {
        if(pthread_rwlock_wrlock(&mutexpolicy) == 0)
        {
            pBaseInfoTemp = pBaseInfo;
            pBaseInfo = local_pBaseInfo;
            g_stProcPcifInfo.nTotBase = loop;
            fcom_MallocFree((void**)&pBaseInfoTemp);
            fcom_SleepWait(5);
            pthread_rwlock_unlock(&mutexpolicy);

            break;

        }
        else
        {
            fcom_Msleep(500);
            local_loop_count++;
        }
        if( local_loop_count >=  500 )
        {
            WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
            break;
        }
    }
    fpcif_PthreadMemoryWriteLockEnd();

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload HW_BASE_TB loop(%d) ", getpid(),loop);

    return 0;
}

int fpcif_LoadGroupLink(dbGroupLink *pGL)
{
    register int	loop            = 0;
    int				rowCnt          = 0;
    char			sqlBuf[127 +1]  = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "select	UG_SQ_P, UG_SQ_C from USER_GROUP_LINK_TB order by UG_SQ_P");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)	pGL[loop].ug_sq_p		= atoi(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)	pGL[loop].ug_sq_c		= atoi( g_stMyCon->row[1]);

            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload USER_GROUP_LINK_TB loop(%d)", getpid(),loop);

    return loop;
}

int fpcif_LoadConfig(dbConfig *pConfigInfo)
{
    int         rowCnt  = 0;
    int         loop    = 0;
    char        sqlBuf[127 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "select CF_NAME, CF_VALUE, CF_FLAG from CONFIG_TB order by CF_NAME");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return (-1);
    }

    if(rowCnt > 0)
    {
        loop = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
            {
                if(strlen(g_stMyCon->row[0]) > 64)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Column Data (%s) Size Error ! ",g_stMyCon->row[0]);
                }
                snprintf(pConfigInfo[loop].cfname,     sizeof(pConfigInfo[loop].cfname),"%s",g_stMyCon->row[0]);
            }

            if (g_stMyCon->row[1] != NULL)
            {
                if(strlen(g_stMyCon->row[1]) > 512)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Column Data (%s) Size Error !",g_stMyCon->row[1]);
                }
                snprintf(pConfigInfo[loop].cfvalue,     sizeof(pConfigInfo[loop].cfvalue),"%s",g_stMyCon->row[1]);
            }
            if (g_stMyCon->row[2] != NULL) pConfigInfo[loop].cfflag         =   *g_stMyCon->row[2];

            loop++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"PID[%d] Reload CONFIG_TB loop(%d)", getpid(),loop);

    return loop;
}

/* 2020.12.08 RULE_DETECT_TB 참조 제거. */
/*int fpcif_LoadDetectSel(unsigned long p_ruSq, dbDetectSel *pDS)
{

    int				loop   = 0;
    int				rowCnt = 0;
    int				type   = 0;
    char			sqlBuf[128] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"select RD_TYPE,RD_VALUE from RULE_DETECT_TB "
                       "where RU_SQ = %lu and RD_FLAG != 'D'", p_ruSq);


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,	"Fail in query, errcode(%d)",
                g_stMyCon->nErrCode);
        WRITE_CRITICAL(CATEGORY_DB,	"Fail in query, errmsg(%s)",
                g_stMyCon->cpErrMsg);

        return -1;
    }

    // Init
    fcom_MallocFree((void**)&(pDS->dp_process_black));
    fcom_MallocFree((void**)&(pDS->dp_process_white));
    fcom_MallocFree((void**)&(pDS->ws_ipaddr));
    fcom_MallocFree((void**)&(pDS->ws_url));

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if( g_stMyCon->row[0] != NULL )
            {
                type = atoi(g_stMyCon->row[0]);
                if( type == 0)
                {
                    if( g_stMyCon->row[1] != NULL )
                    {
                        if(fcom_malloc((void**)&(pDS->dp_process_black),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            break;
                        }

                        strcpy(pDS->dp_process_black, g_stMyCon->row[1]);
                    }
                }
                else if( type == 1 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {
                        if(fcom_malloc((void**)&(pDS->dp_process_white),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            break;
                        }

                        strcpy(pDS->dp_process_white, g_stMyCon->row[1]);
                    }
                }
                else if( type == 2 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {
                        if(fcom_malloc((void**)&(pDS->ws_ipaddr),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                            break;
                        }
                        strcpy(pDS->ws_ipaddr, g_stMyCon->row[1]);
                    }
                }
                else if( type == 3 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {

                        if(fcom_malloc((void**)&(pDS->ws_url),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            break;
                        }

                        strcpy(pDS->ws_url, g_stMyCon->row[1]);
                    }
                }
                else if( type == 4 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {

                        if(fcom_malloc((void**)&(pDS->dp_process_block),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                            break;
                        }

                        strcpy(pDS->dp_process_block, g_stMyCon->row[1]);
                    }
                }
                else if( type == 5 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {
                        if(fcom_malloc((void**)&(pDS->dp_tot_ctrl_cpu_exception),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                            break;
                        }
                        strcpy(pDS->dp_tot_ctrl_cpu_exception, g_stMyCon->row[1]);
                    }
                }
                else if( type == 6 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {

                        if(fcom_malloc((void**)&(pDS->dp_alarm_cpu_exception),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                            break;
                        }
                        strcpy(pDS->dp_alarm_cpu_exception, g_stMyCon->row[1]);
                    }
                }
                else if( type == 7 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {
                        if(fcom_malloc((void**)&(pDS->dp_tot_ctrl_cpu_exception),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                            break;
                        }
                        strcpy(pDS->dp_tot_ctrl_cpu_exception, g_stMyCon->row[1]);
                    }
                }
                else if( type == 8 )
                {
                    if( g_stMyCon->row[1] != NULL )
                    {

                        if(fcom_malloc((void**)&(pDS->dp_ctrl_cpu_exception),sizeof(char)*(strlen(g_stMyCon->row[1])+1)) != 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                            break;
                        }
                        strcpy(pDS->dp_ctrl_cpu_exception, g_stMyCon->row[1]);
                    }
                }
            }
            loop++;
        }//while

    }//if
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"PID[%d] Reload loop(%d) ", getpid(),loop);

    return loop;
}*/

int fpcif_LoadDetectIdx(unsigned long p_ruSq, int maxRuleCnt, dbDetectSel *pDS)
{
    int local_nLoop = 0;
    int local_nType = 0;

    for(local_nLoop = 0; local_nLoop < maxRuleCnt; local_nLoop++)
    {
        if(detectInfo.ru_sq[local_nLoop] == p_ruSq)
        {
            local_nType = detectInfo.rd_type[local_nLoop];

            switch(local_nType)
            {
                case BLACK_PROCESS:
                {
                    if(fcom_malloc((void**)&(pDS->dp_process_black),sizeof(char) * (strlen(detectInfo.rd_value[local_nLoop]) +1) ) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                        break;
                    }
                    strcpy(pDS->dp_process_black, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case WHITE_PROCESS:
                {
                    if(fcom_malloc((void**)&(pDS->dp_process_white), sizeof(char) * (strlen(detectInfo.rd_value[local_nLoop]) +1) ) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                        break;
                    }
                    strcpy(pDS->dp_process_white, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case CONEXT_URL:
                {
                    if(fcom_malloc((void**)&(pDS->ws_ipaddr),sizeof(char) * (strlen(detectInfo.rd_value[local_nLoop]) +1) ) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                        break;
                    }
                    strcpy(pDS->ws_ipaddr, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case KILL_PROCESS:
                {
                    if(fcom_malloc((void**)&(pDS->dp_process_block),sizeof(char)*(strlen(detectInfo.rd_value[local_nLoop]) +1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                        break;
                    }
                    strcpy(pDS->dp_process_block, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case TOTAL_ALARM_CPU_EXCEPTION:
                {
                    if(fcom_malloc((void**)&(pDS->dp_tot_alarm_cpu_exception),sizeof(char)*(strlen(detectInfo.rd_value[local_nLoop])+1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                        break;
                    }
                    strcpy(pDS->dp_tot_alarm_cpu_exception, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case ALARM_CPU_EXCEPTION:
                {
                    if(fcom_malloc((void**)&(pDS->dp_alarm_cpu_exception),sizeof(char) * (strlen(detectInfo.rd_value[local_nLoop]) +1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                        break;
                    }
                    strcpy(pDS->dp_alarm_cpu_exception, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case TOTAL_CONTROL_CPU_EXCEPTION:
                {
                    if(fcom_malloc((void**)&(pDS->dp_tot_ctrl_cpu_exception),sizeof(char) * (strlen(detectInfo.rd_value[local_nLoop]) +1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                        break;
                    }
                    strcpy(pDS->dp_tot_ctrl_cpu_exception, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                case CONTROL_CPU_EXCEPTION:
                {
                    if(fcom_malloc((void**)&(pDS->dp_ctrl_cpu_exception),sizeof(char)*(strlen(detectInfo.rd_value[local_nLoop])+1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                        break;
                    }
                    strcpy(pDS->dp_ctrl_cpu_exception, detectInfo.rd_value[local_nLoop]);

                    break;
                }
                default:
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Unkown RD_TYPE ");
                    break;
                }

            }
        }

    }

    return 0;


}

int fpcif_UpdateConnManager(char *p_userId, int pid, int fqNum)
{
    int				updateCnt = 0;
    char			sqlBuf[255 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    snprintf(sqlBuf, sizeof(sqlBuf),
                       "update MANAGER_TB set "
                       "MN_CONN_PID=%d,"
                       "MN_CONN_FQ=%d "
                       "where MN_ID='%s'", pid,fqNum,p_userId);

    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"Update MANAGER_TB conn pid, id(%s)pid(%d)cnt(%d)",
            p_userId,pid,updateCnt);

    return updateCnt;
}


int fpcif_UpdateSockIpManager(char *p_id, char *p_ip)
{
    int				updateCnt = 0;
    char			sqlBuf[127 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf	(sqlBuf,"update MANAGER_TB set "
                       "MN_SOCKET_IP='%s' "
                       "where MN_ID='%s'", p_ip,p_id);

    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"Update MANAGER_TB sock_ip , id(%s)ip(%s)update(%d)",
            p_id,p_ip,updateCnt);

    return updateCnt;
}

int fpcif_UpdateTerminateManager(char *p_userId)
{
    int				updateCnt       = 0;
    char			sqlBuf[127 +1]  = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf	(sqlBuf,"update MANAGER_TB set "
                       "MN_LOGIN_STATUS=0, "
                       "MN_CONN_PID=0,"
                       "MN_CONN_FQ=-1 "
                       "where MN_ID='%s'", p_userId);

    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"Update MANAGER_TB terminate login status, id(%s)update(%d)",
            p_userId,updateCnt);

    return updateCnt;
}

int fpcif_UpdateFailCountManager(char *p_userId, int cnt)
{
    int				updateCnt       = 0;
    char			sqlBuf[255 +1]  = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    if(cnt == 0)
    {
        sprintf	(sqlBuf,"update MANAGER_TB set MN_FAIL_COUNT=0 where MN_ID='%s'", p_userId);
    }
    else
    {
        sprintf	(sqlBuf,"update MANAGER_TB set MN_FAIL_COUNT=MN_FAIL_COUNT+1 where MN_ID='%s'", p_userId);
    }

    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Update MANAGER_TB login fail count, id(%s)cnt(%d)", p_userId, updateCnt);

    return updateCnt;
}

int fpcif_UpdateConfigChange(
        char*	p_debugName,	// process name
        char*	p_changeName)	// path of touch file
{
    int				updateCnt = 0;
    char			sqlBuf[255 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"update SYS_CONFIG_CHANGE_TB set CHGFLAG=1 "
                       "where PROCESS='%s' and NAME like '%%%s'", p_debugName,p_changeName);

    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_DEBUG(CATEGORY_DB,"Fail in query, errcode(%d), errmsg(%s)",
                    g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Update SYS_CONFIG_CHANGE_TB, proc(%s)change(%s)cnt(%d)",
            p_debugName,p_changeName,updateCnt);

    return updateCnt;
}

int fpcif_CheckLoginManagerSt(
        char *p_userId,
        char *p_userPw,
        int *p_userLevel,
        char *p_userIp,
        int p_msgCode,
        int p_fqNum)
{
    int rxt = 0;
    int mIdx = 0;
    int tokenCnt = 0;
    int sqlLoginFailCnt = 0;
    int loginLimitFailCnt = 0;
    int multiLoginFlag = 0;
    int cpPid = 0;
    int psLine = 0;
    int local_loop_count = 0;
    int tokenLoop        = 0;
    char arrTokenIp[50 + 1] = {0x00,};
    char arrTokenIp_Copy[50 + 1] = {0x00,};
    char cpProc[128] = {0x00,};
    char* local_TokPtr = NULL;
    char* local_RemainPtr = NULL;


    rxt = fpcif_GetIdxByManagerSt(p_userId);
    mIdx = rxt;


    WRITE_INFO_IP(p_userIp, "Check manager info, idx(%d) cpip(%s) st_ip(%s)",
                  rxt, p_userIp, pManagerInfo[mIdx].mn_ipaddr);
    if (rxt > -1)
    {
        //Check fail count
        sqlLoginFailCnt = pManagerInfo[mIdx].mn_fail_count;
        rxt = fpcif_GetIdxByConfigSt("LIMIT_PW_FAIL_COUNT");
        if (rxt < 0)
        {
            WRITE_DEBUG_IP(p_userIp, "Not found config value");
            loginLimitFailCnt = 5; //fixed default '5'
        }
        else
        {
            loginLimitFailCnt = atoi(pConfigInfo[rxt].cfvalue);
            if(loginLimitFailCnt <= 0)
            {
                loginLimitFailCnt = 5;
            }
        }
        WRITE_CRITICAL_IP(p_userIp, "Check login fail count(%d/%d)",
                          sqlLoginFailCnt, loginLimitFailCnt);

        if (sqlLoginFailCnt > loginLimitFailCnt) //Over limit fail count
        {
            WRITE_CRITICAL_IP(p_userIp, "Over login limit count, currCnt(%d)limitCnt(%d)",
                              sqlLoginFailCnt, loginLimitFailCnt);
            return -5;
        }

        //Check multi login
        rxt = fpcif_GetIdxByConfigSt("MULTI_LOGIN_FLAG");
        if (rxt < 0)
        {
            WRITE_DEBUG_IP(p_userIp, "Not found config value");
            multiLoginFlag = 0;
        }
        else
        {
            multiLoginFlag = atoi(pConfigInfo[rxt].cfvalue);
        }

        WRITE_CRITICAL_IP(p_userIp, "Check multiLoginFlag(%d)", multiLoginFlag);
        if (!multiLoginFlag)
        {
            WRITE_INFO_IP(p_userIp, "- mn_login_status(%d)",
                          (pManagerInfo[mIdx].mn_login_status - '0') % 48);
            if ((pManagerInfo[mIdx].mn_login_status - '0') % 48 != 0)
            {
                //MANAGE_FORCED_LOGIN이면
                if (p_msgCode == MANAGE_FORCED_LOGIN)
                {
                    cpPid = pManagerInfo[mIdx].mn_conn_pid;
                    //cpPid가 실제로 존재하는지 확인한다.
                    psLine = fpcif_CheckPid(p_userIp, cpPid, "dap_pcif");
                    if (psLine == 1)
                    {
                        //1. kill PID
                        memset(cpProc, 0x00, sizeof(cpProc));
                        sprintf(cpProc, "%s/bin/%s",
                                g_stServerInfo.stDapComnInfo.szDebugName,
                                "dap_pcif");
                        if (cpPid != 0 && access(cpProc, X_OK) == 0)
                        {
                            WRITE_CRITICAL_IP(p_userIp, "Try manager forced kill, pid(%d)ip(%s)fq(%d)",
                                              cpPid,
                                              pManagerInfo[mIdx].mn_ipaddr,
                                              pManagerInfo[mIdx].mn_conn_fq);

                            fpcif_SendNotiByFq(pManagerInfo[mIdx].mn_ipaddr, pManagerInfo[mIdx].mn_conn_fq, p_userIp);

                        }
                        else
                        {
                            WRITE_CRITICAL_IP(p_userIp, "Can't access, pid(%d)proc(%s)",
                                              cpPid, cpProc);
                        }
                    }
                    else
                    {
                        WRITE_INFO_IP(p_userIp, "- psLine(%d)", psLine);
                    }
                    //2. update mn_login_status=0은 접속끊어질때 처리한다.
                }
                else
                {
                    WRITE_CRITICAL_IP(p_userIp, "Already login user, status(%c)",
                                      pManagerInfo[mIdx].mn_login_status);
                    return -2;
                }
            }
        }

        //행안부를위해
        if (!strncmp(p_userId, "admin", 5)
            && !strncmp(pManagerInfo[mIdx].mn_ipaddr, "0.0.0.0", 7))
        {
            //admin이고 IP가 0.0.0.0 일때 IP체크를 안함
            WRITE_INFO_IP(p_userIp, "Admin Login, Ignore an ip checking, p_userId(%s)mn_ipaddr(%s)",
                              p_userId, pManagerInfo[mIdx].mn_ipaddr);
        }
        else if( strncmp(p_userId,"admin",5) != 0
                 && strncmp(pManagerInfo[mIdx].mn_ipaddr,"0.0.0.0",7) == 0 )
        {
            //admin이 아니고 IP가 0.0.0.0 일때도 IP 체크를 안함.
            WRITE_INFO_IP(p_userIp, "Ignore an ip checking, p_userId(%s)mn_ipaddr(%s)",
                          p_userId, pManagerInfo[mIdx].mn_ipaddr);
        }
        else
        {
            //Check IP
            memset(arrTokenIp, 0x00, sizeof(arrTokenIp));
            strcpy(arrTokenIp, pManagerInfo[mIdx].mn_ipaddr);
            WRITE_CRITICAL_IP(p_userIp, "Check ip p_userId(%s)mn_ipaddr(%s)",
                              p_userId,
                              pManagerInfo[mIdx].mn_ipaddr);
            WRITE_INFO_IP(p_userIp, "- arrTokenIp(%s)", arrTokenIp);

            if (strlen(arrTokenIp) < 7)
            {
                WRITE_CRITICAL_IP(p_userIp, "Invalid ip length, dbip(%s)",
                                  arrTokenIp);
                return -3;
            }
            else
            {
                tokenCnt = fcom_TokenCnt(arrTokenIp, ",");
                if (tokenCnt > 0)
                {
                    // 원본버퍼 Copy하여 Copy한 Buffer로 strtok을 수행한다.
                    memcpy(arrTokenIp_Copy, arrTokenIp, sizeof(arrTokenIp_Copy) );
                    local_TokPtr = strtok_r(arrTokenIp_Copy,",", &local_RemainPtr);
                    if ( strcmp(local_TokPtr, p_userIp) != 0)
                    {
                        // IP 비교
                        for ( tokenLoop = 0; tokenLoop < tokenCnt; tokenLoop++ )
                        {
                            local_TokPtr = strtok_r(NULL,",",&local_RemainPtr);
                            if ( strcmp(local_TokPtr, p_userIp) == 0)
                            {
                                WRITE_DEBUG_IP(p_userIp,"Check Ip (%s) Pass ", p_userIp);
                                rxt = 1;
                                break;
                            }
                        }
                    }
                    else // 첫 IP비교에서 IP가 같으면
                    {
                        rxt = 1;
                    }

                    if (rxt == 0)
                    {
                        WRITE_CRITICAL_IP(p_userIp, "Invalid compare ip  cpip(%s)dbip(%s)",
                                          p_userIp, arrTokenIp);
                        return -3;
                    }
                }
                else
                {
                    WRITE_INFO_IP(p_userIp, "- p_userIp(%s) arrTokenIp(%s)", p_userIp, arrTokenIp);
                    if (strcmp(p_userIp, arrTokenIp) != 0)
                    {
                        WRITE_CRITICAL_IP(p_userIp, "Invalid compare ip, cpip(%s)dbip(%s)",
                                          p_userIp,
                                          arrTokenIp);
                        return -3;
                    }
                }
            } //else
        } //else
    }
    else
    {
        WRITE_CRITICAL_IP(p_userIp, "Not found managerid(%s)", p_userId);
        return 0;
    }


    WRITE_CRITICAL_IP(p_userIp, "Check password");
    rxt = (fsec_BcryptCheckpw(p_userPw, pManagerInfo[mIdx].mn_pw) == 0);
    if (rxt != 1)
    {
        //fail_count 업데이트 처리는 상위 함수에서 한다.
        WRITE_CRITICAL_IP(p_userIp, "Invalid password");

        while (1)
        {
            if (pthread_mutex_trylock(&mutexsum) == 0)
            {
                fpcif_UpdateFailCountManager(p_userId, 1);
                fpcif_UpdateConfigChange(g_stServerInfo.stDapComnInfo.szDebugName, "cp_change");
                pthread_mutex_unlock(&mutexsum);
                break;
            }
            else
            {
                fcom_Msleep(500);
                local_loop_count++;
            }
            if (local_loop_count >= 500)
            {
                WRITE_DEBUG_IP(p_userIp, "Mutex loop break ");
                break;
            }
        }

        return -4;
    }

    //Print user level
    *p_userLevel = pManagerInfo[mIdx].mn_level;
    WRITE_INFO_IP(p_userIp, "User Level (%d)", *p_userLevel);

    while (1)
    {
        if (pthread_mutex_trylock(&mutexsum) == 0)
        {
            //로그인 성공이면 PID,FailCount 업데이트
            fpcif_UpdateFailCountManager(p_userId, 0);
            fpcif_UpdateConnManager(p_userId, getpid(), p_fqNum);
            fpcif_UpdateConfigChange(g_stServerInfo.stDapComnInfo.szDebugName, "cp_change");

            pthread_mutex_unlock(&mutexsum);
            break;
        }
        else
        {
            fcom_Msleep(500);
            local_loop_count++;
        }
        if (local_loop_count >= 500)
        {
            WRITE_DEBUG_IP(p_userIp, "Mutex loop break ");
            break;
        }
    }

    return 1;
}