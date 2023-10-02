//
// Created by KimByoungGook on 2021-06-14.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "com/dap_com.h"

void fFile_SetPidFileSt(void)
{
    int local_nLoop         = 0;
    int local_nStCnt        = sizeof(g_stProcPidFileSt) / sizeof(g_stProcPidFileSt[0]);

    for ( local_nLoop = 0; local_nLoop < local_nStCnt; local_nLoop++ )
    {
        switch(local_nLoop)
        {
            case ENUM_DAP_MASTER:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_MASTER;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_master");
                break;
            }

            case ENUM_DAP_ALARM:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_ALARM;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_alarm");
                break;
            }

            case ENUM_DAP_DBLOG:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_DBLOG;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_dblog");
                break;
            }

            case ENUM_DAP_FRD:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_FRD;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_frd");
                break;
            }

            case ENUM_DAP_PCIF:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_PCIF;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_pcif");
                break;
            }

            case ENUM_DAP_POLICY_FW:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_POLICY_FW;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_policy_fw");
                break;
            }

            case ENUM_DAP_PRMON:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_PRMON;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_prmon");
                break;
            }

            case ENUM_DAP_PROXY:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_PROXY;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_proxy");
                break;
            }

            case ENUM_DAP_REPORT:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_REPORT;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_report");
                break;
            }

            case ENUM_DAP_SCHD:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_SCHD;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_schd");
                break;
            }

            case ENUM_DAP_SYSMAN:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_SYSMAN;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_sysman");
                break;
            }

            case ENUM_DAP_VWLOG:
            {
                g_stProcPidFileSt[local_nLoop].nEnumIdx = ENUM_DAP_VWLOG;
                snprintf(g_stProcPidFileSt[local_nLoop].szProcName, sizeof(g_stProcPidFileSt[local_nLoop].szProcName),
                         "%s", "dap_vwlog");
                break;
            }

            default:
            {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Invalid Client Pid File Index (%d)", local_nLoop);
                break;
            }
        }
    }


    return;

}

char* fFile_GetPidFileName(int param_EnumFileIdx)
{
    int local_nLoop = 0;
    int local_nStClient = sizeof(g_stProcPidFileSt)/sizeof(g_stProcPidFileSt[0]);

    for ( local_nLoop = 0; local_nLoop < local_nStClient; local_nLoop++ )
    {
        if ( param_EnumFileIdx == g_stProcPidFileSt[local_nLoop].nEnumIdx)
        {
            return g_stProcPidFileSt[local_nLoop].szProcName;
        }
    }

    return NULL;
}


// 프로세스 이름으로 파일 PID 삭제한다.
void fFile_cleanupMasterPid(char* preFilePath, char* processName )
{
    char szFullPath[256 +1] = {0x00,};

    sprintf(szFullPath, "%s/%s.pid", preFilePath, processName);

    if( (fcom_fileCheckStatus(szFullPath) ) == 0)
    {
        unlink( szFullPath );
    }

    WRITE_DEBUG(CATEGORY_DEBUG,"%s Service process id deleted. (%s)\n\n" , processName , szFullPath );
}

// 프로세스 이름으로 파일 PID 을 생성한다.
int fFile_MakeMasterPid(char* PidFilePath, char* ProcessName)
{
    FILE	*local_fp = NULL;
    char	 szFullPath[256 +1] = {0x00,};
    char	 local_buff[10 +1] = {0x00,};


    sprintf( szFullPath , "%s/%s.pid" , PidFilePath, ProcessName  );

    if ( ( local_fp = fopen ( szFullPath , "w" ) ) == NULL )
    {
        return -1;
    }
    else
    {
        sprintf( local_buff , "%d" , getpid() );
        fputs ( local_buff , local_fp );

        fclose ( local_fp );

        return 0;
    }
}

//	프로세스 이름으로 PID 값을 리턴 받는다.
int fFile_GetMasterPid(char* PidFilePath, char* ProcessName)
{
    FILE		*local_fp = NULL;
    char		local_buff[256 +1] = {0x00,};
    char        szFullPath[256 +1] = {0x00,};
    int		local_return_value = 0;


    sprintf( szFullPath , "%s/%s.pid" , PidFilePath, ProcessName  );

    if( ( local_fp = fopen( szFullPath , "r" ) ) == NULL )
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"Pid File Open Failed");
        local_return_value = -1;
    }
    else
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"Pid File")
        fread( local_buff , 1 , 10 , local_fp );
        local_return_value = atoi(local_buff);
        fclose( local_fp );
    }

    return local_return_value;
}



//	프로세스 이름으로 PID 생성 여부를 확인한다.
int fFile_CheckMasterPid(char* PidFilePath, char* ProcessName)
{
    char		local_buff[256 +1] = {0x00,};

    sprintf( local_buff , "%s.pid" , ProcessName  );


    if( fcom_procFileCheckStatus(PidFilePath, local_buff) == 0 )
    {
        return 0;	// 파일 있음
    }

    return -1; // 파일 없음
}