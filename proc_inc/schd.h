//
// Created by KimByoungGook on 2020-06-25.
//

#ifndef SCHD_H
#define SCHD_H

#define     SYNC_GROUP_CHANGE		0
#define     SYNC_USER_CHANGE		1
#define     MAX_SYNC_NOTI     2

typedef struct
{
    int		nCfgKeepSession;
    int		nCfgPrmonInterval;
    int		nCfgSchdMakeHistoryInterval;
    int		nCfgSchdMoveHistoryInterval;
    int		nCfgSchdMakeStatsInterval;
    int		nCfgSyncActive;
    int		nCfgSyncInsBatch;
    int		nCfgBackupActive;
    int		nCfgSchdInterval;
    time_t  nConfigLastModify;

    long	nCurTime;
    long	nLastJobTime;
    long	nLastSendTime;
    long	nIngJobTime;

    char	szCfgSyncRunTime[6+1];
    char	szCfgBackupTime[6+1];
    char	szCfgSyncGroupFileName[50];
    char	szCfgSyncUserFileName[50];
    char	szCfgSyncExUserFileName[50];
    char	szCfgSyncDownFilePath[128];
    char	szCfgSyncDownFileCharset[10];
}_DAP_PROC_SCHD_INFO;

_DAP_PROC_SCHD_INFO g_stProcSchdInfo;


typedef struct
{
    char	szCfgSyncGroupFileName[50];
    char	szCfgSyncUserFileName[50];
    char	szCfgSyncDownFilePath[128];
    char	szCfgSyncDownFileCharset[10];
}_DAP_PROC_SYNC_INFO;

_DAP_PROC_SCHD_INFO g_stProcSyncInfo;

/* ------------------------------------------------------------------- */
/* schd_db.c */
void fschd_SetSafeFetchRow();
int fschd_MakeStatsGwTb();
int fschd_CreateHistoryDatabase(char *p_histDBName);
int fschd_GetGroupValueByRule(char *res);
int fschd_UgdGroupByRule();
int fschd_CreateHistoryTables();
int fschd_MoveHistoryBaseTb();
int fschd_MoveHistoryConfigTb();
int fschd_MoveHistoryRuleTb();
int fschd_MoveHistoryRuleExceptTb();
int fschd_MoveHistoryRuleDetectTb();
int fschd_MoveHistoryRuleDetectLinkTb();
int fschd_MoveHistoryRuleScheduleTb();
int fschd_MoveHistoryUpgradeAgentTb();
int fschd_MoveHistoryUserGroupTb();
int fschd_MoveHistoryUserGroupLinkTb();
int fschd_MoveHistoryUserTb();
int fschd_MoveHistoryUserLinkTb();
int fschd_MoveHistoryManagerTb();
int fschd_MoveHistoryDetectGroupLinkTb();
int fschd_MoveHistoryDetectGroupTb();
int fschd_MoveHistorydetectLinkTb();
int fschd_MoveHistoryDetectProcessTb();
int fschd_MoveHistoryWatchServerTb();
int fschd_MoveHistoryDetectPrinterPortTb();
int fschd_MoveHistoryManagerEventTb();
int fschd_MoveHistoryEventTb();
int fschd_MoveHistoryAlarmTb();
int fschd_MoveHistoryDiskRegLinkTb();
int fschd_LoadFileSyncTb(int flag, char *filePath);
void fschd_GetMoveSyncSql(char *tbName, char *histName, char *buf);
int fschd_SetLocalSyncUserTb();
int fschd_SetLocalSyncUserLinkTb();
int fschd_SetLocalSyncGroupTb();
int fschd_SetLocalSyncRenameGroupTb();
int fschd_SetLocalSyncGroupLinkTb();
int fschd_SetLocalSyncGroupLinkRenameTb();
int fschd_InsertSyncGroupTb(char* filepath, char* tbname);
int fschd_InsertSyncUserTb(char* filepath, char* tbName);
int fschd_InsertExSyncUserTb(void);

int fschd_ParseSyncFile(int nFlag, char* filepath, char* SyncType);
/* ------------------------------------------------------------------- */
#endif //SCHD_H
