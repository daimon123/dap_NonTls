//
// Created by KimByoungGook on 2020-10-30.
//


/* ------------------------------------------------------------------- */
/* System Header                                                       */
/* ------------------------------------------------------------------- */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

/* ------------------------------------------------------------------- */
/* User Header                                                         */
/* ------------------------------------------------------------------- */
#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "linuxke/dap_linux.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "sock/dap_sock.h"
#include "dap_version.h"
#include "master.h"

/* Old */
void fmaster_DeleteServicelogByDay(char* param_LogHome)
{
    struct dirent* pstDirYear = NULL;
    struct dirent* pstDirMonth = NULL;
    struct dirent* pstDirDay = NULL;
    DIR* pDirYear = NULL;
    DIR* pDirMonth = NULL;
    DIR* pDirDay = NULL;
    struct tm   pstTmOld;

    time_t AgoTime = 0;
    char szTemp[256 +1] = {0x00,};
    char BufferYYYY[4 +1] = {0x00,};
    char BufferMM[2 +1] = {0x00,};
    char BufferDD[2 +1] = {0x00,};
    char szCmd[256 +1] = {0x00,};
    char szYearBuffer[256 +1] = {0x00,};
    char szMonthBuffer[256 +1] = {0x00,};
    char szDayBuffer[256 +1] = {0x00,};

    memset(&pstTmOld , 0x00, sizeof(struct tm));
    memset(BufferYYYY, 0x00, sizeof(BufferYYYY));
    memset(BufferMM  , 0x00, sizeof(BufferMM));
    memset(BufferDD  , 0x00, sizeof(BufferDD));
    memset(szCmd     , 0x00, sizeof(szCmd));

    /* $DAP_HOME/log/service/YYYY/MM/DD/ */
    /* $DAP_HOME/log/pcif/YYYY/MM/DD/A/B/C/D */
    AgoTime = (time(NULL) - (ONE_DAY * g_stServerInfo.stDapLogInfo.nCfgKeepDay));
    localtime_r(&AgoTime,&pstTmOld);

    /* $DAP_HOME/log/service , $DAP_HOME/log/pcif */
    chdir(param_LogHome);
    if ((pDirYear = opendir(param_LogHome)) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in open(%s)",param_LogHome );
        return;
    }

    while( (pstDirYear = readdir(pDirYear)) != NULL)
    {
        if(pstDirYear->d_name[0] == '.')
            continue;

        /* 디렉토리가 숫자가 아니면 skip */
        if(fcom_IsDigit(pstDirYear->d_name) == 0)
            continue;

        if(strlen(pstDirYear->d_name) != 4)
            continue;

        memset(szYearBuffer, 0x00, sizeof(szYearBuffer));

        if(atoi(pstDirYear->d_name) < (pstTmOld.tm_year+1900) )
        {
            snprintf(szYearBuffer, sizeof(szYearBuffer), "%s/%s",param_LogHome,pstDirYear->d_name); /* Maybe $DAP_HOME/log/service/YYYY */
            snprintf(szCmd, sizeof(szCmd), "rm -rf %s",szYearBuffer);
            system(szCmd);
            continue;
        }
        /* 현재년도랑 같으면 */
        if(atoi(pstDirYear->d_name) == (pstTmOld.tm_year+1900) )
        {
            snprintf(szYearBuffer, sizeof(szYearBuffer), "%s/%s",param_LogHome,pstDirYear->d_name); /* Maybe $DAP_HOME/log/service/YYYY */
            chdir(szYearBuffer);
            pDirMonth = opendir(szYearBuffer);

            /* $DAP_HOMME/log/service/YYYY/MM */
            while((pstDirMonth = readdir(pDirMonth)) != NULL)
            {
                memset(szMonthBuffer, 0x00, sizeof(szMonthBuffer));
                if(pstDirMonth->d_name[0] == '.')
                    continue;

                /* 디렉토리가 숫자가 아니면 skip */
                if(fcom_IsDigit(pstDirMonth->d_name) == 0)
                    continue;

                if( strlen(pstDirMonth->d_name) != 2 )
                    continue;

                if(atoi(pstDirMonth->d_name) < pstTmOld.tm_mon+1)
                {
                    sprintf(szMonthBuffer,"%s/%s",szYearBuffer,pstDirMonth->d_name);
                    snprintf(szCmd, sizeof(szCmd), "rm -rf %s",szMonthBuffer);
                    system(szCmd);
                }

                if(atoi(pstDirMonth->d_name) == pstTmOld.tm_mon+1)
                {
                    /* Maybe $DAP_HOME/log/service/YYYY/MM */
                    sprintf(szMonthBuffer,"%s/%s",szYearBuffer,pstDirMonth->d_name);
                    /* Change Directory*/
                    chdir(szMonthBuffer);
                    pDirDay = opendir(szMonthBuffer);
                    while((pstDirDay = readdir(pDirDay)) != NULL)
                    {
                        memset(szDayBuffer, 0x00, sizeof(szDayBuffer));

                        if(pstDirDay->d_name[0] == '.')
                            continue;

                        /* 디렉토리가 숫자가 아니면 skip */
                        if(fcom_IsDigit(pstDirDay->d_name) == 0)
                            continue;

                        if(strlen(pstDirDay->d_name) != 2)
                            continue;

                        if(atoi(pstDirDay->d_name) < pstTmOld.tm_mday)
                        {
                            sprintf(szDayBuffer,"%s/%s",szMonthBuffer,pstDirDay->d_name);
                            sprintf(szTemp,"rm -rf %s",szDayBuffer);
                            system(szTemp);
                        }
                    } /* pstDirDay */
                    if( pDirDay != NULL)
                    {
                        closedir(pDirDay);
                    }
                }/* if(atoi(pstDirMonth->d_name) <= pstTmCur->tm_mon+1) */

            }/* pstDirMonth */
            if ( pDirMonth != NULL)
            {
                closedir(pDirMonth);
            }
        }
        /* if(atoi(pstDirYear->d_name) <= (pstTmCur->tm_year+1900) ) */
    } /* pstDirYear */
    if ( pDirYear != NULL)
    {
        closedir(pDirYear);
    }

}

void fmaster_DeleteDirPcif(char* FilePath, int CfgDay)
{
    int  nYYYY = 0, nMM = 0, nDD = 0;
    int nYear = 0, nMonth = 0, nDay = 0;
    time_t AgoTime = 0;
    time_t CurrTime = 0;
    struct tm    stTm;
    struct tm    stTmOld ;
    char BufferYYYY[4 +1] = {0x00,};
    char BufferMM[2 +1] = {0x00,};
    char BufferDD[2 +1] = {0x00,};
    char temp[128 +1] = {0x00,};
    char cmd[128 +1] = {0x00,};

    memset(&stTmOld, 0x00, sizeof(struct tm));
    memset(BufferYYYY, 0x00, sizeof(BufferYYYY));
    memset(BufferMM,   0x00, sizeof(BufferMM));
    memset(BufferDD,   0x00, sizeof(BufferDD));
    memset(temp, 0x00, sizeof(temp));
    memset(cmd, 0x00, sizeof(cmd));

    AgoTime = (time(0) - (ONE_DAY * CfgDay));
    localtime_r(&AgoTime, &stTmOld);

    sprintf(BufferYYYY, "%04d",stTmOld.tm_year+1900);
    sprintf(BufferMM  , "%02d",stTmOld.tm_mon+1);
    sprintf(BufferDD  , "%02d",stTmOld.tm_mday);

    CurrTime = time(NULL);
    localtime_r(&CurrTime,&stTm);

    nYYYY = atoi(BufferYYYY);
    nMM = atoi(BufferMM);
    nDD = atoi(BufferDD);

    for(nYear = atoi(BufferYYYY)-1; nYear <= stTm.tm_year+1900; nYear++)
    {
        for(nMonth = 1; nMonth <= 12; nMonth++)
        {
            for(nDay = 1; nDay <= 31; nDay++)
            {
                if(nMonth <= nMM && nDay < nDD)
                {
                    memset(temp, 0x00, sizeof(temp));
                    memset(cmd, 0x00, sizeof(cmd));

                    sprintf(temp,"%s/%d/%02d/%02d",FilePath,nYear,nMonth,nDay);
                    sprintf(cmd,"rm -rf %s",temp);
                    if(rmdir(temp) != 0)
                    {
                        if(errno != ENOENT)
                        {
                            system(cmd);
                            WRITE_DEBUG(CATEGORY_DEBUG,"Delete Pcif Directory %s ",cmd );
                        }
                    }
                }
            }
            if(nMonth < nMM)
            {
                memset(temp, 0x00, sizeof(temp));
                memset(cmd, 0x00, sizeof(cmd));

                sprintf(temp,"%s/%d/%02d",FilePath,nYear,nMonth);
                sprintf(cmd,"rm -rf %s",temp);
                if(rmdir(temp) != 0)
                {
                    if(errno != ENOENT)
                    {
                        system(cmd);
                        WRITE_DEBUG(CATEGORY_DEBUG,"Delete Pcif Directory %s ",cmd );
                    }

                }
            }
        } //for(nMonth = 1; nMonth <= nMonthSize; nMonth++)
        if(nYear < nYYYY)
        {
            memset(temp, 0x00, sizeof(temp));
            memset(cmd, 0x00, sizeof(cmd));

            sprintf(temp,"%s/%d",FilePath,nYear);
            sprintf(cmd,"rm -rf %s",temp);
            if(rmdir(temp) != 0)
            {
                if(errno != ENOENT)
                {
                    system(cmd);
                    WRITE_DEBUG(CATEGORY_DEBUG,"Delete Pcif Directory %s ",cmd );
                }
            }
        }
    } //for(nYear = atoi(BufferYYYY)-1; nYear <= pstTm->tm_year+1900; nYear++)

}

void fmaster_DeletePciflogByDay(char *specify_file_path, int valid_day)
{
    char   CurrentDate[8 +1] = {0x00,};
    char   filedate[8 +1] = {0x00,};
    time_t cDate = 0;
    struct stat statbuf;
    struct dirent *dirp = NULL;
    DIR   *dp = NULL;
    struct tm stTm;

    memset(&stTm, 0x00, sizeof(struct tm));
    memset(filedate, 0x00, sizeof(filedate));
    memset(CurrentDate, 0x00, sizeof(CurrentDate));
    memset(&statbuf, 0x00, sizeof(struct stat));

    dp = opendir(specify_file_path);
    if(dp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in open(%s)",specify_file_path );
        return;
    }
    fcom_GetFormatTime(YYYYMMDD,CurrentDate,sizeof(CurrentDate));

    chdir(specify_file_path);

    while((dirp = readdir(dp)) != NULL)
    {
        stat(dirp->d_name, &statbuf);
        if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
        {
            if (S_ISDIR(statbuf.st_mode))
            {
                fmaster_DeletePciflogByDay(dirp->d_name, valid_day);
            }
            cDate = time(NULL);
            cDate = statbuf.st_mtime;
            localtime_r(&cDate, &stTm);
            sprintf(filedate, "%04d%02d%02d",
                    stTm.tm_year+1900,
                    stTm.tm_mon+1,
                    stTm.tm_mday);

            if (fcom_GetRelativeDate(CurrentDate, filedate) >= valid_day)
            {
                if (unlink(dirp->d_name) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in delete log file(%s)",dirp->d_name );
                }
            }
        }
        else
        {
            continue;
        }
    }
    chdir("..");
    if ( dp != NULL)
    {
        closedir(dp);
    }

}


int fmaster_ManageLogSize(char* Logpath) /* $DAP_HOME/log/service */
{
    struct stat statbuf;
    struct dirent* pstDirLog = NULL;
    DIR* pDirLog = NULL;
    char* cPtr = NULL;
    char szLogHome   [256 +1] = {0x00,};
    char szRenamePath[256 +1] = {0x00,};
    char szTemp      [256 +1] = {0x00,};
    char szLogFile   [256 +1] = {0x00,};
    char szTimeBuf   [32  +1] = {0x00,};


    /* $DAP_HOME/log/process/YYMM */
    if ((pDirLog = opendir(Logpath)) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in open(%s)", szLogHome );
        return (-1);
    }

    chdir(Logpath);

    /* DAP_HOME/log/process */
    while((pstDirLog = readdir(pDirLog)) != NULL)
    {
        memset(szLogFile, 0x00, sizeof(szLogFile));
        memset(szTemp, 0x00, sizeof(szTemp));

        if(pstDirLog->d_name[0] == '.')
            continue;

        snprintf(szTemp, sizeof(szTemp), "%s/%s",Logpath,pstDirLog->d_name);
        stat(szTemp, &statbuf);

        strcpy(szLogFile,szTemp);


        cPtr = strtok(szTemp,".");
        cPtr = strtok(NULL,".");
        cPtr = strtok(NULL,".");
        cPtr = strtok(NULL,".");

        /* ??? Rename?? ?α? */
        if(cPtr != NULL)
            continue;

        stat(szLogFile, &statbuf);

        if(statbuf.st_size > g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize )
        {
            memset(szRenamePath, 0x00, sizeof(szRenamePath));
            memset(szTimeBuf, 0x00, sizeof(szTimeBuf));

            fcom_GetFormatTime(HHmmss,szTimeBuf,sizeof(szTimeBuf));

            snprintf(szRenamePath,
                     sizeof(szRenamePath),
                     "%s.%s"             ,
                     szLogFile           ,
                     szTimeBuf          );

            WRITE_DEBUG(CATEGORY_DEBUG,"Rename [%s] -> [%s]",szLogFile, szRenamePath);

            rename(szLogFile, szRenamePath);
        }
    }

    closedir(pDirLog);
    return 0;

}

int fmaster_DeleteLogProcess(void)
{
    char szLogHome[256 +1] = {0x00,};

    memset(szLogHome, 0x00, sizeof(szLogHome));

    WRITE_DEBUG(CATEGORY_DEBUG,"Delete Log Service Delete Start ");
    /* $DAP_HOME/log/service */
    snprintf(szLogHome, sizeof(szLogHome), "%s/%s",g_stServerInfo.stDapLogInfo.szCfgLogHome,"service");
    fmaster_DeleteServicelogByDay(szLogHome); /* $DAP_HOME/log/ */

    /* $DAP_HOME/log/pcif */
    memset(szLogHome, 0x00, sizeof(szLogHome));
    snprintf(szLogHome, sizeof(szLogHome), "%s/%s",g_stServerInfo.stDapLogInfo.szCfgLogHome,"pcif");

    WRITE_DEBUG(CATEGORY_DEBUG,"Delete Log Pcif Delete Start ");
    fmaster_DeletePciflogByDay(szLogHome,g_stServerInfo.stDapLogInfo.nCfgKeepDay); /* $DAP_HOME/log/ */
    fmaster_DeleteDirPcif(szLogHome, g_stServerInfo.stDapLogInfo.nCfgKeepDay);

    /* $DAP_HOME/mysql/backup */
    fmaster_DeleteBackupProcess();
    WRITE_DEBUG(CATEGORY_DEBUG,"Delete Log Start ");

    return 0;
}

int fmaster_DeleteBackupProcess(void)
{
    int nOffset = 0;
    int nCfgDay = 0;
    time_t AgoTime = 0;
    DIR* pDirPtr = NULL;
    struct dirent* pReadDir = NULL;
    struct tm     stTmOld;
    char* ptrRemain = NULL;
    char* ptrTok = NULL;
    struct 	stat	stStat;
    char szHomeDir[256+1] = {0x00,};
    char szStatBuffer[256 +1] = {0x00,};
    char szFullPath[256 +1] = {0x00,};
    char szCmd[256 +1] = {0x00,};
    char BufferYYYYMMDD[8 +1] = {0x00,};
    char szTemp[8 +1] = {0x00,};


    /* $DAP_HOME/mysql/backup ???? ???. */
    memset(szHomeDir, 0x00, sizeof(szHomeDir));
    memset(&stStat, 0x00, sizeof(struct stat));
    memset(&stTmOld, 0x00, sizeof(struct tm));

    nCfgDay = fcom_GetProfileInt("KEEPLOG","MAX_BACKUP_KEEP_DAY",7);

    AgoTime = time(NULL) - (ONE_DAY * nCfgDay);
    localtime_r(&AgoTime, &stTmOld);

    snprintf(BufferYYYYMMDD,sizeof(BufferYYYYMMDD),"%04d%02d%02d",
             stTmOld.tm_year+1900,
             stTmOld.tm_mon+1,
             stTmOld.tm_mday);

    snprintf(szHomeDir,sizeof(szHomeDir),"%s/backup/mysql",getenv("DAP_HOME"));

    if ((pDirPtr = opendir(szHomeDir)) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in open(%s) ",szHomeDir);
        return (-1);
    }

    while( (pReadDir = readdir(pDirPtr)) != NULL)
    {
        if(pReadDir->d_name[0] == '.')
            continue;

        memset(szStatBuffer,0x00, sizeof(szStatBuffer));

        snprintf(szStatBuffer,sizeof(szStatBuffer),"%s/%s",szHomeDir,pReadDir->d_name);
        stat(szStatBuffer, &stStat);

        //if dir
        if(S_ISDIR(stStat.st_mode))
        {
            memset(szFullPath, 0x00, sizeof(szFullPath));
            memset(szTemp, 0x00, sizeof(szTemp));

            snprintf(szFullPath, sizeof(szFullPath),"%s",szStatBuffer);

            nOffset = fcom_GetReversePos(szStatBuffer,'/');

            memcpy(&(szTemp[0]),&szStatBuffer[nOffset+1],4); /* YYYY */
            memcpy(&(szTemp[4]),&szStatBuffer[nOffset +1 +4],2); /* MM */
            memcpy(&(szTemp[6]),&szStatBuffer[nOffset +1 +4 +2],2); /* DD */

            if(strcmp(szTemp,BufferYYYYMMDD) < 0)
            {
                snprintf(szCmd, sizeof(szCmd),"rm -rf %s",szFullPath);
                system(szCmd);

                WRITE_DEBUG(CATEGORY_DEBUG,"Mysql Backup Delete Directory : [%s] ",
                            szCmd);
            }
        }
        else
        {
            memset(szFullPath, 0x00, sizeof(szFullPath));
            snprintf(szFullPath, sizeof(szFullPath),"%s",szStatBuffer);

            ptrTok = strtok_r(szStatBuffer, "_", &ptrRemain);
            nOffset = fcom_GetReversePos(ptrTok,'/');

            memset(szTemp, 0x00, sizeof(szTemp));
            memcpy(szTemp,&szStatBuffer[nOffset+1],4); /* YYYY */
            memcpy(szTemp+4,&szStatBuffer[nOffset +1 +4],2); /* MM */
            memcpy(szTemp+6,&szStatBuffer[nOffset +1 +4 +2],2); /* DD */

            if(strcmp(szTemp,BufferYYYYMMDD) < 0)
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"Mysql Backup Delete File : [%s]",
                            szFullPath);

                if(unlink(szFullPath) == (-1))
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Mysql Backup Delete File Failed : [%s] ",
                                   szFullPath);
                }
            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Delete Mysql Backup CfgDay(%d) Success",nCfgDay );

    return 0;

}



int fmaster_RenameLogProcess(void)
{
    struct tm stTm;
    time_t tTime = 0;
    char szLogHome[256 +1] = {0x00,};

    tTime = time(NULL);
    localtime_r(&tTime, &stTm);

    memset(&stTm, 0x00, sizeof(struct tm));
    memset(szLogHome, 0x00, sizeof(szLogHome));

    WRITE_INFO(CATEGORY_DEBUG,"Rename Log Start ");
    sprintf(szLogHome,"%s/%s/%04d/%02d/%02d",
            g_stServerInfo.stDapLogInfo.szCfgLogHome,
            "service",
            stTm.tm_year+1900,
            stTm.tm_mon+1,
            stTm.tm_mday);

    fmaster_ManageLogSize(szLogHome);
    memset(szLogHome, 0x00, sizeof(szLogHome));
    sprintf(szLogHome,"%s/%s/%04d/%02d/%02d",
            g_stServerInfo.stDapLogInfo.szCfgLogHome,
            "pcif",
            stTm.tm_year+1900,
            stTm.tm_mon+1,
            stTm.tm_mday);


    return 0;
}


int fmaster_DeleteDbProcess(void)
{
    WRITE_INFO(CATEGORY_INFO,"Delete Table Start " );

    fdb_DeleteDbByDay("SYS_OMC_ERRORLOG_TB",g_stServerInfo.stDapLogInfo.nCfgTableKeepDay);
//    fdb_DeleteDbByDay("SERVER_ERRORLOG_TB",g_stServerInfo.stDapLogInfo.nCfgTableKeepDay);
    fdb_DeleteDbByDay("MANAGER_EVENT_TB",g_stServerInfo.stDapLogInfo.nCfgTableKeepDay);

    WRITE_INFO(CATEGORY_INFO,"Delete Table End " );

    return RET_SUCC;
}

void fmaster_DeleteMngQueue(void)
{
    DIR   *dp = NULL;
    struct dirent *dirp = NULL;
    struct stat statbuf;
    char	mnqPath[128 +1] = {0x00,};

    memset(mnqPath, 0x00, sizeof(mnqPath));
    sprintf(mnqPath, "%s/MNGQ", g_stServerInfo.stDapQueueInfo.szDAPQueueHome);

    if ((dp = opendir(mnqPath)) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in open(%s)",
                       mnqPath);
        return;
    }

    chdir(mnqPath);

    while((dirp = readdir(dp)) != NULL)
    {
        stat(dirp->d_name, &statbuf);
        if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
        {
            if (S_ISDIR(statbuf.st_mode))
                continue;

            if (unlink(dirp->d_name) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in delete queue file(%s) ",
                               dirp->d_name);
            }
        }
        else
        {
            continue;
        }
    }

    chdir("..");
    closedir(dp);
}
