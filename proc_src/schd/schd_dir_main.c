//
// Created by KimByoungGook on 2020-10-06.
//

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "ipc/dap_Queue.h"
#include "secure/dap_secure.h"

#include "schd.h"


int g_SyncType;

static int fstSchdInit();
static void fstHandleSignal(int sid);
static void fstSigHandler();
static int fstMainTask();

int main(int argc, char** argv)
{
    if(argc != 3)
        exit(0);

    fcom_ArgParse(argv);

    if(strlen(argv[2]) > 0)
        g_SyncType = atoi(argv[2]);

    fstSchdInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fstSigHandler();

    fstMainTask();

    exit(0);


}
static int fstMainTask()
{
    int nRet = 0;
    char szSyncType[32 +1] = {0x00,};
    char szFilePath[256 +1] = {0x00,};

    nRet = fdb_ConnectDB();

    if(nRet != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in connect DB " );
        exit(1);
    }

    memset(szSyncType, 0x00, sizeof(szSyncType));

    fcom_GetProfile("SYNC","SYNC_DIR_TYPE",szSyncType,"IBK");

    /* 기업은행 인사연동 */
    if(!strcmp(szSyncType,"IBK"))
    {
        switch(g_SyncType)
        {
            // GROUP
            case 1:
                snprintf(szFilePath,sizeof(szFilePath),"%s/%s",g_stProcSyncInfo.szCfgSyncDownFilePath,g_stProcSyncInfo.szCfgSyncGroupFileName);
                nRet = fschd_ParseSyncFile(1, szFilePath, szSyncType);
                if(nRet == 0)
                {
                    nRet = fschd_SetLocalSyncRenameGroupTb();
                    nRet = fschd_SetLocalSyncGroupLinkRenameTb();
                    if(nRet < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DB,"ParseSyncFile Group Fail " );
                        exit(1);
                    }
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DB,"ParseSyncFile Group Fail " );
                    exit(1);
                }

                break;

            // USER
            case 2:
                snprintf(szFilePath,sizeof(szFilePath),"%s/%s",
                         g_stProcSyncInfo.szCfgSyncDownFilePath,g_stProcSyncInfo.szCfgSyncUserFileName);
                nRet = fschd_ParseSyncFile(2, szFilePath, szSyncType);

                snprintf(szFilePath,sizeof(szFilePath),"%s/%s",
                         g_stProcSyncInfo.szCfgSyncDownFilePath,g_stProcSyncInfo.szCfgSyncExUserFileName);
                nRet =  fschd_ParseSyncFile(3, szFilePath, szSyncType);
                if(nRet == 0)
                {
                    nRet = fschd_SetLocalSyncUserTb();
                    nRet = fschd_SetLocalSyncUserLinkTb();
                    if(nRet < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DB,"ParseSyncFile Group Fail " );
                        exit(1);
                    }
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DB,"ParseSyncFile User Fail " );
                    exit(1);
                }

                break;
            default:
                WRITE_CRITICAL(CATEGORY_DB,"Unknown Sync Type ");
                break;
        }
    }

    exit(0);
}


static int fstSchdInit()
{
    int nRet = 0;
    _DAP_COMN_INFO* pstComnInfo = NULL;
    char szCfgTmp[256 +1] = {0x00,};

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             "%s",
             getenv("DAP_HOME")             );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                     );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    fcom_GetProfile("SYNC","DOWN_FILE_PATH",g_stProcSyncInfo.szCfgSyncDownFilePath,"/tmp/dap");

    if (access(szCfgTmp, W_OK) != 0)
    {
        nRet = fcom_MkPath(szCfgTmp, 0755);
        if (nRet < 0)
        {
            printf("Fail in make path(%s)|%s\n", szCfgTmp, __func__);
        }
        else
        {
            chmod(szCfgTmp, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
            printf("Succeed in make path(%s)|%s\n", szCfgTmp, __func__);
        }
    }

    fcom_GetProfile("SYNC","GROUP_FILE_NAME",g_stProcSyncInfo.szCfgSyncGroupFileName,"");
    fcom_GetProfile("SYNC","USER_FILE_NAME",g_stProcSyncInfo.szCfgSyncUserFileName,"");
    fcom_GetProfile("SYNC","EX_USER_FILE_NAME",g_stProcSyncInfo.szCfgSyncExUserFileName,"");
    fcom_GetProfile("SYNC","DOWN_FILE_CHARSET",g_stProcSyncInfo.szCfgSyncDownFileCharset,"");

    printf("Success Init | %s\n",__func__ );

    return TRUE;

}
static void fstHandleSignal(int sid)
{

    WRITE_CRITICAL(CATEGORY_DEBUG, "Got signal(%d)", sid);

    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);

    fdb_SqlClose(g_stMyCon);


    if(g_pstNotiMaster != NULL)
        free(g_pstNotiMaster);

    fFile_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_schd");

    exit(sid);
}


static void fstSigHandler()
{
    signal( SIGHUP, fstHandleSignal);    /* 1 : hangup */
    signal( SIGINT, fstHandleSignal);    /* 2 : interrupt (rubout) */
    signal( SIGQUIT, fstHandleSignal);   /* 3 : quit (ASCII FS) */
    signal( SIGILL, fstHandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP, fstHandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT, fstHandleSignal);    /* 6 : IOT instruction */
    signal( SIGABRT, fstHandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal( SIGFPE, fstHandleSignal);    /* 8 : floating point exception */
    signal( SIGKILL , fstHandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS , fstHandleSignal);   /* 10: bus error */
    signal( SIGSEGV, fstHandleSignal);   /* 11: segmentation violation */
    signal( SIGSYS , fstHandleSignal);   /* 12: bad argument to system call */
    signal( SIGPIPE, SIG_IGN );              /* 13: write on a pipe with no one to read it */

    signal( SIGALRM, fstHandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fstHandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fstHandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fstHandleSignal);   /* 17: user defined signal 2 */
    signal( SIGCLD, SIG_IGN );              /* 18: child status change */
    signal( SIGCHLD,SIG_IGN );             /* 18: child status change alias (POSIX) */
    signal( SIGPWR , fstHandleSignal);   /* 19: power-fail restart */
    signal( SIGWINCH,SIG_IGN );            /* 20: window size change */
    signal( SIGURG  , fstHandleSignal);  /* 21: urgent socket condition */
    signal( SIGPOLL , fstHandleSignal);  /* 22: pollable event occured */
    signal( SIGIO   , fstHandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP , fstHandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal( SIGTSTP , fstHandleSignal);  /* 24: user stop requested from tty */
    signal( SIGCONT , fstHandleSignal);  /* 25: stopped process has been continued */
    signal( SIGTTIN , fstHandleSignal);  /* 26: background tty read attempted */
    signal( SIGTTOU , fstHandleSignal);  /* 27: background tty write attempted */
    signal( SIGVTALRM , fstHandleSignal);/* 28: virtual timer expired */
    signal( SIGPROF , fstHandleSignal);  /* 29: profiling timer expired */
    signal( SIGXCPU , fstHandleSignal);  /* 30: exceeded cpu limit */
    signal( SIGXFSZ , fstHandleSignal);  /* 31: exceeded file size limit */

}