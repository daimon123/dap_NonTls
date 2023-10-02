//
// Created by KimByoungGook on 2020-07-15.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#include "com/dap_com.h"
#include "json/dap_json.h"

#include "pcif.h"



void fpcif_SetJsonInt(json_t* jsonObject, int ruleValue, const char* GetkeyName, const char* SetkeyName)
{
    json_t		*value;

    value = json_integer(ruleValue);
    json_object_set(json_object_get(jsonObject, GetkeyName), SetkeyName, value);
    json_decref(value);
}

void fpcif_SetJsonStr(json_t* jsonObject, char* ruleValue, const char* GetkeyName,const char* SetkeyName)
{
    json_t		*value;

    value = json_string(ruleValue);
    json_object_set(json_object_get(jsonObject, GetkeyName), SetkeyName, value);
    json_decref(value);
}

int fpcif_MakeJsonCmd(char *logip, int p_msgCode, int p_cmdCode, char** p_resJson)
{
    char    *result = NULL;
    json_t  *root   = NULL;

    root = json_object();
    json_object_set_new(root, "cmd_code", json_integer(p_cmdCode));
    if(p_msgCode == SERVER_CMD_MATCH_MAC)
    {
        json_object_set_new(root, "str_mac", json_string(""+p_cmdCode));
    }

    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL_IP(logip, "Fail in generate json cmd");
        json_decref(root);
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_DEBUG_JSON(logip, "Dump json(%s)", *p_resJson);

    return strlen(*p_resJson);
}

int fpcif_MakeJsonForward(
        char	*logip,
        int		p_msgCode,
        int		*p_convMsgCode,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	*p_findIp,
        char	*p_findStr,
        char	**p_resJson)
{
    char *result = NULL;
    json_t *root = NULL;

    if (p_msgCode == MANAGE_REQ_SERVER_LOGLIST)
    {
        root = json_pack("{s:s, s:s, s:s, s:s, s:s}", "manager_id", p_managerId,
                         "begin_date", p_beginDate,
                         "end_date", p_endDate,
                         "category", p_category,
                         "find_ip", p_findIp);
        *p_convMsgCode = MANAGE_FORWARD_SERVER_LOGLIST;
    }
    else if (p_msgCode == MANAGE_REQ_SERVER_LOGFILE)
    {
        root = json_pack("{s:s, s:s, s:s, s:s, s:s, s:s}", "manager_id", p_managerId,
                         "begin_date", p_beginDate,
                         "end_date", p_endDate,
                         "category", p_category,
                         "find_ip", p_findIp,
                         "find_str", p_findStr);
        *p_convMsgCode = MANAGE_FORWARD_SERVER_LOGFILE;
    }

    result = json_dumps(root, JSON_INDENT(0));
    if (result == NULL)
    {
        json_decref(root);
        WRITE_CRITICAL_IP(logip, "Json pack error");
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_DEBUG_JSON(logip, "Dump json(%s)",  *p_resJson);

    return strlen(*p_resJson);
}


int fpcif_MakeJsonTail(int p_serverId, char* p_fileName, char* p_content, char** p_resJson)
{
    char	*result = NULL;
    json_t	*root   = NULL;

    root = json_pack("{s:i, s:s, s:s}", "server_id",p_serverId,
                     "file_name",p_fileName,
                     "message", p_content );
    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        json_decref(root);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in generate json fwd" );
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;

    //로그찍으면 안됨 tail 시 중복현상 일어남
    //LogCpDRet(4, logip, "|%-8s|Dump json(%s)\n", STR_INFO,*p_resJson);

    return strlen(*p_resJson);
}

int fpcif_MakeJsonUrgentReq(_DAP_AGENT_INFO* p_AI, char *p_cpIp, char** p_resJson)
{
    int idxc;
    int agentPort;
    char        strValue[128+1];
    char		*result;
    json_t      *root, *value;

    root = json_object();

    json_object_set_new(root, "config", json_object());
    idxc = fpcif_GetIdxByConfigSt("AUTO_UPDATE_AGENT_VER");
    memset(strValue, 0x00, sizeof(strValue));

    /* /urgent/update/내에 IP 파일 없을경우 Version NULL로 보낸다. */
    if(fpcif_CheckUrgentFile(p_AI->user_ip) < 0 || idxc < 0)
        strcpy(strValue, "");
    else
        strcpy(strValue, pConfigInfo[idxc].cfvalue);


    value = json_string(strValue);
    json_object_set(json_object_get(root, "config"), "auto_update_agent_ver", value);
    json_decref(value);

    /* Agent Emergency Log요청 포트 */
    idxc = fpcif_GetIdxByConfigSt("AGENT_EM_LISTEN_PORT");
    memset(strValue, 0x00, sizeof(strValue));
    if(idxc < 0)
    {
        WRITE_WARNING_IP(p_cpIp,"Not found %s config value", "AGENT_EM_LISTEN_PORT");
        agentPort = 50119;
    }
    else
    {
        agentPort = atoi(pConfigInfo[idxc].cfvalue);
    }
    snprintf(strValue,sizeof(strValue),"%d",agentPort);
    value = json_string(strValue);
    json_object_set(json_object_get(root, "config"), "agent_listen_port", value);
    json_decref(value);

    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL_IP(p_cpIp,"Fail in generate json dump" );
        json_decref(root);
        return -1;
    }

    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_INFO_JSON(p_cpIp,"Dump json(%s) ",*p_resJson );

    return strlen(*p_resJson);

}

int fpcif_MakeJsonManagerInfo(char *logip, int p_userLevel, char** p_resJson, char* param_RecvManagerVersion)
{
    char	cfgDBIp[15+1]           = {0x00,};
    char	cfgDBId[256]            = {0x00,};
    char	cfgDBPwd[256]           = {0x00,};
    char	cfgDBName[256]          = {0x00,};
    char	cfgHistDBPrefix[30]     = {0x00,};
    char	cfgHistDBPostfix[10]    = {0x00,};
    char	cfgHistTablePostfix[10] = {0x00,};
    char    local_szManagerVersion[15 +1] = {0x00,};
    char*   result = NULL;
    json_t  *root = NULL;
    int		cfgDBPort = 0;
    int		cfgProxyPort = 0;

    fcom_GetProfile("MYSQL","DB_IP"         ,cfgDBIp,   "127.0.0,1");
    fcom_GetProfile("MYSQL","DB_ID"         ,cfgDBId,   "master");
    fcom_GetProfile("MYSQL","DB_PASSWORD"   ,cfgDBPwd,  "master");
    fcom_GetProfile("MYSQL","DB_NAME"       ,cfgDBName, "master");

    cfgDBPort       = fcom_GetProfileInt("MYSQL","DB_PORT"      ,3306);
    cfgProxyPort    = fcom_GetProfileInt("PROXY","LISTEN_PORT"  ,50207);
    fcom_GetProfile("COMMON","HISTORY_DB_PREFIX",       cfgHistDBPrefix,    "DAP_HISTORY");
    fcom_GetProfile("COMMON","HISTORY_DB_POSTFIX",      cfgHistDBPostfix,   "YEAR");
    fcom_GetProfile("COMMON","HISTORY_TABLE_POSTFIX",   cfgHistTablePostfix,"MONTH");

    // Cfg에 설정된 Manager 버전.
    fcom_GetProfile("COMMON", "MANAGER_VERSION", local_szManagerVersion, param_RecvManagerVersion);


    root = json_object();
    json_object_set_new(root, "level",              json_integer(p_userLevel));
    json_object_set_new(root, "db_ip",              json_string(cfgDBIp));
    json_object_set_new(root, "db_id",              json_string(cfgDBId));
    json_object_set_new(root, "db_pw",              json_string(cfgDBPwd));
    json_object_set_new(root, "db_name",            json_string(cfgDBName));
    json_object_set_new(root, "db_port",            json_integer(cfgDBPort));
    json_object_set_new(root, "proxy_port",         json_integer(cfgProxyPort));
    json_object_set_new(root, "history_db_prefix",  json_string(cfgHistDBPrefix));
    json_object_set_new(root, "history_db_postfix", json_string(cfgHistDBPostfix));

    json_object_set_new(root, "history_table_postfix",  json_string(cfgHistTablePostfix));


    json_object_set_new(root, "manager_version",  json_string(local_szManagerVersion));
    if ( strcmp(local_szManagerVersion,param_RecvManagerVersion) != 0)
    {
        json_object_set_new(root, "auto_update_exec", json_integer(TRUE));
    }
    else
    {
        json_object_set_new(root, "auto_update_exec", json_integer(FALSE));
    }

    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL_IP(logip, "Fail in generate json cmd");
        json_decref(root);
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_DEBUG_JSON(logip, "Dump json(%s)", *p_resJson);

    return strlen(*p_resJson);
}

int fpcif_MakeJsonEvent(char *logip, _DAP_EventParam *EP, char** p_resJson)
{
    char	*result = NULL;
    json_t	*root   = NULL;

    root = json_pack("{s:s, s:i, s:i, s:s}",
                     "detect_time"  , EP->detect_time,
                     "ev_type"      , EP->ev_type,
                     "ev_level"     , EP->ev_level,
                     "ev_context"   , EP->ev_context);
    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        json_decref(root);
        WRITE_CRITICAL_IP(logip, "Fail in generate json fwd");
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_DEBUG_IP(logip, "Return resJson(%s)", *p_resJson);

    return strlen(*p_resJson);
}
int fpcif_MakeJsonServerLoglist(
        json_t*		root,
        char*		p_strMgwId,
        char*		p_dir,
        char*		p_strDate,
        int*		p_loop1,
        int*		p_loop2,
        char*		p_logip)
{

    char	strLoop[10] = {0x00,};
    char	currPath[256] = {0x00,};
    struct 	stat	statbuf;
    char*	ptr = NULL;
    struct  dirent* dirp = NULL;
    DIR*    pTmpDir = NULL;
    json_t* value = NULL;


    memset(&statbuf, 0x00, sizeof(struct stat));

    WRITE_INFO_IP( p_logip, "- p_dir(%s)", p_dir);

    if(!(pTmpDir = opendir(p_dir)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in server tmplog dir(%s)", p_dir);
        return -1;
    }


    chdir(p_dir);
    while((dirp = readdir(pTmpDir)) != NULL)
    {
        stat(dirp->d_name, &statbuf);
        if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
        {
            if(S_ISDIR(statbuf.st_mode))
            {
                WRITE_INFO_IP( p_logip, "- directory d_name(%s)",
                               dirp->d_name);

                if( fcom_IsNumber(dirp->d_name))
                {
                    getcwd(currPath, 256);
                    strcat(currPath, "/");
                    strcat(currPath, dirp->d_name);

                    if( (ptr = strstr(currPath, "pcif")) != NULL )
                    {
                        memset	(p_strDate, 0x00, 30);
                        strcpy	(p_strDate, ptr+5); // ex) 2018/07/15
                        ptr = NULL;
                    }
                    memset	(strLoop, 0x00, sizeof(strLoop));
                    sprintf	(strLoop, "%d", *p_loop1);
                }
                else
                {
                    json_object_set(json_object_get(root, p_strMgwId), dirp->d_name, json_object());
                    *p_loop1 = *p_loop1 + 1;
                    *p_loop2 = 0;
                }

                fpcif_MakeJsonServerLoglist(root, p_strMgwId, dirp->d_name,
                                            p_strDate, p_loop1, p_loop2, p_logip);
            }
            else // If file
            {
                *p_loop2 = *p_loop2 + 1;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", *p_loop2);
                WRITE_INFO_IP( p_logip, "- file p_strDate(%s)d_name(%s)strLoop(%s)",
                               p_strDate,dirp->d_name,strLoop);

                if( strlen(p_strDate) > 10 )
                {
                    memset	(currPath, 0x00, sizeof(currPath));
                    sprintf	(currPath, "%s/%s", p_strDate,dirp->d_name);

                    json_object_set(
                            json_object_get(
                                    json_object_get(root, p_strMgwId), "pcif"), strLoop, json_object());

                    value = json_string(currPath);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(root, p_strMgwId), "pcif"), strLoop), "name", value);
                    json_decref(value);

                    value = json_integer(statbuf.st_size);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(root, p_strMgwId), "pcif"), strLoop), "size", value);
                    json_decref(value);
                }
                else
                {
                    json_object_set(
                            json_object_get(
                                    json_object_get(root, p_strMgwId), p_dir), strLoop, json_object());

                    value = json_string(dirp->d_name);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(root, p_strMgwId), p_dir), strLoop), "name", value);
                    json_decref(value);

                    value = json_integer(statbuf.st_size);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(root, p_strMgwId), p_dir), strLoop), "size", value);
                    json_decref(value);
                }
            }
        }
        else
        {
            continue;
        }
    } // while

    value = json_integer(*p_loop2); // Numbers of file
    if( strlen(p_strDate) > 10 && !strcmp(p_dir, "pcif") ) // If last pcif folder
    {
        json_object_set(json_object_get(json_object_get(root, p_strMgwId), "pcif"), "size", value);
        memset(p_strDate, 0x00, 30);
    }
    else
    {
        json_object_set(json_object_get(json_object_get(root, p_strMgwId), p_dir), "size", value);
    }
    json_decref(value);

    chdir("..");
    closedir(pTmpDir);

    return 0;
}
int fpcif_MakeJsonRequestCfg(_DAP_AGENT_INFO *p_AI, char *p_cpIp, char** p_resJson, int p_baseReq)
{
    int			rxt     = 0;
    int			idxc = 0; //pConfigInfo
    //int		idxu = 0; //pUserInfo
    int			idxd = 0; //pDetectInfo
    int			nValue = 0;
    int			loop = 0;
    int			spos = 0, offset= 0, midpos =0;
    int			tokenCnt                = 0;
    unsigned char   strValue[128 +1]    = {0x00,};
    char		strLoop[3+1]            = {0x00,};
    char		resChain[31+1]          = {0x00,};
    char		resMac[1024+1]          = {0x00,};
    char		strProcessName[62 +1]   = {0x00,};
    char		strDetailInfo[1+1]      = {0x00,};
    char        strProtectInfo[1+1]     = {0x00,};
    char        strRoundValue[384 +1]   = {0x00,};
    char        strHashInfo[32+1]       = {0x00,};
    char		ReplaceTemp[32+1]       = {0x00,};
    char        cpuUsage[16+1]          = {0x00,};
    char        cpuSusTime[16+1]        = {0x00,};
    char        cpuLimitRate[16+1]      = {0x00,};
    char        szUpdateVersion[32]     = {0x00,};
    char        szCfgUpdateVersion[32]  = {0x00,};
    char		*result                 = NULL;
    char		*tokenKey               = NULL;
    char        *TempTokenKey           = NULL;
    char		*resValue               = NULL;
    char        *TempPtr                = NULL;
    char        *TempPtr2               = NULL;
    json_t		*root = NULL, *value    = NULL;

    cpRule      cpRuleInfo;
    cpCycle     cpCycleInfo;
    cpFlag      cpFlagInfo;

    memset(&cpRuleInfo  , 0x00, sizeof(cpRule));
    memset(&cpCycleInfo , 0x00, sizeof(cpCycle));
    memset(&cpFlagInfo , 0x00, sizeof(cpFlagInfo));

    root = json_object();

    json_object_set_new(root, "rule", json_object());

    /*
     * user_key and user_mac
     */
    json_object_set_new(root, "user_key", json_string(p_AI->user_key));
    json_object_set_new(root, "user_mac", json_string(p_AI->user_mac));
    json_object_set_new(root, "user_seq", json_integer(p_AI->user_seq));

    if(fpcif_ChkLoadFlag() == 0)
    {
        fpcif_GetRuleSt(p_AI->user_ip, p_cpIp, p_AI->user_sno, &cpRuleInfo, &cpCycleInfo, &cpFlagInfo, 1);
    }
    else
    {
        WRITE_WARNING_IP(p_cpIp, " fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True " );
        return (-1);
    }

    fpcif_SetJsonInt(root, (cpRuleInfo.na_rule-'0'), "rule", "ru_net_adapter");
    fpcif_SetJsonInt(root, (cpRuleInfo.na_over_rule-'0'), "rule", "ru_net_adapter_over");
    fpcif_SetJsonInt(root, (cpRuleInfo.na_dupip_rule-'0'), "rule", "ru_net_adapter_dupip");
    fpcif_SetJsonInt(root, (cpRuleInfo.na_dupmac_rule-'0'), "rule", "ru_net_adapter_dupmac");
    fpcif_SetJsonInt(root, (cpRuleInfo.na_mulip_rule-'0'), "rule","ru_net_adapter_mulip");
    fpcif_SetJsonInt(root, (cpRuleInfo.wf_rule-'0'), "rule","ru_wifi");
    fpcif_SetJsonInt(root, (cpRuleInfo.bt_rule-'0'), "rule", "ru_bluetooth");
    fpcif_SetJsonInt(root, (cpRuleInfo.rt_rule-'0'), "rule","ru_router");
    fpcif_SetJsonInt(root, (cpRuleInfo.np_rule-'0'), "rule","ru_printer");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_rule-'0'), "rule","ru_disk");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_hidden_rule-'0'), "rule","ru_disk_hidden");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_new_rule-'0'), "rule","ru_disk_new");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_mobile_rule-'0'), "rule","ru_disk_mobile");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_mobile_read_rule-'0'), "rule","ru_disk_mobile_read");
    fpcif_SetJsonInt(root, (cpRuleInfo.dk_mobile_write_rule-'0'), "rule","ru_disk_mobile_write");
    fpcif_SetJsonInt(root, (cpRuleInfo.nd_rule-'0'), "rule","ru_net_drive");
    fpcif_SetJsonInt(root, (cpRuleInfo.nc_rule-'0'), "rule","ru_net_connection");
    fpcif_SetJsonInt(root, (cpRuleInfo.sf_rule-'0'), "rule","ru_share_folder");
    fpcif_SetJsonInt(root, (cpRuleInfo.id_rule-'0'), "rule","ru_infrared_device");
    fpcif_SetJsonInt(root, (cpRuleInfo.st_rule_vm-'0'), "rule","ru_virtual_machine");
    fpcif_SetJsonInt(root, (cpRuleInfo.ps_black_rule-'0'), "rule","ru_ps_black");
    fpcif_SetJsonInt(root, (cpRuleInfo.ps_white_rule-'0'), "rule","ru_ps_white");
    fpcif_SetJsonInt(root, (cpRuleInfo.ps_accessmon_rule-'0'), "rule","ru_ps_accessmon");
    fpcif_SetJsonInt(root, (cpRuleInfo.ps_block_rule-'0'), "rule","ru_ps_block");
    fpcif_SetJsonInt(root, (cpRuleInfo.ce_rule-'0'), "rule","ru_ce_svr");
    fpcif_SetJsonInt(root, p_baseReq, "rule","ru_base_req");

    // ru_ip_chain
    memset(resChain, 0x00, sizeof(resChain));
    if(fpcif_ChkLoadFlag() == 0)
    {
        fpcif_GetIpChain(p_AI->user_ip, resChain);
    }
    else
    {
        WRITE_WARNING_IP(p_cpIp,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True ");
        return (-1);
    }
    fpcif_SetJsonStr(root, resChain, "rule", "ru_ip_chain");


    // ru_default_gw_mac
    memset(resMac, 0x00, sizeof(resMac));
    fpcif_GetDefaultMac(p_AI->user_ip, resMac);
    fpcif_SetJsonStr(root, resMac, "rule", "ru_default_gw_mac");

    loop = 0;
    rxt = json_object_set_new(json_object_get(root, "rule"), "ru_process_black", json_object());
    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        idxd = cpRuleInfo.ps_black_idx;
        if((((cpRuleInfo.ps_black_rule-'0')%48) != 0) && (idxd > -1))
        {
            if(fcom_malloc((void**)&resValue,sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            strcpy(resValue, pDetectInfo->rd_value[idxd]);
            tokenCnt = fcom_TokenCnt(resValue, ";");

            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";", &TempPtr);
                while(tokenKey != NULL)
                {
                    rxt = fcom_GetRoundTagValue(tokenKey, strRoundValue);
                    /*(HashInfo:DetailInfo:ProtectInfo)*/
                    spos = fcom_GetStringPos(strRoundValue,':');
                    if(spos > 0) /* DetailInfo NULL이 아닌경우 */
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }
                    else /* DetailInfo NULL인경우  */
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }

                    if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)detailinfo(%s) protectinfo(%s)",
                                          TempTokenKey,strDetailInfo,strProtectInfo);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);

                        continue;
                    }

                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');

                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);
                    if( strlen(strProcessName) == 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                          tokenKey,strProcessName);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);

                        continue;
                    }
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_black"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    value = json_integer(atoi(strDetailInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "detail_info", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                } // while
            }
            else
            {
                rxt = fcom_GetRoundTagValue(resValue, strRoundValue);
                //(HashInfo:DetailInfo:ProtectInfo)
                spos = fcom_GetStringPos(strRoundValue,':');
                if(spos > 0)
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }
                else
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }

                if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s) detailinfo(%s) protectinfo(%s)",
                                      TempTokenKey,strDetailInfo,strProtectInfo);

                }

                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');

                memset(strProcessName, 0x00, sizeof(strProcessName));
                fcom_SubStr(1, spos, resValue, strProcessName);
                if( strlen(strProcessName) == 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, resValue(%s)name(%s)",
                                      resValue,strProcessName);
                }
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_black"),
                                strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "hash", value);
                json_decref(value);

                value = json_integer(atoi(strDetailInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "detail_info", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    else // direct select
    {
        if(	(((cpRuleInfo.ps_black_rule-'0')%48) != 0) &&
               (pDetectSelInfo->dp_process_black != NULL))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectSelInfo->dp_process_black)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            strcpy(resValue, pDetectSelInfo->dp_process_black);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                    rxt = fcom_GetRoundTagValue(tokenKey, strRoundValue);
                    //(HashInfo:DetailInfo:ProtectInfo)
                    spos = fcom_GetStringPos(strRoundValue,':');
                    if(spos > 0)
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }
                    else
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }

                    if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)detailinfo(%s)",
                                          TempTokenKey,strDetailInfo);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);

                        continue;

                    }

                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');

                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);
                    if( strlen(strProcessName) == 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                          tokenKey,strProcessName);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);

                        continue;
                    }
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_black"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    value = json_integer(atoi(strDetailInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_black"), strLoop),
                                    "detail_info", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";", &TempPtr);
                } // while
            }
            else
            {
                memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                rxt = fcom_GetRoundTagValue(resValue, strRoundValue);
                //(HashInfo:DetailInfo:ProtectInfo)
                spos = fcom_GetStringPos(strRoundValue,':');
                if(spos > 0)
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }
                else
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }

                if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)detailinfo(%s)",
                                      TempTokenKey,strDetailInfo);

                }

                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');

                memset(strProcessName, 0x00, sizeof(strProcessName));

                fcom_SubStr(1, spos, resValue, strProcessName);
                if( strlen(strProcessName) == 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, resValue(%s)name(%s)",
                                      resValue,strProcessName);
                }

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_black"),
                                strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "hash", value);
                json_decref(value);

                value = json_integer(atoi(strDetailInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_black"), strLoop),
                                "detail_info", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_black"), "size", value);
    json_decref(value);

    /* RULE PROCESS WHITE*/
    loop = 0;
    json_object_set_new(json_object_get(root, "rule"), "ru_process_white", json_object());
    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        idxd = cpRuleInfo.ps_white_idx;
        if((((cpRuleInfo.ps_white_rule-'0')%48) != 0) && (idxd > -1))
        {
            if( fcom_malloc((void**)&resValue, sizeof(char) * (strlen(pDetectInfo->rd_value[idxd])+1) ) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectInfo->rd_value[idxd]);

            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {

                    memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                    rxt = fcom_GetRoundTagValue(tokenKey, strRoundValue);
                    /*(HashInfo:DetailInfo:ProtectInfo)*/
                    spos = fcom_GetStringPos(strRoundValue,':');
                    if(spos > 0)
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }
                    else
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }

                    if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)detailinfo(%s)",
                                          TempTokenKey,strDetailInfo);

                        tokenKey = strtok_r(NULL, ";",&TempPtr);
                        continue;

                    }

                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');

                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);
                    if( strlen(strProcessName) == 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                          tokenKey,strProcessName);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);

                        continue;
                    }

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_white"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    value = json_integer(atoi(strDetailInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "detail_info", value);
                    json_decref(value);

                    value = json_integer(atoi(strProtectInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "protection", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
            else
            {

                rxt = fcom_GetRoundTagValue(resValue, strRoundValue);
                //(HashInfo:DetailInfo:ProtectInfo)
                spos = fcom_GetStringPos(strRoundValue,':');
                if(spos > 0)
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }
                else
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }

                if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, resValue(%s)name(%s)",
                                      resValue,strProcessName);
                }

                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');

                memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                memset(strProcessName, 0x00, sizeof(strProcessName));

                fcom_SubStr(1, spos, resValue, strProcessName);
                if( strlen(strProcessName) == 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                      tokenKey,strProcessName);
                }

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_white"),
                                strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "hash", value);
                json_decref(value);

                value = json_integer(atoll(strDetailInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "detail_info", value);
                json_decref(value);

                value = json_integer(atoi(strProtectInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "protection", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    else // direct select
    {
        if(	(((cpRuleInfo.ps_white_rule-'0')%48) != 0) &&
               (pDetectSelInfo->dp_process_white != NULL))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectSelInfo->dp_process_white)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectSelInfo->dp_process_white);

            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                    rxt = fcom_GetRoundTagValue(tokenKey, strRoundValue);

                    /* (HashInfo:DetailInfo:ProtectInfo) */
                    spos = fcom_GetStringPos(strRoundValue,':');
                    if(spos > 0)
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }
                    else
                    {
                        TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                        snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                        TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                        snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                    }

                    if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)detailinfo(%s)",
                                          TempTokenKey,strDetailInfo);

                        tokenKey = strtok_r(NULL, ";",&TempPtr);
                        continue;

                    }

                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');

                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);
                    if( strlen(strProcessName) == 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                          tokenKey,strProcessName);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);
                        continue;
                    }

                    //Loop
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_white"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    value = json_integer(atoll(strDetailInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "detail_info", value);
                    json_decref(value);

                    value = json_integer(atoi(strProtectInfo));
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_white"), strLoop),
                                    "protection", value);
                    json_decref(value);
                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
            else
            {
                rxt = fcom_GetRoundTagValue(resValue, strRoundValue);
                //(HashInfo:DetailInfo:ProtectInfo)
                spos = fcom_GetStringPos(strRoundValue,':');
                if(spos > 0)
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }
                else
                {
                    TempTokenKey = strtok_r(strRoundValue, ":", &TempPtr2);
                    snprintf(strDetailInfo, sizeof(strDetailInfo),"%s",TempTokenKey);

                    TempTokenKey = strtok_r(TempPtr2, ":", &TempPtr2);
                    snprintf(strProtectInfo, sizeof(strProtectInfo),"%s",TempTokenKey);
                }

                if(isdigit(strDetailInfo[0]) == 0 || isdigit(strProtectInfo[0]) == 0)
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, resValue(%s)name(%s)",
                                      resValue,strProcessName);
                }

                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');

                memset(strDetailInfo, 0x00, sizeof(strDetailInfo));
                memset(strProcessName, 0x00, sizeof(strProcessName));

                fcom_SubStr(1, spos, resValue, strProcessName);
                if( strlen(strProcessName) == 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid process, token(%s)name(%s)",
                                      tokenKey,strProcessName);
                }

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_white"),
                                    strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "hash", value);
                json_decref(value);

                value = json_integer(atoll(strDetailInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "detail_info", value);
                json_decref(value);

                value = json_integer(atoi(strProtectInfo));
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_white"), strLoop),
                                "protection", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_white"), "size", value);
    json_decref(value);
    /* RULE PROCESS WHITE END */


    /* RULE PROCESS ACCESSMON */
    loop = 0;
    json_object_set_new(json_object_get(root, "rule"), "ru_process_accessmon", json_object());
    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        idxd = cpRuleInfo.ps_accessmon_idx;
        if((((cpRuleInfo.ps_accessmon_rule-'0')%48) != 0) && (idxd > -1))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectInfo->rd_value[idxd]);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    value = json_string(tokenKey);

                    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                    strLoop, value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
                value = json_integer((cpRuleInfo.ps_accessmon_exp-'0')%48);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                "exception", value);
                json_decref(value);
            }
            else
            {
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                value = json_string(resValue);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                strLoop, value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    else // direct select
    {
        if(	(((cpRuleInfo.ps_accessmon_rule-'0')%48) != 0) &&
               (pDetectSelInfo->ws_ipaddr != NULL))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectSelInfo->ws_ipaddr)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            strcpy(resValue, pDetectSelInfo->ws_ipaddr);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";", &TempPtr);
                while(tokenKey != NULL)
                {
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    value = json_string(tokenKey);
                    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                    strLoop, value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
                value = json_integer((cpRuleInfo.ps_accessmon_exp-'0')%48);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                "exception", value);
                json_decref(value);
            }
            else
            {
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                value = json_string(resValue);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"),
                                strLoop, value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"), "size", value);
    json_decref(value);
    /* RULE PROCESS ACCESSMON END */


    // 2020.02.14 jhchoi VM kill을위해 추가
    /* RULE PROCESS BLOCK */
    loop = 0;
    json_object_set_new(json_object_get(root, "rule"), "ru_process_block", json_object());
    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 ) /* Memory Select */
    {
        idxd = cpRuleInfo.ps_block_idx;
        if((((cpRuleInfo.ps_block_rule-'0')%48) != 0) && (idxd > -1))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                return (-1);
            }

            strcpy(resValue, pDetectInfo->rd_value[idxd]);

            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";", &TempPtr);
                while(tokenKey != NULL)
                {
                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');
                    memset(strHashInfo, 0x00, sizeof(strHashInfo));
                    rxt = fcom_GetRoundTagValue(tokenKey, strHashInfo);
                    if( rxt < 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)hash(%s)",
                                          tokenKey,strHashInfo);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);
                        continue;
                    }
                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_block"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_block"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_block"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                } // while
            }
            else
            {
                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');
                memset(strHashInfo, 0x00, sizeof(strHashInfo));
                rxt = fcom_GetRoundTagValue(resValue, strHashInfo);
                if( rxt < 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid format, resValue(%s)hash(%s)",
                                      resValue,strHashInfo);
                }
                memset(strProcessName, 0x00, sizeof(strProcessName));
                fcom_SubStr(1, spos, resValue, strProcessName);

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_block"),
                                strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_block"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_block"), strLoop),
                                "hash", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    else // direct select
    {
        if( (((cpRuleInfo.ps_block_rule-'0')%48) != 0) &&
            (pDetectSelInfo->dp_process_block != NULL))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectSelInfo->dp_process_block)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectSelInfo->dp_process_block);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    spos = fcom_GetReversePos(tokenKey, '(');
                    fcom_GetReversePos(tokenKey, ')');
                    memset(strHashInfo, 0x00, sizeof(strHashInfo));
                    rxt = fcom_GetRoundTagValue(tokenKey, strHashInfo);
                    if( rxt < 0 )
                    {
                        WRITE_CRITICAL_IP(p_cpIp,"Invalid format, token(%s)hash(%s)",
                                          tokenKey,strHashInfo);
                        tokenKey = strtok_r(NULL, ";",&TempPtr);
                        continue;
                    }
                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    fcom_SubStr(1, spos, tokenKey, strProcessName);

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_block"),
                                    strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_block"), strLoop),
                                    "name", value);
                    json_decref(value);

                    value = json_string(strHashInfo);
                    json_object_set(json_object_get(json_object_get(json_object_get(
                            root, "rule"), "ru_process_block"), strLoop),
                                    "hash", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                } // while
            }
            else
            {
                spos = fcom_GetReversePos(resValue, '(');
                fcom_GetReversePos(resValue, ')');
                memset(strHashInfo, 0x00, sizeof(strHashInfo));
                rxt = fcom_GetRoundTagValue(resValue, strHashInfo);
                if( rxt < 0 )
                {
                    WRITE_CRITICAL_IP(p_cpIp,"Invalid format, resValue(%s)hash(%s)",
                                      resValue,strHashInfo);
                }
                memset(strProcessName, 0x00, sizeof(strProcessName));
                fcom_SubStr(1, spos, resValue, strProcessName);

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                json_object_set_new(json_object_get(json_object_get(root, "rule"), "ru_process_block"),
                                strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_block"), strLoop),
                                "name", value);
                json_decref(value);

                value = json_string(strHashInfo);
                json_object_set(json_object_get(json_object_get(json_object_get(
                        root, "rule"), "ru_process_block"), strLoop),
                                "hash", value);
                json_decref(value);

            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_block"), "size", value);
    json_decref(value);
    /* RULE PROCESS BLOCK END */

    /* RULE PROCESS FORCE KILL*/
    fpcif_SetJsonInt(root,cpRuleInfo.ps_forcekill_rule-'0',"rule","ru_process_forcekill");

    /* RULE PRINTER PORT */
    loop = 0;
    json_object_set_new(json_object_get(root, "rule"), "ru_printer_port", json_object());
    if(pRuleInfo->dp_print_port != NULL)
    {
        if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pRuleInfo->dp_print_port)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        strcpy(resValue, pRuleInfo->dp_print_port);

        tokenCnt = fcom_TokenCnt(resValue, ";");
        if( tokenCnt > 0 )
        {
            tokenKey = strtok_r(resValue, ";",&TempPtr);
            while(tokenKey != NULL)
            {
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                value = json_string(tokenKey);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_printer_port"),
                                strLoop, value);
                json_decref(value);

                tokenKey = strtok_r(NULL, ";",&TempPtr);
            }
        }
        else
        {
            loop++;
            memset(strLoop, 0x00, sizeof(strLoop));
            sprintf(strLoop, "%d", loop);
            value = json_string(resValue);
            json_object_set(json_object_get(json_object_get(root, "rule"), "ru_printer_port"),
                            strLoop, value);
            json_decref(value);
        }
        fcom_MallocFree((void**)&resValue);
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_printer_port"), "size", value);
    json_decref(value);
    /* RULE PRINTER PORT END */

    /* RULE CONNECT EXT SVR*/
    loop = 0;
    json_object_set_new(json_object_get(root, "rule"), "ru_connect_ext_svr", json_object());
    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        idxd = cpRuleInfo.connect_ext_idx;
        if((((cpRuleInfo.ce_rule-'0')%48) != 0) && (idxd > -1))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectInfo->rd_value[idxd]);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";", &TempPtr);
                while(tokenKey != NULL)
                {
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    value = json_string(tokenKey);
                    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"),
                                    strLoop, value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
            else
            {
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                value = json_string(resValue);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"),
                                strLoop, value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    else // direct select
    {
        if(	(((cpRuleInfo.ps_accessmon_rule-'0')%48) != 0) &&
               (pDetectSelInfo->ws_url != NULL))
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectSelInfo->ws_url)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            strcpy(resValue, pDetectSelInfo->ws_url);
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);
                    value = json_string(tokenKey);
                    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"),
                                    strLoop, value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
            else
            {
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                value = json_string(resValue);
                json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"),
                                strLoop, value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);
        }
    }
    value = json_integer(loop);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"), "size", value);
    json_decref(value);
    /* RULE CONNECT EXT SVR END */

    /* RULE TOTAL CPU ALARM */
    value = json_integer((cpRuleInfo.cpu_alarm_rule-'0')%48);
    json_object_set(json_object_get(root, "rule"), "cpu_alarm_rule", value);
    json_decref(value);

    loop = 0;
    idxd = cpRuleInfo.total_cpu_alarm_except_idx;
    if(cpRuleInfo.cpu_alarm_rate != 0 && cpRuleInfo.cpu_alarm_sustained_time != 0)
    {
        if (idxd > 0)
        {
            json_object_set_new(json_object_get(root, "rule"), "ru_cpu_usage_manager_all_process_alarm_policy", json_object());
            json_object_set_new(
                    json_object_get(
                            json_object_get(root, "rule"),"ru_cpu_usage_manager_all_process_alarm_policy"),
                                                                     "alarm_exception_policy",json_object());

            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])) +1) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                return (-1);
            }
            strcpy(resValue, pDetectInfo->rd_value[idxd]);
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while(tokenKey != NULL)
                {
                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    memset(ReplaceTemp, 0x00, sizeof(ReplaceTemp));
                    memset(cpuUsage, 0x00, sizeof(cpuUsage));
                    memset(cpuSusTime, 0x00, sizeof(cpuSusTime));

                    spos = fcom_GetStringPos(tokenKey,'<');
                    fcom_SubStr(1,spos,tokenKey,strProcessName);
                    fcom_SubStr(spos+1,strlen(tokenKey) - spos,tokenKey,ReplaceTemp);

                    fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                    fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);

                    spos = fcom_GetStringPos(ReplaceTemp,':');

                    memcpy(cpuUsage,ReplaceTemp,spos);
                    offset = spos+1;
                    memcpy(cpuSusTime,ReplaceTemp+offset,sizeof(cpuSusTime));

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    //Loop Count Set
                    json_object_set_new(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(root, "rule"),
                                            "ru_cpu_usage_manager_all_process_alarm_policy"),
                                    "alarm_exception_policy"),strLoop, json_object());

                    value = json_integer(atoi(cpuUsage));
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(
                                                    json_object_get(
                                                            root, "rule"),
                                                    "ru_cpu_usage_manager_all_process_alarm_policy"),
                                            "alarm_exception_policy"),
                                    strLoop),
                            "cpu_usage_rate", value);
                    json_decref(value);

                    value = json_integer(atoi(cpuSusTime));
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(
                                                    json_object_get(root, "rule"),
                                                    "ru_cpu_usage_manager_all_process_alarm_policy"),
                                            "alarm_exception_policy"),
                                    strLoop),
                            "monitoring_time", value);
                    json_decref(value);

                    value = json_string(strProcessName);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get(
                                                    json_object_get(
                                                            root, "rule"),
                                                    "ru_cpu_usage_manager_all_process_alarm_policy"),
                                            "alarm_exception_policy"),
                                    strLoop),
                            "name", value);

                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                } // while
            }
            else
            {
                memset(strProcessName, 0x00, sizeof(strProcessName));
                memset(ReplaceTemp, 0x00, sizeof(ReplaceTemp));
                memset(cpuUsage, 0x00, sizeof(cpuUsage));
                memset(cpuSusTime, 0x00, sizeof(cpuSusTime));

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                memset(strProcessName, 0x00, sizeof(strProcessName));
                memset(ReplaceTemp, 0x00, sizeof(ReplaceTemp));

                spos = fcom_GetStringPos(resValue,'<');

                fcom_SubStr(1,spos,resValue,strProcessName);
                fcom_SubStr(spos+1,strlen(resValue) - spos,resValue,ReplaceTemp);

                fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);

                spos = fcom_GetStringPos(ReplaceTemp,':');
                memcpy(cpuUsage,ReplaceTemp,spos);
                memcpy(cpuSusTime,ReplaceTemp+spos+1,sizeof(cpuSusTime));

                //Loop Count Set
                json_object_set_new(json_object_get(json_object_get(json_object_get(root, "rule"),
                                                                "ru_cpu_usage_manager_all_process_alarm_policy"),
                                                "alarm_exception_policy"),strLoop, json_object());

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(json_object_get(json_object_get(
                                json_object_get( root, "rule"),
                                "ru_cpu_usage_manager_all_process_alarm_policy"),
                                                        "alarm_exception_policy"), strLoop),
                        "cpu_usage_rate", value);
                json_decref(value);

                value = json_integer(atoi(cpuSusTime));
                json_object_set(
                        json_object_get(json_object_get(json_object_get(
                                json_object_get( root, "rule"), "ru_cpu_usage_manager_all_process_alarm_policy"),
                                                        "alarm_exception_policy"),
                                        strLoop),"monitoring_time", value);
                json_decref(value);

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(json_object_get(json_object_get(
                                json_object_get( root, "rule"),"ru_cpu_usage_manager_all_process_alarm_policy"),
                                                        "alarm_exception_policy"),
                                        strLoop),"name", value);
                json_decref(value);
            }
            fcom_MallocFree((void**)&resValue);

            value = json_integer(loop);
            json_object_set( json_object_get(json_object_get(json_object_get(root, "rule"),
                                                             "ru_cpu_usage_manager_all_process_alarm_policy"),
                                             "alarm_exception_policy"), "size", value);
            json_decref(value);

            value = json_integer(cpRuleInfo.cpu_alarm_rate);
            json_object_set(json_object_get(json_object_get(root, "rule"),
                                            "ru_cpu_usage_manager_all_process_alarm_policy"),"cpu_usage_rate",value);
            json_decref(value);


            value = json_integer(cpRuleInfo.cpu_alarm_sustained_time);
            json_object_set(json_object_get(json_object_get(root, "rule"),
                                            "ru_cpu_usage_manager_all_process_alarm_policy"),"duration_time",value);
            json_decref(value);

        }
        else
        {
            json_object_set_new(json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_all_process_alarm_policy", json_object());

            value = json_integer(cpRuleInfo.cpu_alarm_rate);
            json_object_set(json_object_get(json_object_get(root, "rule"),
                                            "ru_cpu_usage_manager_all_process_alarm_policy"),"cpu_usage_rate",value);
            json_decref(value);


            value = json_integer(cpRuleInfo.cpu_alarm_sustained_time);
            json_object_set(json_object_get(json_object_get(root, "rule"),
                                            "ru_cpu_usage_manager_all_process_alarm_policy"),"duration_time",value);
            json_decref(value);

        }

    }
    /* RULE TOTAL CPU ALARM END */

    /* RULE CPU ALARM */
    loop = 0;
    idxd = cpRuleInfo.cpu_alarm_idx;
    if(idxd > 0)
    {
        json_object_set_new(json_object_get(root, "rule"), "ru_cpu_usage_manager_alarm_policy", json_object());

        if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        strcpy(resValue, pDetectInfo->rd_value[idxd]);

        tokenCnt = fcom_TokenCnt(resValue, ";");
        if( tokenCnt > 0 )
        {
            tokenKey = strtok_r(resValue, ";",&TempPtr);

            while(tokenKey != NULL)
            {
                memset(strProcessName, 0x00, sizeof(strProcessName));
                memset(ReplaceTemp, 0x00, sizeof(ReplaceTemp));
                memset(cpuUsage, 0x00, sizeof(cpuUsage));
                memset(cpuSusTime, 0x00, sizeof(cpuSusTime));

                spos = fcom_GetStringPos(tokenKey,'<');
                if(spos == -1)
                    break;

                fcom_SubStr(1,spos,tokenKey,strProcessName);
                fcom_SubStr(spos+1,strlen(tokenKey)-spos,tokenKey,ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);

                spos = fcom_GetStringPos(ReplaceTemp,':');
                memcpy(cpuUsage,ReplaceTemp,spos);

                offset = spos+1;
                memcpy(cpuSusTime,ReplaceTemp+offset,sizeof(cpuSusTime));

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_alarm_policy"),strLoop, json_object());

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "cpu_usage_rate", value);
                json_decref(value);

                value = json_integer(atoi(cpuSusTime));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "duration_time", value);
                json_decref(value);

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "name", value);
                json_decref(value);

                tokenKey = strtok_r(NULL, ";",&TempPtr);
            } // while
        }
        else
        {
            loop++;
            memset(strLoop, 0x00, sizeof(strLoop));
            sprintf(strLoop, "%d", loop);

            memset(strProcessName, 0x00, sizeof(strProcessName));
            memset(ReplaceTemp, 0x00, sizeof(ReplaceTemp));
            memset(cpuUsage, 0x00, sizeof(cpuUsage));
            memset(cpuSusTime, 0x00, sizeof(cpuSusTime));

            spos = fcom_GetStringPos(resValue,'<');
            if(spos > 0)
            {
                fcom_SubStr(1,spos,resValue,strProcessName);
                fcom_SubStr(spos+1,strlen(resValue)-spos,resValue,ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);
                spos = fcom_GetStringPos(ReplaceTemp,':');

                memcpy(cpuUsage,ReplaceTemp,spos);
                memcpy(cpuSusTime,ReplaceTemp+spos+1,sizeof(cpuSusTime));

                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_alarm_policy"),strLoop, json_object());

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "cpu_usage_rate", value);
                json_decref(value);

                value = json_integer(atoi(cpuSusTime));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "duration_time", value);
                json_decref(value);

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), strLoop),
                        "name", value);
                json_decref(value);
            }


        }
        fcom_MallocFree((void**)&resValue);

        value = json_integer(loop);
        json_object_set(json_object_get(json_object_get(root, "rule"),
                                        "ru_cpu_usage_manager_alarm_policy"), "size", value);
        json_decref(value);
    }
    /* RULE CPU ALARM END */


    /* RULE TOTAL CPU CONTROL */
    value = json_integer((cpRuleInfo.cpu_ctrl_rule-'0')%48);
    json_object_set(json_object_get(root, "rule"), "cpu_ctrl_rule", value);
    json_decref(value);

    if(cpRuleInfo.cpu_ctrl_rate != 0 &&
       cpRuleInfo.cpu_ctrl_limit_rate != 0 &&
       cpRuleInfo.cpu_ctrl_sustained_time != 0 )
    {
        json_object_set_new(json_object_get(root, "rule"), "ru_cpu_usage_manager_all_process_control_policy", json_object());

        value = json_integer(cpRuleInfo.cpu_ctrl_rate);
        json_object_set(
                json_object_get( json_object_get(root, "rule"),"ru_cpu_usage_manager_all_process_control_policy"),
                                                                                "cpu_usage_rate",value);
        json_decref(value);

        value = json_integer(cpRuleInfo.cpu_ctrl_limit_rate);
        json_object_set(
                json_object_get( json_object_get(root, "rule"),"ru_cpu_usage_manager_all_process_control_policy"),
                "cpu_usage_rate_limit",value);
        json_decref(value);

        value = json_integer(cpRuleInfo.cpu_ctrl_sustained_time);
        json_object_set(
                json_object_get( json_object_get(root, "rule"),"ru_cpu_usage_manager_all_process_control_policy"),
                "duration_time",value);
        json_decref(value);

    }
    /* RULE TOTAL CPU CONTROL END */

    /* RULE CPU CONTROL */
    loop = 0;
    idxd = cpRuleInfo.cpu_ctrl_idx;
    if(idxd > 0)
    {
        json_object_set_new(json_object_get(root, "rule"), "ru_cpu_usage_manager_control_policy", json_object());

        if(fcom_malloc((void**)&resValue, sizeof(char) * (strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        strcpy(resValue, pDetectInfo->rd_value[idxd]);
        tokenCnt = fcom_TokenCnt(resValue, ";");
        if( tokenCnt > 0 )
        {
            tokenKey = strtok_r(resValue, ";",&TempPtr);

            while(tokenKey != NULL)
            {
                memset(strProcessName, 0x00, sizeof(strProcessName));
                memset(ReplaceTemp   , 0x00, sizeof(ReplaceTemp));
                memset(cpuUsage      , 0x00, sizeof(cpuUsage));
                memset(cpuSusTime    , 0x00, sizeof(cpuSusTime));
                memset(cpuLimitRate  , 0x00, sizeof(cpuLimitRate));


                spos = fcom_GetStringPos(tokenKey,'<');
                if(spos == -1)
                    break;

                fcom_SubStr(1,spos,tokenKey,strProcessName);
                fcom_SubStr(spos+1,strlen(tokenKey)-spos,tokenKey,ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);

                spos = fcom_GetStringPos(ReplaceTemp,':');
                memcpy(cpuUsage,ReplaceTemp,spos);

                offset = spos+1;
                midpos = fcom_GetStringPos(ReplaceTemp+offset,':');
//                memcpy(cpuSusTime,ReplaceTemp+offset,midpos);
                memcpy(cpuLimitRate,ReplaceTemp+offset,midpos);

                offset += midpos+1;
//                memcpy(cpuLimitRate,ReplaceTemp+offset,offset - spos);
                memcpy(cpuSusTime,ReplaceTemp+offset,offset - spos);

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_control_policy"),strLoop, json_object());

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "cpu_usage_rate", value);
                json_decref(value);

                value = json_integer(atoi(cpuSusTime));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "cpu_usage_rate_limit", value);
                json_decref(value);

                value = json_integer(atoi(cpuLimitRate));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "duration_time", value);
                json_decref(value);

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "name", value);
                json_decref(value);

                tokenKey = strtok_r(NULL, ";",&TempPtr);
            } // while
        }
        else
        {
            memset(strProcessName, 0x00, sizeof(strProcessName));
            memset(ReplaceTemp   , 0x00, sizeof(ReplaceTemp));
            memset(cpuUsage      , 0x00, sizeof(cpuUsage));
            memset(cpuSusTime    , 0x00, sizeof(cpuSusTime));
            memset(cpuLimitRate  , 0x00, sizeof(cpuLimitRate));

            spos = fcom_GetStringPos(resValue,'<');
            if(spos > 0)
            {
                fcom_SubStr(1,spos,resValue,strProcessName);
                fcom_SubStr(spos+1,strlen(resValue)-spos,resValue,ReplaceTemp);

                fcom_ReplaceAll(ReplaceTemp, "<", "", ReplaceTemp);
                fcom_ReplaceAll(ReplaceTemp, ">", "", ReplaceTemp);

                spos = fcom_GetStringPos(ReplaceTemp,':');
                memcpy(cpuUsage,ReplaceTemp,spos);
                offset = spos+1;

                midpos = fcom_GetStringPos(ReplaceTemp+offset,':');
                memcpy(cpuSusTime,ReplaceTemp+offset,midpos);
                offset += midpos+1;
                memcpy(cpuLimitRate,ReplaceTemp+offset,offset - spos);

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);
                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_control_policy"),
                                     strLoop, json_object());

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "cpu_usage_rate", value);
                json_decref(value);

                value = json_integer(atoi(cpuSusTime));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "cpu_usage_rate_limit", value);
                json_decref(value);

                value = json_integer(atoi(cpuLimitRate));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "duration_time", value);
                json_decref(value);

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_control_policy"), strLoop),
                                        "name", value);
                json_decref(value);

            }

        }
        fcom_MallocFree((void**)&resValue);

        value = json_integer(loop);
        json_object_set( json_object_get(json_object_get(root, "rule"),
                                         "ru_cpu_usage_manager_control_policy"), "size", value);
        json_decref(value);
    }

    /* ------------------------------------------------------------------- */

    /* ------------------------------------------------------------------- */
    /* 프로세스 CPU 통제 예외 프로세스 */
    loop = 0;
    idxd = cpRuleInfo.cpu_ctrl_except_idx;
    if(idxd > 0)
    {
        json_object_set_new(json_object_get(root, "rule"), "ru_cpu_usage_manager_uncontrolled_process", json_object());
        if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pDetectInfo->rd_value[idxd])+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
            return (-1);
        }
        strcpy(resValue, pDetectInfo->rd_value[idxd]);

        tokenCnt = fcom_TokenCnt(resValue, ";");
        if( tokenCnt > 0 )
        {
            tokenKey = strtok_r(resValue, ";",&TempPtr);
            while(tokenKey != NULL)
            {
                memset(strProcessName, 0x00, sizeof(strProcessName));
                sprintf(strProcessName,"%s",tokenKey);

                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "rule"),
                                "ru_cpu_usage_manager_uncontrolled_process"),
                                     strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "rule"),
                                        "ru_cpu_usage_manager_uncontrolled_process"), strLoop),
                                        "name", value);
                json_decref(value);

                tokenKey = strtok_r(NULL, ";",&TempPtr);
            } // while
        }
        else
        {
            memset(strProcessName, 0x00, sizeof(strProcessName));
            sprintf(strProcessName,"%s",resValue);

            loop++;
            memset(strLoop, 0x00, sizeof(strLoop));
            sprintf(strLoop, "%d", loop);

            //Loop Count Set
            json_object_set_new(
                    json_object_get(
                            json_object_get(root, "rule"),
                            "ru_cpu_usage_manager_uncontrolled_process"),strLoop, json_object());

            value = json_string(strProcessName);
            json_object_set(
                    json_object_get(
                            json_object_get(
                                    json_object_get( root, "rule"),
                                    "ru_cpu_usage_manager_uncontrolled_process"), strLoop),
                    "name", value);
            json_decref(value);
        }
        fcom_MallocFree((void**)&resValue);

        value = json_integer(loop);
        json_object_set( json_object_get(json_object_get(root, "rule"),
                                         "ru_cpu_usage_manager_control_policy"), "size", value);
        json_decref(value);
    }

    /*
     * ru_sso_cert
     */
    fpcif_SetJsonInt(root,cpRuleInfo.sc_rule-'0',"rule","ru_sso_cert");
    /*
     * ru_win_drv
     */
//    fpcif_SetJsonInt(root,cpRuleInfo.win_drv_rule-'0',"rule","ru_win_drv");
    /*
     * ru_rdp_session
     */
    fpcif_SetJsonInt(root,cpRuleInfo.rdp_session_rule-'0',"rule","ru_rdp_session");
    /*
     * ru_rdp_block_copy
     */
    fpcif_SetJsonInt(root,cpRuleInfo.rdp_block_copy_rule-'0',"rule","ru_rdp_block_copy");

    /*
     * cycle
     */
    fpcif_SetJsonInt(root,cpCycleInfo.agent_process         ,"rule","ru_agent_cycle_process");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_process_access  ,"rule","ru_agent_cycle_process_access");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_net_printer     ,"rule","ru_agent_cycle_net_printer");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_net_scan        ,"rule","ru_agent_cycle_net_scan");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_router          ,"rule","ru_agent_cycle_router");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_ext_access      ,"rule","ru_agent_cycle_ext_access");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_sso_check_cycle ,"rule","ru_agent_sso_check_cycle");
    fpcif_SetJsonInt(root,cpCycleInfo.agent_sso_keep_time   ,"rule","ru_agent_sso_keep_time");

    /*
     * config
     */
    json_object_set_new(root, "config", json_object());


    /*
     * Flag
     */
    //윈도우 드라이버 수집여부
    fpcif_SetJsonInt(root, cpFlagInfo.win_drv_rule,"rule","ru_win_drv");
    /** Agent 프로세스 자기보호 기능
     * 정책에 설정되어 있는경우 정책 값, 아닐경우 Config값
     * 디폴트 비활성화 **/
    fpcif_SetJsonInt(root, cpFlagInfo.agent_self_protect, "config", "agent_self_protection");

    idxc = fpcif_GetIdxByConfigSt("AGENT_LISTEN_PORT");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","agent_listen_port");

    idxc = fpcif_GetIdxByConfigSt("AGENT_DUMMY");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","agent_dummy");

    idxc = fpcif_GetIdxByConfigSt("AGENT_HIDE");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","agent_hide");

    idxc = fpcif_GetIdxByConfigSt("AGENT_WATCHER");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","agent_watcher");

    idxc = fpcif_GetIdxByConfigSt("AGENT_BOOT_DELAY");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","agent_boot_delay");

    idxc = fpcif_GetIdxByConfigSt("AUTO_UPDATE_AGENT_VER");
    memset(strValue, 0x00, sizeof(strValue));

    strcpy(strValue, (idxc < 0 ? "" : (char *)pConfigInfo[idxc].cfvalue) );
    fpcif_SetJsonStr(root,strValue,"config","auto_update_agent_ver");
    snprintf(szCfgUpdateVersion, sizeof(szCfgUpdateVersion), "%s", strValue );

    idxc = fpcif_GetIdxByConfigSt("AUTO_UPDATE_SERVER");
    memset(strValue, 0x00, sizeof(strValue));
    strcpy(strValue,(idxc < 0 ? "" : (char *)pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonStr(root,strValue,"config","auto_update_server");

    idxc = fpcif_GetIdxByConfigSt("AUTO_UPDATE_USE");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","auto_update_use");

    /* DAP Agent Upgrade */
    if(fpcif_ChkLoadFlag() == 0)
    {
        rxt = fpcif_GetUpgradeSt(p_AI->user_ip, p_cpIp, p_AI->agent_ver);
    }
    else
    {
        WRITE_WARNING_IP(p_cpIp,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True (%s)",p_cpIp);
        return (-1);
    }

    nValue = (rxt > 0 ? 1 : 0);
    WRITE_DEBUG_IP(p_cpIp,"Load Flag : (%d)", nValue);

     /*
     2021.14.14 kbg
     Update Server 설정버전 체크하여 일치하지 않는경우 json auto_update_exec false로 전송하도록 변경.
     */

    // Patch True일때
    if ( nValue == 1 )
    {
        // update.config에서 version을 가져온다.
        if ( ConfigGetValue(g_stServerInfo.stDapComnInfo.szUpdateConfigPath, "patch_file", "version", szUpdateVersion) == 0 )
        {
            WRITE_DEBUG_IP(p_cpIp, "Check Update Server Config IP (%s) : Manager Config IP (%s) Update Pass", szUpdateVersion, szCfgUpdateVersion);
            // Manager 설정 버전과 Update 설정 버전을 체크.
            if ( strcmp(szCfgUpdateVersion, szUpdateVersion) != 0 ) //설정 버전이 불일치 한경우
            {
                nValue = 0;
                fpcif_SetJsonInt(root,nValue,"config","auto_update_exec");
            }
            else //설정 버전이 일치하는 경우, 실제 Patch해야하는 상황
            {
                fpcif_SetJsonInt(root,nValue,"config","auto_update_exec");
            }
        }
        else
        {
            nValue = 0;
            fpcif_SetJsonInt(root,nValue,"config","auto_update_exec");
        }
    }
    else // Patch False일때
    {
        fpcif_SetJsonInt(root,nValue,"config","auto_update_exec");
    }


    idxc = fpcif_GetIdxByConfigSt("RULE_UPDATE_CYCLE");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","rule_update_cycle");

    idxc = fpcif_GetIdxByConfigSt("DAPEX_CONN_IP");
    memset(strValue, 0x00, sizeof(strValue));
    strcpy(strValue,(idxc < 0 ? "" : (char *)pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonStr(root,strValue,"config","dapex_conn_ip");

    idxc = fpcif_GetIdxByConfigSt("DAPEX_CONN_PORT");
    nValue = (idxc < 0 ? 0 : atoi(pConfigInfo[idxc].cfvalue));
    fpcif_SetJsonInt(root,nValue,"config","dapex_conn_port");
    idxc = fpcif_GetIdxByConfigSt("AGENT_CPU_LIMIT");
    loop = 0;

    /* AGENT CPU 제한 Config 설정 되어있으면 */
    /* RULE AGENT CPU LIMIT */
    if(idxc > 0)
    {
        if(strlen(cpRuleInfo.agent_str_cpu_limit) > 0)
        {
            if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(cpRuleInfo.agent_str_cpu_limit))+1) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            strcpy(resValue, cpRuleInfo.agent_str_cpu_limit);
            json_object_set_new(json_object_get(root, "rule"), "agent_cpu_limit", json_object());
            tokenCnt = fcom_TokenCnt(resValue, ";");
            if( tokenCnt > 0 )
            {
                tokenKey = strtok_r(resValue, ";",&TempPtr);
                while (tokenKey != NULL)
                {
                    memset(strProcessName, 0x00, sizeof(strProcessName));
                    memset(cpuUsage, 0x00, sizeof(cpuUsage));

                    spos = fcom_GetStringPos(tokenKey,':');
                    if(spos == -1)
                        break;

                    fcom_SubStr(1,spos,tokenKey,strProcessName);
                    fcom_SubStr(spos+2,strlen(tokenKey)-(spos+1),tokenKey,cpuUsage);

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    //Loop Count Set
                    json_object_set_new(
                            json_object_get(json_object_get(root, "rule"),"agent_cpu_limit"),
                            strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(
                            json_object_get(json_object_get(json_object_get( root, "rule"),"agent_cpu_limit"), strLoop),
                            "name", value);
                    json_decref(value);

                    value = json_integer(atoi(cpuUsage));
                    json_object_set(
                            json_object_get(json_object_get(json_object_get( root, "rule"), "agent_cpu_limit"), strLoop),
                            "value", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
            else
            {
                memset(strProcessName, 0x00, sizeof(strProcessName));

                spos = fcom_GetStringPos(resValue,':');
                if(spos > 0)
                {
                    fcom_SubStr(1, spos, resValue, strProcessName);
                    fcom_SubStr(spos+2, strlen(resValue) - (spos+1), resValue, cpuUsage);

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    //Loop Count Set
                    json_object_set_new(
                            json_object_get(json_object_get(root, "rule"), "agent_cpu_limit"),
                            strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get( root, "rule"),
                                            "agent_cpu_limit"), strLoop),
                            "name", value);
                    json_decref(value);

                    value = json_integer(atoi(cpuUsage));
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get( root, "rule"),
                                            "agent_cpu_limit"), strLoop),
                            "value", value);
                    json_decref(value);
                }
            }
            if(spos > 0)
            {
                value = json_string(strLoop);
                json_object_set(
                        json_object_get(
                                json_object_get( root, "rule"),
                                "agent_cpu_limit"),
                        "size", value);
                json_decref(value);

            }
            fcom_MallocFree((void**)&resValue);

        }

    }

    /* CONFIG AGENT CPU LIMIT */
    loop = 0;
    if(idxc > -1)
    {
        if(fcom_malloc((void**)&resValue, sizeof(char)*(strlen(pConfigInfo[idxc].cfvalue)+1)) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        strcpy(resValue, pConfigInfo[idxc].cfvalue);

        json_object_set_new(json_object_get(root, "config"), "agent_cpu_limit", json_object());
        tokenCnt = fcom_TokenCnt(resValue, ";");
        if( tokenCnt > 0 )
        {
            tokenKey = strtok_r(resValue, ";",&TempPtr);
            while (tokenKey != NULL)
            {
                memset(strProcessName,  0x00, sizeof(strProcessName));
                memset(cpuUsage,        0x00, sizeof(cpuUsage));

                spos = fcom_GetStringPos(tokenKey,':');
                if(spos > 0)
                {
                    fcom_SubStr(1,spos,tokenKey,strProcessName);
                    fcom_SubStr(spos+2,strlen(tokenKey)- (spos+1),tokenKey,cpuUsage);

                    loop++;
                    memset(strLoop, 0x00, sizeof(strLoop));
                    sprintf(strLoop, "%d", loop);

                    //Loop Count Set
                    json_object_set_new(
                            json_object_get(
                                    json_object_get(root, "config"),
                                    "agent_cpu_limit"),
                                         strLoop, json_object());

                    value = json_string(strProcessName);
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get( root, "config"),
                                            "agent_cpu_limit"), strLoop),
                                            "name", value);
                    json_decref(value);

                    value = json_integer(atoi(cpuUsage));
                    json_object_set(
                            json_object_get(
                                    json_object_get(
                                            json_object_get( root, "config"),
                                            "agent_cpu_limit"), strLoop),
                            "value", value);
                    json_decref(value);

                    tokenKey = strtok_r(NULL, ";",&TempPtr);
                }
            }
        }
        else
        {
            memset(strProcessName, 0x00, sizeof(strProcessName));

            spos = fcom_GetStringPos(resValue,':');
            if(spos > 0)
            {
                fcom_SubStr(1,spos,resValue,strProcessName);
                fcom_SubStr(spos+2,strlen(resValue)-(spos+1),resValue,cpuUsage);
                loop++;
                memset(strLoop, 0x00, sizeof(strLoop));
                sprintf(strLoop, "%d", loop);

                //Loop Count Set
                json_object_set_new(
                        json_object_get(
                                json_object_get(root, "config"),
                                "agent_cpu_limit"),
                                     strLoop, json_object());

                value = json_string(strProcessName);
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "config"),
                                        "agent_cpu_limit"), strLoop),
                                        "name", value);
                json_decref(value);

                value = json_integer(atoi(cpuUsage));
                json_object_set(
                        json_object_get(
                                json_object_get(
                                        json_object_get( root, "config"),
                                        "agent_cpu_limit"), strLoop),
                                        "value", value);
                json_decref(value);
            }
        }
        if(spos > 0)
        {
            value = json_string(strLoop);
            json_object_set(
                    json_object_get(
                            json_object_get( root, "config"),
                            "agent_cpu_limit"),
                    "size", value);

            json_decref(value);

        }
        fcom_MallocFree((void**)&resValue);
    }

//    /** Agent 프로세스 자기보호 기능 **/
//    memset(strValue, 0x00, sizeof(strValue));
//    idxc = fpcif_GetIdxByConfigSt("AGENT_SELF_PROTECTION");
//    if(idxc > 0)
//    {
//        fpcif_SetJsonInt(root, atoi(pConfigInfo[idxc].cfvalue),"config","agent_self_protection");
//    }
//    else
//    {
//        fpcif_SetJsonInt(root, 0, "config", "agent_self_protection"); /* Default False */
//    }


    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL_IP(p_cpIp,"Fail in generate json dump");
        json_decref(root);
        return -1;
    }

    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_DEBUG_JSON(p_cpIp,"Dump json(%s)", *p_resJson);

    return strlen(*p_resJson);
}


int fpcif_MakeJsonRequestCfgOld(_DAP_AGENT_INFO *p_AI, char** p_resJson, int p_baseReq)
{
    char		*result = NULL;
    json_t		*root = NULL, *value = NULL;

    root = json_object();

    json_object_set_new(root, "rule", json_object());

    // user_key and user_mac
    json_object_set_new(root, "user_key", json_string(p_AI->user_key));
    json_object_set_new(root, "user_mac", json_string(p_AI->user_mac));

    // ru_auto_update
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_auto_update", value);
    json_decref(value);

    // ru_dummy_agent
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_dummy_agent", value);
    json_decref(value);

    // ru_agent_hide
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_agent_hide", value);
    json_decref(value);

    // ru_agent_watcher
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_agent_watcher", value);
    json_decref(value);

    // ru_update_cycle
    value = json_integer(300);
    json_object_set(json_object_get(root, "rule"), "ru_update_cycle", value);
    json_decref(value);

    // ru_net_adapter
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_adapter", value);
    json_decref(value);

    // ru_net_adapter_over
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_adapter_over", value);
    json_decref(value);

    // ru_net_adapter_dupip
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_adapter_dupip", value);
    json_decref(value);

    // ru_net_adapter_dupmac
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_adapter_dupmac", value);
    json_decref(value);

    // ru_net_adapter_mulip
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_adapter_mulip", value);
    json_decref(value);

    // ru_wifi
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_wifi", value);
    json_decref(value);

    // ru_bluetooth
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_bluetooth", value);
    json_decref(value);

    // ru_router
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_router", value);
    json_decref(value);

    // ru_printer
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_printer", value);
    json_decref(value);

    // ru_disk
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk", value);
    json_decref(value);

    // ru_disk_hidden
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk_hidden", value);
    json_decref(value);

    // ru_disk_new
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk_new", value);
    json_decref(value);

    // ru_disk_mobile
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk_mobile", value);
    json_decref(value);

    // ru_disk_mobile_read
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk_mobile_read", value);
    json_decref(value);

    // ru_disk_mobile_write
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_disk_mobile_write", value);
    json_decref(value);

    // ru_net_drive
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_drive", value);
    json_decref(value);

    // ru_net_connection
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_net_connection", value);
    json_decref(value);

    // ru_share_folder
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_share_folder", value);
    json_decref(value);

    // ru_infrared_device
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_infrared_device", value);
    json_decref(value);

    // ru_vritual_machine
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_vritual_machine", value);
    json_decref(value);

    // ru_ext_net_detect_type
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_ext_net_detect_type", value);
    json_decref(value);

    // ru_prt_detect_type
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_prt_detect_type", value);
    json_decref(value);

    // ru_process_detect_type (Agent에서parsing에러로인해'0'그냥넣어줌)
    value = json_integer(p_baseReq);
    json_object_set(json_object_get(root, "rule"), "ru_process_detect_type", value);
    json_decref(value);

    // ru_ip_chain
    value = json_string("");
    json_object_set(json_object_get(root, "rule"), "ru_ip_chain", value);
    json_decref(value);

    // ru_default_gw_mac
    value = json_string("");
    json_object_set(json_object_get(root, "rule"), "ru_default_gw_mac", value);
    json_decref(value);

    // ru_process_detailinfo
    value = json_integer(0);
    json_object_set(json_object_get(root, "rule"), "ru_process_detail_info", value);
    json_decref(value);

    // ru_process_black
    json_object_set_new(json_object_get(root, "rule"), "ru_process_black", json_object());
    value = json_integer(0);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_black"), "size", value);
    json_decref(value);

    // ru_process_white
    json_object_set_new(json_object_get(root, "rule"), "ru_process_white", json_object());
    value = json_integer(0);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_white"), "size", value);
    json_decref(value);

    // ru_process_accessmon
    json_object_set_new(json_object_get(root, "rule"), "ru_process_accessmon", json_object());
    value = json_integer(0);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_process_accessmon"), "size", value);
    json_decref(value);

    // ru_printer_port
    json_object_set_new(json_object_get(root, "rule"), "ru_printer_port", json_object());
    value = json_integer(0);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_printer_port"), "size", value);
    json_decref(value);

    // ru_connect_ext_svr
    json_object_set_new(json_object_get(root, "rule"), "ru_connect_ext_svr", json_object());
    value = json_integer(0);
    json_object_set(json_object_get(json_object_get(root, "rule"), "ru_connect_ext_svr"), "size", value);
    json_decref(value);

    json_object_set_new(root, "config", json_object());

    // agent_listen_port
    value = json_integer(0);
    json_object_set(json_object_get(root, "config"), "agent_listen_port", value);
    json_decref(value);

    if( strncmp(p_AI->agent_ver, "1.1.2.2", 7) <= 0 )
    {
        value = json_string("");
        json_object_set(json_object_get(root, "config"), "external_svr_addr", value);
        json_decref(value);
    }
    // last_agent_ver
    value = json_string("");
    json_object_set(json_object_get(root, "config"), "last_agent_ver", value);
    json_decref(value);

    // update_server
    value = json_string("");
    json_object_set(json_object_get(root, "config"), "update_server", value);
    json_decref(value);

    // dapex_conn_ip
    value = json_string("");
    json_object_set(json_object_get(root, "config"), "dapex_conn_ip", value);
    json_decref(value);

    // dapex_conn_port
    value = json_integer(0);
    json_object_set(json_object_get(root, "config"), "dapex_conn_port", value);
    json_decref(value);

    // agent_boot_delay
    value = json_integer(0);
    json_object_set(json_object_get(root, "config"), "agent_boot_delay", value);
    json_decref(value);

    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in generate json dump");
        return -1;
    }

    *p_resJson = result;
    json_decref(root);
    result = NULL;

    WRITE_INFO(CATEGORY_DEBUG,"Return resJson(%s)", *p_resJson);

    return strlen(*p_resJson);
}