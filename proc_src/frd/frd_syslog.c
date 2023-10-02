//
// Created by KimByoungGook on 2021-11-24.
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

#include "frd.h"
#include "syslog_mon.h"


void frd_ExecSyslog(char* param_ClientIp, int param_EventLevel, int param_EventType)
{
    char local_szSyslogFlag[1 +1]   = {0x00,};
    _SYSLOG_COMM_INFO local_stSyslogInfo;
    memset( &local_stSyslogInfo, 0x00 ,sizeof(local_stSyslogInfo) );

    snprintf(local_stSyslogInfo.szFacility, sizeof(local_stSyslogInfo.szFacility), "%d", 16); // 16(local0)
    snprintf(local_stSyslogInfo.szSeverityLevel, sizeof(local_stSyslogInfo.szSeverityLevel), "%d", 6); // 5(NOTICE), 6(INFO)
    snprintf(local_stSyslogInfo.szClientIp, sizeof(local_stSyslogInfo.szClientIp), "%s", param_ClientIp); //Client IP
    snprintf(local_stSyslogInfo.szProdType, sizeof(local_stSyslogInfo.szProdType), "%s", "DAP"); //제품 구분

    switch(param_EventLevel)
    {
        // PASS, DROP, INFO는 탈일은 없지만 넣어두자.
        case PASS:
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "PASS" ); //이벤트 레벨
            break;
        case DROP:
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "DROP" ); //이벤트 레벨
            break;
        case INFO:
            snprintf(local_stSyslogInfo.szEvent, sizeof(local_stSyslogInfo.szEvent), "%s", "DETECT");
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "INFO" ); //이벤트 레벨
            break;
        case WARNING:
            snprintf(local_stSyslogInfo.szEvent, sizeof(local_stSyslogInfo.szEvent), "%s", "DETECT");
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "WARNING" ); //이벤트 레벨
            break;
        case CRITICAL:
            snprintf(local_stSyslogInfo.szEvent, sizeof(local_stSyslogInfo.szEvent), "%s", "DETECT");
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "CRITICAL" ); //이벤트 레벨
            break;
        case BLOCK:
            snprintf(local_stSyslogInfo.szEvent, sizeof(local_stSyslogInfo.szEvent), "%s", "BLOCK");
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "BLOCK" ); //이벤트 레벨
            break;
        default:
            snprintf(local_stSyslogInfo.szEventLevel, sizeof(local_stSyslogInfo.szEventLevel), "%s", "UNKNOWN" );
            break;
    }

    snprintf(local_stSyslogInfo.szEventType, sizeof(local_stSyslogInfo.szEventType), "%d", param_EventType); //이벤트 타입

    fcom_GetProfile("EVENT_LOGGING","USE_FLAG", local_szSyslogFlag,"N");
    if ( local_szSyslogFlag[0] == 'y' || local_szSyslogFlag[0] == 'Y' )
    {
        // udp sent to syslog_mon
        sendto(g_SyslogUdsSockFd,
              (void *)&local_stSyslogInfo,
              sizeof(local_stSyslogInfo),
              0,
              (struct sockaddr *)&g_stSyslogUdsSockAddr,
              sizeof(struct sockaddr_un));
    }
    else
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"EVENT_LOGGING USE_FLAG Is Disable");
    }


    return;


}


