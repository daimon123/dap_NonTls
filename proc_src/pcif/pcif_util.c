//
// Created by KimByoungGook on 2020-07-15.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "sock/dap_sock.h"
#include "pcif.h"


void fpcif_ReloadCfgFile(void)
{
    g_stProcPcifInfo.cfPrmonInterval        = fcom_GetProfileInt("PRMON","INTERVAL", 600);
    g_stProcPcifInfo.cfgCheckNotiInterval   = fcom_GetProfileInt("PCIF","CHECK_NOTI_INTERVAL",30);
    g_stProcPcifInfo.cfgCheckEventInterval  = fcom_GetProfileInt("PCIF","CHECK_EVENT_INTERVAL",10);
    g_stProcPcifInfo.nAcceptNonBlockFlag    = fcom_GetProfileInt("PCIF","ACCEPT_NON_BLOCK",0);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n CHECK_NOTI_INTERVAL : [%d] \n CHECK_EVENT_INTERVAL : [%d] \n "
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcPcifInfo.cfPrmonInterval,
              g_stProcPcifInfo.cfgCheckNotiInterval,
              g_stProcPcifInfo.cfgCheckEventInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}

int fpcif_ChkFindIp(char *dName, char *findIp)
{
    int 	tokenCnt = 0;
    char	*tokenIp = NULL;
    char	*tmpFindIp = NULL;
    char    *TempPtr   = NULL;


    tokenCnt = fcom_TokenCnt(findIp, ",");
    if( tokenCnt > 0 )
    {
        if(fcom_malloc((void**)&tmpFindIp, sizeof(char)*(strlen(findIp)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        strcpy(tmpFindIp, findIp);
        tokenIp = strtok_r(tmpFindIp, ",",&TempPtr);
        while(tokenIp != NULL)
        {
            if (!strcmp(dName, tokenIp))
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Found ip(%s), file(%s) ",
                               tokenIp,dName);
                fcom_MallocFree((void**)&tmpFindIp);
                tokenIp = NULL;
                return 1;
            }
            tokenIp = strtok_r(NULL, ",",&TempPtr);
        }
        fcom_MallocFree((void**)&tmpFindIp);
    }
    else
    {
        if (!strcmp(dName, findIp))
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Found ip(%s), file(%s) ",
                           findIp,dName);
            return 1;
        }
    }

    return 0;
}

int fpcif_ChkFindStr(char *dName, char *findStr)
{
    int 	tokenCnt = 0;
    char	command[128 +1] = {0x00,};
    char	buf[128 +1] = {0x00,};
    char	*tokenStr = NULL;
    char	*tmpFindStr = NULL;
    char    *TempPtr    = NULL;
    FILE	*pfp = NULL;


    tokenCnt = fcom_TokenCnt(findStr, ",");
    if( tokenCnt > 0 )
    {
        if(fcom_malloc((void**)&tmpFindStr, sizeof(char)*(strlen(findStr)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        strcpy(tmpFindStr, findStr);
        tokenStr = strtok_r(tmpFindStr, ",",&TempPtr);
        while(tokenStr != NULL)
        {
            memset	(command, 0x00, sizeof(command));
            sprintf	(command, "grep %s %s", tokenStr, dName);
            pfp = popen(command, "r");
            if( pfp == NULL )
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Failed to popen, command(%s) ",
                               command);
            }
            else
            {
                if(fgets(buf, 128, pfp) != NULL)
                {
                    WRITE_INFO(CATEGORY_DEBUG,"Found pattern(%s), file(%s) ",
                               tokenStr,dName);
                    fcom_MallocFree((void**)&tmpFindStr);
                    pclose(pfp);
                    tokenStr = NULL;
                    return 1;
                }
            }
            tokenStr = strtok_r(NULL, ",",&TempPtr);
        }
        fcom_MallocFree((void**)&tmpFindStr);
    }
    else
    {
        memset	(command, 0x00, sizeof(command));
        sprintf	(command, "grep %s %s", findStr, dName);
        pfp = popen(command, "r");
        if( pfp == NULL )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Failed to popen, command(%s) ",
                           command);
            return -1;
        }
        else
        {
            if(fgets(buf, 128, pfp) != NULL)
            {
                WRITE_INFO(CATEGORY_DEBUG,"Found grep pattern(%s), file(%s) ",
                           findStr,dName);
                pclose(pfp);
                return 1;
            }
        }
    }

    pclose(pfp);

    return 0;
}
int fpcif_ChkUpgradeFlag(void)
{
    int local_count = 0;

    while(1)
    {
        if(UpgradestFlag == 0x00 || UpgradestFlag == 0x01 ) { break; }
        fcom_SleepWait(5); //0.1sec
        local_count++;
        if( local_count > 300 ) { return -1; } //30��
    }

    return 0;
}

int fpcif_ChkLoadFlag(void)
{
    int local_count = 0;

    while(1)
    {
        if(LoadConfigFlag == 0x01 ) { break; }
        fcom_SleepWait(5); //0.1 sec
        local_count++;
        if( local_count > 30 ) { return -1; } //3초
    }

    return 0;

}


int fpcif_CheckPid(char *logip, int pid, char *proc)
{
    int	 line = 0;
    char command[128] = {0x00,};
    char buff[1024] = {0x00,};
    FILE *fp = NULL;

    memset(command, 0x00, sizeof(command));
    sprintf(command, "ps -ef | grep %d | grep %s | grep -v grep", pid,proc);

    fp = popen(command, "r");
    if(fp == NULL)
    {
        WRITE_CRITICAL_IP(logip, "Fail in (%s)",  command);
        return -1;
    }

    while(fgets(buff, 1024, fp) != NULL)
    {
        line++;
    }

    pclose(fp);

    return line;
}


int fpcif_CheckCommandZip(char* logip)
{
    char buffer[256+1];
    FILE* fp = NULL;

    fp = popen("which zip","r");

    if(fp == NULL)
    {
        WRITE_CRITICAL_IP(logip, "Zip fp is NULL " );
        return (-1);
    }
    fgets(buffer, 256, fp);

    if(strlen(buffer) <= 0)
    {
        WRITE_CRITICAL_IP(logip, "Zip Comand Is Not Found " );
        pclose(fp);
        return (-1);
    }

    pclose(fp);

    return 0;
}

int fpcif_CheckUrgentFile(char* AgentIp)
{
    int rxt = 0;
    char DirPath[255 +1] = {0x00,};
    char FilePath[255 +1] = {0x00,};
    char CheckIpTok[4][3 +1];
    char CopyAgentIP[31 +1] = {0x00,};

    char *Temp = NULL;
    char *TempPtr   = NULL;
    int Idx = 0, i = 0;

    memset(CopyAgentIP, 0x00, sizeof(CopyAgentIP));
    sprintf(CopyAgentIP,"%s",AgentIp);

    sprintf(DirPath, "%s/%s",getenv("DAP_HOME"),"urgent/update/");
    /* 디렉토리 존재 검사 및 없을시 생성 */
    if (access(DirPath, R_OK) != 0 )
    {
        rxt = fcom_MkPath(DirPath, 0755);
        if (rxt < 0)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Fail in make path(%s)", DirPath);
            return -1;
        }
        else
        {
            chmod(DirPath, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
            WRITE_INFO(CATEGORY_DEBUG,"Succeed in make path(%s)", DirPath);
        }
    }

    /* Agent Ip Parsing */
    Temp = (char *) strtok_r(CopyAgentIP, ".",&TempPtr);
    sprintf(CheckIpTok[Idx], "%s", Temp);
    Idx++;
    while (Temp != NULL)
    {
        Temp = strtok_r(NULL, ".",&TempPtr);
        if (Temp != NULL)
        {
            sprintf(CheckIpTok[Idx], "%s", Temp);
            Idx++;
        }
    }
    for(i = 0; i < 4; i++)
    {
        if(i == 0)    sprintf(FilePath,"%s/%s.%s.%s.%s", DirPath,CheckIpTok[0],CheckIpTok[1],CheckIpTok[2],CheckIpTok[3] );
        if(i == 1)    sprintf(FilePath,"%s/%s.%s.%s.0", DirPath,CheckIpTok[0],CheckIpTok[1],CheckIpTok[2]);
        if(i == 2)    sprintf(FilePath,"%s/%s.%s.0.0", DirPath,CheckIpTok[0],CheckIpTok[1]);
        if(i == 3)    sprintf(FilePath,"%s/%s.0.0.0", DirPath,CheckIpTok[0]);
        if(fcom_fileCheckStatus( FilePath) == 0)
        {
            WRITE_INFO_IP(AgentIp,"Urgent File Check Is Success (%s) ",AgentIp );
            return 0;
        }
    }
    WRITE_INFO_IP(AgentIp,"Urgent File Check Is Not Exist (%s) ",AgentIp );

    return (-1);

}
int fpcif_CheckEventNotiFile(void)
{
    register int  i;
    struct stat     statBuf;

    for(i = 0; i < MAX_EVENT_NOTI; i++)
    {
        if(stat(g_pstNotiEvent[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in stat noti files(%s)(%d) ",
                           g_pstNotiEvent[i].szNotiFileName,
                           i);
            return  -1;
        }

        if(statBuf.st_mtime > g_pstNotiEvent[i].lastModify)
        {
            g_pstNotiEvent[i].reload = TRUE;
        }
        else
        {
            g_pstNotiEvent[i].reload = FALSE;
        }
    }

    return TRUE;
}
int fpcif_CheckNotiFile(void)
{
    register int i = 0;
    struct stat     statBuf;

    for(i = 0; i < MAX_PCIF_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in stat noti files(%s)(%d)(%s)",
                           g_pstNotiMaster[i].szNotiFileName,
                           i,
                           strerror(errno));
            return  -1;
        }

        if(statBuf.st_mtime > g_pstNotiMaster[i].lastModify)
        {
            g_pstNotiMaster[i]. reload = TRUE;
        }
        else
        {
            g_pstNotiMaster[i].reload = FALSE;
        }
    }

    return TRUE;
}


/*
 * 0: 지정일		ex) [RS_DATE]20190328[RS_TIME]000000~20190401[RS_TIME]235959
 * 1: 매일			ex) [RS_TIME]000000~235959
 * 2: 매주			ex) [RS_DAYOFWEEK]SUN,MON,TUE.. + [RS_TIME]000000~235959
 * 3: 매월(일자)	ex) [RS_DAY]01,02,03...31 + [RS_TIME]000000~235959
 * 4: 매월(주간)	ex) [RS_WEEKOFDAY]1W,2W...6W + [RS_TIME]000000~235959
 */
int fpcif_ChkAllowSchedule(char *logip, unsigned long long p_sq)
{
    int		rxt;
    int		idx;
    int		type;
    int		exp;

    idx = fpcif_GetIdxByScheduleSt(p_sq);
    if( idx < 0 ) // 등록된 스케쥴이 없으면
    {
        return 1;
    }

    type = (pSchdInfo->rs_type[idx]-'0')%48;
    exp = (pSchdInfo->rs_exception[idx]-'0')%48;

    if( type == 0 || type == 1 ) // 지정일 or 매일
    {
        rxt = fpcif_ValidDate(logip, pSchdInfo->rs_date[idx], pSchdInfo->rs_time[idx], type, exp);
        if( rxt > 0 )
            return 1;
    }
    else if( type == 2 ) // 매주
    {
        rxt = fpcif_ValidDayofWeek(logip, pSchdInfo->rs_dayofweek[idx], pSchdInfo->rs_time[idx], exp);
        if( rxt > 0 )
            return 1;
    }
    else if( type == 3 ) // 매월(일자)
    {
        rxt = fpcif_ValidDay(logip, pSchdInfo->rs_day[idx], pSchdInfo->rs_time[idx], exp);
        if( rxt > 0 )
            return 1;
    }
    else if( type == 4 ) // 매월(요일)
    {
        rxt = fpcif_ValidWeekofDay(logip, pSchdInfo->rs_weekofday[idx], pSchdInfo->rs_time[idx], exp);
        if( rxt > 0 )
            return 1;
    }
    else
    {
        WRITE_INFO_IP(logip, "Unknown type(%d)", type);
        return -1;
    }

    return 0;
}

int fpcif_ChkAllowRule(char *logip, DETECT_ITEM DetectItem, _DAP_DETECT_DATA* pDetectData, cpRule *pCpRuleInfo)
{
    switch(DetectItem)
    {
        case MAIN_BOARD:
            break;
        case SYSTEM:
        WRITE_INFO_IP(logip, "SYSTEM: st_rule_vm(%c)",
                      pCpRuleInfo->st_rule_vm);
            if(	(pCpRuleInfo->st_rule_vm-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->System.st_rule_vm = pCpRuleInfo->st_rule_vm;
                pDetectData->System.st_rusq_vm = pCpRuleInfo->st_rusq_vm;
                WRITE_INFO_IP(logip, "      : st_rusq_vm(%llu)",
                              pDetectData->System.st_rusq_vm);
            }
            break;
        case CONNECT_EXT_SVR:
        WRITE_INFO_IP(logip, "CONNECT_EXT_SVR: ce_rule(%c)",
                      pCpRuleInfo->ce_rule);
            if(	(pCpRuleInfo->ce_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->ConnectExt.ce_rule = pCpRuleInfo->ce_rule;
                pDetectData->ConnectExt.ce_rusq = pCpRuleInfo->ce_rusq;
                WRITE_INFO_IP(logip, "               : ce_rusq(%llu)",
                              pDetectData->ConnectExt.ce_rusq);
            }
        case OPERATING_SYSTEM:
            break;
        case CPU:
            break;
        case NET_ADAPTER:
        WRITE_INFO_IP(logip, "NET_ADAPTER: na_rule(%c)"
                             "na_over_rule(%c)"
                             "na_mulip_rule(%c)"
                             "na_dupip_rule(%c)"
                             "na_dupmac_rule(%c)",

                      pCpRuleInfo->na_rule,
                      pCpRuleInfo->na_over_rule,
                      pCpRuleInfo->na_mulip_rule,
                      pCpRuleInfo->na_dupip_rule,
                      pCpRuleInfo->na_dupmac_rule);
            if(	(pCpRuleInfo->na_over_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->na_mulip_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->na_dupip_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->na_dupmac_rule-'0')%48 < INFO ) {
                return 0;
            }
            else
            {
                pDetectData->NetAdapter.na_rule = pCpRuleInfo->na_rule;
                pDetectData->NetAdapter.na_rusq = pCpRuleInfo->na_rusq;
                pDetectData->NetAdapter.na_over_rule = pCpRuleInfo->na_over_rule;
                pDetectData->NetAdapter.na_over_rusq = pCpRuleInfo->na_over_rusq;
                pDetectData->NetAdapter.na_mulip_rule = pCpRuleInfo->na_mulip_rule;
                pDetectData->NetAdapter.na_mulip_rusq = pCpRuleInfo->na_mulip_rusq;
                pDetectData->NetAdapter.na_dupip_rule = pCpRuleInfo->na_dupip_rule;
                pDetectData->NetAdapter.na_dupip_rusq = pCpRuleInfo->na_dupip_rusq;
                pDetectData->NetAdapter.na_dupmac_rule = pCpRuleInfo->na_dupmac_rule;
                pDetectData->NetAdapter.na_dupmac_rusq = pCpRuleInfo->na_dupmac_rusq;
                //pDetectData->NetAdapter.na_except_rule = pCpRuleInfo->na_except_rule;
                WRITE_INFO_IP(logip, "na_rusq(%llu)"
                                     "na_over_rusq(%llu)"
                                     "na_mulip_rusq(%llu)"
                                     "na_dupip_rusq(%llu)"
                                     "na_dupmac_rusq(%llu)",

                              pDetectData->NetAdapter.na_rusq,
                              pDetectData->NetAdapter.na_over_rusq,
                              pDetectData->NetAdapter.na_mulip_rusq,
                              pDetectData->NetAdapter.na_dupip_rusq,
                              pDetectData->NetAdapter.na_dupmac_rusq);
            }
            break;
        case WIFI:
        WRITE_INFO_IP(logip, "WIFI: wf_rule(%c)", pCpRuleInfo->wf_rule);
            if(	(pCpRuleInfo->wf_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->Wifi.wf_rule = pCpRuleInfo->wf_rule;
                pDetectData->Wifi.wf_rusq = pCpRuleInfo->wf_rusq;
                WRITE_INFO_IP(logip, "    : wf_rusq(%llu)", pDetectData->Wifi.wf_rusq);
            }
            break;
        case BLUETOOTH:
        WRITE_INFO_IP(logip, "BLUETOOTH: bt_rule(%c)",
                      pCpRuleInfo->bt_rule);
            if(	(pCpRuleInfo->bt_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->Bluetooth.bt_rule = pCpRuleInfo->bt_rule;
                pDetectData->Bluetooth.bt_rusq = pCpRuleInfo->bt_rusq;
                WRITE_INFO_IP(logip, "         : bt_rusq(%llu)",
                              pDetectData->Bluetooth.bt_rusq);
            }
            break;
        case NET_CONNECTION:
        WRITE_INFO_IP(logip, "NET_CONNECTION: nc_rule(%c)",
                      pCpRuleInfo->nc_rule);
            if(	(pCpRuleInfo->nc_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->NetConnection.nc_rule = pCpRuleInfo->nc_rule;
                pDetectData->NetConnection.nc_rusq = pCpRuleInfo->nc_rusq;
                WRITE_INFO_IP(logip, "              : nc_rusq(%llu)",
                              pDetectData->NetConnection.nc_rusq);
            }
            break;
        case DISK:
        WRITE_INFO_IP(logip, "DISK: dk_rule(%c)"
                             "dk_reg_rule(%c)"
                             "dk_hidden_rule(%c)"
                             "dk_new_rule(%c)"
                             "dk_mobile_rule(%c)"
                             "dk_mobile_read_rule(%c)"
                             "dk_mobile_write_rule(%c)",
                              pCpRuleInfo->dk_rule,
                              pCpRuleInfo->dk_reg_rule,
                              pCpRuleInfo->dk_hidden_rule,
                              pCpRuleInfo->dk_new_rule,
                              pCpRuleInfo->dk_mobile_rule,
                              pCpRuleInfo->dk_mobile_read_rule,
                              pCpRuleInfo->dk_mobile_write_rule);
            if(	(pCpRuleInfo->dk_reg_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->dk_hidden_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->dk_new_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->dk_mobile_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->dk_mobile_read_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->dk_mobile_write_rule-'0')%48 < INFO )
            {
                return 0;
            }
            else
            {
                pDetectData->Disk.dk_rule = pCpRuleInfo->dk_rule;
                pDetectData->Disk.dk_rusq = pCpRuleInfo->dk_rusq;
                pDetectData->Disk.dk_reg_rule = pCpRuleInfo->dk_reg_rule;
                pDetectData->Disk.dk_reg_rusq = pCpRuleInfo->dk_reg_rusq;
                pDetectData->Disk.dk_hidden_rule = pCpRuleInfo->dk_hidden_rule;
                pDetectData->Disk.dk_hidden_rusq = pCpRuleInfo->dk_hidden_rusq;
                pDetectData->Disk.dk_new_rule = pCpRuleInfo->dk_new_rule;
                pDetectData->Disk.dk_new_rusq = pCpRuleInfo->dk_new_rusq;
                pDetectData->Disk.dk_mobile_rule = pCpRuleInfo->dk_mobile_rule;
                pDetectData->Disk.dk_mobile_rusq = pCpRuleInfo->dk_mobile_rusq;
                pDetectData->Disk.dk_mobile_read_rule = pCpRuleInfo->dk_mobile_read_rule;
                pDetectData->Disk.dk_mobile_read_rusq = pCpRuleInfo->dk_mobile_read_rusq;
                pDetectData->Disk.dk_mobile_write_rule = pCpRuleInfo->dk_mobile_write_rule;
                pDetectData->Disk.dk_mobile_write_rusq = pCpRuleInfo->dk_mobile_write_rusq;
                WRITE_INFO_IP(logip, "    : dk_rusq(%llu)"
                                     "dk_reg_rusq(%llu)"
                                     "dk_hidden_rusq(%llu)"
                                     "dk_new_rusq(%llu)"
                                     "dk_mobile_rusq(%llu)"
                                     "dk_mobile_read_rusq(%llu)"
                                     "dk_mobile_write_rusq(%llu)",
                              pDetectData->Disk.dk_rusq,
                              pDetectData->Disk.dk_reg_rusq,
                              pDetectData->Disk.dk_hidden_rusq,
                              pDetectData->Disk.dk_new_rusq,
                              pDetectData->Disk.dk_mobile_rusq,
                              pDetectData->Disk.dk_mobile_read_rusq,
                              pDetectData->Disk.dk_mobile_write_rusq);
            }
            break;
        case NET_DRIVE:
        WRITE_INFO_IP(logip, "NET_DRIVE: nd_rule(%c)",
                      pCpRuleInfo->nd_rule);
            if(	(pCpRuleInfo->nd_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->NetDrive.nd_rule = pCpRuleInfo->nd_rule;
                pDetectData->NetDrive.nd_rusq = pCpRuleInfo->nd_rusq;
                WRITE_INFO_IP(logip, "         : nd_rusq(%llu)",
                              pDetectData->NetDrive.nd_rusq);
            }
            break;
        case OS_ACCOUNT:
            break;
        case SHARE_FOLDER:
        WRITE_INFO_IP(logip, "SHARE_FOLDER: sf_rule(%c)",
                      pCpRuleInfo->sf_rule);
            if(	(pCpRuleInfo->sf_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->ShareFolder.sf_rule = pCpRuleInfo->sf_rule;
                pDetectData->ShareFolder.sf_rusq = pCpRuleInfo->sf_rusq;
                WRITE_INFO_IP(logip, "            : sf_rusq(%llu)",
                              pDetectData->ShareFolder.sf_rusq);
            }
            break;
        case INFRARED_DEVICE:
        WRITE_INFO_IP(logip, "INFRARED_DEVICE: id_rule(%c)",
                      pCpRuleInfo->id_rule);
            if(	(pCpRuleInfo->id_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->InfraredDevice.id_rule = pCpRuleInfo->id_rule;
                pDetectData->InfraredDevice.id_rusq = pCpRuleInfo->id_rusq;
                WRITE_INFO_IP(logip, "               : id_rusq(%llu)",
                              pDetectData->InfraredDevice.id_rusq);
            }
            break;
        case PROCESS:
        WRITE_INFO_IP(logip, "PROCESS: ps_black_rule(%c)"
                             "ps_white_rule(%c)"
                             "ps_accessmon_rule(%c/%c)",

                      pCpRuleInfo->ps_black_rule,
                      pCpRuleInfo->ps_white_rule,
                      pCpRuleInfo->ps_accessmon_rule,
                      pCpRuleInfo->ps_accessmon_exp);
            if(	(pCpRuleInfo->ps_black_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->ps_white_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->ps_accessmon_rule-'0')%48 < INFO )
            {
                return 0;
            }
            else
            {
                pDetectData->Process.ps_white_rule = pCpRuleInfo->ps_white_rule;
                pDetectData->Process.ps_white_rusq = pCpRuleInfo->ps_white_rusq;
                pDetectData->Process.ps_black_rule = pCpRuleInfo->ps_black_rule;
                pDetectData->Process.ps_black_rusq = pCpRuleInfo->ps_black_rusq;
                pDetectData->Process.ps_accessmon_rule = pCpRuleInfo->ps_accessmon_rule;
                pDetectData->Process.ps_accessmon_rusq = pCpRuleInfo->ps_accessmon_rusq;
                WRITE_INFO_IP(logip, "       : ps_black_rusq(%llu)"
                                     "ps_white_rusq(%llu)"
                                     "ps_accessmon_rusq(%llu)",

                              pDetectData->Process.ps_black_rusq,
                              pDetectData->Process.ps_white_rusq,
                              pDetectData->Process.ps_accessmon_rusq);
            }
            break;
        case ROUTER:
        WRITE_INFO_IP(logip, "ROUTER: rt_rule(%c)",
                      pCpRuleInfo->rt_rule);
            if(	(pCpRuleInfo->rt_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->Router.rt_rule = pCpRuleInfo->rt_rule;
                pDetectData->Router.rt_rusq = pCpRuleInfo->rt_rusq;
                WRITE_INFO_IP(logip, "      : rt_rusq(%llu)",
                              pDetectData->Router.rt_rusq);
            }
            break;
        case NET_PRINTER:
        WRITE_INFO_IP(logip, "NET_PRINTER: np_rule(%c)",
                      pCpRuleInfo->np_rule);
            if(	(pCpRuleInfo->np_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->NetPrinter.np_rule = pCpRuleInfo->np_rule;
                pDetectData->NetPrinter.np_rusq = pCpRuleInfo->np_rusq;
                WRITE_INFO_IP(logip, "           : np_rusq(%llu)",
                              pDetectData->NetPrinter.np_rusq);
            }
            break;
        case NET_SCAN:
            break;
        case ARP:
            break;
        case SSO_CERT:
        WRITE_INFO_IP(logip, "SSO_CERT: sc_rule(%c)",
                      pCpRuleInfo->sc_rule);
            if(	(pCpRuleInfo->sc_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->SsoCert.sc_rule = pCpRuleInfo->sc_rule;
                pDetectData->SsoCert.sc_rusq = pCpRuleInfo->sc_rusq;
                WRITE_INFO_IP(logip, "        : sc_rusq(%llu)",
                              pDetectData->SsoCert.sc_rusq);
            }
            break;
        case WIN_DRV: //단순수집용으로 rule이없음
            /*
            WRITE_INFO_IP(logip, "WIN_DRV: win_drv_rule(%c)",
                        pCpRuleInfo->win_drv_rule);
            if(	(pCpRuleInfo->win_drv_rule-'0')%48 < INFO ) return 0;
            else {
                pDetectData->SsoCert.win_drv_rule = pCpRuleInfo->win_drv_rule;
                pDetectData->SsoCert.win_drv_rusq = pCpRuleInfo->win_drv_rusq;
            }
            */
            break;
        case RDP_SESSION:
        WRITE_INFO_IP(logip, "RDP_SESSION: rdp_session_rule(%c)",
                      pCpRuleInfo->rdp_session_rule);
            if(	(pCpRuleInfo->rdp_session_rule-'0')%48 < INFO )
                return 0;
            else
            {
                pDetectData->RdpSession.rdp_session_rule = pCpRuleInfo->rdp_session_rule;
                pDetectData->RdpSession.rdp_session_rusq = pCpRuleInfo->rdp_session_rusq;
                WRITE_INFO_IP(logip, "        : rdp_session_rusq(%llu)",
                              pDetectData->RdpSession.rdp_session_rusq);
            }
            break;
        case CPU_USAGE:
        WRITE_INFO_IP(logip, "CPU_USAGE: cpu_alarm_rule(%c) cpu_ctrl_rule(%c) ",
                      pCpRuleInfo->cpu_alarm_rule,
                      pCpRuleInfo->cpu_ctrl_rule);
            if(	(pCpRuleInfo->cpu_alarm_rule-'0')%48 < INFO &&
                   (pCpRuleInfo->cpu_ctrl_rule-'0')%48 < INFO )
            {
                return 0;
            }
            else
            {
                pDetectData->CpuUsage.cpu_alarm_rule = pCpRuleInfo->cpu_alarm_rule;
                pDetectData->CpuUsage.cpu_alarm_rusq = pCpRuleInfo->cpu_alarm_rusq;
                pDetectData->CpuUsage.cpu_ctrl_rule  = pCpRuleInfo->cpu_ctrl_rule;
                pDetectData->CpuUsage.cpu_ctrl_rusq  = pCpRuleInfo->cpu_ctrl_rusq;

                WRITE_INFO_IP(logip,"cpu_alarm_rusq(%llu) cpu_ctrl_rusq(%llu) ",
                              pDetectData->CpuUsage.cpu_alarm_rusq,
                              pDetectData->CpuUsage.cpu_ctrl_rusq);
            }
            break;
        default:
            return 1;
    }

    return 1;
}

/** AS-IS **/
/*
int fpcif_GetUpgradeSt(char* p_realip, char *p_logip, char* p_cpVer)
{
    int 		i = 0;
    int         rxt = 0;
    int         idx = 0;
    int         idxU = 0;
    int         tokenCnt = 0;
    int			ua_target_type = 0;
    char        arrTokenUgdIp[31 +1] = {0x00,};
    char		tmpIp1[15 +1] = {0x00,};
    char		tmpIp2[31 +1] = {0x00,};
    char		currDate[14 +1] = {0x00,};
    char		ugdDate[14 +1] = {0x00,};
    char*       local_ptrUsrIp = NULL;
    time_t 		currTime = time((time_t) 0);

    idx = fpcif_GetIdxByConfigSt("AUTO_UPDATE_AGENT_VER");
    if (idx < 0)
    {
        WRITE_CRITICAL_IP(p_logip,"Not found 'AUTO_UPDATE_AGENT_VER' value");
        return 0;
    }

    if (!strlen(pConfigInfo[idx].cfvalue))
    {
        WRITE_CRITICAL_IP(p_logip,"No value 'AUTO_UPDATE_AGENT_VER'");
        return 0;
    }

    if (strverscmp(p_cpVer, pConfigInfo[idx].cfvalue) >= 0)
    {
        WRITE_DEBUG_IP(p_logip,"Passing upgrade rule, ver('%s' >= '%s')",
                       p_cpVer,pConfigInfo[idx].cfvalue);
        return 0;
    }

    WRITE_DEBUG_IP(p_logip,"Checking upgrade rule continue.. ver('%s' < '%s')",
                   p_cpVer,pConfigInfo[idx].cfvalue);

    memset	(currDate, 0x00, sizeof(currDate));
    fcom_time2str(currTime, currDate, "YYYYMMDDhhmmss");

    *//*
     * 0x00 : 초기화
     * 0x01 : 읽기가능
     * 0x02 : 접근금지
     * *//*
    if( fpcif_ChkUpgradeFlag() == 0)
    {
        for(idx=0; idx < pUgdInfo->tot_cnt; idx++) //0부터시작하기때문에
        {
            WRITE_INFO_IP(p_logip,"Check upgrade idx(%d), sq(%llu)name(%s)type(%c)",
                          idx,pUgdInfo->ua_sq[idx],pUgdInfo->ua_name[idx],pUgdInfo->ua_target_type[idx]);

            memset	(ugdDate, 0x00, sizeof(ugdDate));
            sprintf	(ugdDate, "%s%s", pUgdInfo->ua_date[idx],pUgdInfo->ua_time[idx]);
            WRITE_INFO_IP(p_logip,"Check time currDate(%s) >= ugdDate(%s) ", currDate,ugdDate );

            if (strcmp(currDate, ugdDate) < 0 || strlen(ugdDate) <= 0)
                return 0;

            ua_target_type = (pUgdInfo->ua_target_type[idx]-'0')%48;

            if (ua_target_type == 0) //All Range
            {
                WRITE_INFO_IP(p_logip, "Return all range Rule, Rule idx(%d) ", idx );
                return 1;
            }

            idxU = fpcif_GetIdxByUserIp(p_realip);
            if(idxU > -1) // If exists user_tb
            {
                WRITE_INFO_IP(p_logip,"User found, User idx(%d)",idxU);
                if (ua_target_type == 1) //IP Range
                {
                    WRITE_INFO_IP(p_logip,"Check ip range, idx(%d)", idx);
                    memset(arrTokenUgdIp, 0x00, sizeof(arrTokenUgdIp));
                    strcpy(arrTokenUgdIp, (char *)pUgdInfo->ua_target_value[idx]);

                    if(fcom_malloc((void**)&local_ptrUsrIp, sizeof(char)*(strlen(pUserInfo[idxU].us_ip)+1)) != 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                        return (-1);
                    }
                    strcpy(local_ptrUsrIp, pUserInfo[idxU].us_ip);

                    tokenCnt = fcom_TokenCnt(arrTokenUgdIp, ","); // AGENT_UPGRADE_TB IP
                    if(tokenCnt > 0) // AGENT_UPGRADE_TB IP
                    {
                        for(i=0; i<=tokenCnt; i++)
                        {
                            memset(tmpIp1, 0x00, sizeof(tmpIp1));
                            fcom_ParseStringByToken(arrTokenUgdIp, ",", tmpIp1, i); // AGENT_UPGRADE_TB IP
                            strcpy(tmpIp2, local_ptrUsrIp); // USER_TB IP
                            rxt = fsock_CompareIpRange(tmpIp2, tmpIp1); //AGENT_UPGRADE_TB IP랑 USER_TB IP 비교.
                            if(rxt > 0)
                            {
                                WRITE_INFO_IP(p_logip,"Return ip range, ugd(%s)user(%s)", tmpIp2,tmpIp1);
                                fcom_MallocFree((void**)&local_ptrUsrIp);
                                return 1;
                            }
                            else if(rxt < 0)
                            {
                                WRITE_CRITICAL_IP(p_logip, "Fail in Ip Compare, ip(%s)", tmpIp2);
                            }
                        }
                    }
                    else
                    {
                        if(strstr(arrTokenUgdIp, "/") != NULL) // CIDR
                        {
                            rxt = fsock_CompareIpRange(local_ptrUsrIp, arrTokenUgdIp);
                            if(rxt > 0)
                            {
                                WRITE_INFO_IP(p_logip,"Return ip range, ugd(%s)user(%s)",
                                              arrTokenUgdIp,local_ptrUsrIp);
                                fcom_MallocFree((void**)&local_ptrUsrIp);
                                return 1;
                            }
                            else if(rxt < 0)
                            {
                                WRITE_CRITICAL_IP(p_logip, "Fail in convert cidr, ip(%s)",
                                                  arrTokenUgdIp);
                            }
                        }
                        else // 단일비교
                        {
                            if(!strcmp(local_ptrUsrIp, arrTokenUgdIp))
                            {
                                WRITE_INFO_IP(p_logip,"Return ip range, ugd(%s)user(%s)",
                                              arrTokenUgdIp,local_ptrUsrIp);
                                fcom_MallocFree((void**)&local_ptrUsrIp);
                                return 1;
                            }
                        }
                    }
                    fcom_MallocFree((void**)&local_ptrUsrIp);
                }
                else if (ua_target_type == 2) //UG_SQ
                {
                    if(atol(pUgdInfo->ua_target_value[idx]) == pUserInfo[idxU].ug_sq)
                    {
                        WRITE_DEBUG_IP(p_logip,"Return higher-group, ugd(%llu)user(%llu)",
                                       atol(pUgdInfo->ua_target_value[idx]),pUserInfo[idxU].ug_sq);
                        return 1;
                    }
                    else  //하위그룹검색
                    {
                        rxt = fpcif_GetIdxByGroupLinkSt(	atol(pUgdInfo->ua_target_value[idx]),
                                                            pUserInfo[idxU].ug_sq );
                        if(rxt > -1)
                        {
                            WRITE_DEBUG_IP(p_logip,"Return sub-group, ugd(%llu)user(%llu)",
                                           atol(pUgdInfo->ua_target_value[idx]),pUserInfo[idxU].ug_sq);
                            return 1;
                        }
                    }
                }
                else
                {
                    WRITE_DEBUG_IP(p_logip,"Undefined type(%d) ", ua_target_type);
                }
            }
            else //유저정보가 없다면
            {
                WRITE_DEBUG_IP(p_logip,"User not found " );
                memset(arrTokenUgdIp, 0x00, sizeof(arrTokenUgdIp));
                strcpy(arrTokenUgdIp, (char *)pUgdInfo->ua_target_value[idx]); *//* UPGRADE_AGENT_TB IP *//*
                tokenCnt = fcom_TokenCnt(arrTokenUgdIp, ","); *//* AGENT_UPGRADE_TB IP *//*

                if (ua_target_type == 1) //IP Range
                {
                    for(i=0; i<=tokenCnt; i++)
                    {
                        if(tokenCnt > 0) // AGENT_UPGRADE_TB IP
                        {
                            if(strstr(arrTokenUgdIp, "/") != NULL) // CIDR
                            {
                                rxt = fsock_CompareIpRange(p_realip, arrTokenUgdIp);
                                if(rxt > 0)
                                {
                                    WRITE_DEBUG_IP(p_logip,"Return ip range, ugd(%s)real(%s)",
                                                   p_realip,p_realip);
                                    return 1;
                                }
                                else if(rxt < 0)
                                {
                                    WRITE_CRITICAL_IP(p_logip,"Fail in convert cidr, ip(%s)",
                                                      arrTokenUgdIp);
                                }
                            }
                            else // IP��
                            {
                                memset(tmpIp1, 0x00, sizeof(tmpIp1));

                                fcom_ParseStringByToken(arrTokenUgdIp, ",", tmpIp1, i); // AGENT_UPGRADE_TB IP
                                WRITE_DEBUG_IP(p_logip,"Compare Upgrade Ip : [%s:%s] ", tmpIp1,p_realip);
                                rxt = fsock_CompareIpRange(p_realip, tmpIp1); //AGENT_UPGRADE_TB IP�� Agent Real IP ���� ��.
                                if(rxt > 0)
                                {
                                    WRITE_INFO_IP(p_logip,"Return ip range, ugd(%s)user(%s)",
                                                  p_realip,tmpIp1);
                                    fcom_MallocFree((void**)&local_ptrUsrIp);

                                    return 1;
                                }
                                else if(rxt < 0)
                                {
                                    WRITE_CRITICAL_IP(p_logip, "Fail in Ip Compare, ip(%s)",
                                                      tmpIp2);
                                }
                            }
                        }
                    }
                }
            }
        }//for
    }
    else
    {
        WRITE_DEBUG_IP(p_logip, "Upgrade_st Is Not Reload .. " );
        return (-1);
    }

    return 0;
}
*/





/** To-BE **/
int fpcif_GetUpgradeSt(char* p_realip, char *p_logip, char* p_cpVer)
{
    int 		local_nLoop = 0;
    int         local_nRet = 0;
    int         local_nIdx = 0;
    int         local_nIdxU = 0;
    int         local_nUserIpTokenCnt = 0;
    int         local_nUpgradeIpTokenCnt = 0;
    int			local_nUpgradeTargetType = 0;
    char        local_szUpgradeIp[31 +1] = {0x00,};
    char        local_szTmpIp[31 +1]     = {0x00,};
    char		local_szTmpUpgradeIp[15 +1] = {0x00,};
    char		local_szTmpUserIp[15 +1] = {0x00,};
    char		local_szCurrDate[14 +1] = {0x00,};
    char		local_szUpgradeDate[14 +1] = {0x00,};
    char*       local_ptrUsrIp      = NULL;
    time_t 		currTime = time((time_t) 0);

    local_nIdx = fpcif_GetIdxByConfigSt("AUTO_UPDATE_AGENT_VER");
    if (local_nIdx < 0)
    {
        WRITE_CRITICAL_IP(p_logip,"Not found 'AUTO_UPDATE_AGENT_VER' value");
        return 0;
    }

    if (!strlen(pConfigInfo[local_nIdx].cfvalue))
    {
        WRITE_CRITICAL_IP(p_logip,"No value 'AUTO_UPDATE_AGENT_VER'");
        return 0;
    }

    if (strverscmp(p_cpVer, pConfigInfo[local_nIdx].cfvalue) >= 0)
    {
        WRITE_DEBUG_IP(p_logip,"Passing upgrade rule, ver('%s' >= '%s')", p_cpVer, pConfigInfo[local_nIdx].cfvalue);
        return 0;
    }

    WRITE_DEBUG_IP(p_logip,"Checking upgrade rule continue.. ver('%s' < '%s')", p_cpVer, pConfigInfo[local_nIdx].cfvalue);

    memset	(local_szCurrDate, 0x00, sizeof(local_szCurrDate));
    fcom_time2str(currTime, local_szCurrDate, "YYYYMMDDhhmmss");

    for(local_nIdx=0; local_nIdx < pUgdInfo->tot_cnt; local_nIdx++) //0부터시작하기때문에
    {
        WRITE_INFO_IP(p_logip,"Check upgrade idx(%d), sq(%llu)name(%s)type(%c)",
                      local_nIdx, pUgdInfo->ua_sq[local_nIdx], pUgdInfo->ua_name[local_nIdx], pUgdInfo->ua_target_type[local_nIdx]);

        memset	(local_szUpgradeDate, 0x00, sizeof(local_szUpgradeDate));
        sprintf	(local_szUpgradeDate, "%s%s", pUgdInfo->ua_date[local_nIdx], pUgdInfo->ua_time[local_nIdx]);

        WRITE_INFO_IP(p_logip,"Check Update time Is currDate(%s) >= ugdDate(%s) ", local_szCurrDate, local_szUpgradeDate );
        if ( strcmp(local_szCurrDate, local_szUpgradeDate) < 0 || strlen(local_szUpgradeDate) <= 0 )
        {
            return 0; //날짜에 안걸리는 경우
        }

        local_nUpgradeTargetType = (pUgdInfo->ua_target_type[local_nIdx]-'0')%48;

        /** 모든 범위는 무조건 Upgrade 정책 True **/
        if (local_nUpgradeTargetType == 0) //All Range
        {
            WRITE_INFO_IP(p_logip, "Return all range Rule, Rule idx(%d) ", local_nIdx );
            return 1;
        }

        local_nIdxU = fpcif_GetIdxByUserIp(p_realip);
        if(local_nIdxU > -1) // User Table에 존재하는 Agent인경우
        {
            WRITE_INFO_IP(p_logip,"User Find Success, User idx(%d) Ip : (%s)", local_nIdxU, p_realip);
            if (local_nUpgradeTargetType == 1) //IP Range
            {
                WRITE_INFO_IP(p_logip,"Upgrade Check ip range, idx(%d) Ip : (%s) ", local_nIdx, p_realip);

                /** Upgrade IP **/
                memset(local_szUpgradeIp, 0x00, sizeof(local_szUpgradeIp));
                strcpy(local_szUpgradeIp, (char *)pUgdInfo->ua_target_value[local_nIdx]);

                /** User IP **/
                if(fcom_malloc((void**)&local_ptrUsrIp, sizeof(char)*(strlen(pUserInfo[local_nIdxU].us_ip)+1)) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG, "fcom_malloc Failed " );
                    return (-1);
                }
                strcpy(local_ptrUsrIp, pUserInfo[local_nIdxU].us_ip);

                /** Get Token Cnt, IP개수가 2개 이상인지 체크하기 위함. **/
                local_nUserIpTokenCnt       = fcom_TokenCnt(local_ptrUsrIp, "," );
                local_nUpgradeIpTokenCnt    = fcom_TokenCnt(local_szUpgradeIp, "," );

                if(local_nUserIpTokenCnt > 0) // USER_TB의 해당 USER IP가 여러개설정된 경우
                {
                    for ( local_nLoop = 0; local_nLoop <= local_nUserIpTokenCnt; local_nLoop++ )
                    {
                        memset(local_szTmpUserIp, 0x00, sizeof(local_szTmpUserIp));
                        fcom_ParseStringByToken(local_ptrUsrIp, ",", local_szTmpUserIp, local_nLoop);

                        strcpy(local_szTmpIp, local_szUpgradeIp); //UPGRADE_TB IP
                        if ( local_nUpgradeIpTokenCnt > 0) // USER IP 2개, UPGRADE IP 2개
                        {
                            local_nRet = fsock_CompareIpRange(local_szTmpUserIp, local_szTmpIp); //AGENT_UPGRADE_TB IP랑 USER_TB IP 비교.
                            if(local_nRet > 0)
                            {
                                WRITE_INFO_IP(p_logip,"Return ip range, User IP(%s) Upgrade IP (%s)",
                                              local_szTmpUserIp, local_szTmpUpgradeIp);
                                fcom_MallocFree((void**)&local_ptrUsrIp);
                                return 1;
                            }
                            else if(local_nRet < 0)
                            {
                                WRITE_CRITICAL_IP(p_logip, "Fail in Ip Compare, User Ip(%s)", local_szTmpUserIp);
                            }
                        }
                        else // USER IP 2개, UPGRADE IP 1개
                        {
                            if ( strcmp(local_szTmpUserIp, local_szTmpIp) == 0)
                            {
                                WRITE_INFO_IP(p_logip, "Return ip range, User IP(%s) Upgrade IP (%s) ",
                                              local_szTmpUserIp, local_szTmpIp);
                                fcom_MallocFree((void**)&local_ptrUsrIp);
                                return 1;
                            }

                        }
                    }
                }
                else //단일 IP 비교.
                {
                    if ( local_nUpgradeIpTokenCnt > 0) // USER IP 단일, UPGRADE IP 2개
                    {
                        local_nRet = fsock_CompareIpRange(local_ptrUsrIp, local_szUpgradeIp); //AGENT_UPGRADE_TB IP랑 USER_TB IP 비교.
                        if(local_nRet > 0)
                        {
                            WRITE_INFO_IP(p_logip,"Return ip range, User IP(%s) Upgrade IP (%s)",
                                          local_ptrUsrIp, local_szTmpUpgradeIp);
                            fcom_MallocFree((void**)&local_ptrUsrIp);
                            return 1;
                        }
                        else if(local_nRet < 0)
                        {
                            WRITE_CRITICAL_IP(p_logip, "Fail in Ip Compare, User Ip(%s)", local_ptrUsrIp);
                        }

                    }
                    else // USER IP 단일, UPGRADE IP 단일
                    {
                        if ( strcmp(local_ptrUsrIp, local_szUpgradeIp) == 0)
                        {
                            WRITE_INFO_IP(p_logip, "Return ip range, User IP(%s) Upgrade IP (%s)",
                                          local_ptrUsrIp, local_szUpgradeIp);
                            fcom_MallocFree((void**)&local_ptrUsrIp);
                            return 1;
                        }
                    }
                }
                fcom_MallocFree((void**)&local_ptrUsrIp);
            }
            else if (local_nUpgradeTargetType == 2) //UG_SQ
            {
                if( atol(pUgdInfo->ua_target_value[local_nIdx]) == pUserInfo[local_nIdxU].ug_sq )
                {
                    WRITE_DEBUG_IP(p_logip,"Return higher-group, ugd(%llu)user(%llu)",
                                   atol(pUgdInfo->ua_target_value[local_nIdx]), pUserInfo[local_nIdxU].ug_sq);
                    return 1;
                }
                else  //하위그룹검색
                {
                    local_nRet = fpcif_GetIdxByGroupLinkSt( atol(pUgdInfo->ua_target_value[local_nIdx]), pUserInfo[local_nIdxU].ug_sq );
                    if(local_nRet > -1)
                    {
                        WRITE_DEBUG_IP(p_logip, "Return sub-group, ugd(%llu)user(%llu)",
                                       atol(pUgdInfo->ua_target_value[local_nIdx]), pUserInfo[local_nIdxU].ug_sq);
                        return 1;
                    }
                }
            }
            else
            {
                WRITE_DEBUG_IP(p_logip,"Undefined type(%d) ", local_nUpgradeTargetType);
            }
        }
        else //유저정보가 없다면, AGENT IP는 무조건 1개니깐 UPGRADE IP가 2개이상인지만 체크해서 비교.
        {
            WRITE_DEBUG_IP(p_logip,"User not found Agent IP Compare" );
            memset(local_szUpgradeIp, 0x00, sizeof(local_szUpgradeIp));
            strcpy(local_szUpgradeIp, (char *)pUgdInfo->ua_target_value[local_nIdx]); /* UPGRADE_AGENT_TB IP */

            local_nUpgradeIpTokenCnt = fcom_TokenCnt(local_szUpgradeIp, ",");

            if (local_nUpgradeTargetType == 1) //IP Range
            {
                if ( local_nUpgradeIpTokenCnt > 0) // agent IP 1개, UPGRADE IP 2개
                {
                    WRITE_DEBUG_IP(p_logip,"Compare Upgrade Ip : [%s:%s] ", local_szUpgradeIp, p_realip);
                    local_nRet = fsock_CompareIpRange(p_realip, local_szUpgradeIp); // UPGRADE_AGENT_TB와 Real Agent IP 비교.
                    if(local_nRet > 0)
                    {
                        WRITE_INFO_IP(p_logip,"Return ip range, ugd(%s)user(%s)", p_realip, local_szTmpUpgradeIp);
                        return 1;
                    }
                    else if(local_nRet < 0)
                    {
                        WRITE_CRITICAL_IP(p_logip, "Fail in Ip Compare, ip(%s)", p_realip);
                    }
                }
                else // agent IP 1개, UPGRADE IP 1개
                {
                    if ( strcmp(p_realip, local_szUpgradeIp) == 0)
                    {
                        WRITE_INFO_IP(p_logip, "Return ip range, User IP(%s) Upgrade IP (%s)",
                                      local_ptrUsrIp, local_szUpgradeIp);
                        return 1;
                    }
                }
            }
            else
            {
                WRITE_DEBUG(CATEGORY_DEBUG, "Upgrade User Not Found.. Invalid Upgrade Type (%d) ", local_nUpgradeTargetType);
            }
        }
    }//for



    return 0;
}


int fpcif_GetRuleSt(
        char*		p_realip,
        char*		p_logip,
        char*		p_sno,
        cpRule*		pCpRuleInfo,
        cpCycle*	pCpCycleInfo,
        cpFlag*     pCpFlag,
        int			agtFlag)
{
    int 		i = 0;
    int         rxt = 0;
    int         idxR = 0;
    int         idxU = 0;
    int         tokenCnt = 0;
    int			ru_target_type = 0;
    char        *tmpTokenDbIp = NULL;

    char        arrTokenRuleIp[31 +1] = {0x00,};
    char		tmpIp1[15 +1] = {0x00,};
    char		tmpIp2[31 +1] = {0x00,};

    // Rule Info Init
    fpcif_SetRuleInit(pCpRuleInfo);
    // Cycle Info Init
    fpcif_SetCycleInit(pCpCycleInfo);
    // Flag Info Init
    fpcif_SetFlagInit(pCpFlag);


    // if not null sno then, use sno
    if (strlen(p_sno) > 0)
    {
        idxU = fpcif_GetIdxByUserSno(p_sno);
    }
    else  // if null sno then, use ip
    {
        idxU = fpcif_GetIdxByUserIp(p_realip);
        if (idxU > -1)
        {
            // 수동등록 일 경우만 ip로 seq를 가져옴
            if ((pUserInfo[idxU].us_auto_sync-'0')%48 != 0)
            {
                idxU = -1;
            }
        }
    }


    for(idxR = 0; idxR <= pRuleInfo->tot_cnt; idxR++ ) //0부터시작하기때문에
    {
        if (pRuleInfo->ru_sq[idxR] == 0)
            continue;

        // Check rule schedule
        if( fpcif_ChkAllowSchedule(p_logip, pRuleInfo->ru_sq[idxR]) < 1 )
            continue;

        //if(ru_use == 1) {
        ru_target_type = (pRuleInfo->ru_target_type[idxR]-'0')%48;

        if(idxU > -1)
        {
            if(ru_target_type == 0) //US_SQ
            {
                if( atol(pRuleInfo->ru_target_value[idxR]) == pUserInfo[idxU].us_sq )
                {
                    WRITE_INFO_IP(p_logip, "P%d: Return user sq, rule(%llu)user(%llu)",
                                  pRuleInfo->ru_order[idxR],
                                  atol(pRuleInfo->ru_target_value[idxR]),pUserInfo[idxU].us_sq);
                    fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                    if( agtFlag == 1 )
                    {
                        fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                        fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                    }
                }
            }
            else if(ru_target_type == 1) //UG_SQ
            {
                if( atol(pRuleInfo->ru_target_value[idxR]) == pUserInfo[idxU].ug_sq )
                {
                    WRITE_INFO_IP(p_logip, "P%d: Return higher-group, rule(%llu)user(%llu)",
                                  pRuleInfo->ru_order[idxR],
                                  atol(pRuleInfo->ru_target_value[idxR]),pUserInfo[idxU].ug_sq);
                    fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                    if( agtFlag == 1 )
                    {
                        fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                        fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                    }

                }
                else //하위그룹검색
                {
                    rxt = fpcif_GetIdxByGroupLinkSt( atol(pRuleInfo->ru_target_value[idxR] ),
                                                     pUserInfo[idxU].ug_sq );
                    if(rxt > -1)
                    {
                        WRITE_INFO_IP(p_logip, "P%d: Return sub-group, rule(%llu)user(%llu)",
                                      pRuleInfo->ru_order[idxR],
                                      atol(pRuleInfo->ru_target_value[idxR]),pUserInfo[idxU].ug_sq);
                        fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                        if( agtFlag == 1 )
                        {
                            fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                            fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                        }

                    }
                }
            }
            else if(ru_target_type == 2) //IP Range
            {
                memset(arrTokenRuleIp, 0x00, sizeof(arrTokenRuleIp));
                strcpy(arrTokenRuleIp, (char *)pRuleInfo->ru_target_value[idxR]);

                if(fcom_malloc( (void**)&tmpTokenDbIp, sizeof(char) * ( strlen(pUserInfo[idxU].us_ip) + 1 ) ) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG," fcom_malloc Failed ");
                    return (-1);
                }
                strcpy(tmpTokenDbIp, pUserInfo[idxU].us_ip);

                // ALL IP
                if ( memcmp(arrTokenRuleIp,"0.0.0.0,0.0.0.0", 15) == 0 )
                {
                    WRITE_INFO_IP(p_logip, "P%d: Return ip range, rule(%s)user(%s)",
                                  pRuleInfo->ru_order[idxR],arrTokenRuleIp,tmpTokenDbIp);
                    fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                    if( agtFlag == 1 )
                    {
                        fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                        fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                    }
                }
                else // Compare IP
                {
                    tokenCnt = fcom_TokenCnt(tmpTokenDbIp, ",");
                    if(tokenCnt > 0) // USER_IP가 2개범위일때..
                    {
                        for(i=0; i<=tokenCnt; i++)
                        {
                            memset(tmpIp1, 0x00, sizeof(tmpIp1));
                            memset(tmpIp2, 0x00, sizeof(tmpIp2));

                            fcom_ParseStringByToken(tmpTokenDbIp, ",", tmpIp1, i); // 파싱 IP
                            snprintf(tmpIp2, sizeof(tmpIp2), "%s", arrTokenRuleIp); // RU_TARGET_VALUE

                            rxt = fsock_CompareIpRange(tmpIp1, tmpIp2);
                            if(rxt > 0)
                            {
                                WRITE_INFO_IP(p_logip, "P%d: Return ip range, rule(%s)user(%s)",
                                              pRuleInfo->ru_order[idxR], tmpIp2, tmpIp1);
                                fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                                if( agtFlag == 1 )
                                {
                                    fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                                    fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                                }
                            }
                        }
                    }
                    else
                    {
                        /* 모든 IP 범위 정책 */
                        if(memcmp(arrTokenRuleIp, "0.0.0.0", 7) == 0)
                        {
                            WRITE_INFO_IP(p_logip, "P%d: Return All ip range", pRuleInfo->ru_order[idxR] );
                            fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                            if( agtFlag == 1 )
                            {
                                fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                                fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                            }

                        }
                        else
                        {
                            rxt = fsock_CompareIpRange(tmpTokenDbIp, arrTokenRuleIp);
                            if(rxt > 0)
                            {
                                WRITE_INFO_IP(p_logip, "P%d: Return ip range, rule(%s)user(%s)",
                                              pRuleInfo->ru_order[idxR],arrTokenRuleIp,tmpTokenDbIp);
                                fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                                if( agtFlag == 1 )
                                {
                                    fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                                    fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                                }
                            }
                        }


                    }
                }
                fcom_MallocFree((void**)&tmpTokenDbIp);
            }
            else if(ru_target_type == 3) //All Range
            {
                WRITE_INFO_IP(p_logip, "P%d: Return all range",
                              pRuleInfo->ru_order[idxR]);
                fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                if( agtFlag == 1 )
                {
                    fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                    fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                }

            }
            else
            {
                WRITE_CRITICAL_IP(p_logip, "Undefined type(%d)", ru_target_type);
            }
        }
        else  //IP 유저정보가 없다면
        {
            if(ru_target_type == 2) //IP Range
            {
                memset(arrTokenRuleIp, 0x00, sizeof(arrTokenRuleIp));
                snprintf(arrTokenRuleIp, sizeof(arrTokenRuleIp), "%s", (char *)pRuleInfo->ru_target_value[idxR]);
                rxt = fsock_CompareIpRange(p_realip, arrTokenRuleIp);
                if(rxt > 0)
                {
                    WRITE_INFO_IP(p_logip, "P%d: Return ip range, rule(%s)real(%s)",
                                  pRuleInfo->ru_order[idxR], arrTokenRuleIp, p_realip);
                    fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                    if( agtFlag == 1 )
                    {
                        fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                        fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                    }

                }
            }
            else if(ru_target_type == 3) //All Range
            {
                WRITE_INFO_IP(p_logip, "P%d: Return all range",
                              pRuleInfo->ru_order[idxR]);
                fpcif_SetRuleSt(idxR, pCpRuleInfo, agtFlag, p_logip);
                if( agtFlag == 1 )
                {
                    fpcif_SetCycleSt(p_logip, idxR, pCpCycleInfo);
                    fpcif_SetFlagSt(p_logip,  idxR, pCpFlag);
                }
            }
        }
    }//for

    // 정책이 미설정(-1)이면, Config 값 적용시키는 함수.
    fpcif_SetCycleLast( p_logip, pCpCycleInfo );
    fpcif_SetFlagLast( p_logip, pCpFlag );

    return 0;
}


int fpcif_GetDefaultMac(char *cpip, char *res)
{
    int		idx;
    int		firstFlag = 1;
    char	tmpIp[15+1];
    char	bClassIp[11+1];

    memset		(tmpIp, 0x00, sizeof(tmpIp));
    strcpy		(tmpIp, cpip);
    memset		(bClassIp, 0x00, sizeof(bClassIp));

    fcom_getBClassIp(tmpIp, bClassIp);

    for(idx=0; idx <= g_stProcPcifInfo.nTotGw; idx++)
    {
        if(!strcmp(pGwInfo[idx].sg_class_ip, bClassIp))
        {
            if(firstFlag)
            {
                strcpy(res, pGwInfo[idx].sg_default_mac);
                firstFlag = 0;
            }
            else
            {
                strcat(res, ",");
                strcat(res, pGwInfo[idx].sg_default_mac);
            }
        }
    }

    return 0;
}
int fpcif_GetFileToBin(char *fname, char **resData)
{

    int len = 0;
    int offset = 0;
    int alloc_siz = 0;
    int fsize = 0;
    FILE *fp = NULL;
    char* out = NULL;
    char buf[1024 +1] = {0x00,};

    if ((fp = fopen(fname, "rb")) == NULL) {
        return -1;
    }

    //파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    alloc_siz = fsize + 1;

    if(fcom_malloc((void**)&out, alloc_siz) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    offset = 0;
    while (!feof(fp)) {
        len = fread(buf, 1, 1024, fp);
        if (len < 0) {
            fclose(fp);
            free(out);
            return 0;
        }
        memcpy(out+offset, buf, len);

        offset += len;
    }
    fclose(fp);

    *resData = out;
    out = NULL;

    return offset;
}


int fpcif_GetIpChain(char *agentIp, char *res)
{
    int 			j = 0;
    int             local_loop_count = 0;
    int				idx = 0;
    unsigned long	ruleUpdateCycle = 0;
    unsigned long	diffTime = 0;
    unsigned long   uldAgentIp = 0, uldDbIp = 0;
    unsigned long   lchain = 0;
    unsigned long   rchain = 0;

    char            tmpAgentIp[15 +1] = {0x00,};
    char            tmpDbIp[15 +1] = {0x00,};
    char            bClassAgentIp[11 +1] = {0x00,};
    char            bClassDbIp[11 +1] = {0x00,};
    char            slChain[15 +1] = {0x00,};
    char            srChain[15 +1] = {0x00,};

    time_t			currTime, accessTime;

    memset(slChain, 0x00, sizeof(slChain));
    memset(srChain, 0x00, sizeof(srChain));
    memset(tmpAgentIp, 0x00, sizeof(tmpAgentIp));
    memset(bClassAgentIp, 0x00, sizeof(bClassAgentIp));

    lchain = rchain = 0;

    uldAgentIp = fsock_Ip2Ui(agentIp);

    strcpy(tmpAgentIp, agentIp);
    fcom_getBClassIp(tmpAgentIp, bClassAgentIp);


    //현재시간 및 정책주기 가져오기
    currTime = time((time_t) 0);
    idx = fpcif_GetIdxByConfigSt("RULE_UPDATE_CYCLE");

    if(idx < 0) ruleUpdateCycle = 0;
    else        ruleUpdateCycle = (atol(pConfigInfo[idx].cfvalue)*2); //2��

    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
    {
        while(1)
        {
            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
            {
                for(j=0; j < g_stProcPcifInfo.nTotBase; j++)
                {
                    if(!strcmp(pBaseInfo[j].hb_access_time, ""))
                    {
                        continue;
                    }
                    else
                    {
                        accessTime = fcom_str2time(pBaseInfo[j].hb_access_time, "YYYY-MM-DD hh:mm:ss");
                        diffTime = currTime - accessTime;

                        if(diffTime > ruleUpdateCycle)
                        {
                            continue;
                        }
                    }

                    memset(tmpDbIp, 0x00, sizeof(tmpDbIp));
                    memset(bClassDbIp, 0x00, sizeof(bClassDbIp));
                    //token us_ip
                    strcpy(tmpDbIp, pBaseInfo[j].hb_access_ip);

                    fcom_getBClassIp(tmpDbIp, bClassDbIp);

                    if(!strcmp(bClassAgentIp, bClassDbIp))
                    {
                        uldDbIp = fsock_Ip2Ui(pBaseInfo[j].hb_access_ip);

                        if(uldDbIp < uldAgentIp)
                        {
                            if(lchain == 0)
                            {
                                lchain = uldDbIp;
                                memset(slChain, 0x00 ,sizeof(slChain));
                                strcpy(slChain, pBaseInfo[j].hb_access_ip);
                            }
                            else if(uldDbIp > lchain)
                            {
                                lchain = uldDbIp;
                                memset(slChain, 0x00, sizeof(slChain));
                                strcpy(slChain,pBaseInfo[j].hb_access_ip);
                            }
                        }
                        else if(uldDbIp > uldAgentIp)
                        {
                            if(rchain == 0)
                            {
                                rchain = uldDbIp;
                                memset(srChain, 0x00, sizeof(srChain));
                                strcpy(srChain, pBaseInfo[j].hb_access_ip);
                            }
                            else if(uldDbIp < rchain)
                            {
                                rchain = uldDbIp;
                                memset(srChain, 0x00, sizeof(srChain));
                                strcpy(srChain, pBaseInfo[j].hb_access_ip);
                            }
                        }
                        // 앞뒤로 없을경우 default
                        if(strlen(slChain) == 0)
                        {
                            sprintf(slChain, "%s.1", bClassAgentIp);
                        }
                        if(strlen(srChain) == 0)
                        {
                            sprintf(srChain, "%s.254", bClassAgentIp);
                        }
                    }
                }//for
                pthread_rwlock_unlock(&mutexpolicy);
                break;
            }
            else
            {
                fcom_SleepWait(5);
                local_loop_count++;
            }
            if( local_loop_count >=  500 )
            {
                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                break;
            }
        }
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d)",4 );
        return (-1);
    }

    sprintf(res, "%s,%s", slChain,srChain);

    return 0;
}


int fpcif_SetAgentByBbaseKey(_DAP_AGENT_INFO *p_AI)
{
    int 	idx = 0;
    int     local_loop_count = 0;
    time_t	currTime;
    char	currDate[19 +1] = {0x00,};

    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
    {
        while(1)
        {
            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
            {
                for(idx=0; idx < g_stProcPcifInfo.nTotBase; idx++)
                {
                    if(!strcmp(pBaseInfo[idx].hb_unq, p_AI->user_key))
                    {
                        currTime = time((time_t) 0);
                        memset	(currDate, 0x00, sizeof(currDate));
                        fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");

                        pBaseInfo[idx].us_sq = p_AI->user_seq;
                        strcpy(pBaseInfo[idx].hb_access_ip, p_AI->user_ip);
                        strcpy(pBaseInfo[idx].hb_access_mac, p_AI->user_mac);
                        strcpy(pBaseInfo[idx].hb_sock_ip, p_AI->sock_ip);
                        strcpy(pBaseInfo[idx].hb_access_time, currDate);
                        strcpy(pBaseInfo[idx].hb_agent_ver, p_AI->agent_ver);
                        pBaseInfo[idx].hb_del = '0';
                        strcpy(p_AI->access_time, currDate);

                        pthread_rwlock_unlock(&mutexpolicy);
                        return 1;
                    }
                }
                pthread_rwlock_unlock(&mutexpolicy);

                break;

            }
            else
            {
                fcom_SleepWait(4);
                local_loop_count++;
            }
            if( local_loop_count >=  500 )
            {
                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                break;
            }
        }
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d)",4 );
        return (-1);
    }




    return 0;
}

int fpcif_SetTerminateByManager_st(char *p_userId)
{
    int idx;

    for(idx=0; idx < g_stProcPcifInfo.nTotManager; idx++)
    {
        if(!strcmp(pManagerInfo[idx].mn_id, p_userId))
        {
            pManagerInfo[idx].mn_login_status = '0';
            pManagerInfo[idx].mn_conn_pid = 0;
            pManagerInfo[idx].mn_conn_fq = -1;
            return 1;
        }
    }

    return 0;
}
void fpcif_SetFlagSt(char* logip, int idx, cpFlag* pCpFlagInfo)
{
    WRITE_DEBUG_IP(logip,"Set Flag Struct ");

    /** RULE_TB테이블 값이 (-1)이 아니고,
     *  현재 정책값이 (-1)이면 정책 설정.
     *  **/
    if (pRuleInfo->ru_agent_self_protect[idx] != (-1) &&
        pCpFlagInfo->agent_self_protect == (-1) )
    {
        pCpFlagInfo->agent_self_protect = pRuleInfo->ru_agent_self_protect[idx];
    }

    if( (pRuleInfo->ru_win_drv[idx] != (-1)) &&
        (pCpFlagInfo->win_drv_rule == (-1)) )
    {
        pCpFlagInfo->win_drv_rule = pRuleInfo->ru_win_drv[idx];
        pCpFlagInfo->win_drv_rusq = pRuleInfo->ru_sq[idx];
    }

    return;

}
void fpcif_SetCycleSt(char *logip, int idx, cpCycle* pCpCycleInfo)
{
    WRITE_DEBUG_IP(logip,"Set Cycle Struct ");
    /*
     * 0:pass 1:drop 2:info 3:nouse 4:warning 5:critical 6:block
     */
    if (pRuleInfo->ru_agent_cycle_process[idx] != -1 &&
        pCpCycleInfo->agent_process == -1)
    {
        pCpCycleInfo->agent_process = pRuleInfo->ru_agent_cycle_process[idx];

    }
    if (pRuleInfo->ru_agent_cycle_process_access[idx] != -1 &&
        pCpCycleInfo->agent_process_access == -1)
    {
        pCpCycleInfo->agent_process_access = pRuleInfo->ru_agent_cycle_process_access[idx];

    }
    if (pRuleInfo->ru_agent_cycle_net_printer[idx] != -1 &&
        pCpCycleInfo->agent_net_printer == -1)
    {
        pCpCycleInfo->agent_net_printer = pRuleInfo->ru_agent_cycle_net_printer[idx];

    }
    if (pRuleInfo->ru_agent_cycle_net_scan[idx] != -1 &&
        pCpCycleInfo->agent_net_scan == -1)
    {
        pCpCycleInfo->agent_net_scan = pRuleInfo->ru_agent_cycle_net_scan[idx];

    }
    if (pRuleInfo->ru_agent_cycle_router[idx] != -1 &&
        pCpCycleInfo->agent_router == -1)
    {
        pCpCycleInfo->agent_router = pRuleInfo->ru_agent_cycle_router[idx];

    }
    if (pRuleInfo->ru_agent_cycle_ext_access[idx] != -1 &&
        pCpCycleInfo->agent_ext_access == -1)
    {
        pCpCycleInfo->agent_ext_access = pRuleInfo->ru_agent_cycle_ext_access[idx];

    }
    if (pRuleInfo->ru_agent_sso_check_cycle[idx] != -1 &&
        pCpCycleInfo->agent_sso_check_cycle == -1)
    {
        pCpCycleInfo->agent_sso_check_cycle = pRuleInfo->ru_agent_sso_check_cycle[idx];

    }
    if (pRuleInfo->ru_agent_sso_keep_time[idx] != -1 &&
        pCpCycleInfo->agent_sso_keep_time == -1)
    {
        pCpCycleInfo->agent_sso_keep_time = pRuleInfo->ru_agent_sso_keep_time[idx];

    }
}
void fpcif_SetFlagLast(char* logip, cpFlag*  pCpFlagInfo)
{
    int idx = 0;

    if ( pCpFlagInfo->agent_self_protect == (-1) )
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_SELF_PROTECTION");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_SELF_PROTECTION' value");
            pCpFlagInfo->agent_self_protect = 0; // 디폴트 비활성화
        }
        else
        {
            pCpFlagInfo->agent_self_protect = atoi(pConfigInfo[idx].cfvalue);
        }
    }

    if ( pCpFlagInfo->win_drv_rule == (-1) )
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_WIN_DRV");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_WIN_DRV' value");
            pCpFlagInfo->win_drv_rule = 0; // 디폴트 비활성화
        }
        else
        {
            pCpFlagInfo->win_drv_rule = atoi(pConfigInfo[idx].cfvalue);
        }
    }

    return;

}
void fpcif_SetCycleLast(char *logip, cpCycle* pCpCycleInfo)
{
    int idx = 0;

    if (pCpCycleInfo->agent_process == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_PROCESS");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_PROCESS' value");
            pCpCycleInfo->agent_process = 0;
        } else {
            pCpCycleInfo->agent_process = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_process_access == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_PROCESS_ACCESS");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_PROCESS_ACCESS' value");
            pCpCycleInfo->agent_process_access = 0;
        }
        else
        {
            pCpCycleInfo->agent_process_access = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_net_printer == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_NET_PRINTER");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_NET_PRINTER' value");
            pCpCycleInfo->agent_net_printer = 0;
        }
        else
        {
            pCpCycleInfo->agent_net_printer = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_net_scan == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_NET_SCAN");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_NET_SCAN' value");
            pCpCycleInfo->agent_net_scan = 0;
        }
        else
        {
            pCpCycleInfo->agent_net_scan = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_router == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_ROUTER");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_ROUTER' value");
            pCpCycleInfo->agent_router = 0;
        }
        else
        {
            pCpCycleInfo->agent_router = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_ext_access == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_CYCLE_EXT_ACCESS");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_CYCLE_EXT_ACCESS' value");
            pCpCycleInfo->agent_ext_access = 0;
        }
        else
        {
            pCpCycleInfo->agent_ext_access = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_sso_check_cycle == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_SSO_CHECK_CYCLE");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_SSO_CHECK_CYCLE' value");
            pCpCycleInfo->agent_sso_check_cycle = 0;
        }
        else
        {
            pCpCycleInfo->agent_sso_check_cycle = atoi(pConfigInfo[idx].cfvalue);
        }
    }
    if (pCpCycleInfo->agent_sso_keep_time == -1)
    {
        idx = fpcif_GetIdxByConfigSt("AGENT_SSO_KEEP_TIME");
        if(idx < 0)
        {
            WRITE_CRITICAL_IP(logip, "Not found 'AGENT_SSO_KEEP_TIME' value");
            pCpCycleInfo->agent_sso_keep_time = 0;
        }
        else
        {
            pCpCycleInfo->agent_sso_keep_time = atoi(pConfigInfo[idx].cfvalue);
        }
    }

}
int fpcif_SetRuleSt(int idx, cpRule* pCpRuleInfo, int p_agtFlag, char *p_logip)
{
    WRITE_DEBUG_IP(p_logip,"Set Rule Struct ");
    /*
     * 0:pass 1:drop 2:info 3:nouse 4:warning 5:critical 6:block
     */
    // NET_ADAPTER(5)
    if( (pRuleInfo->ru_net_adapter[idx] != '0') &&
        (pCpRuleInfo->na_rule == '0') )
    {
        pCpRuleInfo->na_rule = pRuleInfo->ru_net_adapter[idx];
        pCpRuleInfo->na_rusq = pRuleInfo->ru_sq[idx];
    }
    // WIFI(6)
    if( (pRuleInfo->ru_wifi[idx] != '0') &&
        (pCpRuleInfo->wf_rule == '0') )
    {
        pCpRuleInfo->wf_rule = pRuleInfo->ru_wifi[idx];
        pCpRuleInfo->wf_rusq = pRuleInfo->ru_sq[idx];
    }
    // BLUETOOTH(7)
    if( (pRuleInfo->ru_bluetooth[idx] != '0') &&
        (pCpRuleInfo->bt_rule == '0') )
    {
        pCpRuleInfo->bt_rule = pRuleInfo->ru_bluetooth[idx];
        pCpRuleInfo->bt_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_CONNECTION(8)
    if( (pRuleInfo->ru_net_connection[idx] != '0') &&
        (pCpRuleInfo->nc_rule == '0') )
    {
        pCpRuleInfo->nc_rule = pRuleInfo->ru_net_connection[idx];
        pCpRuleInfo->nc_rusq = pRuleInfo->ru_sq[idx];
    }
    // DISK(9)
    if( (pRuleInfo->ru_disk[idx] != '0') &&
        (pCpRuleInfo->dk_rule == '0') )
    {
        pCpRuleInfo->dk_rule = pRuleInfo->ru_disk[idx];
        pCpRuleInfo->dk_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_DRIVE(10)
    if( (pRuleInfo->ru_net_drive[idx] != '0') &&
        (pCpRuleInfo->nd_rule == '0') )
    {
        pCpRuleInfo->nd_rule = pRuleInfo->ru_net_drive[idx];
        pCpRuleInfo->nd_rusq = pRuleInfo->ru_sq[idx];
    }
    // SHARE_FOLDER(12)
    if( (pRuleInfo->ru_share_folder[idx] != '0') &&
        (pCpRuleInfo->sf_rule == '0') )
    {
        pCpRuleInfo->sf_rule = pRuleInfo->ru_share_folder[idx];
        pCpRuleInfo->sf_rusq = pRuleInfo->ru_sq[idx];
    }
    // INFRARED_DEVICE(13)
    if( (pRuleInfo->ru_infrared_device[idx] != '0') &&
        (pCpRuleInfo->id_rule == '0') )
    {
        pCpRuleInfo->id_rule = pRuleInfo->ru_infrared_device[idx];
        pCpRuleInfo->id_rusq = pRuleInfo->ru_sq[idx];
    }
    // ROUTER(15)
    if( (pRuleInfo->ru_router[idx] != '0') &&
        (pCpRuleInfo->rt_rule == '0') )
    {
        pCpRuleInfo->rt_rule = pRuleInfo->ru_router[idx];
        pCpRuleInfo->rt_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_PRINTER(16)
    if( (pRuleInfo->ru_printer[idx] != '0') &&
        (pCpRuleInfo->np_rule == '0') )
    {
        pCpRuleInfo->np_rule = pRuleInfo->ru_printer[idx];
        pCpRuleInfo->np_rusq = pRuleInfo->ru_sq[idx];
    }
    // VIRTUAL_MACHINE(19)
    if( (pRuleInfo->ru_virtual_machine[idx] != '0') &&
        (pCpRuleInfo->st_rule_vm == '0') )
    {
        pCpRuleInfo->st_rule_vm = pRuleInfo->ru_virtual_machine[idx];
        pCpRuleInfo->st_rusq_vm = pRuleInfo->ru_sq[idx];
    }

    // CONECT_EXT_SVR(20)
    if( (pRuleInfo->ru_connect_ext_svr[idx] != '0') &&
        (pCpRuleInfo->ce_rule == '0') )
    {
        pCpRuleInfo->ce_rule = pRuleInfo->ru_connect_ext_svr[idx];
        pCpRuleInfo->ce_rusq = pRuleInfo->ru_sq[idx];
        if( p_agtFlag == 1 ) //agent 정책요청 시에만
        {
            // get detect white
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->connect_ext_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 3);
            }
        }
    }
    // NET_ADAPTER_OVER(21)
    if( (pRuleInfo->ru_net_adapter_over[idx] != '0') &&
        (pCpRuleInfo->na_over_rule == '0') )
    {
        pCpRuleInfo->na_over_rule = pRuleInfo->ru_net_adapter_over[idx];
        pCpRuleInfo->na_over_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_ADAPTER_DUPIP(22)
    if( (pRuleInfo->ru_net_adapter_dupip[idx] != '0') &&
        (pCpRuleInfo->na_dupip_rule == '0') )
    {
        pCpRuleInfo->na_dupip_rule = pRuleInfo->ru_net_adapter_dupip[idx];
        pCpRuleInfo->na_dupip_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_ADAPTER_DUPMAC(23)
    if( (pRuleInfo->ru_net_adapter_dupmac[idx] != '0') &&
        (pCpRuleInfo->na_dupmac_rule == '0') )
    {
        pCpRuleInfo->na_dupmac_rule = pRuleInfo->ru_net_adapter_dupmac[idx];
        pCpRuleInfo->na_dupmac_rusq = pRuleInfo->ru_sq[idx];
    }
    // DISK_REG(24)
    if( (pRuleInfo->ru_disk_reg[idx] != '0') &&
        (pCpRuleInfo->dk_reg_rule == '0') )
    {
        pCpRuleInfo->dk_reg_rule = pRuleInfo->ru_disk_reg[idx];
        pCpRuleInfo->dk_reg_rusq = pRuleInfo->ru_sq[idx];
    }
    // DISK_HIDDEN(25)
    if( (pRuleInfo->ru_disk_hidden[idx] != '0') &&
        (pCpRuleInfo->dk_hidden_rule == '0') )
    {
        pCpRuleInfo->dk_hidden_rule = pRuleInfo->ru_disk_hidden[idx];
        pCpRuleInfo->dk_hidden_rusq = pRuleInfo->ru_sq[idx];
    }
    // DISK_NEW(26)
    if( (pRuleInfo->ru_disk_new[idx] != '0') &&
        (pCpRuleInfo->dk_new_rule == '0') )
    {
        pCpRuleInfo->dk_new_rule = pRuleInfo->ru_disk_new[idx];
        pCpRuleInfo->dk_new_rusq = pRuleInfo->ru_sq[idx];
    }
    // DISK_MOBILE(27)
    if( (pRuleInfo->ru_disk_mobile[idx] != '0') &&
        (pCpRuleInfo->dk_mobile_rule == '0') )
    {
        pCpRuleInfo->dk_mobile_rule = pRuleInfo->ru_disk_mobile[idx];
        pCpRuleInfo->dk_mobile_rusq = pRuleInfo->ru_sq[idx];
        pCpRuleInfo->dk_mobile_read_rule = pRuleInfo->ru_disk_mobile_read[idx];
        pCpRuleInfo->dk_mobile_read_rusq = pRuleInfo->ru_sq[idx];
        pCpRuleInfo->dk_mobile_write_rule = pRuleInfo->ru_disk_mobile_write[idx];
        pCpRuleInfo->dk_mobile_write_rusq = pRuleInfo->ru_sq[idx];
    }

    // PROCESS_WHITE(30)
    if( (pRuleInfo->ru_process_white[idx] != '0') &&
        (pCpRuleInfo->ps_white_rule == '0') )
    {
        pCpRuleInfo->ps_white_rule = pRuleInfo->ru_process_white[idx];
        pCpRuleInfo->ps_white_rusq = pRuleInfo->ru_sq[idx];
        if( p_agtFlag == 1 ) //agent 정책요청 시에만
        {
            // get detect white
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->ps_white_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 1);
            }
        }
    }

    // PROCESS_BLACK(31)
    if( (pRuleInfo->ru_process_black[idx] != '0') &&
        (pCpRuleInfo->ps_black_rule == '0') )
    {
        pCpRuleInfo->ps_black_rule = pRuleInfo->ru_process_black[idx];
        pCpRuleInfo->ps_black_rusq = pRuleInfo->ru_sq[idx];
        if( p_agtFlag == 1 ) //agent ��å��û �ÿ���
        {
            // get detect black
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->ps_black_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 0);
            }
        }
    }

    // PROCESS_ACCESSMON(32)
    if( (pRuleInfo->ru_process_accessmon[idx] != '0') &&
        (pCpRuleInfo->ps_accessmon_rule == '0') )
    {
        pCpRuleInfo->ps_accessmon_rule = pRuleInfo->ru_process_accessmon[idx];
        pCpRuleInfo->ps_accessmon_rusq = pRuleInfo->ru_sq[idx];
        // get exception option
        pCpRuleInfo->ps_accessmon_exp = pRuleInfo->ru_process_accessmon_exp[idx];
        if( p_agtFlag == 1 ) //agent 정책요청 시에만
        {
            // get detect access
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->ps_accessmon_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 2);
            }
        }
    }
    // PROCESS_BLOCK(따로RULE정의하지않고VIRTUAL_MACHINE과 같이사용, context값으로분기함)
    if( (pRuleInfo->ru_virtual_machine[idx] != '0') &&
        (pCpRuleInfo->ps_block_rule == '0') )
    {
        pCpRuleInfo->ps_block_rule = pRuleInfo->ru_virtual_machine[idx];
        pCpRuleInfo->ps_block_rusq = pRuleInfo->ru_sq[idx];
        if( p_agtFlag == 1 )  //agent 정책요청 시에만
        {
            // get detect block
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->ps_block_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 4);
            }
        }
    }
    // get black, white, accessmon, connect_ext
    if( p_agtFlag == 1 && g_stProcPcifInfo.cfgLoadDetectMem != 1 )
    {
        /* 2020.12.08 RULE_DETECT_TB 참조 제거. */
        fpcif_LoadDetectIdx(pRuleInfo->ru_sq[idx],pDetectInfo->tot_cnt, pDetectSelInfo );
    }
    // PROCESS_DETAIL_INFO for only request_cfg
    if( (pRuleInfo->ru_process_detailinfo[idx] != '0') &&
        (pCpRuleInfo->ps_detailinfo_rule == '0') )
    {
        pCpRuleInfo->ps_detailinfo_rule = pRuleInfo->ru_process_detailinfo[idx];
        pCpRuleInfo->ps_detailinfo_rusq = pRuleInfo->ru_sq[idx];
    }
    // PROCESS_FORCEKILL for only request_cfg
    if( (pRuleInfo->ru_process_forcekill[idx] != '0') &&
        (pCpRuleInfo->ps_forcekill_rule == '0') )
    {
        pCpRuleInfo->ps_forcekill_rule = pRuleInfo->ru_process_forcekill[idx];
        pCpRuleInfo->ps_forcekill_rusq = pRuleInfo->ru_sq[idx];
    }
    // NET_ADAPTER_MULIP(33)
    if( (pRuleInfo->ru_net_adapter_mulip[idx] != '0') &&
        (pCpRuleInfo->na_mulip_rule == '0') )
    {
        pCpRuleInfo->na_mulip_rule = pRuleInfo->ru_net_adapter_mulip[idx];
        pCpRuleInfo->na_mulip_rusq = pRuleInfo->ru_sq[idx];
    }

    // EXTERNAL_CONN(34)
    // SSO_CERT(35)
    if( (pRuleInfo->ru_sso_cert[idx] != '0') &&
        (pCpRuleInfo->sc_rule == '0') )
    {
        pCpRuleInfo->sc_rule = pRuleInfo->ru_sso_cert[idx];
        pCpRuleInfo->sc_rusq = pRuleInfo->ru_sq[idx];
    }
    // WIN_DRV(36)
//    if( (pRuleInfo->ru_win_drv[idx] != '0') &&
//        (pCpRuleInfo->win_drv_rule == '0') )
//    {
//        pCpRuleInfo->win_drv_rule = pRuleInfo->ru_win_drv[idx];
//        pCpRuleInfo->win_drv_rusq = pRuleInfo->ru_sq[idx];
//    }
    // PROCESS_BLOCK(따로RULE정의하지않고VIRTUAL_MACHINE과 같이사용, context값으로분기함)
    if( (pRuleInfo->ru_rdp_session[idx] != '0') &&
        (pCpRuleInfo->rdp_session_rule == '0') )
    {
        //LogDRet(5, "|%-8s|Check schedule rdp_session|%s\n", STR_DEBUG,__func__);
        pCpRuleInfo->rdp_session_rule = pRuleInfo->ru_rdp_session[idx];
        pCpRuleInfo->rdp_session_rusq = pRuleInfo->ru_sq[idx];

        pCpRuleInfo->rdp_block_copy_rule = pRuleInfo->ru_rdp_block_copy[idx];
        pCpRuleInfo->rdp_block_copy_rusq = pRuleInfo->ru_sq[idx];

    }
    /* AGENT CPU LIMIT  */
    if(strlen(pCpRuleInfo->agent_str_cpu_limit) == 0 && strlen(pRuleInfo->ru_str_agent_cpu[idx]) > 0)
    {
        sprintf(pCpRuleInfo->agent_str_cpu_limit,"%s",pRuleInfo->ru_str_agent_cpu[idx]);
    }
    /* PROCESS CPU TOTAL ALARM */
    /* 전체 알람 규칙이 설정 안되어있고, */
    if( (pRuleInfo->ru_cpu_alarm[idx] != '0') &&
        (pCpRuleInfo->cpu_alarm_rule == '0'))
    {
        pCpRuleInfo->cpu_alarm_rule = pRuleInfo->ru_cpu_alarm[idx];
        pCpRuleInfo->cpu_alarm_rusq = pRuleInfo->ru_sq[idx];
        if( pRuleInfo->ru_cpu_alarm_rate[idx] != 0 && pCpRuleInfo->cpu_alarm_rate == 0)
        {
            pCpRuleInfo->cpu_alarm_rate           = pRuleInfo->ru_cpu_alarm_rate[idx];
        }
        if( pRuleInfo->ru_cpu_alarm_sustained_time[idx] != 0 && pCpRuleInfo->cpu_alarm_sustained_time == 0)
        {
            pCpRuleInfo->cpu_alarm_sustained_time = pRuleInfo->ru_cpu_alarm_sustained_time[idx];
        }
        if( p_agtFlag == 1 ) //agent 정책요청 시에만
        {
            // get detect
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->total_cpu_alarm_except_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 5);
                pCpRuleInfo->cpu_alarm_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 6);
            }
        }
    }
    /* PROCESS CPU CONTROL*/
    if( (pRuleInfo->ru_cpu_ctrl[idx] != '0') &&
        (pCpRuleInfo->cpu_ctrl_rule == '0'))
    {
        pCpRuleInfo->cpu_ctrl_rule = pRuleInfo->ru_cpu_ctrl[idx];
        pCpRuleInfo->cpu_ctrl_rusq = pRuleInfo->ru_sq[idx];

        if( pRuleInfo->ru_cpu_ctrl_rate[idx] != 0 && pCpRuleInfo->cpu_ctrl_rate == 0)
        {
            pCpRuleInfo->cpu_ctrl_rate = pRuleInfo->ru_cpu_ctrl_rate[idx];

        }
        if( pRuleInfo->ru_cpu_ctrl_limit_rate[idx] != 0 && pCpRuleInfo->cpu_ctrl_limit_rate == 0)
        {
            pCpRuleInfo->cpu_ctrl_limit_rate = pRuleInfo->ru_cpu_ctrl_limit_rate[idx];

        }
        if( pRuleInfo->ru_cpu_ctrl_sustained_time[idx] != 0 && pCpRuleInfo->cpu_ctrl_sustained_time == 0)
        {
            pCpRuleInfo->cpu_ctrl_sustained_time = pRuleInfo->ru_cpu_ctrl_sustained_time[idx];
        }
        if( p_agtFlag == 1 ) //agent 정책요청 시에만
        {
            // get detect
            if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
            {
                pCpRuleInfo->cpu_ctrl_except_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 7);
                pCpRuleInfo->cpu_ctrl_idx = fpcif_GetIdxByDetectSt(pRuleInfo->ru_sq[idx], 8);
            }
        }
    }

    return 0;
}


int fpcif_SetBinToFile(char* temp, char* wr_file_path, int filesize)
{
    FILE *wr_fp = NULL;

    wr_fp = fopen(wr_file_path, "wb");
    if (!wr_fp)
    {
        WRITE_CRITICAL( CATEGORY_DEBUG,"Failed to file open, path(%s)",
                        wr_file_path);
        return -1;
    }

    if (fwrite((const void *)temp, 1, filesize, wr_fp) != filesize)
    {
        WRITE_CRITICAL( CATEGORY_DEBUG,"Failed to write file, path(%s)",
                        wr_file_path);
        fclose(wr_fp);
        return -1;
    }

    fclose(wr_fp);

    return 0;
}

int fpcif_CidrToIpAndMask(const char *cidr, uint32_t *ip, uint32_t *mask)
{
    uint8_t a, b, c, d, bits;

    if(sscanf(cidr, "%hhu.%hhu.%hhu.%hhu/%hhu", &a, &b, &c, &d, &bits) < 5) {
        return -1;
    }
    if (bits > 32) {
        return -1;
    }
    *ip =
            (a << 24UL) |
            (b << 16UL) |
            (c << 8UL) |
            (d);
    *mask = (0xFFFFFFFFUL << (32 - bits)) & 0xFFFFFFFFUL;

    return 0;
}

int fpcif_ValidDate(char *logip, char* stDate, char* stTime, int stType, int stExp)
{
    int		tokenCnt;
    char	dbTmp[20];
    char	dbBeginDate[14+1];
    char	dbEndDate[14+1];
    char	dbBeginTime[6+1];
    char	dbEndTime[6+1];
    char	curVal[14+1];
    time_t	curTime = time((time_t) 0);

    switch( stType )
    {
        case 0:
            tokenCnt = fcom_TokenCnt(stDate, "~");
            if( tokenCnt > 0 )
            {
                memset(dbTmp, 0x00, sizeof(dbTmp));
                memset(dbBeginDate, 0x00, sizeof(dbBeginDate));
                memset(dbEndDate, 0x00, sizeof(dbEndDate));
                strcpy(dbTmp, stDate);
                fcom_Str2ValToken(dbTmp, "~", dbBeginDate, dbEndDate);
            }
            else
            {
                WRITE_INFO_IP(logip, "Invalid date format(%s)", stDate);
                return -1;
            }
            tokenCnt = fcom_TokenCnt(stTime, "~");
            if( tokenCnt > 0 )
            {
                memset(dbTmp, 0x00, sizeof(dbTmp));
                strcpy(dbTmp, stTime);
                memset(dbBeginTime, 0x00, sizeof(dbBeginTime));
                memset(dbEndTime, 0x00, sizeof(dbEndTime));
                fcom_Str2ValToken(dbTmp, "~", dbBeginTime, dbEndTime);
            }
            else
            {
                WRITE_INFO_IP(logip, "Invalid time format(%s)", stDate);
                return -1;
            }
            strcat(dbBeginDate, dbBeginTime);
            strcat(dbEndDate,	dbEndTime);
            memset(curVal, 0x00, sizeof(curVal));
            fcom_time2str(curTime, curVal, "YYYYMMDDhhmmss\0");

            if( stExp == 1 ) // negative
            {
                if( strcmp(curVal, dbBeginDate) < 0 &&
                    strcmp(curVal, dbEndDate) > 0 ) return 1;
            }
            else
            {
                if( strcmp(curVal, dbBeginDate) >= 0 &&
                    strcmp(curVal, dbEndDate) <= 0 ) return 1;
            }

            WRITE_INFO_IP(logip, "Out of range date, cur(%s)db(%s~%s)",
                          curVal,dbBeginDate,dbEndDate);
            break;
        case 1:
            tokenCnt = fcom_TokenCnt(stTime, "~");
            if( tokenCnt > 0 )
            {
                memset(dbTmp, 0x00, sizeof(dbTmp));
                strcpy(dbTmp, stTime);

                memset(dbBeginTime, 0x00, sizeof(dbBeginTime));
                memset(dbEndTime, 0x00, sizeof(dbEndTime));
                fcom_Str2ValToken(dbTmp, "~", dbBeginTime, dbEndTime);
            }
            else
            {
                WRITE_INFO_IP(logip, "Invalid time format(%s)", stTime);
                return -1;
            }
            memset(curVal, 0x00, sizeof(curVal));
            fcom_time2str(curTime, curVal, "hhmmss\0");
            if( stExp == 1 ) // negative
            {
                if( strcmp(curVal, dbBeginTime) < 0 &&
                    strcmp(curVal, dbEndTime) > 0 ) return 1;
            }
            else
            {
                if( strcmp(curVal, dbBeginTime) >= 0 &&
                    strcmp(curVal, dbEndTime) <= 0 ) return 1;
            }

            WRITE_INFO_IP(logip, "Out of range time, cur(%s)db(%s~%s)",
                          curVal,dbBeginTime,dbEndTime);
            break;
    } // switch

    return 0;
}

int fpcif_ValidDayofWeek(char *logip, char* stDayOfWeek, char* stTime, int stExp)
{
    int		tokenCnt;
    int		todayDayOfWeekNum;
    char	dbTmp[20];
    char	dbBeginTime[6+1];
    char	dbEndTime[6+1];
    char	today[10+1];
    char	curVal[14+1];
    char	todayDayOfWeek[3+1];
    time_t	curTime = time((time_t) 0);

    // get current-time from system
    memset(curVal, 0x00, sizeof(curVal));
    fcom_time2str(curTime, curVal, "hhmmss\0");

    // get begin-time and end-time from struct
    tokenCnt = fcom_TokenCnt(stTime, "~");
    if( tokenCnt > 0 )
    {
        memset(dbTmp, 0x00, sizeof(dbTmp));
        strcpy(dbTmp, stTime);
        memset(dbBeginTime, 0x00, sizeof(dbBeginTime));
        memset(dbEndTime, 0x00, sizeof(dbEndTime));
        fcom_Str2ValToken(dbTmp, "~", dbBeginTime, dbEndTime);
    }
    else
    {
        WRITE_INFO_IP(logip, "Invalid time format(%s)", stTime);
        return -1;
    }

    // get dayofweek from current-time
    memset(today, 0x00, sizeof(today));
    fcom_time2str(curTime, today, "YYYY-MM-DD\0");
    todayDayOfWeekNum = fcom_GetDayOfWeek(today);
    memset(todayDayOfWeek, 0x00, sizeof(todayDayOfWeek));
    fcom_DayofWeek2Str(todayDayOfWeekNum, todayDayOfWeek);

    if( strlen(todayDayOfWeek) < 3 )
    {
        WRITE_INFO_IP(logip, "Invalid today dayofweek(%s)", todayDayOfWeek);
        return -1;
    }

    if( stExp == 1 ) // negative
    {
        if( (strstr(stDayOfWeek, todayDayOfWeek) == NULL) &&
            (strcmp(curVal, dbBeginTime) < 0 &&
             strcmp(curVal, dbEndTime) > 0) )
        {
            return 1;
        }
    }
    else
    {
        if( (strstr(stDayOfWeek, todayDayOfWeek) != NULL) &&
            (strcmp(curVal, dbBeginTime) >= 0 &&
             strcmp(curVal, dbEndTime) <= 0) )
        {
            return 1;
        }
    }

    WRITE_INFO_IP(logip, "Out of range dayofweek(%s), cur(%s)db(%s~%s)",
                  stDayOfWeek,curVal,dbBeginTime,dbEndTime);
    return 0;
}

int fpcif_ValidDay(char *logip, char* stDay, char* stTime, int stExp)
{
    int		tokenCnt;
    char	dbTmp[20];
    char	dbBeginTime[6+1];
    char	dbEndTime[6+1];
    char	today[2+1];
    char	curVal[14+1];
    time_t	curTime = time((time_t) 0);

    // get current-time of system
    memset(curVal, 0x00, sizeof(curVal));
    fcom_time2str(curTime, curVal, "hhmmss\0");

    // get begin-time and end-time of struct
    tokenCnt = fcom_TokenCnt(stTime, "~");
    if( tokenCnt > 0 )
    {
        memset(dbTmp, 0x00, sizeof(dbTmp));
        strcpy(dbTmp, stTime);
        memset(dbBeginTime, 0x00, sizeof(dbBeginTime));
        memset(dbEndTime, 0x00, sizeof(dbEndTime));
        fcom_Str2ValToken(dbTmp, "~", dbBeginTime, dbEndTime);
    }
    else
    {
        WRITE_INFO_IP(logip, "Invalid time format(%s)",
                      stTime);
        return -1;
    }

    // get day of system
    memset(today, 0x00, sizeof(today));
    fcom_time2str(curTime, today, "DD\0");

    if( stExp == 1 ) // negative
    {
        if( (strstr(stDay, today) == NULL) &&
            (strcmp(curVal, dbBeginTime) < 0 &&
             strcmp(curVal, dbEndTime) > 0) )
        {
            return 1;
        }
    }
    else
    {
        if( (strstr(stDay, today) != NULL) &&
            (strcmp(curVal, dbBeginTime) >= 0 &&
             strcmp(curVal, dbEndTime) <= 0) )
        {
            return 1;
        }
    }

    WRITE_INFO_IP(logip, "Out of range day(%s), cur(%s)db(%s~%s)",
                  stDay,today,dbBeginTime,dbEndTime);
    return 0;
}

int fpcif_ValidWeekofDay(char *logip, char* stWeek, char* stTime, int stExp)
{
    int		weekNum;
    int		tokenCnt;
    char	dbTmp[20];
    char	dbBeginTime[6+1];
    char	dbEndTime[6+1];
    char	curWeekOfDay[2+1];
    char	curVal[14+1];
    time_t	curTime = time((time_t) 0);
    struct	tm stTm;

    memset(curVal, 0x00, sizeof(curVal));
    memset(&stTm, 0x00, sizeof(struct tm));

    fcom_time2str(curTime, curVal, "hhmmss\0");

    tokenCnt = fcom_TokenCnt(stTime, "~");
    if( tokenCnt > 0 )
    {
        memset(dbTmp, 0x00, sizeof(dbTmp));
        strcpy(dbTmp, stTime);
        memset(dbBeginTime, 0x00, sizeof(dbBeginTime));
        memset(dbEndTime, 0x00, sizeof(dbEndTime));
        fcom_Str2ValToken(dbTmp, "~", dbBeginTime, dbEndTime);
    }
    else
    {
        WRITE_CRITICAL_IP(logip, "Invalid time format(%s)",
                          stTime);
        return -1;
    }

    memset(curWeekOfDay, 0x00, sizeof(curWeekOfDay));
    localtime_r(&curTime,&stTm);
    weekNum = fcom_GetWeekOfMonth(&stTm);
    sprintf(curWeekOfDay, "%dW", weekNum);

    if( stExp == 1 ) // negative
    {
        if( (strstr(stWeek, curWeekOfDay) == NULL) &&
            (strcmp(curVal, dbBeginTime) < 0 &&
             strcmp(curVal, dbEndTime) > 0) )
        {
            return 1;
        }
    }
    else
    {
        if( (strstr(stWeek, curWeekOfDay) != NULL) &&
            (strcmp(curVal, dbBeginTime) >= 0 &&
             strcmp(curVal, dbEndTime) <= 0) )
        {
            return 1;
        }
    }

    WRITE_INFO_IP(logip, "Out of range week(%s), cur(%s)db(%s~%s)",
                  stWeek,curVal,dbBeginTime,dbEndTime);
    return 0;
}

int fpcif_SetLogLevel(char *cpip)
{
    int		idx;
    int		dbLogLevel;

    idx = fpcif_GetIdxByUserIp(cpip);
    if(idx > -1)
    {
        dbLogLevel = (pUserInfo[idx].us_log_level-'0')%48;
        if(dbLogLevel > 0)
        {
            g_stServerInfo.stDapLogInfo.nCfgDebugLevel = dbLogLevel;
        }
    }

    return idx;
}

void fpcif_DelAllMnq(void)
{
    char	delMnqFilePath[128];
    char	mnqPath[128];
    DIR*    pMngDir;
    struct  dirent* pDirent;

    memset(mnqPath, 0x00, sizeof(mnqPath));
    sprintf(mnqPath, "%s/MNGQ/", g_stServerInfo.stDapQueueInfo.szDAPQueueHome);
    if (access(mnqPath, W_OK) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Can't access path(%s) ", mnqPath);
        return;
    }
    if(!(pMngDir = opendir(mnqPath)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in open manager queue dir(%s) ", mnqPath);
        return;
    }
    else
    {
        while((pDirent = readdir(pMngDir)) != NULL)
        {
            // delete fq file
            if (strcmp(pDirent->d_name, ".") && strcmp(pDirent->d_name, ".."))
            {
                memset  (delMnqFilePath, 0x00, sizeof(delMnqFilePath));
                sprintf (delMnqFilePath, "%s%s", mnqPath,pDirent->d_name);
                if( remove(delMnqFilePath) )
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in delete queue file(%s)error(%s) ",
                                   delMnqFilePath,strerror(errno));
                }
            }
        }
        closedir(pMngDir);
    }
    return;
}

