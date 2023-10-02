#ifndef DEF_H
#define DEF_H

#include "linuxke/machine.h"

#define RET_SUCC 0
#define RET_FAIL (-1)
#define TRUE     1
#define FALSE    0

/* Debug level is 1:critical,2:major,3:warning,4,info,5:debug 6:ALL*/
#define DAP_CRITICAL 1
#define DAP_MAJOR    2
#define DAP_WARN     3
#define DAP_INFO     4
#define DAP_DEBUG    5
#define DAP_ALL      6

#define CATEGORY_DB    1
#define CATEGORY_IPC   2
#define CATEGORY_INFO  3
#define CATEGORY_DEBUG 4

// RULE LEVEL
#define		PASS		0
#define		DROP		1
#define		INFO		2
#define		CAUTION		3 // nouse
#define		WARNING		4
#define		CRITICAL	5
#define		BLOCK		6


#define     PROCESS_CHANGE              0
#define     THRESHOLD_CHANGE            1

/* DAP_Sysman*/
#define     FILESYSTEM_CONFIG_CHANGE    0
#define		MAX_FILESYSTEM	20
#define		MAX_QUEUE		10

/* Common FQ */
#define		PRMON_QUEUE				0

// DAP
/** DBIF이제 안쓰므로 주석처리. **/
//#define		DBIF01_QUEUE			1
//#define		DBIF02_QUEUE			2
//#define		DBIF03_QUEUE			3
//#define		DBIF04_QUEUE			4
//#define		DBIF05_QUEUE			5
//#define		DBIF06_QUEUE			6
//#define		DBIF07_QUEUE			7
//#define		DBIF08_QUEUE			8
//#define		DBIF09_QUEUE			9
//#define		DBIF10_QUEUE			10

#define		DBLOG_QUEUE				11
#define		REPORT_QUEUE			12
#define		ALARM_QUEUE				13
#define		TAIL_QUEUE				14
#define     FW_POLICY               15
#define     FW_SERVICE              16


/* PRMON */
#define		PRMON_LINKER			41
#define		MAX_THRESHOLD_CATEGORY	4		/*	threshold count		*/

/* DBLOG */
//#define		PROCESS_ERROR_LOG		40
#define		PROCESS_SERVER_LOG		40
#define		PROCESS_AGENT_LOG		42
#define		MAX_ALARM_NOTI		    1

#define		PORT_LEN				6
#define		ADDR_LEN				20
#define		VM_LEN					32

#define CUR_RULE_CNT    43
#define MAX_PORT_LENGTH 6
#define MAX_PROCESS_LENGTH 30
#define MAX_IPADDR_LENGTH 16

typedef enum
{
    UNKNOWN, //0
    MAIN_BOARD, //1
    SYSTEM, //2
    OPERATING_SYSTEM, //3
    CPU, //4
    NET_ADAPTER, //5
    WIFI, //6
    BLUETOOTH, //7
    NET_CONNECTION, //8
    DISK, //9
    NET_DRIVE, //10
    OS_ACCOUNT, //11
    SHARE_FOLDER, //12
    INFRARED_DEVICE, //13
    PROCESS, //14
    ROUTER, //15
    NET_PRINTER, //16
    NET_SCAN, //17
    ARP, //18
    VIRTUAL_MACHINE, //19
    CONNECT_EXT_SVR, //20
    NET_ADAPTER_OVER, //21
    NET_ADAPTER_DUPIP, //22
    NET_ADAPTER_DUPMAC, //23
    DISK_REG,	//24
    DISK_HIDDEN, //25
    DISK_NEW, //26
    DISK_MOBILE, //27
    DISK_MOBILE_READ, //28
    DISK_MOBILE_WRITE, //29
    PROCESS_WHITE, //30
    PROCESS_BLACK, //31
    PROCESS_ACCESSMON, //32
    NET_ADAPTER_MULIP, //33
    EXTERNAL_CONN, //34
    SSO_CERT, //35
    WIN_DRV, //36
    PROCESS_BLOCK, //37
    RDP_SESSION, //38
    RDP_BLOCK_COPY, //39
    CPU_USAGE, //40
    CPU_USAGE_ALARM, //41
    CPU_USAGE_CONTROL //42
} DETECT_ITEM;



typedef enum
{
    SYS_STANDALONE_WORKSTATION,
    SYS_MEMBER_WORKSTATION,
    SYS_STANDALONE_SERVER,
    SYS_MEMBER_SERVER,
    SYS_BACKUP_DOMAIN_CONTROLLER,
    SYS_PRIMARY_DOMAIN_CONTROLLER
} SYS_DomainRole;

typedef enum
{
    SYS_RESERVED,
    SYS_OTHER,
    SYS_UNKNOWN,
    SYS_APM_TIMER,
    SYS_MODEM_RING,
    SYS_LAN_REMOTE,
    SYS_POWER_SWITCH,
    SYS_PCI_PME,
    SYS_AC_POWER_RESTORED
} SYS_WakeupType;

typedef enum
{
    NA_DISCONNECTED,
    NA_CONNECTING,
    NA_CONNECTED,
    NA_DISCONNECTING,
    NA_HARDWARE_NOT_PRESENT,
    NA_HARDWARE_DISABLED,
    NA_HARDWARE_MALFUNCTION,
    NA_MEDIA_DISCONNECTED,
    NA_AUTHENTICATING,
    NA_AUTHENTICATION_SUCCEEDED,
    NA_AUTHENTICATION_FAILED,
    NA_INVALID_ADDRESS,
    NA_CREDENTIALS_REQUIRED,
    NA_OTHER
} NA_NetConnectionStatus;

typedef enum
{
    WF_PROFILE_CONNECTION,
    WF_TEMP_PROFILE_CONNECTION,
    WF_SECURE_DISCOVERY_CONNECTION,
    WF_UNSECURE_DISCOVERY_CONNECTION,
    WF_CONNECTION_WIRELESS_PERSISTENT_PROFILE,
    WF_INVALID_CONNECTION_MODE
} WF_ConnectionMode;

typedef enum
{
    WF_NOT_READY,
    WF_CONNECTED,
    WF_FIRST_NODE_AD_HOC,
    WF_DISCONNECTING,
    WF_NOT_CONNECTED,
    WF_ATTEMPTING_ASSOCIATE,
    WF_AUTO_CONFIGURATION_DISCOVERING,
    WF_PROCESS_AUTHENTICATING
} WF_InterfaceStatus;

typedef enum
{
    WF_UNKNOWN,
    WF_FHSS,
    WF_DSSS,
    WF_IRBASEBAND,
    WF_OFDM,
    WF_HRDSSS,
    WF_ERP,
    WF_HT,
    WF_IHV_START = 0x80000000,
    WF_IHV_END = 0xffffffff
} WF_PhyNetworkType;

typedef enum
{
    NC_CONNECTED = 1,
    NC_PAUSED,
    NC_DISCONNECTED,
    NC_ERROR,
    NC_CONNECTING,
    NC_RECONNECTING
} NC_ConnectionState;

typedef enum
{
    NC_DRIVE_REDIRECTION = 1,
    NCPRINT_REDIRECTION
} NC_ConnectionType;

typedef enum
{
    DK_UNKNOWN,
    DK_NO_ROOT_DIRECTORY,
    DK_REMOVABLE_DISK,
    DK_LOCAL_DISK,
    DK_NETWORK_DRIVE,
    DK_COMPACT_DISC,
    DK_RAM_DISK
} DK_DriveType;

typedef enum
{
    DK_NOACCESS,
    DK_READABLE,
    DK_WRITEABLE,
    DK_READ_WRITE,
    DK_WRITE_ONCE
} DK_Access;

typedef enum
{
    ND_DRIVE_REDIRECTION = 1,
    ND_PRINT_REDIRECTION
} ND_ConnectionType;

typedef enum
{
    OA_SID_TYPE_USER = 1,
    OA_SID_TYPE_GROUP,
    OA_SID_TYPE_DOMAIN,
    OA_SID_TYPE_ALIAS,
    OA_SID_TYPE_WELL_KNOWN_GROUP,
    OA_SID_TYPE_DELETED_ACCOUNT,
    OA_SID_TYPE_INVALID,
    OA_SID_TYPE_UNKNOWN,
    OA_SID_TYPE_COMPUTER
} OA_SidType;

typedef enum
{
    OA_TEMPORARY_DUPLICATE_ACCOUNT = 256,
    OA_NORMAL_ACCOUNT = 512,
    OA_INTERDOMAIN_TRUST_ACCOUNT = 2048,
    OA_WORKSTATION_TRUST_ACCOUNT = 4096,
    OA_SERVER_TRUST_ACCOUNT = 8192
} OA_Type;

typedef enum
{
    SF_DISK_DRIVE,
    SF_PRINT_QUEUE,
    SF_DEVICE,
    SF_IPC,
    SF_DISK_DRIVE_ADMIN = 2147483648,
    SF_PRINT_QUEUE_ADMIN = 2147483649,
    SF_DEVICE_ADMIN = 2147483650,
    SF_IPC_ADMIN = 2147483651
} SF_Type;


typedef struct
{
    char szProcName         [ 128+1];
    char szDebugName        [ 10 +1];
    char szDapHome          [128 +1];
    char szComConfigFile    [ 64 +1];
    char szUpdateConfigPath [255 +1];
    char szPidPath          [256 +1];
    char szServerIp         [15 +1];
    int  nCfgKeepSession            ;
    int  nCfgMgwId                  ;
    int  nCfgMaxOpenFileLimit       ;
    int  nArgSecInterval            ;

}_DAP_COMN_INFO;

typedef struct
{
    char szCertPath[256 +1];
    char szCaFile  [30  +1];
    char szCertFile[30  +1];
    char szKeyFile [30  +1];

}_DAP_CERT_INFO;

typedef struct
{
    char msgtype[4];
    char procname[30];
}_DAP_PING;


typedef struct
{
    char szDAPQueueHome       [128 +1]; /* .DAPQ           */
    char szDblogQueueHome     [128 +1]; /* .DAPQ/DBLOGQ    */
    char szPrmonQueueHome     [128 +1]; /* .DAPQ/PRMONQ    */
    char szTailQueueHome      [128 +1]; /* .DAPQ/TAILQ     */
    char szSaveDblogQueueHome [128 +1]; /* back/SAVEQDBLOG */
    char szSaveAlarmQueueHome [128 +1]; /* back/SAVEALARM  */
    char szAlaramQueueHome    [128 +1]; /* .DAPQ/ALARMQ    */
    char szReportQueueHome    [128 +1]; /* .DAPQ/REPORTQ   */
    char szDbifQueueHome      [128 +1]; /* .DAPQ/DBIF01 - DBIF 10*/
}_DAP_QUEUE_INFO;

typedef struct
{
    int nCfgMaxLogFileSize     ;
    int nCfgDebugLevel         ;
    char szCfgDbWriteFlag[1 +1];
    int nCfgDbWriteLevel       ;
    int nCfgKeepDay            ;
    int nCfgTableKeepDay       ;
    int nCfgJobTime            ;

    int    nCurrDay            ;
    time_t LastMofifyTime      ;

    char szCfgLogHome       [128+1];
    char szCfgLogPath       [256+1];
    char szCfgPcifLogPAth   [256+1];
    char szCfgTbOmcErrorLog [ 30+1];
    char szCfgTbSvrErrorLog [ 30+1];
    char szCfgTbManagerEvent[ 30+1];
    char szcfgLogFileName   [64+1];

}_DAP_LOG_INFO;

typedef struct
{
    int  nCfgHistDBPrefix;
    int	 nCfgHistTableKind;
    int  nCfgHistDbKind;
    int  nCfgHistTableNext;
    char szCfgHistDBPrefix[30];
}_DAP_DB_CONFIG;


typedef struct
{
    _DAP_COMN_INFO  stDapComnInfo ;
    _DAP_QUEUE_INFO stDapQueueInfo;
    _DAP_CERT_INFO  stDapCertInfo ;
    _DAP_LOG_INFO   stDapLogInfo  ;
    _DAP_DB_CONFIG stDapDbConfigInfo;

    struct statics stStatics;

}_DAP_SERVER_INFO;



#endif //DEF_H
