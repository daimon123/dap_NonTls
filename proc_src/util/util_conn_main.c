//
// Created by KimByoungGook on 2020-10-22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include "util.h"
#include "com/dap_def.h"
#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "secure/dap_secure.h"

/*********************************************************************
 *
 *	display program version
 *
 ********************************************************************/
void fUtil_DisplayVersion()
{
    puts("\n\nProgram Usage ForeGround \n\t Ex) dap_connect 1 ");
    puts("Program Usage BackGround \n\t Ex) dap_connect 1 2 &\n");
    puts("------------------------------------------");
    puts("Connect Timeout :  ");
    puts("1) 0.03 sec ");
    puts("2) 0.05 sec ");
    puts("3) 0.10 sec ");
    puts("4) 0.30 sec ");
    puts("5) 0.50 sec ");
    puts("6) 1.00 sec ");
    puts("------------------------------------------");
    puts("1) Telnet Test(Tcp Connect)");
    puts("2) HW_BASE_TB Connect Test ");
    puts("3) SSL Connect Test");
    puts("4) HW_BASE_TB SSL Connect Test ");
    puts("5) HW_BASE_TB InActive Connect Test ");
    puts("6) HW_BASE_TB InActive SSL Connect Test ");
    puts("------------------------------------------");

    return;
}

int fUtil_ParseCmdOption(int argc, char **argv)
{
    int nArg = 0;
    int nSelectNumber = 0;
    char szTemp[256 +1] = {0x00,};

    /* arg : 1 -> 기능 */
    if (argc < 2)
    {
        fUtil_DisplayVersion();
        exit(0);
    }
    else
    {
        /* 백그라운드 실행 */
        if (argc == 3)
        {
            nSelectNumber = atoi(argv[2]);
        }
        else /* 포그라운드 실행 */
        {
            printf("Input Connect Timeout : \n");
            printf("1) 0.03 sec \n");
            printf("2) 0.05 sec \n");
            printf("3) 0.10 sec \n");
            printf("4) 0.30 sec \n");
            printf("5) 0.50 sec \n");
            printf("6) 1.00 sec \n");
            scanf("%d", &nSelectNumber);
        }

        g_nTimeOutsec = 0;
        switch (nSelectNumber)
        {
            case 1:
                g_nTimeOutUsec = 30000; /* 0.03 */
                break;
            case 2:
                g_nTimeOutUsec = 50000;
                break;
            case 3:
                g_nTimeOutUsec = 100000;
                break;
            case 4:
                g_nTimeOutUsec = 300000;
                break;
            case 5:
                g_nTimeOutUsec = 500000;
                break;
            case 6:
                g_nTimeOutsec = 1;
                g_nTimeOutUsec = 0;
                break;
            default:
                g_nTimeOutsec = 0;
                g_nTimeOutUsec = 30000;
                break;

        }

        nArg = atoi(argv[1]);
        switch (nArg)
        {
            /* Telnet Test */
            case 1:
                /* IP*/
                printf("Input IP : \n");
                scanf("%s", g_szClientIp);
                break;
            case 2:
                if (fdb_ConnectDB() != 0)
                {
                    printf("Connect DB Failed !\n");
                }
                else
                {
                    printf("Connect Success ! \n");
                }
                break;
            case 3:
                printf("Input IP : \n");
                scanf("%s", g_szClientIp);
                break;
            case 4:
                if (fdb_ConnectDB() != 0)
                {
                    printf("Connect DB Failed !\n");
                }
                else
                {
                    printf("Connect Success ! \n");
                }
                break;
            case 5:
            case 6:
                if (fdb_ConnectDB() != 0)
                {
                    printf("Connect DB Failed !\n");
                }
                else
                {
                    printf("Connect Success ! \n");
                }
                break;
            default:
                printf("Unknown Argument (%d) \n", nArg);
                fUtil_DisplayVersion();
                break;
        }

    }

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "./%s","stdout.log");

    if(fcom_fileCheckStatus(szTemp) == 0)
    {
        unlink(szTemp);
    }

    fcom_InitStdLog(szTemp);

    return nArg;
}

int fUtil_Init(void)
{
    _DAP_COMN_INFO* pstComnInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome          ,
             sizeof(pstComnInfo->szDapHome  ),
             "%s"                            ,
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return FALSE;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);


    /* Cert Config */
    snprintf(szCertPath,sizeof(szCertPath),"%s/bin/util/certs",getenv("DAP_HOME"));
    snprintf(szCaFile,sizeof(szCaFile),"%s","dap_util.pem");
    snprintf(szCertFile,sizeof(szCertFile),"%s","dap_util.pem");
    snprintf(szKeyFile,sizeof(szKeyFile),"%s","dap_util_key.pem");

    printf("Succeed in init, pid(%d) |%s\n", getpid(),__func__);

    return TRUE;

}



int main(int argc, char** argv)
{
    int nCase = 0;

    fUtil_Init();

    /* DAP Process Log Init */
    fcom_LogInit("dap_util");

    nCase = fUtil_ParseCmdOption(argc, argv);


    switch(nCase)
    {
        case 1:
            /* Telnet Connect Test */
            /* Agent Port */
            fUtil_TcpConnectionTest(g_szClientIp, 50204,0,NULL);
            /* Agent EMG Port */
            fUtil_TcpConnectionTest(g_szClientIp, 50119,0,NULL);
            break;
        case 2:
            /* HW_BASE_TB Telnet Connect Test */
            fUtil_HwBaseTcpCnnectionTest();
            break;
        case 3:
            /* SSL Connect Test */
            /* Agent Port */
            fUtil_SslConnectionTest(g_szClientIp,50204);
            /* Agent EMG Port */
            fUtil_SslConnectionTest(g_szClientIp,50119);
            break;
        case 4:
            /* HW_BASE_TB SSL Connect Test */
            fUtil_HwBaseSslCnnectionTest();
            break;
        case 5:
            /* HW_BASE_TB Inactive Telnet Connect Test */
            fUtil_HwBaseInactiveTcpCnnectionTest();
            break;
        case 6:
            fUtil_HwBaseSslInActiveConectionTest();
            break;

        default:
            printf("Unkown Function Case \n");
            break;
    }

    return 0;

}