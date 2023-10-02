//
// Created by KimByoungGook on 2020-07-15.
//
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "com/dap_req.h"
#include "com/dap_def.h"
#include "json/dap_json.h"
#include "sock/dap_sock.h"
#include "secure/dap_secure.h"
#include "pcif.h"

int fpcif_ParseReqShutdown(char *logip, json_t *p_element, char* p_managerId)
{
    int			idx     = 0;
    int			tmpType = 0;
    const char	*tmpManagerId = NULL;


    json_unpack(p_element, "{s:s, s:i}",
                "manager_id", &tmpManagerId, "type", &tmpType
    );

    if(tmpManagerId != NULL)
    {
        strcpy(p_managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s'", p_managerId);
    WRITE_DEBUG_IP(logip, "Unpack type: %d", tmpType);

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_managerId);
    if(idx < 0)
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s)", p_managerId);
        return -1;
    }

    if( tmpType < 0 || tmpType > 1 )
    {
        WRITE_CRITICAL_IP(logip, "Invalid shutdown id(%s)type(%d)", p_managerId,tmpType);
        return -1;
    }

    return tmpType;
}


int fpcif_ParseReqLogin(
        int	sock,
        json_t*	p_element,
        _DAP_MANAGER_INFO* p_ManagerInfo,
        char*	p_cpIp,
        int		p_msgCode,
        int		p_fqNum)
{
    const char	*tmpManagerVer  = NULL;
    const char	*tmpManagerMac  = NULL;
    const char	*tmpUserId      = NULL;
    const char	*tmpUserPw      = NULL;
    char		*resData        = NULL;
    char		*jsonData       = NULL;
    char		arrManagerMac[128 +1]  = {0x00,};
    char		strTokIp[15+1]      = {0x00,};
    char		strTokMac[17+1]     = {0x00,};
    int			jsonSize     = 0;
    int			decryptoSize = 0;
    int			rxt          = 0;
    int			result       = 0;
    int			userLevel    = 0;
    int			tokenCnt     = 0;
    int			pos          = 0;

    json_unpack(p_element,  "{s:{s:s, s:s, s:s, s:s}}", "certi_info",
                "manager_ver", &tmpManagerVer,
                "manager_mac", &tmpManagerMac,
                "id", &tmpUserId,
                "pw", &tmpUserPw);

    if(tmpManagerVer != NULL)
    {
        strcpy(p_ManagerInfo->manager_ver, tmpManagerVer);
        tmpManagerVer = NULL;
    }
    if(tmpManagerMac != NULL)
    {
        strcpy(p_ManagerInfo->manager_mac, tmpManagerMac);
        tmpManagerMac = NULL;
    }
    if(tmpUserId != NULL)
    {
        fsec_DecryptStr((char*)tmpUserId, p_ManagerInfo->manager_id, &decryptoSize);
        tmpUserId = NULL;
    }
    if(tmpUserPw != NULL)
    {
        fsec_DecryptStr((char*)tmpUserPw, p_ManagerInfo->manager_pw, &decryptoSize);
        tmpUserPw = NULL;
    }

    WRITE_DEBUG_IP(p_cpIp, "Unpack manager_ver: '%s' ", p_ManagerInfo->manager_ver );
    WRITE_DEBUG_IP(p_cpIp, "Unpack manager_mac: '%s' ", p_ManagerInfo->manager_mac);
    WRITE_DEBUG_IP(p_cpIp, "Unpack manager_id: '%s' ", p_ManagerInfo->manager_id);

    // 초기화
    memset(arrManagerMac, 0x00, sizeof(arrManagerMac));

    strcpy(arrManagerMac, p_ManagerInfo->manager_mac);

    tokenCnt = fcom_TokenCnt(arrManagerMac, ",");
    if (tokenCnt > 0)
    {
        WRITE_DEBUG_IP(p_cpIp, "Fail in multiple ip or mac(%s) ", arrManagerMac );
        return -1;
    }

    if( strstr(arrManagerMac, "^") != NULL)
    {
        memset(strTokIp, 0x00, sizeof(strTokIp));
        memset(strTokMac, 0x00, sizeof(strTokMac));

        pos = fcom_GetStringPos(arrManagerMac, '^');
        fcom_SubStr(1, pos, arrManagerMac, strTokIp);
        fcom_SubStr(pos+2, strlen(arrManagerMac), arrManagerMac, strTokMac);
        strcpy(p_ManagerInfo->manager_mac, strTokMac);
        strcpy(p_ManagerInfo->manager_ip, strTokIp);
        WRITE_DEBUG_IP(p_cpIp, "Set address, mac(%s)ip(%s) ",
                       p_ManagerInfo->manager_mac,
                       p_ManagerInfo->manager_ip);
    }
    else // If there's only ip and there's no Mac
    {
        WRITE_DEBUG_IP(p_cpIp, "Invalid format from manager, arrManagerMac(%s) ",
                       arrManagerMac);
    }

    if(strlen(p_ManagerInfo->manager_mac) < 17)
    {
        WRITE_DEBUG_IP(p_cpIp, "No match mac address " );
    }

    if(strlen(p_ManagerInfo->manager_ip) < 7)
    {
        strcpy(p_ManagerInfo->manager_ip, p_cpIp);
        WRITE_DEBUG_IP(p_cpIp, "Force manager access ip(%s), because there is no ip ",
                       p_ManagerInfo->manager_ip);
    }

    // set socket ip
    strcpy(p_ManagerInfo->sock_ip, p_cpIp);

    WRITE_DEBUG_IP(p_cpIp, "Get ipaddr, real(%s)sock(%s) ",
                   p_ManagerInfo->manager_ip,
                   p_ManagerInfo->sock_ip);

    result = fpcif_CheckLoginManagerSt(p_ManagerInfo->manager_id,
                                       p_ManagerInfo->manager_pw,
                                       &userLevel,
                                       p_ManagerInfo->manager_ip,
                                       p_msgCode,
                                       p_fqNum);

    if(result < 0)
    {
        WRITE_CRITICAL_IP(p_cpIp, "Fail in check_login_manager_st, user(%s)ip(%s)result(%d) ",
                          p_ManagerInfo->manager_id,
                          p_cpIp,
                          result);
        if(result == -1) //Fail in query
        {
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(p_cpIp, "Fail in send ack(%s)", DAP_MANAGE);
                return -1;
            }
            return -1;
        }
        else if(result == -2) //MANAGE_LOGIN_DUPLICATE_USER(94)
        {
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_LOGIN_DUPLICATE_USER,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
        }
        else if(result == -3) //MANAGE_LOGIN_INVALID_IP(95)
        {
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_LOGIN_INVALID_IP,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
        }
        else if(result == -4) //MANAGE_LOGIN_INVALID_PASSWORD(96)
        {
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_LOGIN_INVALID_PASSWORD,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
        }
        else if(result == -5) //MANAGE_LOGIN_OVER_FAILCOUNT(97)
        {
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_LOGIN_OVER_FAILCOUNT,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
        }
        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(p_cpIp, "Fail in send ack(%s) ", DAP_MANAGE);
            return -1;
        }
    }
    else if(result == 0) //MANAGE_LOGIN_NOTFOUND_USER(93)
    {
        WRITE_DEBUG_IP(p_cpIp, "Not found user");
        rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_LOGIN_NOTFOUND_USER,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(p_cpIp, "Fail in send ack(%s) ", DAP_MANAGE);
            return -1;
        }
    }
    else
    {
        rxt = fpcif_MakeJsonManagerInfo(p_cpIp, userLevel, &resData, p_ManagerInfo->manager_ver);
        if(rxt <= 0)
        {
            WRITE_CRITICAL_IP(p_cpIp, "Fail in make_json_manager_info ");

            fcom_MallocFree((void**)&resData);
            rxt = fsock_SendAck(sock, DAP_MANAGE, MANAGE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(p_cpIp, "Fail in send_ack(%s) ", DAP_MANAGE);
                return -1;
            }
        }

        jsonSize = rxt;

        if(fcom_malloc((void**)&jsonData, sizeof(char)*jsonSize) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }
        memcpy(jsonData, resData, sizeof(char)*jsonSize);

        fcom_MallocFree((void**)&resData);

        rxt = fsock_SendAckJson(sock, DAP_MANAGE, MANAGE_RTN_SUCCESS, jsonData, jsonSize, p_cpIp,
                                g_stProcPcifInfo.cfgRetryAckCount, g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
            fcom_MallocFree((void**)&jsonData);
            return -1;
        }

        fcom_MallocFree((void**)&jsonData);
    }

    return result;
}

int fpcif_ParseReqAgentLog(char *logip, json_t *p_element, char *p_agentIp, char** p_resJson)
{
    int			idx = 0;
    int			tmpFileType = 0;
    const char	*tmpManagerId   = NULL;
    const char	*tmpBeginDate   = NULL;
    const char	*tmpEndDate     = NULL;
    const char	*tmpAgentIp     = NULL;
    char		*result         = NULL;
    json_t  	*root           = NULL;

    char		managerId[30]   = {0x00,};
    char		beginDate[10+1] = {0x00,};
    char		endDate[10+1]   = {0x00,};

    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:i}",
                "manager_id", &tmpManagerId,
                "begin_date", &tmpBeginDate,
                "end_date", &tmpEndDate,
                "agent_ip", &tmpAgentIp,
                "file_type", &tmpFileType
    );

    memset(managerId, 0x00, sizeof(managerId));
    memset(beginDate, 0x00, sizeof(beginDate));
    memset(endDate  , 0x00, sizeof(endDate));

    if(tmpManagerId != NULL)
    {
        strcpy(managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpBeginDate != NULL)
    {
        strcpy(beginDate, tmpBeginDate);
        tmpBeginDate = NULL;
    }
    if(tmpEndDate != NULL)
    {
        strcpy(endDate, tmpEndDate);
        tmpEndDate = NULL;
    }
    if(tmpAgentIp != NULL)
    {
        strcpy(p_agentIp, tmpAgentIp);
        tmpAgentIp = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' ", managerId );
    WRITE_DEBUG_IP(logip, "Unpack begin_date: '%s' ", beginDate);
    WRITE_DEBUG_IP(logip, "Unpack end_date: '%s' ", endDate);
    WRITE_DEBUG_IP(logip, "Unpack agent_ip: '%s' ", p_agentIp);
    WRITE_DEBUG_IP(logip, "Unpack file_type: %d ", tmpFileType);

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(managerId);
    if(idx < 0)
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s)", managerId);
        return -1;
    }

    if(strlen(beginDate) != 10) //ex) 2018-12-04 or 2018-12-20
    {
        WRITE_DEBUG_IP(logip, "Invalid begin_date(%s) ", beginDate );
        return -1;
    }
    if(strlen(endDate) != 10)
    {
        WRITE_DEBUG_IP(logip, "Invalid end_date(%s) ", endDate );
        return -1;
    }
    if(strlen(p_agentIp) < 7)
    {
        WRITE_DEBUG_IP(logip, "Invalid agent_ip(%s) ", p_agentIp );
        return -1;
    }
    if(tmpFileType < 0 && tmpFileType > 1)
    {
        WRITE_DEBUG_IP(logip, "Invalid file_type(%d) ", tmpFileType );
        return -1;
    }

    root = json_object();
    json_object_set_new(root, "begin_date", json_string(beginDate));
    json_object_set_new(root, "end_date", json_string(endDate));
    json_object_set_new(root, "file_type", json_integer(tmpFileType));

    result = json_dumps(root, JSON_INDENT(0));
    if(!result)
    {
        WRITE_CRITICAL_IP(logip, "Fail in generate json logfile");
        json_decref(root);
        return -1;
    }
    *p_resJson = result;
    json_decref(root);
    result = NULL;


    WRITE_DEBUG_JSON(logip, "Dump json(%s) ", *p_resJson);

    return strlen(*p_resJson);
}

int fpcif_ParseReqServerTail(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerId,
        char**	resFile)
{
    int			idx = 0;
    const char	*tmpManagerId = NULL;
    const char	*tmpFile      = NULL;

    json_unpack(p_element, "{s:s, s:s}",
                "manager_id", &tmpManagerId,
                "file_name", &tmpFile);

    if(tmpManagerId != NULL)
    {
        strcpy(p_managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpFile != NULL)
    {
        *resFile = (char*)tmpFile;
        tmpFile = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' ", p_managerId );
    WRITE_DEBUG_IP(logip, "Unpack file_name: '%s' ", *resFile );

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_managerId);
    if( idx < 0 )
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s)", p_managerId);
        return -1;
    }

    if( strlen(*resFile) < 2 )
    {
        WRITE_DEBUG_IP(logip, "Invalid file_name(%s) ", *resFile );
        return -1;
    }

    return 0;
}

int fpcif_ParseServerLoglist(
        char	*logip,
        json_t 	*p_element,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	**resFindIp
)
{
    int			idx = 0;
    const char	*tmpManagerId   = NULL;
    const char	*tmpBeginDate   = NULL;
    const char	*tmpEndDate     = NULL;
    const char	*tmpCategory    = NULL;
    const char	*tmpFindIp      = NULL;

    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:s}",
                "manager_id", &tmpManagerId,
                "begin_date", &tmpBeginDate,
                "end_date", &tmpEndDate,
                "category", &tmpCategory,
                "find_ip", &tmpFindIp
    );

    if(tmpManagerId != NULL)
    {
        strcpy(p_managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpBeginDate != NULL)
    {
        strcpy(p_beginDate, tmpBeginDate);
        tmpBeginDate = NULL;
    }
    if(tmpEndDate != NULL)
    {
        strcpy(p_endDate, tmpEndDate);
        tmpEndDate = NULL;
    }
    if(tmpCategory != NULL)
    {
        strcpy(p_category, tmpCategory);
        tmpCategory = NULL;
    }
    if(tmpFindIp != NULL)
    {
        *resFindIp = (char*)tmpFindIp;
        tmpFindIp = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' ", p_managerId );
    WRITE_DEBUG_IP(logip, "Unpack begin_date: '%s' ", p_beginDate);
    WRITE_DEBUG_IP(logip, "Unpack end_date: '%s' ", p_endDate);
    WRITE_DEBUG_IP(logip, "Unpack category: '%s' ", p_category);
    WRITE_DEBUG_IP(logip, "Unpack find_ip: '%s' ", *resFindIp);

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_managerId);
    if( idx < 0 )
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s)", p_managerId);
        return -1;
    }

    if( strlen(p_beginDate) != 10 ) //ex) 2018-12-04 or 2018-12-20
    {
        WRITE_DEBUG_IP(logip, "Invalid begin_date(%s) ", p_beginDate );
        return -1;
    }
    if( strlen(p_endDate) != 10 )
    {
        WRITE_DEBUG_IP(logip, "Invalid end_date(%s) ", p_endDate);
        return -1;
    }
    if( strlen(p_category) < 2 )
    {
        WRITE_DEBUG_IP(logip, "Invalid category(%s) ", p_category);
        return -1;
    }
    if( (strstr(p_category, "all") != NULL ||
         strstr(p_category, "pcif") != NULL) &&
        strlen(*resFindIp) < 7 )
    {
        WRITE_DEBUG_IP(logip, "Invalid find_ip(%s) ", *resFindIp);
        return -1;
    }

    return 0;
}

int fpcif_ParseUrgentReq(int sock, char *p_cpIp, json_t *p_element, _DAP_AGENT_INFO* p_AgentInfo)
{
    char arrUserMac[128 +1] = {0x00,};
    char strTokMac[17 +1]   = {0x00,};
    char strTokIp[15 +1]    = {0x00,};
    const char* tmpAgentVer = NULL;
    const char* tmpUserKey  = NULL;
    const char* tmpUserMac  = NULL;
    const char* tmpUserSno  = NULL;
    char* jsonData          = NULL;
    char* resData           = NULL;
    unsigned long long tmpUserSeq = 0;
    int tokenCnt = 0;
    int jsonSize = 0;
    int pos      = -1;
    int rxt      = 0;

    /* Json Unpack */
    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:I}",
                "agent_ver", &tmpAgentVer,
                "user_key", &tmpUserKey,
                "user_mac", &tmpUserMac,
                "user_sno", &tmpUserSno,
                "user_seq", &tmpUserSeq);
    if (tmpAgentVer != NULL)
    {
        strcpy(p_AgentInfo->agent_ver, tmpAgentVer);
        tmpAgentVer = NULL;
    }
    if (tmpUserKey != NULL)
    {
        strcpy(p_AgentInfo->user_key, tmpUserKey);
        tmpUserKey = NULL;
    }
    if (tmpUserMac != NULL)
    {
        strcpy(p_AgentInfo->user_mac, tmpUserMac);
        tmpUserMac = NULL;
    }
    if (tmpUserSno != NULL)
    {
        strcpy(p_AgentInfo->user_sno, tmpUserSno);
        tmpUserSno = NULL;
    }
    p_AgentInfo->user_seq = tmpUserSeq;

    WRITE_DEBUG_IP(p_cpIp,"Urgent Patch Unpack agent_ver: '%s ",p_AgentInfo->agent_ver);
    WRITE_DEBUG_IP(p_cpIp,"Urgent Patch Unpack user_key: '%s' ",p_AgentInfo->user_key );
    WRITE_DEBUG_IP(p_cpIp,"Urgent Patch Unpack user_mac: '%s' ",p_AgentInfo->user_mac );
    WRITE_DEBUG_IP(p_cpIp,"Urgent Patch Unpack user_sno: '%s' ",p_AgentInfo->user_sno );
    WRITE_DEBUG_IP(p_cpIp,"Urgent Patch Unpack user_seq: %llu ",p_AgentInfo->user_seq );

    memset(arrUserMac, 0x00, sizeof(arrUserMac));

    /* AGENT MAC 주소 Copy */
    /* 10.20.20.92^6C:4B:90:39:CD:3E */
    strcpy(arrUserMac, p_AgentInfo->user_mac);
    memset(p_AgentInfo->user_mac, 0x00, sizeof(p_AgentInfo->user_mac));

    tokenCnt = fcom_TokenCnt(arrUserMac, ",");
    if (tokenCnt > 0)
    {
        WRITE_CRITICAL_IP(p_cpIp,"Fail in multiple ip or mac(%s) ",arrUserMac );
        return -1;
    }

    if (strstr(arrUserMac, "^") != NULL)
    {
        memset(strTokIp, 0x00, sizeof(strTokIp));
        memset(strTokMac, 0x00, sizeof(strTokMac));
        pos = fcom_GetStringPos(arrUserMac, '^');
        fcom_SubStr(1, pos, arrUserMac, strTokIp);
        fcom_SubStr(pos + 2, strlen(arrUserMac), arrUserMac, strTokMac);
        strcpy(p_AgentInfo->user_mac, strTokMac);
        strcpy(p_AgentInfo->user_ip, strTokIp);

        WRITE_DEBUG_IP(p_cpIp,"Set address, mac(%s)ip(%s) ",p_AgentInfo->user_mac, p_AgentInfo->user_ip );
    }

    if (strlen(p_AgentInfo->user_mac) < 17)
    {
        WRITE_DEBUG_IP(p_cpIp,"No match mac address " );
    }
    // set socket ip
    strcpy(p_AgentInfo->sock_ip, p_cpIp);

    WRITE_INFO_IP(p_cpIp,"Get ipaddr, real(%s)sock(%s) ",p_AgentInfo->user_ip, p_AgentInfo->sock_ip);

    if ( strlen(p_AgentInfo->user_ip) <= 0){
        WRITE_INFO_IP(p_cpIp,"Urgent User IP Is Null (%s) ", p_AgentInfo->user_ip);
        rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        return (-1);
    }

    rxt = fpcif_MakeJsonUrgentReq(p_AgentInfo, p_cpIp, &resData);
    if (rxt <= 0)
    {
        WRITE_CRITICAL_IP(p_cpIp,"Fail in make_request_cfg" );
        fcom_MallocFree((void**)&resData); // json_dump한것은 free()
        rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if (rxt < 0)
        {
            WRITE_CRITICAL_IP(p_cpIp,"Fail in send_ack(%s) ",DAP_AGENT );
        }
        return -1;
    }
    jsonSize = rxt + 1;
    if(fcom_malloc((void**)&jsonData, sizeof(char) * jsonSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    strcpy(jsonData, resData);
    fcom_MallocFree((void**)&resData);

    rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_SUCCESS, jsonData, jsonSize, p_cpIp,
                            g_stProcPcifInfo.cfgRetryAckCount, g_stProcPcifInfo.retryAckSleep);

    if (rxt < 0)
    {
        WRITE_CRITICAL_IP(p_cpIp,"Fail in send_ack_json(%s)", DAP_AGENT );
        fcom_MallocFree((void**)&jsonData);
        return -2;
    }

    fcom_MallocFree((void**)&jsonData);

    return 0;
}

int fpcif_ParseServiceStatus(int sock,
                             char*              param_CpIp,
                             json_t*            param_Element,
                             _DAP_AGENT_INFO*   param_AgentInfo,
                             char*              param_ServiceStatus,
                             int                thrNum)
{
    const char	*tmpUserKey         = NULL;
    const char	*tmpAgentStatus     = NULL;

    json_unpack(param_Element, "{s:s, s:s}",
                "user_key", 	&tmpUserKey,
                "status"  ,		&tmpAgentStatus);

    if(tmpUserKey != NULL)
    {
        snprintf(param_AgentInfo->user_key, sizeof(param_AgentInfo->user_key), "%s", tmpUserKey);
        tmpUserKey = NULL;
    }

    if(tmpAgentStatus != NULL)
    {
        strcpy(param_ServiceStatus, tmpAgentStatus);
    }

    if( strlen(param_AgentInfo->user_key) <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User Key Is Null Recv (%s) ", param_AgentInfo->user_key);
        return (-1);
    }

    WRITE_DEBUG_IP(param_CpIp, "Unpack Service Status User Key: '%s' ",     param_AgentInfo->user_key);
    WRITE_DEBUG_IP(param_CpIp, "Unpack Service Status Agent Status: %llu ", param_AgentInfo->user_seq);

    return 0;

}

int fpcif_ParseRequestAgentLog( char* param_CpIp,
                               json_t* param_Element,
                               _DAP_AGENTLOG_INFO* param_AgentLogInfo)
{
    int                 local_nBaseIdx      = 0;
    int                 local_loop_count = 0;
    unsigned long long  local_nHbSq        = 0;
    const char* local_TmpUserKey    = NULL;
    const char* local_TmpUserIp     = NULL;
    const char* local_TmpProcess    = NULL;
    const char* local_TmpLogDate    = NULL;
    const char* local_TmpLogLevel   = NULL;
    const char* local_TmpLogMsg     = NULL;

    // Json Unpack
    json_unpack(param_Element, "{s:s, s:s, s:s, s:s, s:s, s:s}",
                "user_key",	 &local_TmpUserKey,
                "user_ip", 	 &local_TmpUserIp,
                "process", 	 &local_TmpProcess,
                "log_date",  &local_TmpLogDate,
                "log_level", &local_TmpLogLevel,
                "log_msg",   &local_TmpLogMsg);

    if(local_TmpUserKey != NULL)
    {
        snprintf(param_AgentLogInfo->user_key, sizeof(param_AgentLogInfo->user_key), "%s", local_TmpUserKey);
        local_TmpUserKey = NULL;
    }

    if(local_TmpUserIp != NULL)
    {
        snprintf(param_AgentLogInfo->user_ip, sizeof(param_AgentLogInfo->user_ip), "%s", local_TmpUserIp);
        local_TmpUserIp = NULL;
    }

    if(local_TmpProcess != NULL)
    {
        snprintf(param_AgentLogInfo->process, sizeof(param_AgentLogInfo->process), "%s", local_TmpProcess);
        local_TmpProcess = NULL;
    }

    if(local_TmpLogDate != NULL)
    {
        snprintf(param_AgentLogInfo->log_date, sizeof(param_AgentLogInfo->log_date), "%s", local_TmpLogDate);
        local_TmpLogDate = NULL;
    }

    if(local_TmpLogLevel != NULL)
    {
        snprintf(param_AgentLogInfo->log_level, sizeof(param_AgentLogInfo->log_level), "%s", local_TmpLogLevel);
        local_TmpLogLevel = NULL;
    }

    if(local_TmpLogMsg != NULL)
    {
        snprintf(param_AgentLogInfo->log_msg, sizeof(param_AgentLogInfo->log_msg), "%s", local_TmpLogMsg);
        local_TmpLogMsg = NULL;
    }

    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy User_Key: '%s' ",     param_AgentLogInfo->user_key    );
    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy User_Ip: '%s' ",      param_AgentLogInfo->user_ip     );
    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy Process: '%s' ",      param_AgentLogInfo->process     );
    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy Log_Date: '%s' ",     param_AgentLogInfo->log_date    );
    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy Log_Level: %llu ",    param_AgentLogInfo->log_level   );
    WRITE_DEBUG_IP(param_CpIp, "Unpack Policy LogMsg: %s ",         param_AgentLogInfo->log_msg     );

    /** Get HW_BASE_TB HBSQ(장치번호) **/
    while(1)
    {
        if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
        {
            local_nBaseIdx = fpcif_GetIdxByBaseKey(param_AgentLogInfo->user_key, 0);
            local_nHbSq = pBaseInfo[local_nBaseIdx].hb_sq;
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
            return (-1);
        }
    }

    snprintf(param_AgentLogInfo->hb_sq, sizeof(param_AgentLogInfo->hb_sq), "%llu", local_nHbSq);

    return 0;

}
int fpcif_ParseRequestCfg(int sock,
                          char *p_cpIp,
                          json_t *p_element,
                          _DAP_AGENT_INFO* p_AgentInfo,
                          int thrNum)
{
    int         local_loop_count = 0;
    int			idx      = -1;
    int			idxU     = -1;
    int			pos      = -1;
    int			jsonSize = 0;
    int			rxt      = 0;
    int			tokenCnt = 0;
    int			bBaseReq = 0;
    unsigned long long tmpUserSeq = 0;
    unsigned char local_cHbDel      = 0;
    char		clientMac[17 +1]    = {0x00,};
    char		arrUserMac[128 +1]  = {0x00,};
    char		strTokMac[17 +1]    = {0x00,};
    char		strTokIp[15 +1]     = {0x00,};
    const char	*tmpAgentVer    = NULL;
    const char	*tmpUserKey     = NULL;
    const char	*tmpUserMac     = NULL;
    const char	*tmpUserSno     = NULL;
    char		*resData        = NULL;
    char		*jsonData       = NULL;

    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:I}",
                "agent_ver",	&tmpAgentVer,
                "user_key", 	&tmpUserKey,
                "user_mac", 	&tmpUserMac,
                "user_sno", 	&tmpUserSno,
                "user_seq",		&tmpUserSeq);

    if(tmpAgentVer != NULL)
    {
        snprintf(p_AgentInfo->agent_ver, sizeof(p_AgentInfo->agent_ver), "%s", tmpAgentVer);
        tmpAgentVer = NULL;
    }
    if(tmpUserKey != NULL)
    {
        snprintf(p_AgentInfo->user_key, sizeof(p_AgentInfo->user_key), "%s", tmpUserKey);
        tmpUserKey = NULL;
    }
    if(tmpUserMac != NULL)
    {
        snprintf(p_AgentInfo->user_mac, sizeof(p_AgentInfo->user_mac), "%s", tmpUserMac);
        tmpUserMac = NULL;
    }
    if(tmpUserSno != NULL)
    {
        snprintf(p_AgentInfo->user_sno, sizeof(p_AgentInfo->user_sno), "%s", tmpUserSno);
        tmpUserSno = NULL;
    }
    p_AgentInfo->user_seq = tmpUserSeq;

    WRITE_DEBUG_IP(p_cpIp, "Unpack Policy agent_ver: '%s' ", p_AgentInfo->agent_ver);
    WRITE_DEBUG_IP(p_cpIp, "Unpack Policy user_key: '%s' ", p_AgentInfo->user_key);
    WRITE_DEBUG_IP(p_cpIp, "Unpack Policy user_mac: '%s' ", p_AgentInfo->user_mac);
    WRITE_DEBUG_IP(p_cpIp, "Unpack Policy user_sno: '%s' ", p_AgentInfo->user_sno);
    WRITE_DEBUG_IP(p_cpIp, "Unpack Policy user_seq: %llu ", p_AgentInfo->user_seq);

    /*
     *	20181128 기존 ip^mac 복수개가 들어오던것이, ip^mac 1개만 들어오며,
     *	user_ip = ip, user_mac = mac 대입만 하면 됨
     */
    memset(arrUserMac, 0x00, sizeof(arrUserMac));
    snprintf(arrUserMac, sizeof(arrUserMac), "%s", p_AgentInfo->user_mac);

    //p_AgentInfo->user_mac은 db에 넣어야하므로 초기화하고
    //매칭되는 mac이 없으면 null 있으면 1개의 mac주소가 들어간다.
    //초기화
    memset(p_AgentInfo->user_mac, 0x00, sizeof(p_AgentInfo->user_mac));

    tokenCnt = fcom_TokenCnt(arrUserMac, ",");
    if (tokenCnt > 0)
    {
        WRITE_DEBUG_IP(p_cpIp, "Fail in multiple ip or mac(%s) ", arrUserMac );
        return -1;
    }

    if(g_stProcPcifInfo.cfgUseGetArpMac == 0)
        goto PASSARP;

    //check user_mac
    memset(clientMac, 0x00, sizeof(clientMac));
    fsock_GetMacAddress(p_cpIp, NULL, clientMac);
    WRITE_DEBUG_IP(p_cpIp, "Get real mac address(%s) ", clientMac );

    if(strlen(clientMac) > 16) //arp 던져서 구한 clientMac이 있다면
    {
        WRITE_DEBUG_IP(p_cpIp, "Succeed to obtain real mac(%s), tokenCnt(%d) ",clientMac, tokenCnt );

        if( strstr(arrUserMac, "^") != NULL)
        {
            memset(strTokIp, 0x00, sizeof(strTokIp));
            memset(strTokMac, 0x00, sizeof(strTokMac));
            pos = fcom_GetStringPos(arrUserMac, '^');
            fcom_SubStr(1, pos, arrUserMac, strTokIp);
            fcom_SubStr(pos+2, strlen(arrUserMac), arrUserMac, strTokMac);
            snprintf( p_AgentInfo->user_mac, sizeof(p_AgentInfo->user_mac), "%s", strTokMac );
            snprintf(p_AgentInfo->user_ip,   sizeof(p_AgentInfo->user_ip), "%s", strTokIp);
            WRITE_DEBUG_IP(p_cpIp, "Match mac address, mac(%s)ip(%s)",
                           p_AgentInfo->user_mac,
                           p_AgentInfo->user_ip );
        }
        else // If there's only Mac and there's no ip
        {
            if(strcmp(arrUserMac, "") != 0)
            {
                if(!strncasecmp(clientMac, arrUserMac, 17))
                {
                    snprintf(p_AgentInfo->user_mac, sizeof(p_AgentInfo->user_mac), "%s", clientMac);
                    snprintf(p_AgentInfo->user_ip, sizeof(p_AgentInfo->user_ip), "%s", p_cpIp);
                    WRITE_DEBUG_IP(p_cpIp, "Match mac address, mac(%s)ip(%s) ",
                                   p_AgentInfo->user_mac,p_AgentInfo->user_ip );
                }
            }
            else //가끔 Agent에서 빈값으로 올경우 구해온mac을 넣는다.
            {
                snprintf(p_AgentInfo->user_mac, sizeof(p_AgentInfo->user_mac), "%s", clientMac);
                WRITE_DEBUG_IP(p_cpIp, "Force set real mac(%s), because null from agent ",
                               p_AgentInfo->user_mac );
            }
        }
    }
    else
    {
        WRITE_DEBUG_IP(p_cpIp, "Can't get real mac(%s), tokenCnt(%d) ", clientMac, tokenCnt );

PASSARP:
        if( strstr(arrUserMac, "^") != NULL)
        {
            memset(strTokIp, 0x00, sizeof(strTokIp));
            memset(strTokMac, 0x00, sizeof(strTokMac));
            pos = fcom_GetStringPos(arrUserMac, '^');
            fcom_SubStr(1, pos, arrUserMac, strTokIp);
            fcom_SubStr(pos+2, strlen(arrUserMac), arrUserMac, strTokMac);
            snprintf(p_AgentInfo->user_mac, sizeof(p_AgentInfo->user_mac), "%s", strTokMac);
            snprintf( p_AgentInfo->user_ip, sizeof(p_AgentInfo->user_ip), "%s", strTokIp);
            WRITE_DEBUG_IP(p_cpIp, "Set address, mac(%s)ip(%s)",
                           p_AgentInfo->user_mac,
                           p_AgentInfo->user_ip );
        }
        else // If there's only ip and there's no Mac
        {
            WRITE_DEBUG_IP(p_cpIp, "Invalid format from agent, arrUserMac(%s)", arrUserMac );
        }
    }

    if(strlen(p_AgentInfo->user_mac) < 17)
    {
        WRITE_DEBUG_IP(p_cpIp, "No match mac address " );
    }

    if(strlen(p_AgentInfo->user_ip) < 7)
    {
        snprintf(p_AgentInfo->user_ip, sizeof(p_AgentInfo->user_ip), "%s", p_cpIp);
        WRITE_DEBUG_IP(p_cpIp,	"Force agent access ip(%s), because there is no ip ",
                       p_AgentInfo->user_ip);
    }

    g_stThread_arg[thrNum].threadStepStatus = 16;

    // 여기서 user_key가 unknown으로 들어왔을때
    // ip, mac으로 HW_BASE_TB에 기존값이 있는지 확인
    if(strcmp(p_AgentInfo->user_key, "unknown") == 0)
    {
        memset(p_AgentInfo->user_key, 0x00, sizeof(p_AgentInfo->user_key));

        if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
        {
            while(1)
            {
                if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                {
                    idx = fpcif_GetIdxByBaseIp(p_AgentInfo->user_ip);
                    pthread_rwlock_unlock(&mutexpolicy);
                    break;
                }
                else
                {
                    fcom_SleepWait(5);
                    local_loop_count++;
                }

                if( local_loop_count >=  20 )
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                    break;
                }
            }
        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",4 );
            return (-1);
        }

        g_stThread_arg[thrNum].threadStepStatus = 17;
        if (idx < 0)
        {
            fcom_GetUniqId(p_AgentInfo->user_key);

            WRITE_DEBUG_IP(p_cpIp, "Issue a key(%s) because the data doesn't exist, idx(%d) ",
                           p_AgentInfo->user_key, idx);
        }
        else
        {
            if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
            {
                while(1)
                {
                    if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                    {
                        snprintf( p_AgentInfo->user_key, sizeof(p_AgentInfo->user_key), "%s", pBaseInfo[idx].hb_unq);

                        WRITE_DEBUG_IP(p_cpIp, "Found a key(%s) in existing data, idx(%d)",
                                       p_AgentInfo->user_key,
                                       idx);

                        pthread_rwlock_unlock(&mutexpolicy);
                        break;
                    }
                    else
                    {
                        fcom_SleepWait(5);
                        local_loop_count++;
                    }

                    if( local_loop_count >=  20 )
                    {
                        WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                        break;
                    }
                }
            }
            else
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",2 );
                return (-1);
            }

        }
        bBaseReq = 1;
    }
    else // UserKey가 존재하는 경우
    {
        if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
        {
            while(1)
            {
                if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                {
                    idx = fpcif_GetIdxByBaseKey(p_AgentInfo->user_key, 0);
                    pthread_rwlock_unlock(&mutexpolicy);
                    break;
                }
                else
                {
                    fcom_SleepWait(5);
                    local_loop_count++;
                }

                if( local_loop_count >=  20 )
                {
                    WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                    break;
                }
            }
        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",2 );
            return (-1);
        }


        if(idx == -1)
        {
            char szTmpKey[20 +1] = {0x00,};
            snprintf(szTmpKey, sizeof(szTmpKey), "%s", p_AgentInfo->user_key );

            bBaseReq = 1;

            fcom_GetUniqId(p_AgentInfo->user_key);
            WRITE_DEBUG(CATEGORY_DEBUG,"Request Key Is Not Exist (%s) Issue A Key (%s) ",
                        szTmpKey, p_AgentInfo->user_key);
        }
    }

    g_stThread_arg[thrNum].threadStepStatus = 18;

    // set socket ip
    snprintf(p_AgentInfo->sock_ip, sizeof(p_AgentInfo->sock_ip), "%s", p_cpIp);

    WRITE_DEBUG_IP(p_cpIp, "Get ipaddr, real(%s)sock(%s)",
                   p_AgentInfo->user_ip,
                   p_AgentInfo->sock_ip);

    if(strverscmp(p_AgentInfo->agent_ver, "1.1.2.7.0") <= 0)
    {
        rxt = fpcif_MakeJsonRequestCfgOld(p_AgentInfo, &resData, bBaseReq);
    }
    else
    {
        // 버그나 오류로 인해 agent로부터 seq 값이 잘못 들어올 수 있으므로, 무조건 리턴해준다.
        // if not null sno then, use sno
        if (strlen(p_AgentInfo->user_sno) > 0)
        {
            if(fpcif_ChkLoadFlag() == 0)
            {
                idxU = fpcif_GetIdxByUserSno(p_AgentInfo->user_sno);
                if (idxU > -1)
                {
                    p_AgentInfo->user_seq = pUserInfo[idxU].us_sq;
                    WRITE_DEBUG_IP(p_cpIp, "Set user_seq(%llu), idxU(%d)",
                                   p_AgentInfo->user_seq,
                                   idxU);
                }
            }
            else
            {
                WRITE_WARNING_IP(p_cpIp,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True " );
                return (-1);
            }
        }
        else // if null sno then, use ip
        {
            if(fpcif_ChkLoadFlag() == 0)
            {
                idxU = fpcif_GetIdxByUserIp(p_AgentInfo->user_ip);
                if (idxU > -1)
                {
                    // 수동등록 일 경우만 ip로 seq를 가져옴
                    if ((pUserInfo[idxU].us_auto_sync-'0')%48 == 0)
                    {
                        p_AgentInfo->user_seq = pUserInfo[idxU].us_sq;
                        WRITE_DEBUG_IP(p_cpIp, "Set user_seq(%llu), idxU(%d)",
                                       p_AgentInfo->user_seq,
                                       idxU);
                    }
                }
            }
            else
            {
                WRITE_WARNING_IP(p_cpIp,"fpcif_MakeJsonRequestCfg -> LoadConfigFlag Not True " );
                return (-1);
            }

        }
        rxt = fpcif_MakeJsonRequestCfg(p_AgentInfo, p_cpIp, &resData, bBaseReq);
    }
    g_stThread_arg[thrNum].threadStepStatus = 19;

    if(rxt <= 0)
    {
        WRITE_CRITICAL_IP(p_cpIp, "Fail in make_request_cfg ");
        fcom_MallocFree((void**)&resData); // json_dump한것은 free()
        rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(p_cpIp, "Fail in send_ack(%s) ", DAP_AGENT);
        }
        return -1;

    }

    jsonSize = rxt + 1;
    if(fcom_malloc((void**)&jsonData, sizeof(char)*jsonSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    strcpy(jsonData, resData);
    fcom_MallocFree((void**)&resData);

    g_stThread_arg[thrNum].threadStepStatus = 20;
    if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
    {
        while(1)
        {
            if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
            {
                local_cHbDel = pBaseInfo[idx].hb_del;
                pthread_rwlock_unlock(&mutexpolicy);
                break;
            }
            else
            {
                fcom_SleepWait(5);
                local_loop_count++;
            }
            if( local_loop_count >=  20 )
            {
                WRITE_WARNING(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid());
                return (-1);
            }
        }

        if( (local_cHbDel-'0')%48 == 1 ) // del(except.key)
        {
            WRITE_DEBUG_IP(p_cpIp,	"Send delete to agent, hb_del(%c) ", local_cHbDel);
            rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_DELETE, jsonData, jsonSize, p_cpIp,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
        }

        else if( (local_cHbDel-'0')%48 == 2 ) // del(all)
        {
            WRITE_DEBUG_IP(p_cpIp,	"Send delete to agent, hb_del(%c)", local_cHbDel);
            rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_DELETE_ALL, jsonData, jsonSize, p_cpIp,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
        }
        else if( (local_cHbDel-'0')%48 == 3 ) // restart
        {
            WRITE_DEBUG_IP(p_cpIp,	"Send restart to agent, hb_del(%c)", local_cHbDel);
            rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_RESTART, jsonData, jsonSize, p_cpIp,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
        }
        else if( (local_cHbDel-'0')%48 == 4 ) // reqbase
        {
            WRITE_DEBUG_IP(p_cpIp,	"Send request_base to agent, hb_del(%c)",
                           local_cHbDel);

            rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_BASE_REQ, jsonData, jsonSize, p_cpIp,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
        }
        else
        {
            rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_SUCCESS, jsonData, jsonSize, p_cpIp,
                                    g_stProcPcifInfo.cfgRetryAckCount,
                                    g_stProcPcifInfo.retryAckSleep);
        }
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",2 );
        fcom_MallocFree((void**)&jsonData);
        return (-1);
    }


    if(rxt < 0)
    {
        WRITE_CRITICAL_IP(p_cpIp, "Fail in fsock_SendAckJson(%s) ", DAP_AGENT);
        fcom_MallocFree((void**)&jsonData);
        return -2;
    }


    fcom_MallocFree((void**)&jsonData);

    return 0;
}
int fpcif_ParseReqUploadFile(char *logip, int sock, json_t *p_element, char* p_managerId)
{
    int			rxt      = 0;
    int			idx      = 0;
    int			fileType = 0;
    int			fileSize = 0;
    char		fileName[50 +1] = {0x00,};
    const char	*tmpManagerId   = NULL;
    const char	*tmpFileName    = NULL;

    json_unpack(p_element, "{s:s, s:i, s:s, s:i}",
                "manager_id", 	&tmpManagerId,
                "file_type", 	&fileType,
                "file_name", 	&tmpFileName,
                "file_size", 	&fileSize
    );

    if(tmpManagerId != NULL)
    {
        strcpy(p_managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpFileName != NULL)
    {
        memset	(fileName, 0x00, sizeof(fileName));
        strcpy	(fileName, tmpFileName);
        tmpFileName = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' " , p_managerId );
    WRITE_DEBUG_IP(logip, "Unpack file_type: %d "    , fileType    );
    WRITE_DEBUG_IP(logip, "Unpack file_name: '%s' "  , fileName    );
    WRITE_DEBUG_IP(logip, "Unpack file_size: %d "    , fileSize    );

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_managerId);
    if(idx < 0)
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s) ", p_managerId);
        return -1;
    }

    if( fileType < 0 || fileType > 1 )
    {
        WRITE_CRITICAL_IP(logip, "Invalid shutdown id(%s)file_type(%d) ",
                          p_managerId,fileType);
        return -1;
    }

    if( strlen(fileName) > 0 && fileSize > 0 )
    {
        char* tmpBin = NULL;
        if(fcom_malloc((void**)&tmpBin, sizeof(char)*fileSize) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

//        rxt = fsock_SslSocketRecv(ssl, tmpBin, fileSize);
        rxt = fsock_Recv(sock, tmpBin, fileSize,0);
        if(rxt <= 0)
        {
            fcom_MallocFree((void**)&tmpBin);
            WRITE_CRITICAL_IP(logip, "[RCV] Fail in recv binary errno(%d)",
                              errno);
            return -1;
        }
        WRITE_DEBUG_IP(logip, "[RCV] Succeed in recv binary size(%d) ", fileSize );

        // 파일쓰기
        char recvFullPath[128 +1] = {0x00,};
        memset(recvFullPath, 0x00, sizeof(recvFullPath));

        if (fileType == 0) // server
        {
            sprintf(recvFullPath, "%s/%s", getenv("DAP_HOME"),"tmp");
            if (access(recvFullPath, W_OK) != 0) // If not exist
            {
                rxt = fcom_MkPath(recvFullPath, 0755);
                if (rxt < 0)
                {
                    fcom_MallocFree((void**)&tmpBin);
                    WRITE_CRITICAL_IP(logip, "Fail in make path(%s)", recvFullPath);
                    return -1;
                }
                else
                {
                    chmod(recvFullPath, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                    WRITE_DEBUG_IP(logip, "Succeed in make path(%s) ", recvFullPath );
                }
            }
            strcat(recvFullPath, "/");
            strcat(recvFullPath, fileName);
        }
        else if(fileType == 1) // agent update
        {
            sprintf(recvFullPath, "%s/%s/%s", getenv("DAP_HOME"),"update",fileName);
        }
        fpcif_SetBinToFile(tmpBin, recvFullPath, fileSize);

        fcom_MallocFree((void**)&tmpBin);
    }
    else if( strlen(fileName) > 0 && fileSize<= 0 )
    {
        WRITE_CRITICAL_IP(logip, "Invalied file, name(%s)size(%d)",
                          fileName,fileSize);
        return -1;
    }
    else
    {
        WRITE_CRITICAL_IP(logip, "Not found file, name(%s)size(%d)",
                          fileName,fileSize);
    }

    return 0;
}

int	fpcif_ProcReqServerReport(
        int	sock,
        char*	mngIp,
        char*	mngId,
        char*	sDate,
        char*	eDate)
{
    int				rxt      = 0;
    int				jsonSize = 0;
    int				loop     = 0;
    int				fileCnt  = 0;
    int				fileSize = 0;
    char			repFileName[50 +1]  = {0x00,};
    char			currYear[4 +1]      = {0x00,};
    char			currMonth[2 +1]     = {0x00,};
    char			currDate[8 +1]      = {0x00,};
    char			repFullPath[256 +1] = {0x00,};
    char			strMgwId[10 +1]     = {0x00,};
    char            local_szReportFullPath[512 +1] = {0x00,};
    struct	stat 	buf;
    char			*str     = NULL;
    struct dirent	*pDirent = NULL;
    DIR				*pRepDir = NULL;
    json_t			*root    = NULL;


    memset(&buf, 0x00, sizeof(struct stat));
    memset	(strMgwId, 0x00, sizeof(strMgwId));

    sprintf	(strMgwId, "%d", g_stServerInfo.stDapComnInfo.nCfgMgwId);

    time_t 	currTime = time((time_t) 0);

    memset	(currDate, 0x00, sizeof(currDate));
    memset	(currYear, 0x00, sizeof(currYear));
    memset	(currMonth, 0x00, sizeof(currMonth));

    fcom_time2str(currTime, currDate, "YYYYMMDD\0");
    fcom_time2str(currTime, currYear, "YYYY\0");
    fcom_time2str(currTime, currMonth, "MM\0");

    memset	(repFullPath, 0x00, sizeof(repFullPath));
    sprintf	(repFullPath, "%s/html/%s/%s/", g_stProcPcifInfo.cfgReportPath,currYear,currMonth);


    if (access(repFullPath, W_OK) != 0)
    {
        WRITE_CRITICAL_IP(mngIp, "Not found report dir(%s) ", repFullPath);
        return -1;
    }


    if(!(pRepDir = opendir(repFullPath)))
    {
        WRITE_DEBUG_IP(mngIp, "Fail in report dir(%s) ", repFullPath );
        return -1;
    }
    else
    {
        fcom_ReplaceAll(sDate, "-", "", sDate);
        fcom_ReplaceAll(eDate, "-", "", eDate);
        memset	(repFileName, 0x00, sizeof(repFileName));
        sprintf	(repFileName, "%s_%s_DAP_REPORT_%s-%s.html", mngId,currDate,sDate,eDate);

        WRITE_INFO_IP( mngIp, "- repFullPath(%s) ", repFullPath );
        WRITE_INFO_IP( mngIp, "- repFileName(%s) ", repFileName);

        snprintf(local_szReportFullPath, sizeof(local_szReportFullPath), "%s/%s", repFullPath, repFileName);

        // 이미 파일 있는경우 실시간이므로 레포트파일 삭제한다.
        if ( fcom_fileCheckStatus(local_szReportFullPath) == 0 )
        {
            WRITE_CRITICAL_IP(mngIp, "Exist found report dir(%s) ", local_szReportFullPath);
            unlink( local_szReportFullPath );
        }

        WRITE_INFO_IP(mngIp,"Check File Report (%s)", local_szReportFullPath);

        for(loop=0; loop < g_stProcPcifInfo.cfgFileDownDelayTime; loop++) // 보고서 생성까지 대기
        {
            if ( fcom_fileCheckStatus(local_szReportFullPath) == 0 ) //파일 있음
            {
                break;
            }
            sleep(1);
        }

        for(loop=0; loop < g_stProcPcifInfo.cfgFileDownDelayTime; loop++) // 5번 loop
        {
            while((pDirent = readdir(pRepDir)) != NULL)
            {
                if (strlen(pDirent->d_name) < 10)
                    continue;

                if (!strncmp(repFileName, pDirent->d_name, strlen(repFileName)))
                {
                    fileCnt++;
                    break;
                }
            }
            if (fileCnt > 0) break;
            sleep(1);
            rewinddir(pRepDir); // rewind
        }

        closedir(pRepDir);
    }

    if( fileCnt > 0 )
    {
        char statPath[256 +1] = {0x00,};

        memset	(statPath, 0x00, sizeof(statPath));
        sprintf	(statPath, "%s%s", repFullPath,repFileName);
        if( stat(statPath, &buf) < 0 )
        {
            WRITE_CRITICAL_IP(mngIp, "Stat error, file(%s)",
                              statPath);
            return -1;
        }
        fileSize = buf.st_size;

        root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",repFileName,"file_size",fileSize);
    }
    else
    {
        memset(repFileName, 0x00, sizeof(repFileName));
        strcpy(repFileName, "");
        root = json_pack("{s:{s:s, s:i}}", strMgwId,"file_name",repFileName,"file_size",fileSize);
    }

    str = json_dumps(root, JSON_INDENT(0));
    if( str == NULL )
    {
        json_decref(root);
        WRITE_CRITICAL_IP(mngIp, "Json pack error");
        return -1;
    }
    json_decref(root);

    WRITE_DEBUG_JSON(mngIp, "Dump json(%s) ", str );
    jsonSize = strlen(str);

    // Send result json (server -> manager)
    rxt = fpcif_SendJsonToManager(sock, MANAGE_RSP_REPORTFILE, str, jsonSize);
    if( rxt < 0 )
    {
        WRITE_CRITICAL_IP(mngIp, "[RSP] Fail in send json to manager(%s)errno(%s)",
                          mngIp,strerror(errno));
        fcom_MallocFree((void**)&str);
        return -1;
    }

    WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                   mngIp,MANAGE_RSP_REPORTFILE,jsonSize );

    if( strlen(repFileName) > 0 && fileSize > 0 )
    {
        // Send file binary (server -> manager)
        int		binFile_len = 0;
        char	downFilePath[256 +1] = {0x00,};
        char* tmpBin = NULL;
        if(fcom_malloc((void**)&tmpBin, sizeof(char)*fileSize) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        memset	(tmpBin, 0x00, sizeof(char)*fileSize);
        memset	(downFilePath, 0x00, sizeof(downFilePath));

        sprintf	(downFilePath, "%s%s", repFullPath,repFileName);

        WRITE_INFO_IP( mngIp, "- downFilePath(%s) ", downFilePath );
        binFile_len = fpcif_GetFileToBin(downFilePath, &tmpBin);
        if( binFile_len != fileSize )
        {
            WRITE_CRITICAL_IP(mngIp, "[RSP] Invalid file size(%d<>%d) manager(%s)errno(%s)",
                              binFile_len,fileSize,mngIp,strerror(errno));
            fcom_MallocFree((void**)&str);
            fcom_MallocFree((void**)&tmpBin);
            return -1;
        }

        rxt = fpcif_SendToBinary(sock, tmpBin, binFile_len);
        if( rxt < 0 )
        {
            WRITE_DEBUG_IP(mngIp, "[RSP] Fail in send binary to manager(%s)code(%d)size(%d) ",
                           mngIp,
                           MANAGE_RSP_REPORTFILE,
                           fileSize);
        }
        else
        {
            WRITE_DEBUG_IP(mngIp, "[RSP] Succeed in send binary to manager(%s)code(%d)size(%d)",
                           mngIp,
                           MANAGE_RSP_REPORTFILE,
                           fileSize);
        }

        fcom_MallocFree((void**)&tmpBin);
    }

    fcom_MallocFree((void**)&str);

    return 0;
}


int fpcif_ParseServerLogfile(
        char	*logip,
        json_t 	*p_element,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	**resFindIp,
        char	**resFindStr)
{
    int			idx = 0;
    const char	*tmpManagerId   = NULL;
    const char	*tmpBeginDate   = NULL;
    const char	*tmpEndDate     = NULL;
    const char	*tmpCategory    = NULL;
    const char	*tmpFindIp      = NULL;
    const char	*tmpFindStr     = NULL;

    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:s, s:s}",
                "manager_id", &tmpManagerId,
                "begin_date", &tmpBeginDate,
                "end_date"  , &tmpEndDate,
                "category"  , &tmpCategory,
                "find_ip"   , &tmpFindIp,
                "find_str"  , &tmpFindStr
    );

    if(tmpManagerId != NULL)
    {
        strcpy(p_managerId, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpBeginDate != NULL)
    {
        strcpy(p_beginDate, tmpBeginDate);
        tmpBeginDate = NULL;
    }
    if(tmpEndDate != NULL)
    {
        strcpy(p_endDate, tmpEndDate);
        tmpEndDate = NULL;
    }
    if(tmpCategory != NULL)
    {
        strcpy(p_category, tmpCategory);
        tmpCategory = NULL;
    }
    if(tmpFindIp != NULL)
    {
        *resFindIp = (char*)tmpFindIp;
        tmpFindIp = NULL;
    }
    if(tmpFindStr != NULL)
    {
        *resFindStr = (char*)tmpFindStr;
        tmpFindStr = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' ", p_managerId);
    WRITE_DEBUG_IP(logip, "Unpack begin_date: '%s' ", p_beginDate);
    WRITE_DEBUG_IP(logip, "Unpack end_date: '%s'   ", p_endDate  );
    WRITE_DEBUG_IP(logip, "Unpack category: '%s'   ", p_category );
    WRITE_DEBUG_IP(logip, "Unpack find_ip: '%s'    ", *resFindIp );
    WRITE_DEBUG_IP(logip, "Unpack find_str: '%s'   ", *resFindStr);

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_managerId);
    if( idx < 0 )
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s) ", p_managerId);
        return -1;
    }

    if( strlen(p_beginDate) != 10 ) //ex) 2018-12-04 or 2018-12-20
    {
        WRITE_DEBUG_IP(logip, "Invalid begin_date(%s) ", p_beginDate );
        return -1;
    }
    if( strlen(p_endDate) != 10 )
    {
        WRITE_DEBUG_IP(logip, "Invalid end_date(%s) ", p_endDate );
        return -1;
    }
    if( strlen(p_category) < 2 )
    {
        WRITE_DEBUG_IP(logip, "Invalid category(%s) ", p_category );
        return -1;
    }

    return 0;
}
int fpcif_ParseNotifyReport(char *logip, json_t *p_element, _DAP_REPORT_INFO* p_ReportInfo)
{

    int			idx                 = 0;
    int			tmpViewType         = 0;
    unsigned long long	tmpGroupSq  = 0;
    const char	*tmpManagerId   = NULL;
    const char	*tmpBeginDate   = NULL;
    const char	*tmpEndDate     = NULL;
    const char	*tmpMailType    = NULL;
    const char	*tmpMailLang    = NULL;
    const char	*tmpMailFrom    = NULL;
    const char	*tmpMailTo      = NULL;

    json_unpack(p_element, "{s:s, s:s, s:s, s:I, s:i, s:s, s:s, s:s, s:s}",
                "manager_id", &tmpManagerId,
                "begin_date", &tmpBeginDate,
                "end_date", &tmpEndDate,
                "group_sq", &tmpGroupSq,
                "view_type", &tmpViewType,
                "mail_type", &tmpMailType,
                "mail_lang", &tmpMailLang,
                "mail_from", &tmpMailFrom,
                "mail_to", &tmpMailTo
    );

    if(tmpManagerId != NULL)
    {
        strcpy(p_ReportInfo->manager_id, tmpManagerId);
        tmpManagerId = NULL;
    }
    if(tmpBeginDate != NULL)
    {
        strcpy(p_ReportInfo->begin_date, tmpBeginDate);
        tmpBeginDate = NULL;
    }
    if(tmpEndDate != NULL)
    {
        strcpy(p_ReportInfo->end_date, tmpEndDate);
        tmpEndDate = NULL;
    }
    p_ReportInfo->group_sq = tmpGroupSq;
    p_ReportInfo->view_type = tmpViewType;
    if(tmpMailType != NULL)
    {
        strcpy(p_ReportInfo->mail_type, tmpMailType);
        tmpMailType = NULL;
    }
    if(tmpMailLang != NULL)
    {
        strcpy(p_ReportInfo->mail_lang, tmpMailLang);
        tmpMailLang = NULL;
    }
    if(tmpMailFrom != NULL)
    {
        strcpy(p_ReportInfo->mail_from, tmpMailFrom);
        tmpMailFrom = NULL;
    }
    if(tmpMailTo != NULL)
    {
        strcpy(p_ReportInfo->mail_to, tmpMailTo);
        tmpMailTo = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_id: '%s' ", p_ReportInfo->manager_id );
    WRITE_DEBUG_IP(logip, "Unpack begin_date: '%s' ", p_ReportInfo->begin_date);
    WRITE_DEBUG_IP(logip, "Unpack end_date: '%s'   ", p_ReportInfo->end_date);
    WRITE_DEBUG_IP(logip, "Unpack group_sq: %llu   ", p_ReportInfo->group_sq);
    WRITE_DEBUG_IP(logip, "Unpack view_type: %d ", p_ReportInfo->view_type);
    WRITE_DEBUG_IP(logip, "Unpack mail_type: '%s' ", p_ReportInfo->mail_type);
    WRITE_DEBUG_IP(logip, "Unpack mail_lang: '%s' ", p_ReportInfo->mail_lang);
    WRITE_DEBUG_IP(logip, "Unpack mail_from: '%s' ", p_ReportInfo->mail_from);
    WRITE_DEBUG_IP(logip, "Unpack mail_to: '%s'  ", p_ReportInfo->mail_to);

    //manager_id 체크
    idx = fpcif_GetIdxByManagerSt(p_ReportInfo->manager_id);
    if(idx < 0)
    {
        WRITE_CRITICAL_IP(logip, "Not exist manager_id(%s)",
                          p_ReportInfo->manager_id);
        return -1;
    }

    if(strcasecmp(p_ReportInfo->mail_lang, "kr") != 0
       && strcasecmp(p_ReportInfo->mail_lang, "en") != 0)
    {
        WRITE_DEBUG_IP(logip, "Invalid mail_lang(%s) ", p_ReportInfo->mail_lang);
        return -1;
    }

    if(strlen(p_ReportInfo->mail_from) < 1)
    {
        WRITE_DEBUG_IP(logip, "Invalid mail_from(%s) ", p_ReportInfo->mail_from);
        return -1;
    }

    if(strlen(p_ReportInfo->mail_to) < 1)
    {
        WRITE_DEBUG_IP(logip, "Invalid mail_to(%s) ", p_ReportInfo->mail_to);
        return -1;
    }

    //mail_type이 today면 date가 필요없지만, 아닐경우 date 검사
    if(!strcmp(p_ReportInfo->mail_type, "day") || !strcmp(p_ReportInfo->mail_type, "week"))
    {
        if(strlen(p_ReportInfo->begin_date) != 10) //ex) 2018-12-04 or 2018-12-2W
        {
            WRITE_DEBUG_IP(logip, "Invalid %s format, begin_date(%s) ",
                           p_ReportInfo->mail_type,p_ReportInfo->begin_date );
            return -1;
        }
        if(strlen(p_ReportInfo->end_date) != 10)
        {
            WRITE_DEBUG_IP(logip, "Invalid %s format, end_date(%s) ",
                           p_ReportInfo->mail_type,p_ReportInfo->end_date );
            return -1;
        }
    }
    else if(!strcmp(p_ReportInfo->mail_type, "month"))
    {
        if(strlen(p_ReportInfo->begin_date) != 7) //ex) 2018-12
        {
            WRITE_DEBUG_IP(logip, "Invalid %s format, begin_date(%s) ",
                           p_ReportInfo->mail_type,p_ReportInfo->begin_date );
            return -1;
        }
        if(strlen(p_ReportInfo->end_date) != 7)
        {
            WRITE_DEBUG_IP(logip, "Invalid %s format, end_date(%s) ",
                           p_ReportInfo->mail_type,p_ReportInfo->end_date );
            return -1;
        }
    }

    return 0;
}

int fpcif_ParseExternalRequestCfg(int sock, char *p_cpIp, json_t *p_element, _DAP_AGENT_INFO *p_AgentInfo)
{
    int			idx      = -1;
    int			pos      = -1;
    int			rxt      = 0;
    int			tokenCnt = 0;
    int         local_loop_count = 0;
    unsigned long long	tmpUserSeq = 0;

    char		arrUserMac[128 +1]  = {0x00,};
    char		strTokMac[17 +1]    = {0x00,};
    char		strTokIp[15 +1]     = {0x00,};
    const char	*tmpAgentVer    = NULL;
    const char	*tmpUserKey     = NULL;
    const char	*tmpUserMac     = NULL;
    const char	*tmpUserSno     = NULL;

    json_unpack(p_element, "{s:s, s:s, s:s, s:s, s:I}",
                "agent_ver",&tmpAgentVer,
                "user_key", &tmpUserKey,
                "user_mac", &tmpUserMac,
                "user_sno", &tmpUserSno,
                "user_seq",	&tmpUserSeq);

    if(tmpAgentVer != NULL)
    {
        strcpy(p_AgentInfo->agent_ver, tmpAgentVer);
        tmpAgentVer = NULL;
    }
    if(tmpUserKey != NULL)
    {
        strcpy(p_AgentInfo->user_key, tmpUserKey);
        tmpUserKey = NULL;
    }
    if(tmpUserMac != NULL)
    {
        strcpy(p_AgentInfo->user_mac, tmpUserMac);
        tmpUserMac = NULL;
    }
    if(tmpUserSno != NULL)
    {
        strcpy(p_AgentInfo->user_sno, tmpUserSno);
        tmpUserSno = NULL;
    }
    p_AgentInfo->user_seq = tmpUserSeq;

    WRITE_DEBUG_IP(p_cpIp, "Unpack agent_ver: '%s' ", p_AgentInfo->agent_ver);
    WRITE_DEBUG_IP(p_cpIp, "Unpack user_key: '%s' ", p_AgentInfo->user_key);
    WRITE_DEBUG_IP(p_cpIp, "Unpack user_mac: '%s' ", p_AgentInfo->user_mac);
    WRITE_DEBUG_IP(p_cpIp, "Unpack user_sno: '%s' ", p_AgentInfo->user_sno);
    WRITE_DEBUG_IP(p_cpIp, "Unpack user_seq: %llu ", p_AgentInfo->user_seq);

    if(strcmp(p_AgentInfo->user_key, "unknown") == 0)
    {
        WRITE_CRITICAL_IP(p_cpIp,	"Fail in user_key(%s)",
                          p_AgentInfo->user_key);
        return -1;
    }
    else
    {
        if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
        {
            while(1)
            {
                if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                {
                    idx = fpcif_GetIdxByBaseKey(p_AgentInfo->user_key, 1);
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
            WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",4 );
            return (-1);
        }


        if(idx == -1)
        {
            //외부에서 접속 시 재발급하지않는다.
            WRITE_CRITICAL_IP(p_cpIp,	"Not found idx, user_key(%s)",
                              p_AgentInfo->user_key);
            //return -1; 리턴하지않고 hw_base_tb에 기록하기위해 주석처리
        }

        // ip가 내부ip이므로 db에 저장된 ip를 넣어준다.
        memset(p_cpIp, 0x00, 15+1);
        if(fpcif_PthreadMemoryReadLockCheck(4) == 0)
        {
            while(1)
            {
                if(pthread_rwlock_tryrdlock(&mutexpolicy) == 0)
                {
                    strcpy(p_cpIp, pBaseInfo[idx].hb_access_ip);
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
                    return (-1);
                }
            }

        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Read Lock Check Fail sec (%d) ",4);
            return (-1);
        }

    }

    if(strlen(p_AgentInfo->user_key) < 20)
    {
        WRITE_CRITICAL_IP(p_cpIp,	"Fail in user_key(%s)len(%d)",
                          p_AgentInfo->user_key, strlen(p_AgentInfo->user_key));
        return -1;
    }

    /*
     *20181128 기존 ip^mac 복수개가 들어오던것이, ip^mac 1개만 들어오며,
     *user_ip = ip, user_mac = mac 대입만 하면 됨
     */

    memset(arrUserMac, 0x00, sizeof(arrUserMac));
    strcpy(arrUserMac, p_AgentInfo->user_mac);
    WRITE_INFO_IP( p_cpIp, "- arrUserMac(%s)", arrUserMac);

    //p_AgentInfo->user_mac은 db에 넣어야하므로 초기화하고
    //매칭되는 mac이 없으면 null 있으면 1개의 mac주소가 들어간다.
    //초기화
    memset(p_AgentInfo->user_mac, 0x00, sizeof(p_AgentInfo->user_mac));

    tokenCnt = fcom_TokenCnt(arrUserMac, ",");
    if (tokenCnt > 0)
    {
        WRITE_DEBUG_IP(p_cpIp, "Fail in multiple ip or mac(%s)", arrUserMac);
        return -1;
    }

    if( strstr(arrUserMac, "^") != NULL)
    {
        memset(strTokIp, 0x00, sizeof(strTokIp));
        memset(strTokMac, 0x00, sizeof(strTokMac));
        pos = fcom_GetStringPos(arrUserMac, '^');
        fcom_SubStr(1, pos, arrUserMac, strTokIp);
        fcom_SubStr(pos+2, strlen(arrUserMac), arrUserMac, strTokMac);
        strcpy(p_AgentInfo->user_mac, strTokMac);
        strcpy(p_AgentInfo->user_ip, strTokIp);
        WRITE_DEBUG_IP(p_cpIp, "Set address, mac(%s)ip(%s) ",
                       p_AgentInfo->user_mac,p_AgentInfo->user_ip );
    }
    else
    {
        WRITE_DEBUG_IP(p_cpIp, "Invalid format from agent, arrUserMac(%s) ", arrUserMac );
    }

    if(strlen(p_AgentInfo->user_mac) < 17)
    {
        WRITE_DEBUG_IP(p_cpIp, "No match mac address " );
    }

    if(strlen(p_AgentInfo->user_ip) < 7)
    {
        strcpy(p_AgentInfo->user_ip, p_cpIp);
        WRITE_DEBUG_IP(p_cpIp, "Force agent access ip(%s), because there is no ip ",
                       p_AgentInfo->user_ip );
    }

    // set socket ip
    strcpy(p_AgentInfo->sock_ip, p_cpIp);

    WRITE_DEBUG_IP(p_cpIp, "Get ipaddr, real(%s)sock(%s) ",
                   p_AgentInfo->user_ip,p_AgentInfo->sock_ip );

    rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_SUCCESS,
                        g_stProcPcifInfo.cfgRetryAckCount,
                        g_stProcPcifInfo.retryAckSleep);
    if(rxt < 0)
    {
        return -2;
    }

    return 0;
}
int fpcif_ParseForwardServerLogtail(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerIp,
        int 	*p_managerFq,
        char**	resFile)
{

    int			tmpManagerFq    = 0;
    const char	*tmpManagerIp   = NULL;
    const char	*tmpFile        = NULL;

    json_unpack(p_element, "{s:s, s:i, s:s}",
                "manager_ip", &tmpManagerIp,
                "manager_fq", &tmpManagerFq,
                "file_name", &tmpFile
    );

    if(tmpManagerIp != NULL)
    {
        strcpy(p_managerIp, tmpManagerIp);
        tmpManagerIp = NULL;
    }
    *p_managerFq = tmpManagerFq;
    if(tmpFile != NULL)
    {
        *resFile = (char*)tmpFile;
        tmpFile = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_ip: '%s' ", p_managerIp );
    WRITE_DEBUG_IP(logip, "Unpack manager_fq: %d ", *p_managerFq );
    WRITE_DEBUG_IP(logip, "Unpack file_name: '%s' ", *resFile );

    if( strlen(*resFile) < 2 )
    {
        WRITE_DEBUG_IP(logip, "Invalid file_name(%s) ", *resFile );
        return -1;
    }

    return 0;
}

int fpcif_ParseForwardServerTerm(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerIp,
        char** 	resFile)
{
    const char	*tmpManagerIp = NULL;
    const char	*tmpFile      = NULL;

    json_unpack(p_element, "{s:s, s:s}",
                "manager_ip", &tmpManagerIp,
                "file_name", &tmpFile
    );

    if(tmpManagerIp != NULL)
    {
        strcpy(p_managerIp, tmpManagerIp);
        tmpManagerIp = NULL;
    }
    if(tmpFile != NULL)
    {
        *resFile = (char *)tmpFile;
        tmpFile = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack manager_ip: '%s' ", p_managerIp );
    WRITE_DEBUG_IP(logip, "Unpack file_name: '%s' ", *resFile     );

    if( strlen(*resFile) < 2 )
    {
        WRITE_DEBUG_IP(logip, "Invalid file_name(%s) ", *resFile );
        return -1;
    }

    return 0;
}


int fpcif_ParseJsonCmd(char *logip, int sock, int p_msgCode)
{
    char		*resData    = NULL;
    char		*jsonData   = NULL;
    int			jsonSize = 0;
    int			rxt      = 0;
    int			cmdCode  = 0;

    rxt = fpcif_MakeJsonCmd(logip, p_msgCode, cmdCode, &resData);
    if(rxt <= 0)
    {
        WRITE_CRITICAL_IP(logip, "Fail in make json cmd ");

        fcom_MallocFree((void**)&resData);
        rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(logip, "Fail in send ack(%s) ", DAP_AGENT);
            return -1;
        }
    }

    jsonSize = rxt;
    if(fcom_malloc((void**)&jsonData, sizeof(char)*jsonSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    strcpy(jsonData, resData);
    fcom_MallocFree((void**)&resData);

    rxt = fsock_SendAckJson(sock, DAP_AGENT, DATACODE_RTN_SUCCESS, jsonData, jsonSize, logip,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
    if(rxt < 0)
    {
        fcom_MallocFree((void**)&jsonData);
        return -1;
    }

    fcom_MallocFree((void**)&jsonData);

    return 0;
}

int fpcif_ParseCollectServerTail(
        char*	logip,
        json_t*	p_element,
        int* 	p_serverId,
        char*	p_managerIp,
        int* 	p_managerFq,
        char*	p_fileName,
        char**	resData)
{

    int			tmpServerId = 0;
    int			tmpManagerFq = 0;
    const char	*tmpManagerIp   = NULL;
    const char	*tmpFileName    = NULL;
    const char	*tmpData        = NULL;

    json_unpack(p_element, "{s:i, s:s, s:i, s:s, s:s}",
                "server_id", &tmpServerId,
                "manager_ip", &tmpManagerIp,
                "manager_fq", &tmpManagerFq,
                "file_name", &tmpFileName,
                "tail_data", &tmpData
    );

    *p_serverId = tmpServerId;
    if(tmpManagerIp != NULL)
    {
        strcpy(p_managerIp, tmpManagerIp);
        tmpManagerIp = NULL;
    }
    *p_managerFq = tmpManagerFq;
    if(tmpFileName != NULL)
    {
        strcpy(p_fileName, tmpFileName);
        tmpFileName = NULL;
    }
    if(tmpData != NULL)
    {
        *resData = (char*)tmpData;
        tmpData = NULL;
    }

    WRITE_DEBUG_IP(logip, "Unpack server_id: %d ", *p_serverId   );
    WRITE_DEBUG_IP(logip, "Unpack manager_ip: '%s' ", p_managerIp);
    WRITE_DEBUG_IP(logip, "Unpack manager_fq: %d ", *p_managerFq );
    WRITE_DEBUG_IP(logip, "Unpack file_name: '%s' ", p_fileName  );
    WRITE_DEBUG_IP(logip, "Unpack tail_data: '%s' ", *resData    );

    if( strlen(p_fileName) < 1 )
    {
        WRITE_DEBUG_IP(logip, "Invalid file_name(%s)", p_fileName );
        return -1;
    }
    if( strlen(*resData) < 1 )
    {
        WRITE_DEBUG_IP(logip, "Invalid tail_data(%s) ", *resData );
        return -1;
    }

    return 0;
}

int fpcif_ParseMatchMac(char *logip, int sock, json_t* p_element)
{
    char		*cmd_code = NULL;
    char		*str_mac = NULL;
    int			rxt = 0;

    json_unpack(p_element, "{s:s, s:s}",
                "cmd_code", &cmd_code,
                "str_mac", &str_mac);

    WRITE_DEBUG_IP(logip, "Unpack cmd_code: '%s' ", cmd_code );
    WRITE_DEBUG_IP(logip, "Unpack str_mac: '%s' ", str_mac );

    if(atoi((char *)cmd_code) == 1)
    {
        WRITE_DEBUG_IP(logip, "Succeed in match code(%s)mac(%s) ", cmd_code,str_mac );
        rxt = fsock_SendAck(sock, DAP_AGENT, DATACODE_RTN_SUCCESS,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
    }
    else
    {
        WRITE_CRITICAL_IP(logip, "Fail in match code(%s)mac(%s)", cmd_code,str_mac);
        rxt = fsock_SendAck(sock, DAP_AGENT,  DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
    }

    if(rxt < 0)
    {
        WRITE_CRITICAL_IP(logip, "Fail in send ack(%s)", DAP_AGENT);

        fcom_MallocFree((void**)&cmd_code);
        fcom_MallocFree((void**)&str_mac);
        return -1;
    }

    fcom_MallocFree((void**)&cmd_code);
    fcom_MallocFree((void**)&str_mac);

    return 0;
}


void fpcif_ParseDuplicateCheck(_DAP_AGENT_INFO* p_AgentInfo, int idx)
{
    /* US_SQ 체크 */
    if(p_AgentInfo->user_seq == pBaseInfo[idx].us_sq)
    {
        /* AGENT INFO 초기화하여 DBIF 프로세스에서 HW_BASE_TB Update하는 쿼리의 Set에서 제외시킨다. */
        p_AgentInfo->user_seq = 0;
    }

    /* HB_ACCESS_IP 체크 */
    if(memcmp(p_AgentInfo->user_ip,pBaseInfo[idx].hb_access_ip, 15) == 0)
    {
        /* AGENT INFO 초기화하여 DBIF 프로세스에서 HW_BASE_TB Update하는 쿼리의 Set에서 제외시킨다. */
        memset(p_AgentInfo->user_ip, 0x00, sizeof(p_AgentInfo->user_ip));
    }

    /* HB_ACCESS_MAC 체크 */
    if(memcmp(p_AgentInfo->user_mac,pBaseInfo[idx].hb_access_mac, 17) == 0)
    {
        /* AGENT INFO 초기화하여 DBIF 프로세스에서 HW_BASE_TB Update하는 쿼리의 Set에서 제외시킨다. */
        memset(p_AgentInfo->user_mac, 0x00, sizeof(p_AgentInfo->user_mac));
    }

    /* HB_SOCKET_IP 체크 */
    if(memcmp(p_AgentInfo->sock_ip,pBaseInfo[idx].hb_sock_ip, 15) == 0)
    {
        /* AGENT INFO 초기화하여 DBIF 프로세스에서 HW_BASE_TB Update하는 쿼리의 Set에서 제외시킨다. */
        memset(p_AgentInfo->sock_ip, 0x00, sizeof(p_AgentInfo->sock_ip));
    }


    /* HB_AGENT_VER 체크 */
    if(memcmp(p_AgentInfo->agent_ver,pBaseInfo[idx].hb_agent_ver, 16) == 0)
    {
        /* AGENT INFO 초기화하여 DBIF 프로세스에서 HW_BASE_TB Update하는 쿼리의 Set에서 제외시킨다. */
        memset(p_AgentInfo->agent_ver, 0x00, sizeof(p_AgentInfo->agent_ver));
    }

    /* HB_DEL 체크*/
    /** Agent가  전송한 HW_BASE_TB 테이블의 HB_DEL컬럼 / HB_EXTERNAL 컬럼은 0으로 Update **/
//    if(pBaseInfo[idx].hb_del != '0')
//    {
//        p_AgentInfo->hb_del = pBaseInfo[idx].hb_del-'0';
//    }
//    else
//        p_AgentInfo->hb_del = 0;

    /* HB EXTERNAL 유무 체크 */
//    if(pBaseInfo[idx].hb_external != '0')
//    {
//        p_AgentInfo->hb_external = pBaseInfo[idx].hb_external-'0';
//    }
//    else
//        p_AgentInfo->hb_external = 0;


    return;

}
