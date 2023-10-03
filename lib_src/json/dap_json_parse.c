//
// Created by KimByoungGook on 2020-07-01.
//

#include <stdio.h>
#include <string.h>
#include <openssl/des.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "json/dap_json.h"
#include "json/jansson.h"
#include "json/jansson_config.h"
#include "json/dap_defstr.h"
#include "com/dap_req.h"
#include "com/dap_com.h"


void fjson_ParseJsonString(json_t *element)
{
    (void *)json_string_value(element);
}

void fjson_ParseJsonInteger(json_t *element)
{
    json_integer_value(element);
}
void fjson_ParseJsonObjectFt(
        json_t*			element,
        char*			pkey,
        char*			skey,
        int				itemIdx,
        _DAP_DETECT*	Dt,
        _DAP_DETECT_DATA* DtDa,
        char*			cpip)
{
    const char *key = NULL;
    json_t *value;

    json_object_size(element);

    json_object_foreach(element, key, value)
    {
        if( !strcmp(key, STR_AGENT_VER) )
        {
            fjson_LogJsonAuxFt(5, key, value, 0, cpip);
            snprintf(Dt->AgentInfo.agent_ver, sizeof(Dt->AgentInfo.agent_ver), "%s", json_string_value(value));
        }
        else if( !strcmp(key, STR_USER_KEY) )
        {
            fjson_LogJsonAuxFt(5, key, value, 0, cpip);
            snprintf(Dt->AgentInfo.user_key, sizeof(Dt->AgentInfo.user_key), "%s", json_string_value(value));
        }
        else if( !strcmp(key, STR_USER_SEQ) )
        {
            fjson_LogJsonAuxFt(5, key, value, 0, cpip);
            Dt->AgentInfo.user_seq = json_integer_value(value);
        }
        else if( !strcmp(key, STR_CHANGE_ITEM) )
        {
            fjson_LogJsonAuxFt(5, key, value, 0, cpip);
            snprintf( Dt->change_item, sizeof(Dt->change_item), "%s", json_string_value(value));
        }
        else if( !strcmp(key, STR_DETECT_TIME) )
        {
            fjson_LogJsonAuxFt(5, key, value, 0, cpip);
            snprintf( Dt->detect_time, sizeof(Dt->detect_time), "%s", json_string_value(value));
        }
        else if( !strcmp(key, STR_DATA) )
        {
            WRITE_INFO_IP( cpip, "\""STR_DATA"\"\n");
        }
        // MAIN_BOARD(1)
        if( !strcmp(key, STR_MAIN_BOARD) )
        {
            strcpy(pkey, STR_MAIN_BOARD);
            WRITE_INFO_IP( cpip, "--\""STR_MAIN_BOARD"\"\n");
        }
        else if( !strcmp(pkey, STR_MAIN_BOARD) )
        {
            fjson_ParseJsonMainBoardFt(key, value, DtDa, cpip);
        }
        // SYSTEM(2)
        if( !strcmp(key, STR_SYSTEM) )
        {
            strcpy(pkey, STR_SYSTEM);
            WRITE_INFO_IP( cpip, "--\""STR_SYSTEM"\"\n");
        }
        else if( !strcmp(pkey, STR_SYSTEM) )
        {
            jfson_ParseJsonSystemFt(key, skey, value, DtDa, cpip);
        }
        // OPERATING_SYSTEM(3)
        if( !strcmp(key, STR_OPERATING_SYSTEM) )
        {
            strcpy(pkey, STR_OPERATING_SYSTEM);
            WRITE_INFO_IP( cpip, "--\""STR_OPERATING_SYSTEM"\"\n");
        }
        else if( !strcmp(pkey, STR_OPERATING_SYSTEM) )
        {
            fjson_ParseJsonOperatingSystemFt(key, value, DtDa, cpip);
        }
        // CPU(4)
        if( !strcmp(key, STR_CPU) )
        {
            strcpy(pkey, STR_CPU);
            WRITE_INFO_IP( cpip, "--\""STR_CPU"\"\n");
        }
        else if( !strcmp(pkey, STR_CPU) )
        {
            fjson_ParseJsonCpuFt(key, &itemIdx, value, DtDa, cpip);
        }
        // NET_ADAPTER(5)
        if( !strcmp(key, STR_NET_ADAPTER) )
        {
            strcpy(pkey, STR_NET_ADAPTER);
            WRITE_INFO_IP( cpip, "--\""STR_NET_ADAPTER"\"\n");
        }
        else if( !strcmp(pkey, STR_NET_ADAPTER) )
        {
            fjson_ParseJsonNetAdapterFt(key, &itemIdx, value, DtDa, cpip);
        }
        // WIFI(6)
        if( !strcmp(key, STR_WIFI) )
        {
            strcpy(pkey, STR_WIFI);
            WRITE_INFO_IP( cpip, "--\""STR_WIFI"\"\n");
        }
        else if( !strcmp(pkey, STR_WIFI) )
        {
            fjson_ParseJsonWifiFt(key, &itemIdx, value, DtDa, cpip);
        }
        // BLUETOOTH(7)
        if( !strcmp(key, STR_BLUETOOTH) )
        {
            strcpy(pkey, STR_BLUETOOTH);
            WRITE_INFO_IP( cpip, "--\""STR_BLUETOOTH"\"\n");
        }
        else if( !strcmp(pkey, STR_BLUETOOTH) )
        {
            fjson_ParseJsonBluetoothFt(key, &itemIdx, value, DtDa, cpip);
        }
        // NET_CONNECTION(8)
        if( !strcmp(key, STR_NET_CONNECTION) )
        {
            strcpy(pkey, STR_NET_CONNECTION);
            WRITE_INFO_IP( cpip, "--\""STR_NET_CONNECTION"\"\n");
        }
        else if( !strcmp(pkey, STR_NET_CONNECTION) )
        {
            fjson_ParseJsonNetConnectionFt(key, &itemIdx, value, DtDa, cpip);
        }
        // DISK(9)
        if( !strcmp(key, STR_DISK) )
        {
            strcpy(pkey, STR_DISK);
            WRITE_INFO_IP( cpip, "--\""STR_DISK"\"\n");
        }
        else if( !strcmp(pkey, STR_DISK) )
        {
            fjson_ParseJsonDiskFt(key, &itemIdx, value, DtDa, cpip);
        }
        // NET_DRIVE(10)
        if( !strcmp(key, STR_NET_DRIVE) )
        {
            strcpy(pkey, STR_NET_DRIVE);
            WRITE_INFO_IP( cpip, "--\""STR_NET_DRIVE"\"\n");
        }
        else if( !strcmp(pkey, STR_NET_DRIVE) )
        {
            fjson_ParseJsonNetDriveFt(key, &itemIdx, value, DtDa, cpip);
        }
        // 2023.02.11 기업은행 디버깅을 위해 주석처리
        // OS_ACCOUNT, NET_SCAN, WIN_DRV 데이터 수신 Drop 처리
        // OS_ACCOUNT(11)
//        if( !strcmp(key, STR_OS_ACCOUNT) )
//        {
//            strcpy(pkey, STR_OS_ACCOUNT);
//            WRITE_INFO_IP( cpip, "--\""STR_OS_ACCOUNT"\"\n");
//        }
//        else if( !strcmp(pkey, STR_OS_ACCOUNT) )
//        {
//            fjson_ParseJsonOsAccountFt(key, &itemIdx, value, DtDa, cpip);
//        }

        // SHARE_FOLDER(12)
        if( !strcmp(key, STR_SHARE_FOLDER) )
        {
            strcpy(pkey, STR_SHARE_FOLDER);
            WRITE_INFO_IP( cpip, "--\""STR_SHARE_FOLDER"\"\n");
        }
        else if( !strcmp(pkey, STR_SHARE_FOLDER) )
        {
            fjson_ParseJsonShareFolderFt(key, &itemIdx, value, DtDa, cpip);
        }
        // INFRARED_DEVICE(13)
        if( !strcmp(key, STR_INFRARED_DEVICE) )
        {
            strcpy(pkey, STR_INFRARED_DEVICE);
            WRITE_INFO_IP( cpip, "--\""STR_INFRARED_DEVICE"\"\n");
        }
        else if( !strcmp(pkey, STR_INFRARED_DEVICE) )
        {
            fjson_ParseJsonInfraredDeviceFt(key, &itemIdx, value, DtDa, cpip);
        }
        // PROCESS(14)
        if( !strcmp(key, STR_PROCESS) )
        {
            strcpy(pkey, STR_PROCESS);
            WRITE_INFO_IP( cpip, "--\""STR_PROCESS"\"\n");
        }
        else if( !strcmp(pkey, STR_PROCESS) )
        {
            fjson_ParseJsonProcessFt(key, skey, &itemIdx, value, DtDa, cpip);
        }
        // ROUTER(15)
        if( !strcmp(key, STR_ROUTER) )
        {
            strcpy(pkey, STR_ROUTER);
            WRITE_INFO_IP( cpip, "--\""STR_ROUTER"\"\n");
        }
        else if( !strcmp(pkey, STR_ROUTER) )
        {
            fjson_ParseJsonRouterFt(key, &itemIdx, value, DtDa, cpip);
        }
        // NET_PRINTER(16)
        if( !strcmp(key, STR_NET_PRINTER) )
        {
            strcpy(pkey, STR_NET_PRINTER);
            WRITE_INFO_IP( cpip, "--\""STR_NET_PRINTER"\"\n");
        }
        else if( !strcmp(pkey, STR_NET_PRINTER) )
        {
            fjson_ParseJsonNetPrinterFt(key, skey, &itemIdx, value, DtDa, cpip);
        }
        // NET_SCAN(17)
        // 2023.02.11 기업은행 디버깅을 위해 주석처리
        // OS_ACCOUNT, NET_SCAN, WIN_DRV 데이터 수신 Drop 처리
//        if( !strcmp(key, STR_NET_SCAN) )
//        {
//            strcpy(pkey, STR_NET_SCAN);
//            WRITE_INFO_IP( cpip, "--\""STR_NET_SCAN"\"\n");
//        }
//        else if( !strcmp(pkey, STR_NET_SCAN) )
//        {
//            fjson_ParseJsonNetScanFt(key, skey, &itemIdx, value, DtDa, cpip);
//        }

        // ARP(18)
        // VIRTUAL_MACHINE(19)
        // CONNECT_EXT_SVR(20)
        if( !strcmp(key, STR_CONNECT_EXT_SVR) )
        {
            strcpy(pkey, STR_CONNECT_EXT_SVR);
            WRITE_INFO_IP( cpip, "--\""STR_CONNECT_EXT_SVR"\"\n");
        }
        else if( !strcmp(pkey, STR_CONNECT_EXT_SVR) )
        {
            fjson_ParseJsonConnectExtSvrFt(key, &itemIdx, value, DtDa, cpip);
        }
        // NET_ADAPTER_OVER(21)
        // NET_ADAPTER_DUPIP(22)
        // NET_ADAPTER_DUPMAC(23)
        // DISK_REG(24)
        // DISK_HIDDEN(25)
        // DISK_NEW(26)
        // DISK_MOBILE(27)
        // DISK_MOBILE_READ(28)
        // DISK_MOBILE_WRITE(29)
        // PROCESS_WHITE(30)
        // PROCESS_BLACK(31)
        // PROCESS_ACCESSMON(32)
        // NET_ADAPTER_MULIP(33)
        // EXTERNAL_CONN(34)
        // SSO_CERT(35)
        // WIN_DRV(36)
        if( !strcmp(key, STR_SSO_CERT) )
        {
            strcpy(pkey, STR_SSO_CERT);
            WRITE_INFO_IP( cpip, "--\""STR_SSO_CERT"\"\n");
        }
        else if( !strcmp(pkey, STR_SSO_CERT) )
        {
            fjson_ParseJsonSsoCertFt(key, value, DtDa, cpip);
        }

        // 2023.02.11 기업은행 디버깅을 위해 주석처리
        // OS_ACCOUNT, NET_SCAN, WIN_DRV 데이터 수신 Drop 처리
        // WIN_DRV(36)
//        if( !strcmp(key, STR_WIN_DRV) )
//        {
//            strcpy(pkey, STR_WIN_DRV);
//            WRITE_INFO_IP( cpip, "--\""STR_WIN_DRV"\"\n");
//        }
//        else if( !strcmp(pkey, STR_WIN_DRV) )
//        {
//            fjson_ParseJsonWinDrvFt(key, &itemIdx, value, DtDa, cpip);
//        }

        // RDP_SESSION(38)
        if( !strcmp(key, STR_RDP_SESSION) )
        {
            strcpy(pkey, STR_RDP_SESSION);
            WRITE_INFO(CATEGORY_INFO,"--\""STR_RDP_SESSION"\"\n");
        }
        else if( !strcmp(pkey, STR_RDP_SESSION) )
        {
            fjson_ParseJsonRdpSession(key, &itemIdx, value, DtDa, cpip);
        }
        // CPU_USAGE(39)
        if( !strcmp(key, STR_CPU_USAGE ) )
        {
            strcpy(pkey, STR_CPU_USAGE);
            WRITE_INFO_IP(cpip,"--\""STR_CPU_USAGE"\"\n");
        }
        else if( !strcmp(pkey, STR_CPU_USAGE) )
        {
            fjson_ParseJsonCpuUsage(key, &itemIdx, value, DtDa, cpip);
        }

        if( !strcmp(key, STR_SUMMARY) )
        {
            char	strSummary[4096];
            memset(strSummary, 0x00, sizeof(strSummary));

            WRITE_INFO_IP( cpip, "----\""STR_SUMMARY"\"\n");
//            fjson_GetSummaryFt(value, pkey, strSummary);
            if(!strcmp(pkey,STR_CPU_USAGE))
            {
                fjson_GetSummaryCpuUsage(element, DtDa, cpip);
            }
            else
            {
                fjson_GetSummaryFt(value, pkey, strSummary);
            }
            WRITE_INFO_IP( cpip, "------\"%s\"\n", strSummary);

            if( !strcmp(pkey, STR_SYSTEM) )
            {
                snprintf(DtDa->System.st_summary, sizeof(DtDa->System.st_summary),
                         "%s", strSummary);
            }

            else if( !strcmp(pkey, STR_OPERATING_SYSTEM) )
            {
                snprintf(DtDa->OperatingSystem.os_summary, sizeof(DtDa->OperatingSystem.os_summary),
                         "%s", strSummary);
            }
            else if( !strcmp(pkey, STR_CPU) )
            {
                snprintf(DtDa->Cpu.cu_summary,
                         sizeof(DtDa->Cpu.cu_summary),
                         "%s", strSummary);
            }
            else if( !strcmp(pkey, STR_NET_ADAPTER) )
            {
                snprintf(DtDa->NetAdapter.na_summary,
                         sizeof(DtDa->NetAdapter.na_summary), "%s", strSummary);
            }
            else if( !strcmp(pkey, STR_WIFI) )
            {
                snprintf(DtDa->Wifi.wf_summary,
                         sizeof(DtDa->Wifi.wf_summary),
                         "%s", strSummary);
            }
            else if( !strcmp(pkey, STR_BLUETOOTH) )
            {
                snprintf(DtDa->Bluetooth.bt_summary,
                         sizeof(DtDa->Bluetooth.bt_summary),
                         "%s", strSummary);
            }
            else if( !strcmp(pkey, STR_NET_CONNECTION) )
            {
                snprintf(DtDa->NetConnection.nc_summary,
                         sizeof(DtDa->NetConnection.nc_summary),
                         "%s", strSummary);
            }

            else if( !strcmp(pkey, STR_DISK) )
            {
                snprintf(DtDa->Disk.dk_summary,
                         sizeof(DtDa->Disk.dk_summary),
                         "%s", strSummary);
            }

            else if( !strcmp(pkey, STR_NET_DRIVE) )
            {
                snprintf(DtDa->NetDrive.nd_summary,
                         sizeof(DtDa->NetDrive.nd_summary),
                         "%s", strSummary);
            }

            // 2023.02.11 기업은행 디버깅을 위해 주석처리
            // OS_ACCOUNT, NET_SCAN, WIN_DRV 데이터 수신 Drop 처리
//            else if( !strcmp(pkey, STR_OS_ACCOUNT) )
//            {
//                snprintf(DtDa->OSAccount.oa_summary,
//                         sizeof(DtDa->OSAccount.oa_summary),
//                         "%s", strSummary);
//
//            }

            else if( !strcmp(pkey, STR_SHARE_FOLDER) )
            {
                snprintf(DtDa->ShareFolder.sf_summary,
                         sizeof(DtDa->ShareFolder.sf_summary),
                         "%s",  strSummary);
            }

            else if( !strcmp(pkey, STR_INFRARED_DEVICE) )
            {
                snprintf(DtDa->InfraredDevice.id_summary,
                         sizeof(DtDa->InfraredDevice.id_summary),
                         "%s", strSummary);
            }

            else if( !strcmp(pkey, STR_PROCESS) )
            {
                snprintf(DtDa->Process.ps_summary,
                         sizeof(DtDa->Process.ps_summary),
                         "%s", strSummary);
            }

            else if( !strcmp(pkey, STR_ROUTER) )
            {
                snprintf(DtDa->Router.rt_summary,
                         sizeof(DtDa->Router.rt_summary),
                         "%s",
                         strSummary);
            }

            else if( !strcmp(pkey, STR_NET_PRINTER) )
            {
                snprintf(DtDa->NetPrinter.np_summary,
                         sizeof(DtDa->NetPrinter.np_summary),
                         "%s",
                         strSummary);
            }

            else if( !strcmp(pkey, STR_CONNECT_EXT_SVR) )
            {
                snprintf(DtDa->ConnectExt.ce_summary,
                         sizeof(DtDa->ConnectExt.ce_summary),
                         "%s",
                         strSummary);
            }
            //else if( !strcmp(pkey, STR_WIN_DRV) )
            //	strcpy(DtDa->WinDrv.dv_summary, strSummary);
            else if( !strcmp(pkey, STR_RDP_SESSION) )
            {
                snprintf(DtDa->RdpSession.rdp_summary,
                         sizeof(DtDa->RdpSession.rdp_summary),
                         "%s",
                         strSummary);
            }

            memset(pkey, 0x00, 30 * sizeof(char) );
        }

        // recursive
        fjson_ParseJsonAuxFt(value, pkey, skey, itemIdx, Dt, DtDa, cpip);
    }

    // jansson.c 에서 print 할때는 없음, 샘플소스에도 사용안함, 자동해지되는가봄
    // TMAXOS에서는 정책 파싱 때 signal(6)나서 주석처리함
    // Centos에서는 오류가 없는 것으로보아 그냥 나둠
    //json_decref(value);
}

void fjson_ParseJsonAuxFt(
        json_t*			element,
        char*			pkey,
        char*			skey,
        int				itemIdx,
        _DAP_DETECT*		Detect,
        _DAP_DETECT_DATA*	DetectData,
        char*			cpip)
{
    switch (json_typeof(element))
    {
        case JSON_OBJECT:
            fjson_ParseJsonObjectFt(element, pkey, skey, itemIdx, Detect, DetectData, cpip);
            break;
        case JSON_STRING:
            fjson_ParseJsonString(element);
            break;
        case JSON_INTEGER:
            fjson_ParseJsonInteger(element);
            break;
        default:
            WRITE_WARNING_IP(cpip, "Unrecognized JSON type %d ",
                      json_typeof(element));
    }
}
void fjson_LogJsonAux(int level, const char* key, json_t* element, int indent)
{
    switch (json_typeof(element))
    {
        case JSON_STRING:
            fjson_LogJsonString(key, element, indent);
            break;
        case JSON_INTEGER:
            fjson_LogJsonInteger(level, key, element, indent);
            break;
/*
    case JSON_ARRAY:
        log_json_array(level, key, element, indent);
        break;

    case JSON_REAL:
        log_json_real(level, key, element, indent);
        break;
    case JSON_TRUE:
        log_json_true(level, key, element, indent);
        break;
    case JSON_FALSE:
        log_json_false(level, key, element, indent);
        break;
    case JSON_NULL:
        log_json_null(level, key, element, indent);
        break;
*/
        default:
            WRITE_WARNING(CATEGORY_DEBUG,	"Unrecognized JSON type %d ",
                    json_typeof(element));
    }
}

void fjson_LogJsonAuxFt(int level, const char* key, json_t* element, int indent, char* cpip)
{
    switch (json_typeof(element))
    {
        case JSON_STRING:
            fjson_LogJsonStringFt(level, key, element, indent, cpip);
            break;
        case JSON_INTEGER:
            fjson_LogJsonIntegerFt(level, key, element, indent, cpip);
            break;
/*
    case JSON_ARRAY:
        log_json_array(level, key, element, indent, cpip);
        break;
    case JSON_REAL:
        log_json_real(level, key, element, indent, cpip);
        break;
    case JSON_TRUE:
        log_json_true(level, key, element, indent, cpip);
        break;
    case JSON_FALSE:
        log_json_false(level, key, element, indent, cpip);
        break;
    case JSON_NULL:
        log_json_null(level, key, element, indent, cpip);
        break;
*/
        default:
            WRITE_WARNING_IP(cpip, "Unrecognized JSON type %d ",
                      json_typeof(element));
    }
}




void fjson_LogJsonIndentFt(int indent, char* param_blank)
{
    int i;

    for (i = 0; i < indent; i++)
    {
        strcat(param_blank, "-");
    }
}
void fjson_LogJsonIndent(int indent, char* param_blank)
{
    int i;

    for (i = 0; i < indent; i++)
    {
        strcat(param_blank, "-");
    }

}




void fjson_ParseJsonFt(json_t *root, _DAP_DETECT* Detect, _DAP_DETECT_DATA* DetectData, char *cpip)
{
    int			itemIdx = 0;
    char		pKey[30];
    char		sKey[30];

    memset(pKey, 0x00, sizeof(pKey));
    memset(sKey, 0x00, sizeof(sKey));

    WRITE_INFO_IP(cpip, "*** BEGIN PARSE FT ***");
    fjson_ParseJsonAuxFt(root, pKey, sKey, itemIdx, Detect, DetectData, cpip);
    WRITE_INFO_IP(cpip, "*** END PARSE FT ***");
}

void fjson_LogJsonString(const char* key, json_t *element, int indent)
{
    char szBlank[10+1];

    memset(szBlank, 0x00, sizeof(szBlank));

    fjson_LogJsonIndent(indent,szBlank);

    WRITE_INFO(CATEGORY_INFO,"%s\"%s\" : \"%s\"",
            szBlank,
            key,
            json_string_value(element)
            );
}

void fjson_LogJsonInteger(int level, const char* key, json_t *element, int indent)
{
    char szBlank[10+1];

    memset(szBlank, 0x00, sizeof(szBlank));

    fjson_LogJsonIndent(indent, szBlank);
    switch (level)
    {
        case DAP_CRITICAL:
            WRITE_CRITICAL(CATEGORY_DEBUG,"%s\"%s\" : %" JSON_INTEGER_FORMAT" ",
                           szBlank,
                           key,
                           json_integer_value(element)
                           );
            break;
        case DAP_WARN:
        WRITE_WARNING(CATEGORY_DEBUG,"%s\"%s\" : %" JSON_INTEGER_FORMAT" ",
                      szBlank,
                       key,
                       json_integer_value(element)
                       );
            break;
        case DAP_DEBUG:
        WRITE_DEBUG(CATEGORY_DEBUG,"%s\"%s\" : %" JSON_INTEGER_FORMAT" ",
                       szBlank,
                       key,
                       json_integer_value(element)
                       );
            break;
        case DAP_INFO:
        WRITE_INFO(CATEGORY_DEBUG,"%s\"%s\" : %" JSON_INTEGER_FORMAT" ",
                       szBlank,
                       key,
                       json_integer_value(element)
                       );
            break;
        case DAP_MAJOR:
        WRITE_MAJOR(CATEGORY_DEBUG,"%s\"%s\" : %" JSON_INTEGER_FORMAT" ",
                       szBlank,
                       key,
                       json_integer_value(element)
                       );
        break;
    }
/*    LogDRet(level, "%s\"%s\" : %" JSON_INTEGER_FORMAT "\n",
            fjson_LogJsonIndent(indent), key, json_integer_value(element));*/
}


void fjson_LogJsonStringFt(int level, const char* key, json_t *element, int indent, char* cpip)
{
    char szBlank[10+1];

    memset(szBlank, 0x00, sizeof(szBlank));

    switch (level)
    {
        fjson_LogJsonIndentFt(indent,szBlank);

        case DAP_CRITICAL:
            WRITE_CRITICAL_IP(cpip, "%s\"%s\" : \"%s\"",
                       szBlank,
                       key,
                       json_string_value(element)
                       );
            break;
        case DAP_WARN:
            WRITE_WARNING_IP(cpip, "%s\"%s\" : \"%s\"",
                       szBlank,
                       key,
                       json_string_value(element)
                       );
            break;
        case DAP_DEBUG:
            WRITE_DEBUG_IP(cpip, "%s\"%s\" : \"%s\"",
                       szBlank,
                       key,
                       json_string_value(element)
                       );
            break;
        case DAP_INFO:
            WRITE_INFO_IP(cpip, "%s\"%s\" : \"%s\"",
                       szBlank,
                       key,
                       json_string_value(element)
                       );
            break;
        case DAP_MAJOR:
            WRITE_MAJOR_IP(cpip, "%s\"%s\" : \"%s\"",
                       szBlank,
                       key,
                       json_string_value(element)
                       );
            break;
    }
 /*   LogCpDRet(level, cpip, "%s\"%s\" : \"%s\"\n",
              fjson_LogJsonIndentFt(indent),key,json_string_value(element));*/
}

void fjson_LogJsonIntegerFt(int level, const char* key, json_t *element, int indent, char* cpip)
{
    char szBlank[10+1];

    memset(szBlank, 0x00, sizeof(szBlank));
    fjson_LogJsonIndentFt(indent,szBlank);
    switch (level)
    {
        case DAP_CRITICAL:
        WRITE_CRITICAL_IP(cpip, "%s%s : %lld ",
                       szBlank,
                       (char*)key,
                       json_integer_value(element)
                       );
            break;
        case DAP_WARN:
        WRITE_WARNING_IP(cpip, "%s%s : %lld ",
                      szBlank,
                      (char*)key,
                      json_integer_value(element)
                      );
            break;
        case DAP_DEBUG:
        WRITE_DEBUG_IP(cpip, "%s%s : %lld ",
                       szBlank,
                    (char*)key,
                    json_integer_value(element)
                    );
            break;
        case DAP_INFO:
        WRITE_INFO_IP(cpip, "%s%s : %lld ",
                      szBlank,
                   (char*)key,
                   json_integer_value(element)
                   );
            break;
        case DAP_MAJOR:
        WRITE_MAJOR_IP(cpip, "%s%s : %lld ",
                       szBlank,
                    (char*)key,
                    json_integer_value(element)
                    );
            break;

    }
    /*LogCpDRet(level, cpip, "%s\"%s\" : %" JSON_INTEGER_FORMAT "\n",
              fjson_LogJsonIndentFt(indent),key,json_integer_value(element));*/
}


void fjson_GetSummaryFt(json_t *value, char *type, char *res)
{
    int			loop;
    char		strAdd[10];
    char		strMod[10];
    char		strDel[10];
    char		strBlk[10];
    //char		tmpValue[256];
    char		tmpSummary[2048];

    memset(strAdd, 0x00, sizeof(strAdd));
    memset(strMod, 0x00, sizeof(strMod));
    memset(strDel, 0x00, sizeof(strDel));
    memset(strBlk, 0x00, sizeof(strBlk));
    if( !strcmp(type, STR_PROCESS) )
    {
        strcpy(strAdd, STR_BLACK);
        strcpy(strMod, STR_WHITE);
        strcpy(strDel, STR_ACCESS);
    }
    else
    {
        strcpy(strAdd, STR_ADD);
        strcpy(strMod, STR_MOD);
        strcpy(strDel, STR_DEL);
    }
    strcpy(strBlk, STR_BLOCK); //for vm block

    memset(tmpSummary, 0x00, sizeof(tmpSummary));

    void *iter = json_object_iter(value);
    while( iter )
    {
        if      (!strcmp(json_object_iter_key(iter), strAdd))
            strcat(tmpSummary, strAdd);
        else if (!strcmp(json_object_iter_key(iter), strDel))
            strcat(tmpSummary, strDel);
        else if (!strcmp(json_object_iter_key(iter), strMod))
            strcat(tmpSummary, strMod);
        else if (!strcmp(json_object_iter_key(iter), strBlk))
            strcat(tmpSummary, strBlk);
        loop = 0;
        void *iter2 = json_object_iter(json_object_iter_value(iter));
        while( iter2 )
        {
            if(strcmp(json_object_iter_key(iter2), "size") == 0)
            {
                if(loop == 0)   strcat(tmpSummary, "(");
            }
            else
            {

                if(loop == 0)   strcat(tmpSummary, "(");
                else            strcat(tmpSummary, ",");
                sprintf(tmpSummary+strlen(tmpSummary), "'%s'", json_string_value(json_object_iter_value(iter2)));
            }
            loop++;
            iter2 = json_object_iter_next(json_object_iter_value(iter), iter2);
        }
        strcat(tmpSummary, ")");
        iter = json_object_iter_next(value, iter);
    }
    if(strlen(tmpSummary) > 1023)    fcom_SubStr(1, 1023, res, tmpSummary);
    else                            strcpy(res, tmpSummary);

}
void fjson_GetSummaryCpuUsage(json_t* element, _DAP_DETECT_DATA* DtDa, char* cpip)
{
    int Cnt = 0;
    char StrCnt[5+1];
    char tmpSummary[2048];


    memset(tmpSummary, 0x00, sizeof(tmpSummary));
    memset(StrCnt, 0x00, sizeof(StrCnt));

    /* Summary Count */
    DtDa->CpuUsage.historysize = json_integer_value(
            json_object_get(json_object_get(element,"summary"),"size"));

    WRITE_DEBUG_IP(cpip,"Cpu Usage Summary Size : [%d] ",DtDa->CpuUsage.historysize);

    if(DtDa->CpuUsage.historysize > MAX_PROCESS_COUNT)
    {
        WRITE_DEBUG_IP(cpip, "CpuUsage Summary Size (%d) > Max process count (%d) ", DtDa->CpuUsage.historysize, MAX_PROCESS_COUNT);
        DtDa->CpuUsage.historysize = MAX_PROCESS_COUNT;
    }

    for(Cnt = 0; Cnt < DtDa->CpuUsage.historysize; Cnt++)
    {
        snprintf(StrCnt,sizeof(StrCnt),"%d",Cnt+1);
        /* CPU USAGE _RATE */
        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate),
                 "%.2lf",
                 json_real_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_RATE)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate_condition,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate_condition),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_CONDITION)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate_limit,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate_limit),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_RATE_LIMIT)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_detect_time,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_detect_time),
                 "%s",
                 json_string_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DETECT_TIME)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_process_name,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_process_name),
                 "%s",
                 json_string_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_PROCESS_NAME)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_process_id,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_process_id),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_PROCESS_ID)));
        if( json_is_integer(json_object_get(json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DURATION_TIME)))
        {
            snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time,
                     sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time_condition),
                     "%lld",
                     json_integer_value(
                             json_object_get(
                                     json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DURATION_TIME) ));
        }
        else if( json_is_real(json_object_get(json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DURATION_TIME)))
        {
            snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time,
                     sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time_condition),
                     "%.0lf",
                     json_real_value(
                             json_object_get(
                                     json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DURATION_TIME) ));
        }


        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time_condition,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time_condition),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_DURATION_TIME_CONDITION)));

        DtDa->CpuUsage.CpuHistoryValue[Cnt].is_dap_agent =
                json_integer_value(
                        json_object_get(
                                json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_IS_DAP));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_STATUS)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status_name,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status_name),
                 "%s",
                 json_string_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_STATUS_NAME)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type),
                 "%lld",
                 json_integer_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_TYPE)));

        snprintf(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type_name,
                 sizeof(DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type_name),
                 "%s",
                 json_string_value(
                         json_object_get(
                                 json_object_get(json_object_get(element,"summary"),StrCnt),STR_CPU_USAGE_TYPE_NAME)));


        WRITE_DEBUG_IP(cpip,"cpu usage_rate : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate );
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_rate_condition : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_rate_condition);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_detect_time : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_detect_time);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_process_name : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_process_name);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_duration_time : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_duration_time_condition : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_duration_time_condition);
        WRITE_DEBUG_IP(cpip,"cpu is_dap_agent : [%d] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].is_dap_agent);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_status : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_status_name : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_status_name);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_type : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type);
        WRITE_DEBUG_IP(cpip,"cpu cpu_usage_type_name : [%s] ",DtDa->CpuUsage.CpuHistoryValue[Cnt].cpu_usage_type_name);

    }
    return;

}

void fjson_ParseJsonMainBoardFt(
        const char*		key,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( !strcmp(key, STR_HB_MB_MF) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->MainBoard.hb_mb_mf, sizeof(DtDa->MainBoard.hb_mb_mf), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->MainBoard.hb_mb_mf, sizeof(DtDa->MainBoard.hb_mb_mf), "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_HB_MB_PN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->MainBoard.hb_mb_pn, sizeof(DtDa->MainBoard.hb_mb_pn), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->MainBoard.hb_mb_pn, sizeof(DtDa->MainBoard.hb_mb_pn), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_HB_MB_SN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->MainBoard.hb_mb_sn, sizeof(DtDa->MainBoard.hb_mb_sn), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->MainBoard.hb_mb_sn, sizeof(DtDa->MainBoard.hb_mb_pn), "%s", json_string_value(value));
        }
    }
}

void jfson_ParseJsonSystemFt(
        const char*		key,
        char*			skey,
        json_t*			value,
        _DAP_DETECT_DATA *	DtDa,
        char*			cpip)
{
    if( !strcmp(key, STR_ST_BOOTUP_STATE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf(DtDa->System.st_bootup_state, sizeof(DtDa->System.st_bootup_state), "%s", subValue );
        }
        else
        {
            snprintf(DtDa->System.st_bootup_state, sizeof(DtDa->System.st_bootup_state), "%s",  json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ST_DNS_HOST_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->System.st_dns_host_name, sizeof(DtDa->System.st_dns_host_name), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_dns_host_name, sizeof(DtDa->System.st_dns_host_name), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ST_DOMAIN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->System.st_domain, sizeof(DtDa->System.st_domain), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_domain, sizeof(DtDa->System.st_domain), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ST_DOMAIN_ROLE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->System.st_domain_role = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ST_MEMORY) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->System.st_memory = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ST_VGA) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31, (char *)json_string_value(value), subValue);
            snprintf( DtDa->System.st_vga, sizeof(DtDa->System.st_vga), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_vga, sizeof(DtDa->System.st_vga), "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "    \"%s\" : \"%s\"\n", key,DtDa->System.st_vga);
    }
    else if( !strcmp(key, STR_ST_INSTALLED_VM) )
    {
        memset(skey, 0x00, 30);
        strcpy(skey, STR_ST_INSTALLED_VM);
        WRITE_INFO_IP(cpip, "----\""STR_ST_INSTALLED_VM"\"\n");
    }
    else if(!strcmp(skey, STR_ST_INSTALLED_VM) &&  !json_is_object(value))
    {
        if( !strcmp(key, STR_SIZE) )
        {
            fjson_LogJsonAuxFt(5, key, value, 6, cpip);
            DtDa->System.st_installed_vm_size = json_integer_value(value);
            memset(skey, 0x00, 30);
        }
        else
        {
            WRITE_INFO_IP(cpip, "------\"%s\" : \"%s\"\n", key,json_string_value(value));
//            int subIdx = atoi(key)-1;
            int result = 0;
            int subIdx = 0;
            if (fcom_SafeAtoi(key, &result)) {
                subIdx = result-1;
                if ( subIdx  < 0 )
                    subIdx = 0;
            } else {
                subIdx = 0;
            }
            if ( subIdx >= MAX_COMMON_COUNT ) {
                WRITE_WARNING_IP(cpip,"System Detect Data Max Count Over ");
                return;
            }

            snprintf( DtDa->System.st_installed_vm[subIdx],
                      sizeof(DtDa->System.st_installed_vm[subIdx]),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ST_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf( DtDa->System.st_name, sizeof(DtDa->System.st_name), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_name, sizeof(DtDa->System.st_name), "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "    \"%s\" : \"%s\"\n", key,DtDa->System.st_name);
    }
    else if( !strcmp(key, STR_ST_TIME_ZONE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->System.st_time_zone = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ST_VM_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf( DtDa->System.st_vm_name, sizeof(DtDa->System.st_vm_name), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_vm_name, sizeof(DtDa->System.st_vm_name), "%s",  json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ST_WAKEUP_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->System.st_wakeup_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ST_WORK_GROUP) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char*)json_string_value(value), subValue);
            snprintf( DtDa->System.st_work_group, sizeof(DtDa->System.st_work_group), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->System.st_work_group, sizeof(DtDa->System.st_work_group), "%s", json_string_value(value));
        }
    }
}

void fjson_ParseJsonOperatingSystemFt(
        const char*		key,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( !strcmp(key, STR_OS_ARCHITECTURE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15, (char*)json_string_value(value), subValue);
            snprintf( DtDa->OperatingSystem.os_architecture, sizeof(DtDa->OperatingSystem.os_architecture),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->OperatingSystem.os_architecture, sizeof(DtDa->OperatingSystem.os_architecture),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_OS_LANG) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OperatingSystem.os_lang = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OS_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf( DtDa->OperatingSystem.os_name, sizeof(DtDa->OperatingSystem.os_name), "%s",  subValue);
        }
        else
        {
            snprintf( DtDa->OperatingSystem.os_name, sizeof(DtDa->OperatingSystem.os_name), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_OS_PORTABLE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OperatingSystem.os_portable = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OS_SP_MAJOR_VER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OperatingSystem.os_sp_major_ver = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OS_SP_MINOR_VER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OperatingSystem.os_sp_minor_ver = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OS_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OperatingSystem.os_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OS_VERSION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31, (char*)json_string_value(value), subValue);
            snprintf( DtDa->OperatingSystem.os_version, sizeof(DtDa->OperatingSystem.os_version), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->OperatingSystem.os_version, sizeof(DtDa->OperatingSystem.os_version), "%s", json_string_value(value));
        }
    }
}

void fjson_ParseJsonCpuFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA *	DtDa,
        char*			cpip)
{

    if( fcom_IsNumber((char*)key) )
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP(cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"Cpu Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_CU_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255,(char *) json_string_value(value), subValue);
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_desc, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_desc), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_desc, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_desc), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_CU_MF) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_mf, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_mf), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_mf, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_mf), "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->Cpu.CpuValue[*itemIdx].cu_mf);
    }
    else if( !strcmp(key, STR_CU_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127,(char *) json_string_value(value), subValue);
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_name, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_name), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_name, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_name), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_CU_PID) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31,(char *) json_string_value(value), subValue);
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_pid, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_pid), "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Cpu.CpuValue[*itemIdx].cu_pid, sizeof(DtDa->Cpu.CpuValue[*itemIdx].cu_pid), "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Cpu.size = json_integer_value(value);
    }
}

void fjson_ParseJsonNetAdapterFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char *)key) )
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP(cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"NetAdapter Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_NA_ALTE_DNS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_alte_dns, sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_alte_dns),
                  "%s", json_string_value(value));
    }
    else if( !strcmp(key, STR_NA_DEFAULT_GW) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char *)json_string_value(value), subValue);

            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw, sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_DEFAULT_GW_MAC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw_mac,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw_mac),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw_mac,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_default_gw_mac),
                      "%s",json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_desc,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_desc),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_desc,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_desc),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_DEVICE_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_device_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NA_IPV4) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_ipv4,
                  sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_ipv4),
                  "%s",  (char *)json_string_value(value));
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_ipv4);
    }
    else if( !strcmp(key, STR_NA_IPV6) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_ipv6,
                  sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_ipv6),
                  "%s",  (char *)json_string_value(value));

    }
    else if( !strcmp(key, STR_NA_MAC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_mac,
                 sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_mac),
                 "%s",
                 (char *)json_string_value(value));
    }
    else if( !strcmp(key, STR_NA_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_name,
                 sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_name),
                 "%s",
                 (char *)json_string_value(value));
    }
    else if( !strcmp(key, STR_NA_NET_CONNECTION_ID) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63,(char *) json_string_value(value), subValue);
            snprintf(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_connection_id,
                     sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_connection_id),
                     "%s",
                     subValue);
        }
        else
        {
            snprintf(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_connection_id,
                     sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_connection_id),
                     "%s",
                     (char *)json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_NET_CONNECTION_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_connection_status = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NA_NET_ENABLED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_net_enabled = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NA_PHYSICAL_ADAPTER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_physical_adapter = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NA_PN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pn,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pn),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pn,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pn),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_PNP_DEVICE_ID) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pnp_device_id,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pnp_device_id),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pnp_device_id,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pnp_device_id),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NA_PREF_DNS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pref_dns,
                 sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_pref_dns),
                 "%s", json_string_value(value));
    }
    else if( !strcmp(key, STR_NA_SERVICE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31,(char *) json_string_value(value), subValue);
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_service_name,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_service_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_service_name,
                      sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_service_name),
                      "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_service_name);
    }
    else if( !strcmp(key, STR_NA_SUBNET) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf( DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_subnet,
                  sizeof(DtDa->NetAdapter.NetAdapterValue[*itemIdx].na_subnet),
                  "%s", json_string_value(value));
    }
    else if( !strcmp(key, STR_PHYSICAL_NIC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetAdapter.physical_nic = json_integer_value(value);
        //WRITE_INFO_IP( cpip, "    \"%s\" : %d\n", key,DtDa->NetAdapter.physical_nic);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetAdapter.size = json_integer_value(value);
    }
}

void fjson_ParseJsonWifiFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP(cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"Wifi Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_WF_8021X) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_8021x = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_AUTH_ALGO) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_auth_algo = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_BSS_NETWORK_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_bss_network_type = json_integer_value(value);
        //WRITE_INFO_IP( cpip, "      \"%s\" : %d\n", key,DtDa->Wifi.WifiValue[*itemIdx].wf_bss_network_type);
    }
    else if( !strcmp(key, STR_WF_CIPHER_ALGO) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_cipher_algo = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_CONNECTION_MODE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_connection_mode = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_INTERFACE_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255,(char *) json_string_value(value), subValue);
            snprintf( DtDa->Wifi.WifiValue[*itemIdx].wf_interface_desc, sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_interface_desc),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Wifi.WifiValue[*itemIdx].wf_interface_desc, sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_interface_desc),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_WF_INTERFACE_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_interface_status = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_MAC_ADDR) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->Wifi.WifiValue[*itemIdx].wf_mac_addr,
                 sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_mac_addr), "%s", json_string_value(value));

    }
    else if( !strcmp(key, STR_WF_PHY_NETWORK_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_phy_network_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_PROFILE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->Wifi.WifiValue[*itemIdx].wf_profile_name, sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_profile_name),
                      "%s", subValue);
        }
        else
        {
            snprintf(DtDa->Wifi.WifiValue[*itemIdx].wf_profile_name, sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_profile_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_WF_SECURITY) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Wifi.WifiValue[*itemIdx].wf_security = json_integer_value(value);
    }
    else if( !strcmp(key, STR_WF_SSID) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63,(char *) json_string_value(value), subValue);
            snprintf( DtDa->Wifi.WifiValue[*itemIdx].wf_ssid,
                      sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_ssid),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Wifi.WifiValue[*itemIdx].wf_ssid,
                      sizeof(DtDa->Wifi.WifiValue[*itemIdx].wf_ssid),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Wifi.size = json_integer_value(value);
    }
}

void fjson_ParseJsonBluetoothFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP(cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"Bluetooth Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_BT_AUTHENTICATED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_authenticated = json_integer_value(value);
    }
    else if( !strcmp(key, STR_BT_CONNECTED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_connected = json_integer_value(value);
    }
    else if( !strcmp(key, STR_BT_DANGER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_danger = json_integer_value(value);
    }
    else if( !strcmp(key, STR_BT_DEVICE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_device,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_device),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_device,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_device),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_BT_INSTANCE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_instance_name,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_instance_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_instance_name,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_instance_name),
                      "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_instance_name);
    }
    else if( !strcmp(key, STR_BT_MAC_ADDR) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_mac_addr,
                  sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_mac_addr),
                  "%s", json_string_value(value));

    }
    else if( !strcmp(key, STR_BT_MINOR_DEVICE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_minor_device,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_minor_device),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_minor_device,
                      sizeof(DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_minor_device),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_BT_REMEMBERED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Bluetooth.BluetoothValue[*itemIdx].bt_remembered= json_integer_value(value);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Bluetooth.size = json_integer_value(value);
    }
}

void fjson_ParseJsonNetConnectionFt(
        const char*		key,
        int*			itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP(cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"NetConnection Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_NC_CONNECTION_STATE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_connection_state = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NC_CONNECTION_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_connection_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NC_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_desc,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_desc),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_desc,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_desc),
                      "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_desc);
    }
    else if( !strcmp(key, STR_NC_DISPLAY_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_display_type,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_display_type),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_display_type,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_display_type),
                      "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_display_type);
    }
    else if( !strcmp(key, STR_NC_LOCAL_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_local_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_local_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_local_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_local_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NC_PROVIDER_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127,(char *) json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_provider_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_provider_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_provider_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_provider_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NC_REMOTE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_name,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NC_REMOTE_PATH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_path,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_path),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_path,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_remote_path),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NC_RESOURCE_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15,(char *) json_string_value(value), subValue);
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_resource_type,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_resource_type),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_resource_type,
                      sizeof(DtDa->NetConnection.NetConnectionValue[*itemIdx].nc_resource_type),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetConnection.size = json_integer_value(value);
    }
}

void fjson_ParseJsonDiskFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"Disk Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_DK_ACCESS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Disk.DiskValue[*itemIdx].dk_access = json_integer_value(value);
    }
    else if( !strcmp(key, STR_DK_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_desc,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_desc),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_desc,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_desc),
                      "%s", json_string_value(value));

        }
    }
    else if( !strcmp(key, STR_DK_DRIVE_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Disk.DiskValue[*itemIdx].dk_drive_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_DK_FILE_SYSTEM) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->Disk.DiskValue[*itemIdx].dk_file_system,
                 sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_file_system),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_DK_INTERFACE_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->Disk.DiskValue[*itemIdx].dk_interface_type,
                 sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_interface_type),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_DK_MF) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_mf,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_mf),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_mf,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_mf),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DK_MODEL) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_model,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_model),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_model,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_model),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DK_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_name,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_name, sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DK_PHYSICAL_SN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63,(char*) json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_physical_sn,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_physical_sn),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_physical_sn,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_physical_sn),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DK_VOLUME_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_volume_name,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_volume_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_volume_name,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_volume_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DK_VOLUME_SN) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_volume_sn,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_volume_sn),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Disk.DiskValue[*itemIdx].dk_volume_sn,
                      sizeof(DtDa->Disk.DiskValue[*itemIdx].dk_volume_sn),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Disk.size = json_integer_value(value);
    }
}

void fjson_ParseJsonNetDriveFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"NetDrive Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_ND_CONNECTION_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetDrive.NetDriveValue[*itemIdx].nd_connection_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ND_DEFER_FLAGS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetDrive.NetDriveValue[*itemIdx].nd_defer_flags = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ND_DRIVE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_drive_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_drive_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_drive_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_drive_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ND_PROVIDER_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char *)json_string_value(value), subValue);
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_provider_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_provider_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_provider_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_provider_name),
                     "%s", json_string_value(value));

        }
    }
    else if( !strcmp(key, STR_ND_PROVIDER_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetDrive.NetDriveValue[*itemIdx].nd_provider_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ND_REMOTE_PATH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_remote_path,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_remote_path),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_remote_path,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_remote_path),
                     "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->NetDrive.NetDriveValue[*itemIdx].nd_remote_path);
    }
    else if( !strcmp(key, STR_ND_USER_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31, (char *)json_string_value(value), subValue);
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_user_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_user_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_user_name,
                     sizeof(DtDa->NetDrive.NetDriveValue[*itemIdx].nd_user_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetDrive.size = json_integer_value(value);
    }
}

void fjson_ParseJsonOsAccountFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
        *itemIdx = atoi(key)-1;
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if( !strcmp(key, STR_OA_CAPTION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char *)json_string_value(value), subValue);
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption, subValue);

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption),
                      "%s", subValue);
        }
        else
        {
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption, json_string_value(value));

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_caption),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_OA_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255,(char *)json_string_value(value), subValue);
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc, subValue);

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc),
                      "%s", subValue);
        }
        else
        {
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc, json_string_value(value));

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_desc),
                      "%s", json_string_value(value));

        }
    }
    else if( !strcmp(key, STR_OA_DISABLED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->OSAccount.OSAccountValue[*itemIdx].oa_disabled = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OA_LOCAL) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->OSAccount.OSAccountValue[*itemIdx].oa_local = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OA_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name, subValue);

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name),
                      "%s", subValue);
        }
        else
        {
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name, json_string_value(value));

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_name),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_OA_SID) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid, subValue);

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid),
                      "%s", subValue);
        }
        else
        {
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid, json_string_value(value));

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_OA_SID_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->OSAccount.OSAccountValue[*itemIdx].oa_sid_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_OA_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31,(char*) json_string_value(value), subValue);
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status, subValue);

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status),
                      "%s", subValue);
        }
        else
        {
//            strcpy(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status, json_string_value(value));

            snprintf( DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status,
                      sizeof(DtDa->OSAccount.OSAccountValue[*itemIdx].oa_status),
                      "%s", json_string_value(value));

        }
    }
    else if( !strcmp(key, STR_OA_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->OSAccount.OSAccountValue[*itemIdx].oa_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->OSAccount.size = json_integer_value(value);
    }
}

void fjson_ParseJsonShareFolderFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"ShareFolder Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_SF_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_name,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_name),
                     "%s",
                     subValue);
        }
        else
        {
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_name,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SF_PATH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char*)json_string_value(value), subValue);
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_path,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_path),
                     "%s",
                     subValue);
        }
        else
        {
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_path,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_path),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SF_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 10)
        {
            char subValue[10+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 10, (char*)json_string_value(value), subValue);
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_status,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_status),
                     "%s",
                     subValue);
        }
        else
        {
            snprintf(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_status,
                     sizeof(DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_status),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SF_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->ShareFolder.ShareFolderValue[*itemIdx].sf_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->ShareFolder.size = json_integer_value(value);
    }
}

void fjson_ParseJsonInfraredDeviceFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= 5 ) {
        WRITE_WARNING_IP(cpip,"InfraredDevice Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_ID_MF) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
    }

    else if( !strcmp(key, STR_ID_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_name,
                     sizeof(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_name,
                     sizeof(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ID_PROTOCOL_SUPPORTED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_protocol_supported = json_integer_value(value);
    }
    else if( !strcmp(key, STR_ID_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 10)
        {
            char subValue[10+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 10, (char*)json_string_value(value), subValue);
            snprintf(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status,
                     sizeof(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status,
                     sizeof(DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_ID_STATUS_INFO) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status_info = json_integer_value(value);
        //WRITE_INFO_IP( cpip, "      \"%s\" : %d\n", key,DtDa->InfraredDevice.InfraredDeviceValue[*itemIdx].id_status_info);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->InfraredDevice.size = json_integer_value(value);
    }
}

void fjson_ParseJsonProcessFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    } else {
        return;
    }

    if ( *itemIdx >= MAX_PROCESS_COUNT ) {
        WRITE_WARNING(CATEGORY_DEBUG,"Process Detect Data Max Process Count Over ");
        return;
    }

    if( !strcmp(key, STR_PS_COMPANY_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf(DtDa->Process.ProcessValue[*itemIdx].ps_company_name,
                     sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_company_name),
                     "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_company_name,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_company_name),
                      "%s", json_string_value(value));
        }
        //WRITE_INFO_IP( cpip, "      \"%s\" : \"%s\"\n", key,DtDa->Process.ProcessValue[*itemIdx].ps_company_name);
    }
    else if( !strcmp(key, STR_PS_CONNECTED_SVR_ADDR) )
    {
        memset(skey, 0x00, 30);
        strcpy(skey, STR_PS_CONNECTED_SVR_ADDR);
        WRITE_INFO_IP( cpip, "------\""STR_PS_CONNECTED_SVR_ADDR"\"\n");
    }
    else if(!strcmp(skey, STR_PS_CONNECTED_SVR_ADDR) &&
            //fcom_IsNumber(key) &&
            !json_is_object(value))
    {
        if( !strcmp(key, STR_SIZE) )
        {
            fjson_LogJsonAuxFt(5, key, value, 8, cpip);
            DtDa->Process.ProcessValue[*itemIdx].ps_connected_svr_addr_size = json_integer_value(value);
            memset(skey, 0x00, 30);
        }
        else
        {
            WRITE_INFO_IP( cpip, "--------\"%s\" : \"%s\"\n", key,json_string_value(value));
//            int subIdx = atoi(key)-1;
            int result = 0;
            int subIdx = 0;
            if (fcom_SafeAtoi(key, &result)) {
                subIdx = result-1;
                if ( subIdx  < 0 )
                    subIdx = 0;
            } else {
                subIdx = 0;
            }

            if (subIdx >= MAX_COMMON_COUNT ) {
                WRITE_WARNING(CATEGORY_DEBUG,"Process Detect Max Count Over ");
                return;
            }

            snprintf(DtDa->Process.ProcessValue[*itemIdx].ps_connected_svr_addr[subIdx],
                     sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_connected_svr_addr[subIdx]),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_PS_COPY_RIGHT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_copy_right,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_copy_right),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_copy_right,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_copy_right),
                      "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_PS_FILE_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_desc,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_desc),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_desc,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_desc),
                      "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_PS_FILE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_name,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_name,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_name),
                      "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_PS_FILE_PATH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 255)
        {
            char subValue[255+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 255,(char*) json_string_value(value), subValue);
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_path,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_path),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_path,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_path),
                      "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_PS_FILE_VER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 31)
        {
            char subValue[31+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 31, (char*)json_string_value(value), subValue);
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_ver,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_ver),
                      "%s", subValue);

        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_file_ver,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_file_ver),
                      "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_PS_ORIGINAL_FILE_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127,(char*) json_string_value(value), subValue);

            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_original_file_name,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_original_file_name),
                      "%s", subValue);
        }
        else
        {
            snprintf( DtDa->Process.ProcessValue[*itemIdx].ps_original_file_name,
                      sizeof(DtDa->Process.ProcessValue[*itemIdx].ps_original_file_name),
                      "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_PS_RUNNING) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Process.ProcessValue[*itemIdx].ps_running = json_integer_value(value);
    }
    else if( !strcmp(key, STR_PS_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Process.ProcessValue[*itemIdx].ps_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Process.size = json_integer_value(value);
    }
}

void fjson_ParseJsonRouterFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"Router Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_RT_CAPTION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->Router.RouterValue[*itemIdx].rt_caption,
                     sizeof(DtDa->Router.RouterValue[*itemIdx].rt_caption),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->Router.RouterValue[*itemIdx].rt_caption,
                     sizeof(DtDa->Router.RouterValue[*itemIdx].rt_caption),
                     "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_RT_DETECT_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->Router.RouterValue[*itemIdx].rt_detect_type = json_integer_value(value);
    }
    else if( !strcmp(key, STR_RT_IPADDR) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->Router.RouterValue[*itemIdx].rt_ipaddr,
                 sizeof(DtDa->Router.RouterValue[*itemIdx].rt_ipaddr),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_RT_MAC_ADDR) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->Router.RouterValue[*itemIdx].rt_mac_addr,
                 sizeof(DtDa->Router.RouterValue[*itemIdx].rt_mac_addr),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_RT_WEB_TEXT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->Router.RouterValue[*itemIdx].rt_web_text,
                     sizeof(DtDa->Router.RouterValue[*itemIdx].rt_web_text),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->Router.RouterValue[*itemIdx].rt_web_text,
                     sizeof(DtDa->Router.RouterValue[*itemIdx].rt_web_text),
                     "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->Router.size = json_integer_value(value);
    }
}

void fjson_ParseJsonNetPrinterFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"NetPrinter Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_NP_CONNECTED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_connected = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NP_DISCORDANCE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_discordance = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NP_HOST_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_host_name,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_host_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_host_name,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_host_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_NP_OPEN_PORT) )
    {
        memset(skey, 0x00, 30 * sizeof(char));
        strcpy(skey, STR_NP_OPEN_PORT);
        WRITE_INFO_IP( cpip, "------\""STR_NP_OPEN_PORT"\"\n");
    }
    else if(!strcmp(skey, STR_NP_OPEN_PORT) &&
            !json_is_object(value))
    {
        if( !strcmp(key, STR_SIZE) )
        {
            fjson_LogJsonAuxFt(5, key, value, 8, cpip);
            DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_open_port_size = json_integer_value(value);
            memset(skey, 0x00, 30 * sizeof(char));
        }
        else
        {
            WRITE_INFO_IP( cpip, "--------\"%s\" : %d\n", key,json_integer_value(value));
//            int subIdx = atoi(key)-1;
            int result = 0;
            int subIdx = 0;
            if (fcom_SafeAtoi(key, &result)) {
                subIdx = result-1;
                if ( subIdx  < 0 )
                    subIdx = 0;
            } else {
                subIdx = 0;
            }

            if ( subIdx >= MAX_PORT_COUNT ) {
                WRITE_WARNING_IP(cpip,"NetPrinter Detect Data Max Count Over ");
                return;
            }

            DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_open_port[subIdx] = json_integer_value(value);
        }
    }
    else if( !strcmp(key, STR_NP_PRINTER_PORT) )
    {
        memset(skey, 0x00, 30 * sizeof(char));
        strcpy(skey, STR_NP_PRINTER_PORT);
        WRITE_INFO_IP( cpip, "------\""STR_NP_PRINTER_PORT"\"\n");
    }
    else if(!strcmp(skey, STR_NP_PRINTER_PORT) &&
            !json_is_object(value))
    {
        if( !strcmp(key, STR_SIZE) )
        {
            fjson_LogJsonAuxFt(5, key, value, 8, cpip);
            DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_printer_port_size = json_integer_value(value);
            memset(skey, 0x00, 30 * sizeof(char));
        }
        else
        {
            WRITE_INFO_IP( cpip, "--------\"%s\" : %d\n", key,json_integer_value(value));
//            int subIdx = atoi(key)-1;
            int result = 0;
            int subIdx = 0;
            if (fcom_SafeAtoi(key, &result)) {
                subIdx = result-1;
                if ( subIdx  < 0 )
                    subIdx = 0;
            } else {
                subIdx = 0;
            }

            if ( subIdx >= MAX_PORT_COUNT ) {
                WRITE_WARNING_IP(cpip,"NetPrinter Detect Data Max Count Over ");
                return;
            }

            DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_printer_port[subIdx] = json_integer_value(value);
        }
    }
    else if( !strcmp(key, STR_NP_WEB_CONNECT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_web_connect = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NP_WEB_TEXT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_web_text,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_web_text),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_web_text,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_web_text),
                     "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_NP_WSD_LOCATION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_wsd_location,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_wsd_location),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_wsd_location,
                     sizeof(DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_wsd_location),
                     "%s", json_string_value(value) );
        }
    }
    else if( !strcmp(key, STR_NP_WSD_PRINTER_DEVICE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetPrinter.NetPrinterValue[*itemIdx].np_wsd_printer_device = json_integer_value(value);
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetPrinter.size = json_integer_value(value);
    }
}

void fjson_ParseJsonConnectExtSvrFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"ConnectExt Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_CE_CONNECTED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->ConnectExt.ConnectExtValue[*itemIdx].ce_connected = json_integer_value(value);
    }
    else if( !strcmp(key, STR_CE_URL) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 63)
        {
            char subValue[63+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 63, (char*)json_string_value(value), subValue);
            snprintf(DtDa->ConnectExt.ConnectExtValue[*itemIdx].ce_url,
                     sizeof(DtDa->ConnectExt.ConnectExtValue[*itemIdx].ce_url),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->ConnectExt.ConnectExtValue[*itemIdx].ce_url,
                     sizeof(DtDa->ConnectExt.ConnectExtValue[*itemIdx].ce_url),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->ConnectExt.size = json_integer_value(value);
    }
}

void fjson_ParseJsonNetScanFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
        *itemIdx = atoi(key)-1;
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if( !strcmp(key, STR_NS_DAP_AGENT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetScan.NetScanValue[*itemIdx].ns_dap_agent = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NS_IP) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
//        strcpy(DtDa->NetScan.NetScanValue[*itemIdx].ns_ip, json_string_value(value));
        snprintf(DtDa->NetScan.NetScanValue[*itemIdx].ns_ip,
                 sizeof(DtDa->NetScan.NetScanValue[*itemIdx].ns_ip),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_NS_MAC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
//        strcpy(DtDa->NetScan.NetScanValue[*itemIdx].ns_mac, json_string_value(value));
        snprintf(DtDa->NetScan.NetScanValue[*itemIdx].ns_mac,
                 sizeof(DtDa->NetScan.NetScanValue[*itemIdx].ns_mac),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_NS_MAC_MATCH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->NetScan.NetScanValue[*itemIdx].ns_mac_match = json_integer_value(value);
    }
    else if( !strcmp(key, STR_NS_OPEN_PORT) )
    {
        memset(skey, 0x00, 30 * sizeof(char));
        strcpy(skey, STR_NS_OPEN_PORT);
        WRITE_INFO_IP( cpip, "------\""STR_NS_OPEN_PORT"\"\n");
    }
    else if(!strcmp(skey, STR_NS_OPEN_PORT) &&
            !json_is_object(value))
    {
        if( !strcmp(key, STR_SIZE) )
        {
            fjson_LogJsonAuxFt(5, key, value, 8, cpip);
            DtDa->NetScan.NetScanValue[*itemIdx].ns_open_port_size = json_integer_value(value);
            memset(skey, 0x00, 30 * sizeof(char));
        }
        else
        {
            WRITE_INFO_IP( cpip, "--------\"%s\" : %d\n", key,json_integer_value(value));
//            int subIdx = atoi(key)-1;
            int result = 0;
            int subIdx = 0;
            if (fcom_SafeAtoi(key, &result)) {
                subIdx = result-1;
                if ( subIdx  < 0 )
                    subIdx = 0;
            } else {
                subIdx = 0;
            }
            DtDa->NetScan.NetScanValue[*itemIdx].ns_open_port[subIdx] = json_integer_value(value);
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetScan.size = json_integer_value(value);
    }
    else if( !strcmp(key, STR_UNCHANGED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->NetScan.unchanged = json_integer_value(value);
    }
}

void fjson_ParseJsonSsoCertFt(
        const char* 	key,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip)
{
    if( !strcmp(key, STR_SC_UNCERTIFIED) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->SsoCert.sc_uncertified = json_integer_value(value);
    }
}

void fjson_ParseJsonWinDrvFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
        *itemIdx = atoi(key)-1;
        WRITE_INFO_IP( cpip, "----\"%d\"\n", *itemIdx);
    }

    if( !strcmp(key, STR_DV_CLASS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 14)
        {
            char subValue[14+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 14, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_CLASS_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class_desc),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class_desc),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_desc),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_desc),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_DRIVER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_driver,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_driver),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_driver,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_driver),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_ENUM) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 49)
        {
            char subValue[49+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 49, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_enum,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_enum),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_enum,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_enum),
                     "%s", json_string_value(value));
        }
    }

    else if( !strcmp(key, STR_DV_FILE_COMPANY) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 29)
        {
            char subValue[29+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 29, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_company,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_company),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_company,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_company),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_FILE_COPY_RIGHT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_copy_right,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_copy_right),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_copy_right,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_copy_right),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_FILE_DESC) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127,(char*) json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_desc),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_desc,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_desc),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_FILE_PATH) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_path,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_path),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_path,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_path),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_FILE_PRODUCT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 29)
        {
            char subValue[29+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 29, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_product,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_product),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_product,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_product),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_FILE_VER) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 14)
        {
            char subValue[14+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 14,(char*) json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_ver,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_ver),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_ver,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_file_ver),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_LOCATION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_location,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_location),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_location,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_location),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_MFG) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_mfg,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_mfg),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_mfg,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_mfg),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_NAME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 127)
        {
            char subValue[127+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 127, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_name,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_name),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_name,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_name),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_SERVICE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 29)
        {
            char subValue[29+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 29, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_service,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_service),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_service,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_service),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_START) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 14)
        {
            char subValue[14+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 14, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_start,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_start),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_start,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_start),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 14)
        {
            char subValue[14+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 14, (char*)json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_status,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_status),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_status,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_status),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 29)
        {
            char subValue[29+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 29,(char*) json_string_value(value), subValue);

            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_type,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_type),
                     "%s", subValue );
        }
        else
        {
            snprintf(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_type,
                     sizeof(DtDa->WinDrv.WinDrvValue[*itemIdx].dv_type),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_DV_DATA_TYPE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->WinDrv.WinDrvValue[*itemIdx].dv_data_type = json_integer_value(value);
    }

    /** Device Class Reset("dv_class_reset") **/
    else if( !strcmp(key, STR_DV_CLASS_RESET) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->WinDrv.WinDrvValue[*itemIdx].dv_class_reset = json_integer_value(value);
    }


    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->WinDrv.size = json_integer_value(value);
    }
}

void fjson_ParseJsonRdpSession(const char* key, int *itemIdx, json_t* value, _DAP_DETECT_DATA * DtDa,char*	cpip)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_INFO(CATEGORY_INFO, "----\"%d\"", *itemIdx);
    }

    if ( *itemIdx >= MAX_COMMON_COUNT ) {
        WRITE_WARNING_IP(cpip,"RdpSession Detect Data Max Count Over ");
        return;
    }

    if( !strcmp(key, STR_RDP_CLIENT_IP) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15, (char*)json_string_value(value), subValue);

            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_RDP_CLIENT_NAME) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 128)
        {
            char subValue[128+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 128, (char*)json_string_value(value), subValue);

            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name),
                     "%s", json_string_value(value));
        }
        //LogDRet(4, "      \"%s\" : %d\n", key,DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name);
    }
    else if( !strcmp(key, STR_RDP_CONNECT_TIME) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 19)
        {
            char subValue[19+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 19, (char*)json_string_value(value), subValue);

            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_RDP_USER_ID) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 30)
        {
            char subValue[30+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 30, (char*)json_string_value(value), subValue);

            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id),
                     "%s", subValue);
        }
        else
        {
            snprintf(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id,
                     sizeof(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id),
                     "%s", json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAux(5, key, value, 4);
        DtDa->RdpSession.size = json_integer_value(value);
    }
}

void fjson_ParseJsonRdpSessionFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa)
{
    if( fcom_IsNumber((char*)key) && json_is_object(value))
    {
        *itemIdx = atoi(key)-1;
        WRITE_INFO(CATEGORY_INFO,"----\"%d\"\n", *itemIdx);
    }

    if( !strcmp(key, STR_RDP_CLIENT_IP) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 15)
        {
            char subValue[15+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 15, (char*)json_string_value(value), subValue);
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip, subValue);
        }
        else
        {
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_ip, json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_RDP_CLIENT_NAME) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 128)
        {
            char subValue[128+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 128,(char*) json_string_value(value), subValue);
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name, subValue);
        }
        else
        {
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_client_name, json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_RDP_CONNECT_TIME) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 19)
        {
            char subValue[19+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 19,(char*) json_string_value(value), subValue);
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time, subValue);
        }
        else
        {
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_connect_time, json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_RDP_USER_ID) )
    {
        fjson_LogJsonAux(5, key, value, 6);
        if(strlen(json_string_value(value)) > 30)
        {
            char subValue[30+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 30,(char*) json_string_value(value), subValue);
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id, subValue);
        }
        else
        {
            strcpy(DtDa->RdpSession.RdpSessionValue[*itemIdx].rdp_user_id, json_string_value(value));
        }
    }
    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAux(5, key, value, 4);
        DtDa->RdpSession.size = json_integer_value(value);
    }
}

void fjson_ParseJsonCpuUsage(
        const char* 	  key,
        int*			  itemIdx,
        json_t* 		  value,
        _DAP_DETECT_DATA* DtDa,
        char*			  cpip
)
{
    if( fcom_IsNumber((char *)key) && json_is_object(value))
    {
//        *itemIdx = atoi(key)-1;
        int result = 0;
        if (fcom_SafeAtoi(key, &result)) {
            *itemIdx = result-1;
            if ( *itemIdx  < 0 )
                *itemIdx = 0;
        } else {
            *itemIdx = 0;
        }
        WRITE_DEBUG_IP(cpip,"----\"%d\"\n", *itemIdx);
    }

    if( *itemIdx >= MAX_PROCESS_COUNT )
    {
        WRITE_DEBUG_IP(cpip,"itemIdx (%d) > MAX_PROCESS_COUNT (%d)",*itemIdx, MAX_PROCESS_COUNT);
        DtDa->CpuUsage.size = MAX_PROCESS_COUNT;

        return;
    }


    if( !strcmp(key, STR_CPU_USAGE_RATE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);

        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate),
                 "%.2lf",json_real_value(value));
    }

    if( !strcmp(key, STR_CPU_USAGE_RATE_LIMIT) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);

        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate_limit,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate_limit),
                 "%lld",json_integer_value(value));
    }


    else if( !strcmp(key, STR_CPU_USAGE_CONDITION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);

        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate_condition,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_rate_condition),
                 "%lld",json_integer_value(value));
    }


    else if( !strcmp(key, STR_CPU_USAGE_DETECT_TIME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);

        if(strlen(json_string_value(value)) > 20)
        {
            char subValue[20+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 20, (char *)json_string_value(value), subValue);
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_detect_time,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_detect_time),
                     "%s",subValue);
        }
        else
        {
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_detect_time,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_detect_time),
                     "%s",json_string_value(value));
        }

    }


    else if( !strcmp(key, STR_CPU_USAGE_DURATION_TIME) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(json_is_integer(value))
        {
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time),
                     "%lld",json_integer_value(value));
        }
        else if(json_is_real(value))
        {
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time),
                     "%.0lf",json_real_value(value));
        }
    }

    else if( !strcmp(key, STR_CPU_USAGE_DURATION_TIME_CONDITION) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);

        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time_condition,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_duration_time_condition),
                 "%lld",json_integer_value(value));

    }


    else if( !strcmp(key, STR_CPU_USAGE_STATUS) )
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status),
                 "%lld",json_integer_value(value));

    }


    else if( !strcmp(key, STR_CPU_USAGE_STATUS_NAME ))
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        if(strlen(json_string_value(value)) > 30)
        {
            char subValue[30+1];
            memset(subValue, 0x00, sizeof(subValue));
            fcom_SubStr(1, 30, (char *)json_string_value(value), subValue);
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status_name,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status_name),
                     "%s",
                     subValue);
        }
        else
        {
            snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status_name,
                     sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_status_name),
                     "%s",
                     json_string_value(value));
        }
    }


    else if( !strcmp(key, STR_CPU_USAGE_TYPE ))
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_type,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_type),
                 "%lld",json_integer_value(value));
    }
    else if( !strcmp(key, STR_CPU_USAGE_PROCESS_ID ))
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_process_id,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_process_id),
                 "%lld",json_integer_value(value));
    }

    else if( !strcmp(key, STR_CPU_USAGE_PROCESS_NAME ))
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        snprintf(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_process_name,
                 sizeof(DtDa->CpuUsage.CpuUsageValue[*itemIdx].cpu_usage_process_name),
                 "%s",
                 json_string_value(value));
    }
    else if( !strcmp(key, STR_CPU_USAGE_IS_DAP ))
    {
        fjson_LogJsonAuxFt(5, key, value, 6, cpip);
        DtDa->CpuUsage.CpuUsageValue[*itemIdx].is_dap_agent = json_integer_value(value);
    }

    else if( !strcmp(key, STR_SIZE) )
    {
        fjson_LogJsonAuxFt(5, key, value, 4, cpip);
        DtDa->CpuUsage.size = json_integer_value(value);
        if(DtDa->CpuUsage.size > MAX_PROCESS_COUNT)
            DtDa->CpuUsage.size = MAX_PROCESS_COUNT;
    }
    return;

}