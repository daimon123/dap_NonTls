//
// Created by KimByoungGook on 2020-06-23.
//

#ifndef DAP_TRANDB_H
#define DAP_TRANDB_H

#include "db/dap_defield.h"


int fdb_InsertServerErrorlogHistory(_DAP_DB_SERVERLOG_INFO* p_dbLog, char *histTableName);
int fdb_GetSubGroupSq               (unsigned long long p_grpSq , char* result                          );
int fdb_GetGroupName                (int p_grpSq, char* p_grpStr                                        );
int fdb_InsertReportHistoryTb       (char* gubun, char* p_histTableName                                 );
int fdb_InsertHwBaseHistory         (char* p_userKey, char *histTableName                               );
int fdb_InsertUserHistory           (unsigned long long p_sq  , char* histTableName                     );
int fdb_InsertHistoryEventTb        (unsigned long long p_evSq, char* p_histTableName, int p_evExist    );
int fdb_InsertHistory               (unsigned long long p_tbSq, char* p_tableName, char* p_histTableName);

#endif //DAP_NEW_DAP_TRANDB_H
