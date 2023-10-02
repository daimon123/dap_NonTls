//
// Created by KimByoungGook on 2020-06-26.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "com/dap_com.h"
#include <syslog.h>

#include "com/dap_req.h"
#include "json/dap_json.h"
#include "dbif.h"


int fdbif_TaskMainboard(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        char*               param_cmd,
        int*                retryBuffLen,
        int*                buffLen
)
{

    int     nRet    = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_INFO, "[MRG] MainBoard ");
    memcpy(	(void *)&DetectData.MainBoard,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.MainBoard) );
    *buffLen += sizeof(DetectData.MainBoard);
    fjson_PrintAllMainBoard(&DetectData.MainBoard);

    nRet = fdbif_MergeHwBaseTb(	&Detect->AgentInfo,
                                Detect->detect_time,
                                &DetectData.MainBoard,
                                param_Prefix, param_Postfix );
    if(nRet < 0)
    {
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.MainBoard,
                   sizeof(DetectData.MainBoard) );
        *retryBuffLen += sizeof(Detect);
    }
    else
    {
        system(param_cmd);
    }
    return RET_SUCC;

}


int fdbif_TaskSystem(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    EventQData,
        _DAP_QUEUE_BUF*  	AlarmQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_EventParam*    EventParam,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                EventCount,
        int*		        bNotiFlag,
        int*                retryBuffLen,
        int*                buffLen
)
{
    int nRet;

    int		stLen = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    memcpy((void *) &DetectData.System,
           (void *) &RsltQData->buf[*buffLen],
           sizeof(DetectData.System));
    *buffLen += sizeof(DetectData.System);
    fjson_PrintAllSystem(&DetectData.System);

    nRet = fdbif_MergeSystemTb(Detect->AgentInfo.user_key,
                              Detect->AgentInfo.user_seq,
                              Detect->detect_time,
                              &DetectData.System,
                              param_Prefix, param_Postfix);
    if (nRet < 0)
    {
        if (nRet == -2)
            *param_bEmptyBase = 1;
        memcpy((void *) &RetryQData->buf[*retryBuffLen],
               (void *) &DetectData.System,
               sizeof(DetectData.System));
        *retryBuffLen += sizeof(DetectData.System);
    }
    else
    {
        WRITE_INFO(CATEGORY_INFO, "[EVENT] VIRTUAL_MACHINE ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        snprintf(EventParam->user_key, sizeof(EventParam->user_key),
                 "%s",   Detect->AgentInfo.user_key);
        snprintf(EventParam->user_ip, sizeof(EventParam->user_ip),
                 "%s",    Detect->AgentInfo.user_ip);
        EventParam->user_seq = Detect->AgentInfo.user_seq;
        snprintf(EventParam->detect_time,sizeof(EventParam->detect_time),"%s", Detect->detect_time);
        EventParam->ev_type = VIRTUAL_MACHINE;
        EventParam->ev_level = (DetectData.System.st_rule_vm - '0') % 48;
        snprintf(EventParam->prefix, sizeof(EventParam->prefix), "%s", param_Prefix);
        snprintf(EventParam->postfix, sizeof(EventParam->postfix), "%s", param_Postfix);
        EventParam->ru_sq = DetectData.System.st_rusq_vm;
        if (RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(EventParam->ev_context, "ext");
        else
            strcpy(EventParam->ev_context, "");
        stLen = fjson_GetLengthSystemVm(&DetectData.System);
        // NEW이거나 MOD일때만 이벤트를 발생시킨다.
        if (stLen > 0 && fjson_GetLengthSystemVmSummary(DetectData.System.st_summary) == 0)
            return 0;
        nRet = fdbif_MergeEventTb(EventParam, stLen);
        if (nRet < 0)
        {
            memcpy((void *) &EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                   (void *) EventParam,
                   sizeof(_DAP_EventParam));
            *EventCount += 1;
        }
        else
        {
            if (nRet > 0)
            {
                *bNotiFlag = 1;
                if (nRet != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if (g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if (g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}


int fdbif_TaskConnExt  (
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    EventQData,
        _DAP_QUEUE_BUF*  	AlarmQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_EventParam*    EventParam,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                EventCount,
        int*		        bNotiFlag,
        int*                retryBuffLen,
        int*		        buffLen
)
{
    int nRet;

    int		stLen = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] ConnectExt ");

    memcpy((void *) &DetectData.ConnectExt,
           (void *) &RsltQData->buf[*buffLen],
           sizeof(DetectData.ConnectExt));
    *buffLen += sizeof(DetectData.ConnectExt);
    fjson_PrintAllConnectExt(&DetectData.ConnectExt);

    nRet = fdbif_MergeConnectExtTb(Detect->AgentInfo.user_key,
                                  Detect->AgentInfo.user_seq,
                                  Detect->detect_time,
                                  &DetectData.ConnectExt,
                                  param_Prefix, param_Postfix);

    if (nRet < 0)
    {
        if (nRet == -2)
            *param_bEmptyBase = 1;
        memcpy((void *) &RetryQData->buf[*retryBuffLen],
               (void *) &DetectData.ConnectExt,
               sizeof(DetectData.ConnectExt));
        *retryBuffLen += sizeof(DetectData.ConnectExt);
    }
    else
    {
        WRITE_INFO(CATEGORY_INFO, "[EVENT] CONNECT_EXT_SVR ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(EventParam->user_key, Detect->AgentInfo.user_key);
        strcpy(EventParam->user_ip, Detect->AgentInfo.user_ip);
        EventParam->user_seq = Detect->AgentInfo.user_seq;
        strcpy(EventParam->detect_time, Detect->detect_time);
        EventParam->ev_type = CONNECT_EXT_SVR;
        EventParam->ev_level = (DetectData.ConnectExt.ce_rule - '0') % 48;
        strcpy(EventParam->prefix, param_Prefix);
        strcpy(EventParam->postfix, param_Postfix);
        EventParam->ru_sq = DetectData.ConnectExt.ce_rusq;
        if (RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(EventParam->ev_context, "ext");
        else
            strcpy(EventParam->ev_context, "");
        stLen = fjson_GetLengthConnectExt(&DetectData.ConnectExt);
        // ADD일때만 이벤트를 발생시킨다.
        if (stLen > 0 &&
            fjson_GetLengthConnectExtSummary(DetectData.ConnectExt.ce_summary) == 0)
            return 0;

        nRet = fdbif_MergeEventTb(EventParam, stLen);
        if (nRet < 0)
        {
            memcpy((void *) &EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                   (void *) EventParam,
                   sizeof(_DAP_EventParam));
            *EventCount += 1;
        }
        else
        {
            if (nRet > 0)
            {
                *bNotiFlag = 1;
                if (nRet != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if (g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if (g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskOperation(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                retryBuffLen,
        int*                buffLen
)
{
    int nRet;

    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_INFO, "[MRG] OperatingSystem ");
    memcpy(	(void *)&DetectData.OperatingSystem,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.OperatingSystem) );
    *buffLen += sizeof(DetectData.OperatingSystem);
    fjson_PrintAllOperatingSystem(&DetectData.OperatingSystem);

    nRet = fdbif_MergeOsTb(	Detect->AgentInfo.user_key,
                              Detect->AgentInfo.user_seq,
                              Detect->detect_time,
                              &DetectData.OperatingSystem,
                              param_Prefix, param_Postfix);
    if(nRet < 0)
    {
        if(nRet == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.OperatingSystem,
                   sizeof(DetectData.OperatingSystem) );
        *retryBuffLen += sizeof(DetectData.OperatingSystem);
    }
    return RET_SUCC;

}

int fdbif_TaskCpu(
            _DAP_QUEUE_BUF* 	RsltQData,
            _DAP_QUEUE_BUF*	    RetryQData,
            _DAP_DETECT* 		Detect,
            char*               param_Prefix,
            char*               param_Postfix,
            int*                param_bEmptyBase,
            int*                retryBuffLen,
            int*                buffLen
)
{
    int nRet = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] Cpu ");
    memcpy(	(void *)&DetectData.Cpu,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Cpu) );
    *buffLen += sizeof(DetectData.Cpu);
    fjson_PrintAllCpu(&DetectData.Cpu);
    nRet = fdbif_MergeCpuTb( Detect->AgentInfo.user_key,
                           Detect->AgentInfo.user_seq,
                           Detect->detect_time,
                           &DetectData.Cpu,
                           param_Prefix, param_Postfix);
    if(nRet < 0)
    {
        if(nRet == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Cpu,
                   sizeof(DetectData.Cpu));
        *retryBuffLen += sizeof(DetectData.Cpu);
    }
    return RET_SUCC;
}

int fdbif_TaskNetAdapter(
            _DAP_QUEUE_BUF* 	RsltQData,
            _DAP_QUEUE_BUF*	    EventQData,
            _DAP_QUEUE_BUF*  	AlarmQData,
            _DAP_QUEUE_BUF*	    RetryQData,
            _DAP_EventParam*    EventParam,
            _DAP_DETECT* 		Detect,
            char*               param_Prefix,
            char*               param_Postfix,
            int*                param_bEmptyBase,
            int*                EventCount,
            int*		        bNotiFlag,
            int*                retryBuffLen,
            int*    		    buffLen

)
{
    int     rxt = 0;
    int		stLen = 0;
    char	strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] NetAdapter " );
    memcpy(	(void *)&DetectData.NetAdapter,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.NetAdapter));
    *buffLen += sizeof(DetectData.NetAdapter);
    fjson_PrintAllNetAdapter(&DetectData.NetAdapter);

    rxt = fdbif_MergeNetAdapterTb(	Detect->AgentInfo.user_key,
                                   Detect->AgentInfo.user_seq,
                                   Detect->detect_time,
                                   &DetectData.NetAdapter,
                                   param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.NetAdapter,
                   sizeof(DetectData.NetAdapter) );
        retryBuffLen += sizeof(DetectData.NetAdapter);
    }
    else
    {
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,	Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq		= DetectData.NetAdapter.na_over_rusq;

        WRITE_INFO(CATEGORY_INFO, "[EVENT] NET_ADAPTER_OVER " ) ;
        EventParam->ev_type		= NET_ADAPTER_OVER;
        EventParam->ev_level     = (DetectData.NetAdapter.na_over_rule-'0')%48;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(EventParam->ev_context, "ext");
        else
            strcpy(EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockNetAdapterSummary(DetectData.NetAdapter.na_summary, strDetail);
            if (stLen > 1)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthNetAdapterPhysicalNic(&DetectData.NetAdapter);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[(*EventCount) * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_DB, "[EVENT] NET_ADAPTER_DUPIP ");
        EventParam->ev_type		= NET_ADAPTER_DUPIP;
        EventParam->ev_level     = (DetectData.NetAdapter.na_dupip_rule-'0')%48;
        EventParam->ru_sq		= DetectData.NetAdapter.na_dupip_rusq;
        memset(strDetail, 0x00, sizeof(strDetail));

        stLen = fdbif_SelectCountDuplByBase(	"IP",
                                              Detect->AgentInfo.user_key,
                                              &DetectData.NetAdapter,
                                              strDetail );

        strcpy(	EventParam->ev_context,		strDetail);
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcat(	EventParam->ev_context, ",ext");
        else
            strcat(	EventParam->ev_context, "");
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[(*EventCount) * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_INFO,"[EVENT] NET_ADAPTER_DUPMAC " );
        memset(strDetail, 0x00, sizeof(strDetail));

        stLen = fdbif_SelectCountDuplByBase(	"MAC",
                                              Detect->AgentInfo.user_key,
                                              &DetectData.NetAdapter,
                                              strDetail );
        EventParam->ev_type		= NET_ADAPTER_DUPMAC;
        EventParam->ev_level     = (DetectData.NetAdapter.na_dupmac_rule-'0')%48;
        EventParam->ru_sq		= DetectData.NetAdapter.na_dupmac_rusq;
        strcpy(	EventParam->ev_context,		strDetail);
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcat(	EventParam->ev_context, ",ext");
        else
            strcat(	EventParam->ev_context, "");
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[(*EventCount) * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
             *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_DB, "[EVENT] NET_ADAPTER_MULIP " );
        memset(strDetail, 0x00, sizeof(strDetail));
        stLen = fjson_GetLengthNetAdapterMulip(&DetectData.NetAdapter, strDetail);
        EventParam->ev_type		= NET_ADAPTER_MULIP;
        EventParam->ev_level     = (DetectData.NetAdapter.na_mulip_rule-'0')%48;
        EventParam->ru_sq		= DetectData.NetAdapter.na_mulip_rusq;
        strcpy(	EventParam->ev_context,		strDetail);
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcat(	EventParam->ev_context, ",ext");
        else
            strcat(	EventParam->ev_context, "");
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&(EventQData->buf[*EventCount * sizeof(_DAP_EventParam)]),
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
//                        WriteEor(&EventParam);
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

/* WIFI���� HW_BASE_TB ó�� */
int fdbif_TaskWifi(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int* buffLen

)
{
    int rxt = 0;
    int stLen = 0;
    char    strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_INFO, "[MRG] Wifi " );
    memcpy(	(void *)&DetectData.Wifi,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Wifi) );
    *buffLen += sizeof(DetectData.Wifi);
    fjson_PrintAllWifi(&DetectData.Wifi);

    rxt = fdbif_MergeWifiTb(Detect->AgentInfo.user_key,
                        Detect->AgentInfo.user_seq,
                        Detect->detect_time,
                        &DetectData.Wifi,
                        param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Wifi,
                   sizeof(DetectData.Wifi) );
        retryBuffLen += sizeof(DetectData.Wifi);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] WIFI " );
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= WIFI;
        EventParam->ev_level		= (DetectData.Wifi.wf_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq		= DetectData.Wifi.wf_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");

        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.Wifi.wf_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else
                    strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthWifi(&DetectData.Wifi);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;

}

/* ����������� HW_BASE_TB ó�� */
int fdbif_TaskBlueTooth(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *bBtDanger,
        int* buffLen
)
{
    int rxt = 0;
    int idx = 0;
    int	stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] Bluetooth ");
    memcpy(	(void *)&DetectData.Bluetooth,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Bluetooth) );
    *buffLen += sizeof(DetectData.Bluetooth);
    fjson_PrintAllBluetooth(&DetectData.Bluetooth);

    rxt = fdbif_MergeBluetoothTb(	Detect->AgentInfo.user_key,
                                 Detect->AgentInfo.user_seq,
                                 Detect->detect_time,
                                 &DetectData.Bluetooth,
                                 param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Bluetooth,
                   sizeof(DetectData.Bluetooth) );
        *retryBuffLen += sizeof(DetectData.Bluetooth);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] BLUETOOTH " );
        *bBtDanger = 0;
        for(idx=0; idx<DetectData.Bluetooth.size; idx++)
        {
            if(DetectData.Bluetooth.BluetoothValue[idx].bt_danger == 1)
            {
                *bBtDanger = 1;
            }
        }

        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= BLUETOOTH;
        EventParam->ev_level		= (DetectData.Bluetooth.bt_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,	param_Postfix);
        EventParam->ru_sq		= DetectData.Bluetooth.bt_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.Bluetooth.bt_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            //danger가 0과1이 혼합되어 들어오므로 stLen을 사용하지 않는다.
            if(*bBtDanger == 1) //danger=1이 한개 이상 있으면, 한번만 이벤트
            {
                rxt = fdbif_MergeEventTb(EventParam, 1);
            }
            else //danger=1 이 없으면 해지
            {
                rxt = fdbif_MergeEventTb(EventParam, 0);
            }
        }
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskNetConnection(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int* buffLen
)
{
    int rxt = 0;
    int		stLen = 0;
    char	strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] NetConnection ");
    memcpy(	(void *)&DetectData.NetConnection,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.NetConnection) );
    *buffLen += sizeof(DetectData.NetConnection);
    fjson_PrintAllNetConnection(&DetectData.NetConnection);

    rxt = fdbif_MergeNetConnectionTb(	Detect->AgentInfo.user_key,
                                      Detect->AgentInfo.user_seq,
                                      Detect->detect_time,
                                      &DetectData.NetConnection,
                                      param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.NetConnection,
                   sizeof(DetectData.NetConnection) );
        *retryBuffLen += sizeof(DetectData.NetConnection);
    }
    else
    {
        WRITE_INFO(CATEGORY_INFO, "[EVENT] NET_CONNECTION ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= NET_CONNECTION;
        EventParam->ev_level		= (DetectData.NetConnection.nc_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq		= DetectData.NetConnection.nc_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.NetConnection.nc_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthNetConnection(&DetectData.NetConnection);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)&EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskDisk(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int* buffLen
)
{
    int rxt = 0;
    int	stLen = 0;
    int	diskNewCnt = 0; //추가된디스크개수
    char strDetail[512 +1] = {0x00,};
    char diskNewStr[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));


    WRITE_INFO(CATEGORY_DB, "[MRG] Disk ");
    memcpy(	(void *)&DetectData.Disk,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Disk) );
    *buffLen += sizeof(DetectData.Disk);
    fjson_PrintAllDisk(&DetectData.Disk);
    diskNewCnt = 0;
    memset(diskNewStr, 0x00, sizeof(diskNewStr));

    rxt = fdbif_MergeDiskTb(Detect->AgentInfo.user_key,
                        Detect->AgentInfo.user_seq,
                        Detect->detect_time,
                        &DetectData.Disk,
                        param_Prefix, param_Postfix,
                        &diskNewCnt,
                        diskNewStr);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Disk,
                   sizeof(DetectData.Disk) );
        *retryBuffLen += sizeof(DetectData.Disk);
    }
    else
    {
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);

        rxt = fdbif_CheckUnregDisk(&DetectData.Disk, Detect->AgentInfo.user_key);
        if(rxt > 0) //미등록 디스크가 1개이상 있으면 이벤트 발생
        {
            WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_REG " );
            EventParam->ev_type		= DISK_REG;
            EventParam->ev_level	= (DetectData.Disk.dk_reg_rule-'0')%48;
            EventParam->ru_sq		= DetectData.Disk.dk_reg_rusq;
            if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                strcpy(	EventParam->ev_context, "ext");
            else
                strcpy(	EventParam->ev_context, "");
            stLen = 1;
            rxt = fdbif_MergeEventTb(EventParam, stLen);
            if(rxt < 0)
            {
                memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                           (void *)EventParam,
                           sizeof(_DAP_EventParam) );
                *EventCount += 1;
            }
            else
            {
                if(rxt > 0)
                {
                    *bNotiFlag = 1;
                    if(rxt != 99)
                    {
                        fdbif_FQEventToPcif(EventParam);
                        if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                        {
                            memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                            fdbif_FQEventToAlarm(AlarmQData, EventParam);
                        }
                        if(g_stProcDbifInfo.nEorActive == 1)
                        {
                            fdbif_WriteEor(EventParam);
                        }
                    }
                }
            }
        }
        else
        {
            if(diskNewCnt > 0 || diskNewCnt < 0)
            {
                //디스크개수가0보다크면디스크추가
                //DISK_NEW 검출
                //DISK_NEW는 한번 검출되면 변경된내용이 DISK_TB에 남기때문에
                //Select를 해봐도 변경 전 기록이 History에 있어서 찾기 힘들다.
                //그래서 자동해지되지 않으므로 추후 수작업 해지 조치가 필요
                WRITE_INFO(CATEGORY_DB,	"[EVENT] DISK_NEW, diskNewCnt(%d) ", diskNewCnt);
                EventParam->ev_type		= DISK_NEW;
                EventParam->ev_level     = (DetectData.Disk.dk_new_rule-'0')%48;
                EventParam->ru_sq		= DetectData.Disk.dk_new_rusq;
                strcpy(	EventParam->ev_context, diskNewStr);
                stLen = diskNewCnt;
                if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                {
                    if(strlen(EventParam->ev_context) > 0)
                        strcat(EventParam->ev_context, ",ext");
                    else
                        strcpy(EventParam->ev_context, "ext");
                }
                else
                {
                    strcat(EventParam->ev_context, "");
                }
                rxt = fdbif_MergeEventTb(EventParam, stLen);
                if(rxt < 0)
                {
                    memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                               (void *)EventParam,
                               sizeof(_DAP_EventParam) );
                    *EventCount += 1;
                }
                else
                {
                    if(rxt > 0)
                    {
                        *bNotiFlag = 1;
                        if(rxt != 99)
                        {
                            fdbif_FQEventToPcif(EventParam);
                            if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                            {
                                memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                fdbif_FQEventToAlarm(AlarmQData, EventParam);
                            }
                            if(g_stProcDbifInfo.nEorActive == 1)
                            {
                                fdbif_WriteEor(EventParam);
                            }
                        }
                    }
                }
            }
            else
            {
                WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_REG ");
                EventParam->ev_type		= DISK_REG;
                EventParam->ev_level		= (DetectData.Disk.dk_reg_rule-'0')%48;
                EventParam->ru_sq		= DetectData.Disk.dk_reg_rusq;
                stLen = 0; //�̵�ϵ�ũ����
                if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                {
                    if(strlen(EventParam->ev_context) > 0)
                        strcat(EventParam->ev_context, ",ext");
                    else
                        strcpy(EventParam->ev_context, "ext");
                }
                else
                {
                    strcat(EventParam->ev_context, "");
                }
                rxt = fdbif_MergeEventTb(EventParam, stLen);
                if(rxt < 0)
                {
                    memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                            (void *)EventParam,
                               sizeof(_DAP_EventParam) );
                    *EventCount += 1;
                }
                else
                {
                    if(rxt > 0)
                    {
                        *bNotiFlag = 1;
                        if(rxt != 99)
                        {
                            fdbif_FQEventToPcif(EventParam);
                            if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                            {
                                memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                fdbif_FQEventToAlarm(AlarmQData, EventParam);
                            }
                            if(g_stProcDbifInfo.nEorActive == 1)
                            {
                                fdbif_WriteEor(EventParam);
                            }
                        }
                    }
                }
            }
        }


        if(((DetectData.Disk.dk_mobile_rule-'0')%48) > 0)
        {
            //정책레벨은 상위 mobile에 따라감
            EventParam->ev_level     = (DetectData.Disk.dk_mobile_rule-'0')%48;

            if(((DetectData.Disk.dk_mobile_read_rule-'0')%48) == 1)
            {
                WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_MOBILE_READ ");
                EventParam->ev_type		= DISK_MOBILE_READ;
                EventParam->ru_sq		= DetectData.Disk.dk_mobile_read_rusq;
                if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                    strcpy(	EventParam->ev_context, "ext");
                else
                    strcpy(	EventParam->ev_context, "");
                // check BLOCK
                if (EventParam->ev_level == BLOCK)
                {
                    memset(strDetail, 0x00, sizeof(strDetail));
                    stLen = fjson_GetBlockcommonSummary(DetectData.Disk.dk_summary, strDetail);
                    if (stLen > 0)
                    {
                        if (strlen(EventParam->ev_context) > 0)
                        {
                            strcat(EventParam->ev_context, ",");
                            strcat(EventParam->ev_context, strDetail);
                        }
                        else strcpy(EventParam->ev_context, strDetail);
                    }
                }
                else
                {
                    stLen = fdbif_GetLengthDiskMobileRead(&DetectData.Disk);
                }
                rxt = fdbif_MergeEventTb(EventParam, stLen);
                if(rxt < 0)
                {
                    memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                               (void *)&EventParam,
                               sizeof(_DAP_EventParam) );
                    *EventCount += 1;
                }
                else
                {
                    if(rxt > 0)
                    {
                        *bNotiFlag = 1;
                        if(rxt != 99)
                        {
                            fdbif_FQEventToPcif(EventParam);
                            if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                            {
                                memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                fdbif_FQEventToAlarm(AlarmQData, EventParam);
                            }
                            if(g_stProcDbifInfo.nEorActive == 1)
                            {
                                fdbif_WriteEor(EventParam);
                            }
                        }
                    }
                }
            }
            if(((DetectData.Disk.dk_mobile_write_rule-'0')%48) == 1)
            {
                WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_MOBILE_WRITE ");
                EventParam->ev_type		= DISK_MOBILE_WRITE;
                EventParam->ru_sq		= DetectData.Disk.dk_mobile_write_rusq;
                if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                    strcpy(	EventParam->ev_context, "ext");
                else
                    strcpy(	EventParam->ev_context, "");
                // check BLOCK
                if (EventParam->ev_level == BLOCK)
                {
                    memset(strDetail, 0x00, sizeof(strDetail));
                    stLen = fjson_GetBlockcommonSummary(DetectData.Disk.dk_summary, strDetail);
                    if (stLen > 0)
                    {
                        if (strlen(EventParam->ev_context) > 0)
                        {
                            strcat(EventParam->ev_context, ",");
                            strcat(EventParam->ev_context, strDetail);
                        }
                        else strcpy(EventParam->ev_context, strDetail);
                    }
                }
                else
                {
                    stLen = fdbif_GetLengthDiskMobileWrite(&DetectData.Disk);
                }
                rxt = fdbif_MergeEventTb(EventParam, stLen);
                if(rxt < 0)
                {
                    memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                               (void *)&EventParam,
                               sizeof(_DAP_EventParam) );
                    *EventCount += 1;
                }
                else
                {
                    if(rxt > 0)
                    {
                        *bNotiFlag = 1;
                        if(rxt != 99)
                        {
                            fdbif_FQEventToPcif(EventParam);
                            if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                            {
                                memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                fdbif_FQEventToAlarm(AlarmQData, EventParam);
                            }
                            if(g_stProcDbifInfo.nEorActive == 1)
                            {
                                fdbif_WriteEor(EventParam);
                            }
                        }
                    }
                }
            }
            //읽기/쓰기권한탐지가 비활성화일경우, 이동형만본다
            if(((DetectData.Disk.dk_mobile_read_rule-'0')%48) == 0
               && ((DetectData.Disk.dk_mobile_write_rule-'0')%48) == 0)
            {
                WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_MOBILE ");
                EventParam->ev_type		= DISK_MOBILE;
                EventParam->ru_sq		= DetectData.Disk.dk_mobile_rusq;
                if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                    strcpy(	EventParam->ev_context, "ext");
                else
                    strcpy(	EventParam->ev_context, "");
                // check BLOCK
                if (EventParam->ev_level == BLOCK)
                {
                    memset(strDetail, 0x00, sizeof(strDetail));
                    stLen = fjson_GetBlockcommonSummary(DetectData.Disk.dk_summary, strDetail);
                    if (stLen > 0)
                    {
                        if (strlen(EventParam->ev_context) > 0)
                        {
                            strcat(EventParam->ev_context, ",");
                            strcat(EventParam->ev_context, strDetail);
                        }
                        else strcpy(EventParam->ev_context, strDetail);
                    }
                }
                else
                {
                    stLen = fjson_GetLengthDiskRemovable(&DetectData.Disk);
                }
                rxt = fdbif_MergeEventTb(EventParam, stLen);
                if(rxt < 0)
                {
                    memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                               (void *)EventParam,
                               sizeof(_DAP_EventParam) );
                    *EventCount += 1;
                }
                else
                {
                    if(rxt > 0)
                    {
                        *bNotiFlag = 1;
                        if(rxt != 99)
                        {
                            fdbif_FQEventToPcif(EventParam);
                            if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                            {
                                memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                fdbif_FQEventToAlarm(AlarmQData, EventParam);
                            }
                            if(g_stProcDbifInfo.nEorActive == 1)
                            {
                                fdbif_WriteEor(EventParam);
                            }
                        }
                    }
                }
                //DISK_MOBILE_READ 가 0이면 해지해준다.
                stLen = fdbif_GetLengthDiskMobileRead(&DetectData.Disk);
                if(stLen == 0)
                {
                    EventParam->ev_type		= DISK_MOBILE_READ;
                    if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                        strcpy(	EventParam->ev_context, "ext");
                    else
                        strcpy(	EventParam->ev_context, "ext");
                    rxt = fdbif_MergeEventTb(EventParam, stLen);
                    if(rxt < 0)
                    {
                        memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                                   (void *)EventParam,
                                   sizeof(_DAP_EventParam) );
                        *EventCount += 1;
                    }
                    else
                    {
                        if(rxt > 0)
                        {
                            *bNotiFlag = 1;
                            if(rxt != 99)
                            {
                                fdbif_FQEventToPcif(EventParam);
                                if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                                {
                                    memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                    fdbif_FQEventToAlarm(AlarmQData, EventParam);
                                }
                                if(g_stProcDbifInfo.nEorActive == 1)
                                {
                                    fdbif_WriteEor(EventParam);
                                }
                            }
                        }
                    }
                }
                //DISK_MOBILE_WRITE 가 0이면 해지해준다.
                stLen = fdbif_GetLengthDiskMobileWrite(&DetectData.Disk);
                if(stLen == 0)
                {
                    EventParam->ev_type		= DISK_MOBILE_WRITE;
                    if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                        strcpy(	EventParam->ev_context, "ext");
                    else
                        strcpy(	EventParam->ev_context, "");
                    rxt = fdbif_MergeEventTb(EventParam, stLen);
                    if(rxt < 0)
                    {
                        memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                                   (void *)EventParam,
                                   sizeof(_DAP_EventParam) );
                        *EventCount += 1;
                    }
                    else
                    {
                        if(rxt > 0)
                        {
                            *bNotiFlag = 1;
                            if(rxt != 99)
                            {
                                fdbif_FQEventToPcif(EventParam);
                                if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                                {
                                    memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                    fdbif_FQEventToAlarm(AlarmQData, EventParam);
                                }
                                if(g_stProcDbifInfo.nEorActive == 1)
                                {
                                    fdbif_WriteEor(EventParam);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                //DISK_MOBILE_READ or WRITE가활성화되어있다면 DISK_MOBILE이 해지된지 보고 해지한다
                stLen = fjson_GetLengthDiskRemovable(&DetectData.Disk);
                if(stLen == 0)
                {
                    EventParam->ev_type		= DISK_MOBILE;
                    if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
                        strcpy(	EventParam->ev_context, "ext");
                    else
                        strcpy(	EventParam->ev_context, "");
                    rxt = fdbif_MergeEventTb(EventParam, stLen);
                    if(rxt < 0)
                    {
                        memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                                   (void *)EventParam,
                                   sizeof(_DAP_EventParam) );
                        *EventCount += 1;
                    }
                    else
                    {
                        if(rxt > 0)
                        {
                            *bNotiFlag = 1;
                            if(rxt != 99)
                            {
                                fdbif_FQEventToPcif(EventParam);
                                if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                                {
                                    memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                                    fdbif_FQEventToAlarm(AlarmQData, EventParam);
                                }
                                if(g_stProcDbifInfo.nEorActive == 1)
                                {
                                    fdbif_WriteEor(EventParam);
                                }
                            }
                        }
                    }
                }
            }
        }

        //DISK_HIDDEN 검출
        WRITE_INFO(CATEGORY_DB, "[EVENT] DISK_HIDDEN ");
        EventParam->ev_type		= DISK_HIDDEN;
        EventParam->ev_level     = (DetectData.Disk.dk_hidden_rule-'0')%48;
        EventParam->ru_sq     	= DetectData.Disk.dk_hidden_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthDiskHidden(&DetectData.Disk);
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskNetDrive(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int* buffLen
)
{
    int rxt = 0;
    int	stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] NetDrive ");
    memcpy(	(void *)&DetectData.NetDrive,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.NetDrive) );
    *buffLen += sizeof(DetectData.NetDrive);
    fjson_PrintAllNetDrive(&DetectData.NetDrive);

    rxt = fdbif_MergeNetDriveTb(
                                 Detect->AgentInfo.user_key,
                                 Detect->AgentInfo.user_seq,
                                 Detect->detect_time,
                                 &DetectData.NetDrive,
                                 param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.NetDrive,
                   sizeof(DetectData.NetDrive) );
        *retryBuffLen += sizeof(DetectData.NetDrive);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "|[EVENT] NET_DRIVE ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,	Detect->AgentInfo.user_ip);
        EventParam->user_seq		  = Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		    = NET_DRIVE;
        EventParam->ev_level		= (DetectData.NetDrive.nd_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq     	= DetectData.NetDrive.nd_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");

        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.NetDrive.nd_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthNetDrive(&DetectData.NetDrive);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskOsAccount(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int	*buffLen
)
{
    int rxt = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] OSAccount ");
    memcpy(	(void *)&DetectData.OSAccount,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.OSAccount) );
    *buffLen += sizeof(DetectData.OSAccount);
    fjson_PrintAllOsAccount(&DetectData.OSAccount);

    rxt = fdbif_MergeOsAccountTb(
                                  Detect->AgentInfo.user_key,
                                  Detect->AgentInfo.user_seq,
                                  Detect->detect_time,
                                  &DetectData.OSAccount,
                                  param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.OSAccount,
                   sizeof(DetectData.OSAccount) );
        *retryBuffLen += sizeof(DetectData.OSAccount);
    }
    return RET_SUCC;
}

int fdbif_TaskShareFolder(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
)
{
    int rxt = 0;

    int stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] ShareFolder ");
    memcpy(	(void *)&DetectData.ShareFolder,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.ShareFolder) );
    *buffLen += sizeof(DetectData.ShareFolder);
    fjson_PrintAllShareFolder(&DetectData.ShareFolder);
    rxt = fdbif_MergeShareFolderTb(Detect->AgentInfo.user_key,
                                Detect->AgentInfo.user_seq,
                                Detect->detect_time,
                                &DetectData.ShareFolder,
                                param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.ShareFolder,
                   sizeof(DetectData.ShareFolder) );
        *retryBuffLen += sizeof(DetectData.ShareFolder);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] SHARE_FOLDER ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,	Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		    = SHARE_FOLDER;
        EventParam->ev_level		= (DetectData.ShareFolder.sf_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq     	= DetectData.ShareFolder.sf_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.ShareFolder.sf_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else
                    strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthShareFolder(&DetectData.ShareFolder);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskInfrared(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen)
{
    int rxt = 0;
    int	stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] InfraredDevice ");
    memcpy(	(void *)&DetectData.InfraredDevice,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.InfraredDevice) );
    *buffLen += sizeof(DetectData.InfraredDevice);
    fjson_PrintAllInfraredDevice(&DetectData.InfraredDevice);

    rxt = fdbif_MergeInfraredDeviceTb(	Detect->AgentInfo.user_key,
                                       Detect->AgentInfo.user_seq,
                                       Detect->detect_time,
                                       &DetectData.InfraredDevice,
                                       param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy( (void *)&RetryQData->buf[*retryBuffLen],
                (void *)&DetectData.InfraredDevice,
                sizeof(DetectData.InfraredDevice) );
        *retryBuffLen += sizeof(DetectData.InfraredDevice);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] INFRARED_DEVICE ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= INFRARED_DEVICE;
        EventParam->ev_level		= (DetectData.InfraredDevice.id_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq     	= DetectData.InfraredDevice.id_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.InfraredDevice.id_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else
                    strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthInfraredDevice(&DetectData.InfraredDevice);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

/* ���μ������� HW_BASE_TB ó�� */
int fdbif_TaskProcess(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
)
{
    int rxt = 0;
    int stLen = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));


    WRITE_INFO(CATEGORY_DB, "[MRG] Process ");
    memcpy(	(void *)&DetectData.Process,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Process) );
    *buffLen += sizeof(DetectData.Process);
    fjson_PrintAllProcess(&DetectData.Process);
    rxt = fdbif_MergeProcessTb(	Detect->AgentInfo.user_key,
                               Detect->AgentInfo.user_seq,
                               Detect->detect_time,
                               &DetectData.Process,
                               param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Process,
                   sizeof(DetectData.Process) );
        *retryBuffLen += sizeof(DetectData.Process);
    }
    else
    {
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);

        WRITE_INFO(CATEGORY_DB, "[EVENT] PROCESS_WHITE ");
        EventParam->ev_type		= PROCESS_WHITE;
        EventParam->ev_level	= (DetectData.Process.ps_white_rule-'0')%48;
        EventParam->ru_sq     	= DetectData.Process.ps_white_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthProcessWhiteSummary(DetectData.Process.ps_summary);
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_DB, "[EVENT] PROCESS_BLACK ");
        EventParam->ev_type		= PROCESS_BLACK;
        EventParam->ev_level     = (DetectData.Process.ps_black_rule-'0')%48;
        EventParam->ru_sq     	= DetectData.Process.ps_black_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthProcessBlackSummary(DetectData.Process.ps_summary);
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA && stLen == -1)
            strcat(	EventParam->ev_context, ",kill");
        else
            strcat(	EventParam->ev_context, "kill");
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_DB, "[EVENT] PROCESS_ACCESSMON ");
        EventParam->ev_type		= PROCESS_ACCESSMON;
        EventParam->ev_level     = (DetectData.Process.ps_accessmon_rule-'0')%48;
        EventParam->ru_sq     	= DetectData.Process.ps_accessmon_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthProcessAccessmon(&DetectData.Process);
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_DB, "[EVENT] PROCESS_BLOCK(VM) ");
        // PROCESS_BLOCK 룰이 정의되지않고 PROCESS_BLACK 룰을 사용
        EventParam->ev_type		= PROCESS_BLOCK;
        // ps_block_rule이 없으므로 5(CRITICAL)를 그냥 넣음
        EventParam->ev_level     = 5;
        EventParam->ru_sq     	= DetectData.Process.ps_black_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthProcessBlockSummary(DetectData.Process.ps_summary);
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
        {
            if (stLen == 1)  		strcat(EventParam->ev_context, ",off");
            else if (stLen == 2)	strcat(EventParam->ev_context, ",kill");
        }
        else
        {
            if (stLen == 1) 		strcat(	EventParam->ev_context, "off");
            else if (stLen == 2)	strcat(	EventParam->ev_context, "kill");
        }
        if (stLen == 1 || stLen == 2) // VM kill 또는 off가 발생했을때만
        {
            rxt = fdbif_MergeEventTb(EventParam, stLen);
            if(rxt < 0)
            {
                memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                           (void *)EventParam,
                           sizeof(_DAP_EventParam) );
                *EventCount += 1;
            }
            else
            {
                if(rxt > 0)
                {
                    *bNotiFlag = 1;
                    if(rxt != 99)
                    {
                        fdbif_FQEventToPcif(EventParam);
                        if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                        {
                            memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                            fdbif_FQEventToAlarm(AlarmQData, EventParam);
                        }
                        if(g_stProcDbifInfo.nEorActive == 1)
                        {
                            fdbif_WriteEor(EventParam);
                        }
                    }
                }
            }
        } // if (stLen == 1 || stLen == 2)
    }

    return RET_SUCC;
}

int fdbif_TaskRouter(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen)
{
    int rxt = 0;
    int stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] Router ");
    memcpy(	(void *)&DetectData.Router,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.Router) );
    *buffLen += sizeof(DetectData.Router);
    fjson_PrintAllRouter(&DetectData.Router);

    rxt = fdbif_MergeRouterTb(	Detect->AgentInfo.user_key,
                              Detect->AgentInfo.user_seq,
                              Detect->detect_time,
                              &DetectData.Router,
                              param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.Router,
                   sizeof(DetectData.Router) );
        *retryBuffLen += sizeof(DetectData.Router);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] ROUTER ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= ROUTER;
        EventParam->ev_level		= (DetectData.Router.rt_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,		param_Postfix);
        EventParam->ru_sq     	= DetectData.Router.rt_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.Router.rt_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else
                    strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthRouter(&DetectData.Router);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }

    return RET_SUCC;

}

int fdbif_TaskNetPrinter(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen)
{
    int rxt = 0;
    int	stLen = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] NetPrinter " );
    memcpy(	(void *)&DetectData.NetPrinter,
               (void *)&(RsltQData->buf[*buffLen]),
               sizeof(DetectData.NetPrinter) );
    *buffLen += sizeof(DetectData.NetPrinter);
    fjson_PrintAllNetPrinter(&DetectData.NetPrinter);
    rxt = fdbif_MergeNetPrinterTb(	Detect->AgentInfo.user_key,
                                   Detect->AgentInfo.user_seq,
                                   Detect->detect_time,
                                   &DetectData.NetPrinter,
                                   param_Prefix, param_Postfix);

    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.NetPrinter,
                   sizeof(DetectData.NetPrinter) );
        *retryBuffLen += sizeof(DetectData.NetPrinter);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] NET_PRINTER ");
        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,	Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= NET_PRINTER;
        EventParam->ev_level		= (DetectData.NetPrinter.np_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,	param_Postfix);
        EventParam->ru_sq     	= DetectData.NetPrinter.np_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        stLen = fjson_GetLengthNetPrinter(&DetectData.NetPrinter);
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&(EventQData->buf[*EventCount * sizeof(_DAP_EventParam)]),
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskNetScan(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen)
{
    int rxt = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] NET_SCAN ");
    memcpy(	(void *)&DetectData.NetScan,
               (void *)&(RsltQData->buf[*buffLen]),
               sizeof(DetectData.NetScan) );
    *buffLen += sizeof(DetectData.NetScan);

    fjson_PrintAllNetScan(&DetectData.NetScan);
    rxt = fdbif_MergeNetScanTb(Detect->AgentInfo.user_key,
                            Detect->AgentInfo.user_seq,
                            Detect->detect_time,
                            &DetectData.NetScan,
                            param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.NetScan,
                   sizeof(DetectData.NetScan) );
        *retryBuffLen += sizeof(DetectData.NetScan);
    }
    return RET_SUCC;
}

int fdbif_TaskSsoCert(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *EventCount,
        int *bNotiFlag,
        int *buffLen
)
{
    int rxt = 0;
    int	stLen = 0;
    _DAP_DETECT_DATA  DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] SSO_CERT ");

    memcpy(	(void *)&DetectData.SsoCert,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.SsoCert) );
    *buffLen += sizeof(DetectData.SsoCert);

    fjson_PrintAllSsoCert(&DetectData.SsoCert);

    WRITE_INFO(CATEGORY_DB, "[EVENT] SSO_CERT ");
    memset(EventParam, 0x00, sizeof(_DAP_EventParam));
    strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
    strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
    EventParam->user_seq		= Detect->AgentInfo.user_seq;
    strcpy(	EventParam->detect_time,	Detect->detect_time);
    EventParam->ev_type		    = SSO_CERT;
    EventParam->ev_level		= (DetectData.SsoCert.sc_rule-'0')%48;
    strcpy(	EventParam->prefix,		param_Prefix);
    strcpy(	EventParam->postfix,		param_Postfix);
    EventParam->ru_sq     	= DetectData.SsoCert.sc_rusq;
    if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
        strcpy(	EventParam->ev_context, "ext");
    else
        strcpy(	EventParam->ev_context, "");
    stLen = fjson_GetLengthSsoCert(&DetectData.SsoCert);
    rxt = fdbif_MergeEventTb(EventParam, stLen);
    if(rxt < 0)
    {
        memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                   (void *)&EventParam,
                   sizeof(_DAP_EventParam) );
        *EventCount += 1;
    }
    else
    {
        if(rxt > 0)
        {
            *bNotiFlag = 1;
            if(rxt != 99)
            {
                fdbif_FQEventToPcif(EventParam);
                if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                {
                    memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                    fdbif_FQEventToAlarm(AlarmQData, EventParam);
                }
                if(g_stProcDbifInfo.nEorActive == 1)
                {
                    fdbif_WriteEor(EventParam);
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskWinDrv(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen
)
{
    int rxt = 0;
    _DAP_DETECT_DATA DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_DB, "[MRG] WIN_DRV ");

    memcpy(	(void *)&DetectData.WinDrv,
               (void *)&(RsltQData->buf[*buffLen]),
               sizeof(DetectData.WinDrv) );
    *buffLen += sizeof(DetectData.WinDrv);
    fjson_PrintAllWinDrv(&DetectData.WinDrv);

    rxt = fdbif_MergeWinDrvTb(
                               Detect->AgentInfo.user_key,
                               Detect->AgentInfo.user_seq,
                               Detect->detect_time,
                               &DetectData.WinDrv,
                               param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.WinDrv,
                   sizeof(DetectData.WinDrv) );
        *retryBuffLen += sizeof(DetectData.WinDrv);
    }
    return RET_SUCC;
}

int fdbif_TaskRdpSession(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen)
{
    int rxt = 0;
    int stLen = 0;
    char strDetail[512 +1] = {0x00,};
    _DAP_DETECT_DATA DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));

    WRITE_INFO(CATEGORY_INFO, "[MRG] RDP_SESSION " );
    memcpy(	(void *)&DetectData.RdpSession,
               (void *)&RsltQData->buf[*buffLen],
               sizeof(DetectData.RdpSession) );
    *buffLen += sizeof(DetectData.RdpSession);

    fjson_PrintAllRdpSession(&DetectData.RdpSession);

    rxt = fdbif_MergeRdpSessionTb(
                                   Detect->AgentInfo.user_key,
                                   Detect->AgentInfo.user_seq,
                                   Detect->detect_time,
                                   &DetectData.RdpSession,
                                   param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.RdpSession,
                   sizeof(DetectData.RdpSession) );
        *retryBuffLen += sizeof(DetectData.RdpSession);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "[EVENT] RDP_SESSION ");

        memset(EventParam, 0x00, sizeof(_DAP_EventParam));

        strcpy(	EventParam->user_key,	Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,	Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        EventParam->ev_type		= RDP_SESSION;
        EventParam->ev_level		= (DetectData.RdpSession.rdp_session_rule-'0')%48;
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,	param_Postfix);
        EventParam->ru_sq		= DetectData.RdpSession.rdp_session_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");
        // check BLOCK
        if (EventParam->ev_level == BLOCK)
        {
            memset(strDetail, 0x00, sizeof(strDetail));
            stLen = fjson_GetBlockcommonSummary(DetectData.RdpSession.rdp_summary, strDetail);
            if (stLen > 0)
            {
                if (strlen(EventParam->ev_context) > 0)
                {
                    strcat(EventParam->ev_context, ",");
                    strcat(EventParam->ev_context, strDetail);
                }
                else strcpy(EventParam->ev_context, strDetail);
            }
        }
        else
        {
            stLen = fjson_GetLengthRdpSession(&DetectData.RdpSession);
        }
        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[*EventCount * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return RET_SUCC;
}

int fdbif_TaskCpuUsage(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen

)
{
    int rxt = 0;
    int stLen = 0;
    _DAP_DETECT_DATA DetectData;

    memset(&DetectData, 0x00, sizeof(_DAP_DETECT_DATA));
    memset(&DetectData.CpuUsage, 0x00, sizeof(_DAP_CPU_USAGE));

    WRITE_INFO(CATEGORY_INFO, "[MRG] CPU_USAGE " );
    memcpy(  (void *)&DetectData.CpuUsage,
             (void *)&RsltQData->buf[*buffLen],
             sizeof(DetectData.CpuUsage) );
    *buffLen += sizeof(DetectData.CpuUsage);
    fjson_PrintAllCpuUsage(&DetectData.CpuUsage);
    rxt = fdbif_MergeCpuUsageTb( Detect->AgentInfo.user_key,
                             Detect->AgentInfo.user_seq,
                             Detect->detect_time,
                             &DetectData.CpuUsage,
                             param_Prefix, param_Postfix);
    if(rxt < 0)
    {
        if(rxt == -2)
            *param_bEmptyBase = 1;
        memcpy(	(void *)&RetryQData->buf[*retryBuffLen],
                   (void *)&DetectData.CpuUsage,
                   sizeof(DetectData.CpuUsage) );
        *retryBuffLen += sizeof(DetectData.CpuUsage);
    }
    else
    {

        memset(EventParam, 0x00, sizeof(_DAP_EventParam));
        strcpy(	EventParam->user_key,	    Detect->AgentInfo.user_key);
        strcpy(	EventParam->user_ip,		Detect->AgentInfo.user_ip);
        EventParam->user_seq		= Detect->AgentInfo.user_seq;
        strcpy(	EventParam->detect_time,	Detect->detect_time);
        strcpy(	EventParam->prefix,		param_Prefix);
        strcpy(	EventParam->postfix,	param_Postfix);

        WRITE_INFO(CATEGORY_DB,"[EVENT] CPU_USAGE_ALARM " );
        EventParam->ev_type		= CPU_USAGE_ALARM;
        EventParam->ev_level	= (DetectData.CpuUsage.cpu_alarm_rule-'0')%48;
        EventParam->ru_sq     	= DetectData.CpuUsage.cpu_alarm_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");

        // stLen = (-1) : Alarm Off , stLen = 0 : Alarm On
        stLen = fjson_GetLengthAlarmCpuUsage(&DetectData.CpuUsage);

        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[ (*EventCount) * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
        WRITE_INFO(CATEGORY_INFO, "[EVENT] CPU_USAGE_CONTROL " );
        EventParam->ev_type	     	= CPU_USAGE_CONTROL;
        EventParam->ev_level        = (DetectData.CpuUsage.cpu_ctrl_rule-'0')%48;
        EventParam->ru_sq       	= DetectData.CpuUsage.cpu_ctrl_rusq;
        if(RsltQData->packtype == EXTERNAL_DETECT_DATA)
            strcpy(	EventParam->ev_context, "ext");
        else
            strcpy(	EventParam->ev_context, "");

        // stLen = (-1) : Control Off , stLen = 0 : Control On
        stLen = fjson_GetLengthControlCpuUsage(&DetectData.CpuUsage);

        rxt = fdbif_MergeEventTb(EventParam, stLen);
        if(rxt < 0)
        {
            memcpy(	(void *)&EventQData->buf[ (*EventCount) * sizeof(_DAP_EventParam)],
                       (void *)EventParam,
                       sizeof(_DAP_EventParam) );
            *EventCount += 1;
        }
        else
        {
            if(rxt > 0)
            {
                *bNotiFlag = 1;
                if(rxt != 99)
                {
                    fdbif_FQEventToPcif(EventParam);
                    if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                    {
                        memset(AlarmQData, 0x00, sizeof(_DAP_QUEUE_BUF));
                        fdbif_FQEventToAlarm(AlarmQData, EventParam);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(EventParam);
                    }
                }
            }
        }
    }
    return 0;
}

