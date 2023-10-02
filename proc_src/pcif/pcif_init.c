//
// Created by KimByoungGook on 2020-10-13.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#include <string.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>



#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "json/dap_json.h"

#include "pcif.h"



int fpcif_PcifInit(void)
{
    int nLoop       = 0;
    int cfgMaxUser  = 0;
    int cfgMaxGroup = 0;

    char szTemp[255 +1] = {0x00,};
    char szPath[256]    = {0x00,};
    char local_szDefaultIp[15 +1] = {0x00,};

    _DAP_COMN_INFO*  pstComnInfo    = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;


    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome          ,
             sizeof(pstComnInfo->szDapHome  ),
             "%s"                     ,
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return FALSE;
    }
    fcom_SetIniName(pstComnInfo->szComConfigFile);

    /* Signal Handle */
    fpcif_SigHandler();

    /* Cert Config */
    fcom_GetProfile("SSL","CERT_PATH"   ,g_stProcPcifInfo.certPath  ,"/home/intent/dap/config/cert");
    fcom_GetProfile("SSL","CA_FILE"     ,g_stProcPcifInfo.caFile    ,"root.crt");
    fcom_GetProfile("SSL","CERT_FILE"   ,g_stProcPcifInfo.certFile  ,"server.crt");
    fcom_GetProfile("SSL","KEY_FILE"    ,g_stProcPcifInfo.keyFile   ,"server.key");

    fcom_GetProfile( "PCIF", "LISTEN_PORT", g_stProcPcifInfo.pcPort, "50203");

    g_stProcPcifInfo.cfgThreadCnt   = fcom_GetProfileInt("PCIF","THREAD_COUNT"  ,5);
    g_stProcPcifInfo.cfgForkCnt     = fcom_GetProfileInt("PCIF","FORK_COUNT"    ,10);

    /* Unit MB */
    g_stProcPcifInfo.nCfgMaxFileSize  = fcom_GetProfileInt( "DBFILE", "MAX_DBFILE_SIZE" , 10   );
    g_stProcPcifInfo.nCfgMaxFileIndex = fcom_GetProfileInt( "DBFILE", "MAX_DBFILE_INDEX", 9999 );
    g_stProcPcifInfo.nCfgMaxFileSize *= (1024*1024);
    if ( g_stProcPcifInfo.nCfgMaxFileSize > MAX_DBFILE_SIZE )
    {
        g_stProcPcifInfo.nCfgMaxFileSize = MAX_DBFILE_SIZE;
    }

    if ( g_stProcPcifInfo.nCfgMaxFileIndex > MAX_DBFILE_INDEX)
    {
        g_stProcPcifInfo.nCfgMaxFileIndex = MAX_DBFILE_INDEX;
    }

    if(g_stProcPcifInfo.cfgForkCnt > MAX_CHILD)
        g_stProcPcifInfo.cfgForkCnt = MAX_CHILD;

    if(g_stProcPcifInfo.cfgThreadCnt > MAX_THREAD)
        g_stProcPcifInfo.cfgThreadCnt = MAX_THREAD;

    g_stProcPcifInfo.time_out               = fcom_GetProfileInt("PCIF","MANAGER_SESSION_TIMEOUT"   ,1000);
    g_stProcPcifInfo.cfgQCount              = fcom_GetProfileInt("DBIF","PROC_COUNT"                ,3);
    g_stProcPcifInfo.cfgRetryAckCount       = fcom_GetProfileInt("PCIF","ACK_RETRY_LIMIT_COUNT"     ,3);
    g_stProcPcifInfo.retryAckSleep          = fcom_GetProfileInt("PCIF","ACK_RETRY_WAIT_TIME"       ,100);
    g_stProcPcifInfo.cfgUseGetArpMac        = fcom_GetProfileInt("PCIF","USE_GET_ARP_MAC"           ,1);
    g_stProcPcifInfo.cfgLoadDetectMem       = fcom_GetProfileInt("PCIF","LOAD_DETECT_MEMORY"        ,1);
    g_stProcPcifInfo.cfgKeepSession         = fcom_GetProfileInt("MYSQL","KEEP_SESSION"             ,60);
    g_stProcPcifInfo.cfgFileDownDelayTime   = fcom_GetProfileInt("REPORT","FILE_DOWN_DELAY_TIME"    ,5);
    g_stProcPcifInfo.cfgCheckNotiInterval   = fcom_GetProfileInt("PCIF","CHECK_NOTI_INTERVAL"       ,30);
    g_stProcPcifInfo.cfgUseDebugRealIp      = fcom_GetProfileInt("PCIF","USE_DEBUG_REAL_IP"         ,1);
    g_stProcPcifInfo.cfgCheckEventInterval  = fcom_GetProfileInt("PCIF","CHECK_EVENT_INTERVAL"      ,10);
    g_stProcPcifInfo.cfPrmonInterval        = fcom_GetProfileInt("PRMON","INTERVAL"                 ,60);
    g_stProcPcifInfo.nAcceptNonBlockFlag    = fcom_GetProfileInt("PCIF", "ACCEPT_NON_BLOCK"         , 0);

    fcom_GetProfile("PCIF"  ,"DROP_L4_SWITCH_IP", g_stProcPcifInfo.cfgDropL4SwIp ,"");
    fcom_GetProfile("REPORT", "WORK_PATH"       , g_stProcPcifInfo.cfgReportPath, "/home/intent/dap/script/report");

    pstComnInfo->nCfgMgwId = fcom_GetProfileInt("COMMON","SERVER_ID",1);

    fsock_GetNic(szTemp);
    fsock_GetIpAddress(szTemp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );
    if(fcom_malloc((void**)&g_pstNotiMaster,MAX_PCIF_NOTI * sizeof(_CONFIG_NOTI)) != 0)
    {
        printf("fcom_malloc failed |%s\n",__func__ );
        exit(0);
    }
    /** Fork Child 저장 Array Init **/
    fcom_malloc((void**)&g_stProcPcifInfo.ptrChildPid,sizeof(int) * g_stProcPcifInfo.cfgForkCnt);

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "%s/%s/%s",pstComnInfo->szDapHome,"log","stdout.log");

    /* 표준출력(1) 표준오류(2) 리다이렉션 처리 */
    fcom_InitStdLog(szTemp);

    memset(szTemp, 0x00, sizeof(szTemp));
    for(nLoop = 0; nLoop < MAX_PCIF_NOTI; nLoop++)
    {
        g_pstNotiMaster[nLoop].lastModify = 0;
        g_pstNotiMaster[nLoop].reload = TRUE;
        switch (nLoop)
        {
            case CONFIG_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/config_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "CONFIG_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case RULE_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/rule_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "RULE_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case RULE_SCHEDULE_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/rule_schedule_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "RULE_SCHEDULE_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case RULE_DETECT_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/rule_detect_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "RULE_DETECT_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case AGENT_UPGRADE_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/agent_upgrade_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "AGENT_UPGRADE_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case GW_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/gw_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "GW_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case CP_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/cp_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "CP_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
            case BASE_CHANGE:
                snprintf(szTemp,sizeof(szTemp),"%s/config/base_change",pstComnInfo->szDapHome);
                fcom_GetProfile("COMMON", "BASE_CHANGE", g_pstNotiMaster[nLoop].szNotiFileName, szTemp);
                break;
        }
    }

    if(fcom_malloc((void**)&g_pstNotiEvent,MAX_EVENT_NOTI * sizeof(_CONFIG_NOTI)) != 0)
    {
        printf("fcom_malloc Failed |%s\n",__func__ );
        exit(0);
    }

    snprintf(szTemp, sizeof(szTemp),"%s/config/event_change",pstComnInfo->szDapHome);
    fcom_GetProfile("COMMON","EVENT_CHANGE", g_pstNotiEvent[EVENT_CHANGE].szNotiFileName, szTemp);

    for(nLoop = 0; nLoop < MAX_EVENT_NOTI; nLoop++)
    {
        g_pstNotiEvent[nLoop].lastModify    =   0;
        g_pstNotiEvent[nLoop].reload        =   TRUE;
    }

    /* DAP Process Log Init */
    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    /* Queue Init */
    snprintf(pstQueueInfo->szDAPQueueHome,sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ",
             pstComnInfo->szDapHome);
    if (access(pstQueueInfo->szDAPQueueHome,R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s) |%s\n", pstQueueInfo->szDAPQueueHome, __func__);
            return RET_FAIL;
        }
    }
    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
             "%s/PRMONQ", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szPrmonQueueHome,__func__);
        return RET_FAIL;
    }

    /** 신규 dap_fw 프로세스용 Unix Socket Init **/
    /** 정책 데이터  **/
    memset( szTemp, 0x00 ,sizeof(szTemp) );
    sprintf(szTemp,"%s/FW_POLICY", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(FW_POLICY, szTemp) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s \n", szTemp, __func__);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue init, path(%s) |%s \n", szTemp,__func__);
        return RET_FAIL;
    }

    /** Agent 서비스 상태 데이터  **/
    memset( szTemp, 0x00 ,sizeof(szTemp) );
    sprintf(szTemp,"%s/FW_SERVICE", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(FW_SERVICE, szTemp) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s \n", szTemp, __func__);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue init, path(%s) |%s \n", szTemp,__func__);
        return RET_FAIL;
    }

    snprintf(g_stProcPcifInfo.szDtFilePath, sizeof(g_stProcPcifInfo.szDtFilePath),
             "%s/data/dt/", pstComnInfo->szDapHome );
    snprintf(g_stProcPcifInfo.szPolicyFilePath, sizeof(g_stProcPcifInfo.szPolicyFilePath),
             "%s/data/policy/", pstComnInfo->szDapHome);

    if( fcom_MkPath(g_stProcPcifInfo.szDtFilePath, 0755) != 0)
    {
        printf("Init Make Dir (%s) Failed \n", g_stProcPcifInfo.szDtFilePath);
        return (-1);
    }

    if( fcom_MkPath(g_stProcPcifInfo.szPolicyFilePath, 0755) != 0)
    {
        printf("Init Make Dir (%s) Failed \n", g_stProcPcifInfo.szPolicyFilePath);
        return (-1);
    }

    snprintf(pstQueueInfo->szDblogQueueHome,
             sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szDblogQueueHome,__func__);
        return RET_FAIL;
    }

    snprintf(pstQueueInfo->szReportQueueHome,
             sizeof(pstQueueInfo->szReportQueueHome),
             "%s/REPORTQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(REPORT_QUEUE, pstQueueInfo->szReportQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szReportQueueHome,__func__);
        return RET_FAIL;
    }

    snprintf(pstQueueInfo->szTailQueueHome,
             sizeof(pstQueueInfo->szTailQueueHome),
             "%s/TAILQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(TAIL_QUEUE, pstQueueInfo->szTailQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szTailQueueHome,__func__);
        return RET_FAIL;
    }

    cfgMaxUser  = fcom_GetProfileInt("PCIF", "MAX_USER_COUNT", MAX_USER_COUNT);
    cfgMaxGroup = fcom_GetProfileInt("PCIF", "MAX_GROUP_COUNT",MAX_GROUP_COUNT);
    cfgMaxBase  = fcom_GetProfileInt("PCIF", "MAX_BASE_COUNT",MAX_BASE_COUNT);

    if(cfgMaxUser   > 100000)   cfgMaxUser  = 100000;
    if(cfgMaxGroup  > 100000)   cfgMaxGroup = 100000;
    if(cfgMaxBase   > 100000)   cfgMaxBase  = 100000;

    if(fcom_malloc((void**)&pUserInfo,sizeof(dbUser) * cfgMaxUser) != 0)
    {
        printf("Fail in pUserInfo malloc init |%s\n",__func__ );
        return RET_FAIL;
    }

    if(fcom_malloc((void**)&pGroupLink, sizeof(dbGroupLink) * cfgMaxGroup) != 0)
    {
        printf("Fail in pGroupLink malloc init|%s",__func__ );
        return RET_FAIL;
    }

    if(fcom_malloc((void**)&pBaseInfo, (sizeof(dbBase) * cfgMaxBase) +1) != 0)
    {
        printf("Fail in pBaseInfo malloc init|%s\n",__func__ );
        return RET_FAIL;
    }

    /* Get Config Thread Stack Size */
    g_stProcPcifInfo.nThreadStackSize = fcom_GetProfileInt("PCIF", "THREAD_STACK_SIZE", 4);/* Default 4MB */
    g_stProcPcifInfo.nThreadStackSize = g_stProcPcifInfo.nThreadStackSize * 1024 * 1024;

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "%s","N");
    fcom_GetProfile("PCIF", "THREAD_HANG_CHECK_YN", g_stProcPcifInfo.cfgThreadHangCheck, szTemp);

    memset(szTemp, 0x00, sizeof(szTemp) );

    snprintf(szPath, sizeof(szPath), "%s/%s", g_stServerInfo.stDapComnInfo.szDapHome,"update/cnf/update_server.config");
    snprintf(g_stServerInfo.stDapComnInfo.szUpdateConfigPath, sizeof(g_stServerInfo.stDapComnInfo.szUpdateConfigPath), "%s", szPath);



    printf("Succeed in init, pid(%d) |%s\n", getpid(),__func__);

    return TRUE;

}

int fpcif_SetFlagInit(cpFlag* pCpFlagInfo)
{
    // WIN_DRV(36)
    // 윈도우 드라이버 수집여부.
    pCpFlagInfo->win_drv_rule = (-1);
    pCpFlagInfo->win_drv_rusq = 0;

    pCpFlagInfo->agent_self_protect = (-1);

    return 0;
}

int fpcif_SetRuleInit(cpRule* pCpRuleInfo)
{

    // NET_ADAPTER(5)
    pCpRuleInfo->na_rule = '0';
    pCpRuleInfo->na_rusq = 0;
    // WIFI(6)
    pCpRuleInfo->wf_rule = '0';
    pCpRuleInfo->wf_rusq = 0;
    // BLUETOOTH(7)
    pCpRuleInfo->bt_rule = '0';
    pCpRuleInfo->bt_rusq = 0;
    // NET_CONNECTION(8)
    pCpRuleInfo->nc_rule = '0';
    pCpRuleInfo->nc_rusq = 0;
    // DISK(9)
    pCpRuleInfo->dk_rule = '0';
    pCpRuleInfo->dk_rusq = 0;
    // NET_DRIVE(10)
    pCpRuleInfo->nd_rule = '0';
    pCpRuleInfo->nd_rusq = 0;
    // SHARE_FOLDER(12)
    pCpRuleInfo->sf_rule = '0';
    pCpRuleInfo->sf_rusq = 0;
    // INFRARED_DEVICE(13)
    pCpRuleInfo->id_rule = '0';
    pCpRuleInfo->id_rusq = 0;
    // ROUTER(15)
    pCpRuleInfo->rt_rule = '0';
    pCpRuleInfo->rt_rusq = 0;
    // NET_PRINTER(16)
    pCpRuleInfo->np_rule = '0';
    pCpRuleInfo->np_rusq = 0;
    // VIRTUAL_MACHINE(19)
    pCpRuleInfo->st_rule_vm = '0';
    pCpRuleInfo->st_rusq_vm = 0;
    // CONECT_EXT_SVR(20)
    pCpRuleInfo->ce_rule = '0';
    pCpRuleInfo->ce_rusq = 0;
    // NET_ADAPTER_OVER(21)
    pCpRuleInfo->na_over_rule = '0';
    pCpRuleInfo->na_over_rusq = 0;
    // NET_ADAPTER_DUPIP(22)
    pCpRuleInfo->na_dupip_rule = '0';
    pCpRuleInfo->na_dupip_rusq = 0;
    // NET_ADAPTER_DUPMAC(23)
    pCpRuleInfo->na_dupmac_rule = '0';
    pCpRuleInfo->na_dupmac_rusq = 0;
    // DISK_REG(24)
    pCpRuleInfo->dk_reg_rule = '0';
    pCpRuleInfo->dk_reg_rusq = 0;
    // DISK_HIDDEN(25)
    pCpRuleInfo->dk_hidden_rule = '0';
    pCpRuleInfo->dk_hidden_rusq = 0;
    // DISK_NEW(26)
    pCpRuleInfo->dk_new_rule = '0';
    pCpRuleInfo->dk_new_rusq = 0;
    // DISK_MOBILE(27)
    pCpRuleInfo->dk_mobile_rule = '0';
    pCpRuleInfo->dk_mobile_rusq = 0;
    // DISK_MOBILE_READ(28)
    pCpRuleInfo->dk_mobile_read_rule = '0';
    pCpRuleInfo->dk_mobile_read_rusq = 0;
    // DISK_MOBILE_WRITE(29)
    pCpRuleInfo->dk_mobile_write_rule = '0';
    pCpRuleInfo->dk_mobile_write_rusq = 0;
    // PROCESS_WHITE(30)
    pCpRuleInfo->ps_white_rule = '0';
    pCpRuleInfo->ps_white_rusq = 0;
    // PROCESS_BLACK(31)
    pCpRuleInfo->ps_black_rule = '0';
    pCpRuleInfo->ps_black_rusq = 0;
    // PROCESS_ACCESSMON(32)
    pCpRuleInfo->ps_accessmon_rule = '0';
    pCpRuleInfo->ps_accessmon_rusq = 0;
    pCpRuleInfo->ps_accessmon_exp = '0';
    // PROCESS_BLOCK(37)
    pCpRuleInfo->ps_block_rule = '0';
    pCpRuleInfo->ps_block_rusq = 0;
    // NET_ADAPTER_MULIP(33)
    pCpRuleInfo->na_mulip_rule = '0';
    pCpRuleInfo->na_mulip_rusq = 0;
    // PROCESS_DETAIL_INFO for only request_cfg
    pCpRuleInfo->ps_detailinfo_rule = '0';
    pCpRuleInfo->ps_detailinfo_rusq = 0;
    // PROCESS_FORCEKILL
    pCpRuleInfo->ps_forcekill_rule = '0';
    pCpRuleInfo->ps_forcekill_rusq = 0;
    // NET_ADAPTER_EXCEPT(nouse)
    //pCpRuleInfo->na_except_rule = '0';
    // SSO_CERT(35)
    pCpRuleInfo->sc_rule = '0';
    pCpRuleInfo->sc_rusq = 0;
//    // WIN_DRV(36)
//    pCpRuleInfo->win_drv_rule = '0';
//    pCpRuleInfo->win_drv_rusq = 0;
    // PROCESS_BLOCK(37 �̺�Ʈ���߻�)
    // RDP_SESSION(38)
    pCpRuleInfo->rdp_session_rule = '0';
    pCpRuleInfo->rdp_session_rusq = 0;
    // RDP_BLOCK_COPY(39)
    pCpRuleInfo->rdp_block_copy_rule = '0';
    pCpRuleInfo->rdp_block_copy_rusq = 0;
    // CPU ALARM, USAGE(40)
    pCpRuleInfo->cpu_alarm_rule = '0';
    pCpRuleInfo->cpu_ctrl_rule = '0';
    pCpRuleInfo->cpu_alarm_rusq = 0;
    pCpRuleInfo->cpu_ctrl_rusq = 0;
    // CPU USAGE ALARM(41) / CONTRON(42)
    pCpRuleInfo->cpu_alarm_rate = 0;
    pCpRuleInfo->cpu_alarm_sustained_time = 0;
    pCpRuleInfo->cpu_ctrl_rate = 0;
    pCpRuleInfo->cpu_ctrl_sustained_time = 0;
    pCpRuleInfo->cpu_ctrl_limit_rate = 0;
    memset(pCpRuleInfo->agent_str_cpu_limit, 0x00, sizeof(pCpRuleInfo->agent_str_cpu_limit));

    return 0;
}

void fpcif_SetCycleInit(cpCycle* pCpCycleInfo)
{
    pCpCycleInfo->agent_process         = -1;
    pCpCycleInfo->agent_process_access  = -1;
    pCpCycleInfo->agent_net_printer     = -1;
    pCpCycleInfo->agent_net_scan        = -1;
    pCpCycleInfo->agent_router          = -1;
    pCpCycleInfo->agent_ext_access      = -1;
    pCpCycleInfo->agent_sso_check_cycle = -1;
    pCpCycleInfo->agent_sso_keep_time   = -1;
}

int fpcif_ScheduleInit(dbSchd *pSchdInfo, int start, int end)
{
    register int nLoop = 0;

    for(nLoop = start +1; nLoop < end; nLoop++)
    {
        memset(pSchdInfo->rs_name[nLoop]        ,0x00, sizeof(pSchdInfo->rs_name[nLoop]));
        memset(pSchdInfo->rs_weekofday[nLoop]   ,0x00, sizeof(pSchdInfo->rs_weekofday[nLoop]));
        memset(pSchdInfo->rs_dayofweek[nLoop]   ,0x00, sizeof(pSchdInfo->rs_dayofweek[nLoop]));
        memset(pSchdInfo->rs_day[nLoop]         ,0x00, sizeof(pSchdInfo->rs_day[nLoop]));
        memset(pSchdInfo->rs_date[nLoop]        ,0x00, sizeof(pSchdInfo->rs_date[nLoop]));
        memset(pSchdInfo->rs_time[nLoop]        ,0x00, sizeof(pSchdInfo->rs_time[nLoop]));

        pSchdInfo->ru_sq[nLoop]         = 0;
        pSchdInfo->rs_type[nLoop]       = '0';
        pSchdInfo->rs_exception[nLoop]  = '0';
    }

    return end;
}

int fpcif_UpgradeInit(dbUgd *pUgdInfo, int start, int end)
{
    register int nLoop;

    for(nLoop = start +1; nLoop < end; nLoop++)
    {
        memset(pUgdInfo->ua_target_value[nLoop] , 0x00, sizeof(pUgdInfo->ua_target_value[nLoop]));
        memset(pUgdInfo->ua_name[nLoop]         , 0x00, sizeof(pUgdInfo->ua_name[nLoop]));
        memset(pUgdInfo->ua_date[nLoop]         , 0x00, sizeof(pUgdInfo->ua_date[nLoop]));
        memset(pUgdInfo->ua_time[nLoop]         , 0x00, sizeof(pUgdInfo->ua_time[nLoop]));

        pUgdInfo->ua_sq[nLoop]          = 0;
        pUgdInfo->ua_target_type[nLoop] = '0';
    }

    return end;
}


int fpcif_RuleInit(dbRule *pRuleInfo, int start, int end)
{
    register int nLoop = 0;

    for(nLoop = start +1; nLoop < end; nLoop++)
    {
        memset(pRuleInfo->ru_target_value[nLoop], 0x00, sizeof(pRuleInfo->ru_target_value[nLoop]));

        pRuleInfo->ru_sq[nLoop]                         = 0;
        pRuleInfo->ru_modify_cnt[nLoop]                 = 0;
        pRuleInfo->ru_order[nLoop]                      = 0;
        pRuleInfo->mn_sq[nLoop]                         = 0;
        pRuleInfo->ru_target_type[nLoop]                = '0';

        pRuleInfo->ru_agent_cycle_process[nLoop]        = -1;
        pRuleInfo->ru_agent_cycle_process_access[nLoop] = -1;
        pRuleInfo->ru_agent_cycle_net_printer[nLoop]    = -1;
        pRuleInfo->ru_agent_cycle_net_scan[nLoop]       = -1;
        pRuleInfo->ru_agent_cycle_router[nLoop]         = -1;
        pRuleInfo->ru_agent_cycle_ext_access[nLoop]     = -1;
        pRuleInfo->ru_agent_sso_check_cycle[nLoop]      = -1;
        pRuleInfo->ru_agent_sso_keep_time[nLoop]        = -1;
        pRuleInfo->ru_win_drv[nLoop]                    = -1; // Windows 드라이버 수집여부(Flag)
        pRuleInfo->ru_agent_self_protect[nLoop]         = -1; // Agent 자기보호 기능여부(Flag)
    }

    return end;
}
