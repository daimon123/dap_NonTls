//
// Created by KimByoungGook on 2020-06-26.
//

#ifndef _DAP_JSON_H
#define _DAP_JSON_H

#include "com/dap_req.h"
#include "com/dap_def.h"
#include "json/jansson.h"

/* ------------------------------------------------------------------- */
/* dap_json.c */
void fjson_PrintAllAgentInfo            (_DAP_AGENT_INFO *p_AgentInfo                   );
void fjson_PrintAllManagerInfo          (_DAP_MANAGER_INFO* p_ManagerInfo               );
void fjson_PrintAllMainBoard            (_DAP_MAINBOARD *p_MainBoard                    );
void fjson_PrintAllSystem               (_DAP_SYSTEM *p_System                          );
void fjson_PrintAllConnectExt           (_DAP_CONNECT_EXT *p_ConnectExt                 );
void fjson_PrintAllOperatingSystem      (_DAP_OPERATING_SYSTEM* p_OperatingSystem       );
void fjson_PrintAllCpu                  (_DAP_CPU *p_Cpu                                );
void fjson_PrintAllNetAdapter           (_DAP_NETADAPTER *p_NetAdapter                  );
void fjson_PrintAllWifi                 (_DAP_WIFI *p_Wifi                              );
void fjson_PrintAllBluetooth            (_DAP_BLUETOOTH *p_Bluetooth                    );
void fjson_PrintAllNetConnection        (_DAP_NETCONNECTION *p_NetConnection            );
void fjson_PrintAllDisk                 (_DAP_DISK* p_Disk                              );
void fjson_PrintAllNetDrive             (_DAP_NETDRIVE *p_NetDrive                      );
void fjson_PrintAllOsAccount            (_DAP_OSACCOUNT *p_OA                           );
void fjson_PrintAllShareFolder          (_DAP_SHARE_FOLDER *p_ShareFolder               );
void fjson_PrintAllInfraredDevice       (_DAP_INFRARED_DEVICE *p_InfraredDevice         );
void fjson_PrintAllProcess              (_DAP_PROCESS *p_Process                        );
void fjson_PrintAllRouter               (_DAP_ROUTER *p_Router                          );
void fjson_PrintAllNetPrinter           (_DAP_NETPRINTER *p_NetPrinter                  );
void fjson_PrintAllNetScan              (_DAP_NETSCAN* p_NetScan                        );
void fjson_PrintAllSsoCert              (_DAP_SSO_CERT *p_SsoCert                       );
void fjson_PrintAllWinDrv               (_DAP_WINDRV* p_WinDrv                          );
void fjson_PrintAllRdpSession           (_DAP_RDP_SESSION *p_RdpSession                 );
void fjson_PrintAllDetectData           (char *p_changeItem, _DAP_DETECT_DATA *p_DetectData);
void fjson_PrintAllDetect               (_DAP_DETECT *p_Detect                          );
void fjson_PrintAllCpuUsage             (_DAP_CPU_USAGE* p_cpuUsage                     );
int  fjson_GetLengthProcessWhiteSummary (char *p_summary                                );
int fjson_GetLengthProcessAccessSummary(char* p_summary);
int  fjson_GetBlockcommonSummary        (char *p_summary, char *detail                  );
int  fjson_GetLengthProcessBlackSummary (char *p_summary                                );
int  fjson_GetLengthMainBoard           (_DAP_MAINBOARD* p_MainBoard                    );
int  fjson_GetLengthSystem              (_DAP_SYSTEM* p_System                          );
int  fjson_GetLengthSystemVm            (_DAP_SYSTEM* p_System                          );
int  fjson_GetLengthSystemVmSummary     (char* p_summary                                );
int  fjson_GetLengthConnectExtSummary   (char* p_summary                                );
int  fjson_GetLengthConnectExt          (_DAP_CONNECT_EXT* p_ConnectExt                 );
int  fjson_GetLengthOperatingSystem     (_DAP_OPERATING_SYSTEM* p_OperatingSystem       );
int  fjson_GetLengthCpu                 (_DAP_CPU *p_Cpu                                );
int  fjson_GetLengthNetAdapter          (_DAP_NETADAPTER *p_NetAdapter                  );
int  fjson_GetLengthNetAdapterPhysicalNic(_DAP_NETADAPTER* p_NetAdapter                 );
int  fjson_GetBlockNetAdapterSummary    (char *p_summary, char *detail                  );
int  fjson_GetLengthNetAdapterMulip     (_DAP_NETADAPTER* p_NetAdapter, char *detail    );
int  fjson_GetLengthWifi                (_DAP_WIFI* p_Wifi                              );
int  fjson_GetLengthBluetooth           (_DAP_BLUETOOTH* p_Bluetooth                    );
int  fjson_GetLengthNetConnection       (_DAP_NETCONNECTION *p_NetConnection            );
int  fjson_GetLengthDisk                (_DAP_DISK* p_Disk                              );
int  fjson_GetLengthDiskRemovable       (_DAP_DISK* p_Disk                              );
int  fjson_GetLengthDiskHidden          (_DAP_DISK* p_Disk                              );
int  fjson_GetLengthDiskMobileRead      (_DAP_DISK* p_Disk                              );
int  fjson_GetLengthShareFolder         (_DAP_SHARE_FOLDER* p_ShareFolder               );
int  fjson_GetLengthNetDrive            (_DAP_NETDRIVE* p_NetDrive                      );
int  fjson_GetLengthInfraredDevice      (_DAP_INFRARED_DEVICE* p_InfraredDevice         );
int  fjson_GetLengthProcessAccessmon    (_DAP_PROCESS* p_Process                        );
int  fjson_GetLengthProcessBlockSummary (char *p_summary                                );
int  fjson_GetLengthRouter              (_DAP_ROUTER* p_Router                          );
int  fjson_GetLengthNetPrinter          (_DAP_NETPRINTER* p_NetPrinter                  );
int  fjson_GetLengthSsoCert             (_DAP_SSO_CERT* p_SsoCert                       );
int  fjson_GetLengthRdpSession          (_DAP_RDP_SESSION* p_RdpSession                 );
int  fjson_GetLengthAlarmCpuUsage       (_DAP_CPU_USAGE* p_CpuUsage                     );
int  fjson_GetLengthControlCpuUsage     (_DAP_CPU_USAGE* p_CpuUsage                     );
DETECT_ITEM fjson_GetDetectItem         (char *p_type                                   );
/* ------------------------------------------------------------------- */


/* ------------------------------------------------------------------- */
/* dap_json_parse.c*/
void fjson_ParseJsonInteger     (json_t *element                                                            );
void fjson_ParseJsonString      (json_t *element                                                            );
void fjson_LogJsonIndentFt      (int indent, char* param_blank                                              );
void fjson_LogJsonIndent        (int indent, char* param_blank                                              );
void fjson_ParseJsonFt          (json_t *root, _DAP_DETECT* Detect, _DAP_DETECT_DATA* DetectData, char *cpip);
void fjson_LogJsonInteger       (int level, const char* key, json_t *element, int indent                    );
void fjson_LogJsonIntegerFt     (int level, const char* key, json_t *element, int indent, char* cpip        );
void fjson_LogJsonString        (const char* key, json_t *element, int indent                               );
void fjson_LogJsonStringFt      (int level, const char* key, json_t *element, int indent, char* cpip        );
void fjson_ParseJsonObjectFt    (
                                json_t*			   element  ,
                                char*			   pkey     ,
                                char*			   skey     ,
                                int				   itemIdx  ,
                                _DAP_DETECT*	   Dt       ,
                                _DAP_DETECT_DATA*  DtDa     ,
                                char*			   cpip                                                     );
void jfson_ParseJsonSystemFt    (
                                const char*		  key  ,
                                char*			  skey ,
                                json_t*			  value,
                                _DAP_DETECT_DATA* DtDa ,
                                char*			  cpip                                                      );
void fjson_LogJsonAux           (int level, const char* key, json_t* element, int indent                    );
void fjson_LogJsonAuxFt         (int level, const char* key, json_t* element, int indent, char* cpip        );
void fjson_ParseJsonAuxFt       (
                                json_t*			  element   ,
                                char*			  pkey      ,
                                char*			  skey      ,
                                int				  itemIdx   ,
                                _DAP_DETECT*	  Detect    ,
                                _DAP_DETECT_DATA* DetectData,
                                char*             cpip                                                      );
void fjson_GetSummaryFt         (json_t *value, char *type, char *res                                       );
void fjson_GetSummaryCpuUsage(json_t* element, _DAP_DETECT_DATA* 	DtDa, char* cpip);
void fjson_ParseJsonMainBoardFt (
                                const char*		  key,
                                json_t*           value,
                                _DAP_DETECT_DATA* DtDa,
                                char*			  cpip                                                      );


void fjson_ParseJsonOperatingSystemFt(
        const char*		key,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip);

void fjson_ParseJsonCpuFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA *	DtDa,
        char*			cpip);

void fjson_ParseJsonNetAdapterFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip);

void fjson_ParseJsonWifiFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip);


void fjson_ParseJsonBluetoothFt(
        const char*		key,
        int				*itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip);

void fjson_ParseJsonNetConnectionFt(
        const char*		key,
        int*			itemIdx,
        json_t*			value,
        _DAP_DETECT_DATA*	DtDa,
        char*			cpip);

void fjson_ParseJsonDiskFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonNetDriveFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip);

void fjson_ParseJsonOsAccountFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonShareFolderFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonInfraredDeviceFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonProcessFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonRouterFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonNetPrinterFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonConnectExtSvrFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonNetScanFt(
        const char* 	key,
        char* 			skey,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char* 			cpip);

void fjson_ParseJsonSsoCertFt(
        const char* 	key,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip);
void fjson_ParseJsonRdpSession(const char* key, int *itemIdx, json_t* value, _DAP_DETECT_DATA * DtDa, char*	cpip);

void fjson_ParseJsonWinDrvFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa,
        char*			cpip);
void fjson_ParseJsonRdpSessionFt(
        const char* 	key,
        int*			itemIdx,
        json_t* 		value,
        _DAP_DETECT_DATA* 	DtDa);
void fjson_ParseJsonCpuUsage(
        const char* 	  key,
        int*			  itemIdx,
        json_t* 		  value,
        _DAP_DETECT_DATA* DtDa,
        char*			  cpip
);
/* ------------------------------------------------------------------- */


#endif //_DAP_JSON_H
