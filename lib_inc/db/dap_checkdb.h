//
// Created by KimByoungGook on 2020-06-15.
//

#ifndef DAP_CHECKDB_H
#define DAP_CHECKDB_H

#define		TABLE_LENGTH		30

#define     TABLE_KIND_FIX       0
#define     TABLE_KIND_DD        1
#define     TABLE_KIND_MM        2


extern char g_exTbItem  [62][TABLE_LENGTH];
extern char g_exHistItem[51][TABLE_LENGTH];
extern char g_exViewItem[15][TABLE_LENGTH];


int fdb_ViewFilter          (char *sqlbuff, char *view_name, char *event_name       );
int fdb_NameFilter          (char *sqlbuff, char *table_name                        );
int fdb_ExecuteScriptTB     (char *filepath                                         );
int fdb_ExecuteScript       (char *table_name, char *filepath                       );
int fdb_GetExistsDatabase   (char *p_DBName                                         );
int fdb_CreateDatabase      (char* p_DBName                                         );
int fdb_CheckExistsTable    (char *p_histDBName, char *p_histTBName                 );
int fdb_GetExistsColumns    (char *p_tbDBName, char *p_tbTableName, char *p_column  );
int fdb_GetExistsTable      (char *p_histDBName, char *p_histTBName                 );
int fdb_CheckScriptColumns  (char *cpLFile, char *p_tbTableName                     );
int fdb_CheckTbTables       (void                                                   );
int fdb_ExecuteQuery        (char *buf, char *p_tbTableName                         );
int fdb_CheckNotiDb         (char *proc                                             );
int fdb_DeleteDbByDay       (char *tbName, int valid_day                            );
int fdb_DeleteSyncTableByDay(void);
int fdb_CleanAlarm          (                                                       );
int fdb_PutSessionQuery     (                                                       );
int fdb_LsystemInit         (                                                       );
int fdb_SelectCountHbSq     (
                             unsigned long long p_hbSq,                             // hb_sq of HW_BASE_TB
                             char*				p_tableName                         ); // table name
int fdb_SelectSqWinDrv      (
                             unsigned long long p_hbSq     , 		                    // hb_sq of HW_BASE_TB
                             char*				p_dvName   ,		                    // db_name of WIN_DRV
                             char*				p_dvService,                            // dv_service of WIN_DRV
                             char*				p_dvDriver ,		                    // dv_driver of WIN_DRV
                             unsigned long long* p_tbSq                             );  // result variable
int fdb_GetHistoryTableName (
                             char *tableName,
                             char *logTable ,
                             char *p_prefix ,
                             char *p_postfix                                        );
int fdb_GetHistoryTableTime (char *p_tableFixed,
                             char *p_yearbuffer,
                             int iday);
int fdb_GetHistoryDbTime    (char *p_dbFixed   , int iday                           );

int fdb_GetIpByBase         (
                             char*	p_type, 		                                    // distinguished columns
                             char*	p_userKey, 		                                    // pc unique key
                             char*	p_hbAccessRes                                   );	// result variable
#endif //DAP_CHECKDB_H
