//
// Created by KimByoungGook on 2020-06-29.
//

#ifndef PCIF_H
#define PCIF_H

#include <stdbool.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <sys/epoll.h>


#include "com/dap_req.h"
#include "com/dap_def.h"
#include "com/dap_com.h"
#include "ipc/dap_qdef.h"
#include "json/jansson.h"
#include "secure/dap_SSLHelper.h"



#define         MAX_RULE_COUNT      5000

#define			MAX_PCIF_NOTI		8
#define			CONFIG_CHANGE			0
#define			RULE_CHANGE				1
#define			CP_CHANGE				2
#define			BASE_CHANGE				3
#define			RULE_SCHEDULE_CHANGE	4
#define			RULE_DETECT_CHANGE		5
#define			AGENT_UPGRADE_CHANGE	6
#define			GW_CHANGE				7


#define         FORK_POLICY         0
#define         MAX_DBFILE_INDEX      99999
#define         MAX_DBFILE_SIZE       41943040 //40MB

#define			MAX_EVENT_NOTI		1
#define			EVENT_CHANGE		0

/* RULE DETECT TYPE 정의 */
/*
 * 0:BLACK,
 * 1:WHITE,
 * 2:ACCESS,
 * 3:CONEXT,
 * 4:KILL,
 * 5:TOTAL ALARM CPU EXCEPTION,
 * 6:ALARM CPU EXCEPTION,
 * 7:TOTAL CONTROL CPU EXCEPTION,
 * 8:CONTROL CPU EXCEPTION
*/

#define BLACK_PROCESS               0
#define WHITE_PROCESS               1
#define ACCESS_PROCESS              2
#define CONEXT_URL                  3
#define KILL_PROCESS                4
#define TOTAL_ALARM_CPU_EXCEPTION   5
#define ALARM_CPU_EXCEPTION         6
#define TOTAL_CONTROL_CPU_EXCEPTION 7
#define CONTROL_CPU_EXCEPTION       8
#define MAX_DETECT_TYPE             9

#define MAX_CHILD	40
#define EPOLL_TABLE_SIZE 2048
#define MAX_POLL_EVENTS 1000

#define         MAX_UPGRADE_COUNT      100
#define         MAX_GW_COUNT        20000
#define			MAX_MANAGER_COUNT	100
#define			MAX_USER_COUNT		30000
#define			MAX_GROUP_COUNT		20000
#define			MAX_BASE_COUNT		30000
#define         MAX_CONFIG_COUNT    100

#define			MAX_THREAD	255


#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) * __mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

enum ev_magic
{
    EV_MAGIC_LISTENER = 0x1010,
    EV_MAGIC_CLIENT = 0x2010,
    EV_MAGIC_SERVER = 0x3020,
};

/* Statues indicators of proxy sessions. */
enum conn_state
{
    S_INVALID,
    //S_CLIENT_CONNECTED,
    S_CLIENT_WORKING,
    S_CLOSING,
};


typedef struct
{
    unsigned int    	tot_cnt;
    unsigned int		ru_order[MAX_RULE_COUNT];
    unsigned long long	ru_sq[MAX_RULE_COUNT];
    unsigned long   	ru_modify_cnt[MAX_RULE_COUNT];
    unsigned long long 	mn_sq[MAX_RULE_COUNT];
    unsigned char		ru_target_type[MAX_RULE_COUNT];
    unsigned char   	ru_target_value[MAX_RULE_COUNT][31+1];
    unsigned char		ru_net_adapter[MAX_RULE_COUNT];
    unsigned char		ru_net_adapter_over[MAX_RULE_COUNT];
    unsigned char		ru_net_adapter_dupip[MAX_RULE_COUNT];
    unsigned char		ru_net_adapter_dupmac[MAX_RULE_COUNT];
    unsigned char		ru_net_adapter_mulip[MAX_RULE_COUNT];
    unsigned char		ru_wifi[MAX_RULE_COUNT];
    unsigned char		ru_bluetooth[MAX_RULE_COUNT];
    unsigned char		ru_router[MAX_RULE_COUNT];
    unsigned char		ru_printer[MAX_RULE_COUNT];
    unsigned char		ru_disk[MAX_RULE_COUNT];
    unsigned char		ru_disk_reg[MAX_RULE_COUNT];
    unsigned char		ru_disk_hidden[MAX_RULE_COUNT];
    unsigned char		ru_disk_new[MAX_RULE_COUNT];
    unsigned char		ru_disk_mobile[MAX_RULE_COUNT];
    unsigned char		ru_disk_mobile_read[MAX_RULE_COUNT];
    unsigned char		ru_disk_mobile_write[MAX_RULE_COUNT];
    unsigned char		ru_net_drive[MAX_RULE_COUNT];
    unsigned char		ru_net_connection[MAX_RULE_COUNT];
    unsigned char		ru_share_folder[MAX_RULE_COUNT];
    unsigned char		ru_infrared_device[MAX_RULE_COUNT];
    unsigned char		ru_virtual_machine[MAX_RULE_COUNT];
    unsigned char		ru_process_white[MAX_RULE_COUNT];
    unsigned char		ru_process_black[MAX_RULE_COUNT];
    unsigned char		ru_process_accessmon[MAX_RULE_COUNT];
    unsigned char		ru_process_accessmon_exp[MAX_RULE_COUNT];
    unsigned char		ru_process_detailinfo[MAX_RULE_COUNT];
    unsigned char		ru_process_forcekill[MAX_RULE_COUNT];
    unsigned char		ru_connect_ext_svr[MAX_RULE_COUNT];
    unsigned char		ru_ext_net_detect_type[MAX_RULE_COUNT]; //not yet
    unsigned char  		ru_sso_cert[MAX_RULE_COUNT];
    short		        ru_win_drv[MAX_RULE_COUNT];             //윈도우 드라이버 수집여부.
    unsigned char		ru_rdp_session[MAX_RULE_COUNT];
    unsigned char		ru_rdp_block_copy[MAX_RULE_COUNT];
    short  				ru_agent_cycle_process[MAX_RULE_COUNT];
    short  				ru_agent_cycle_process_access[MAX_RULE_COUNT];
    short  				ru_agent_cycle_net_printer[MAX_RULE_COUNT];
    short  				ru_agent_cycle_net_scan[MAX_RULE_COUNT];
    short  				ru_agent_cycle_router[MAX_RULE_COUNT];
    short  				ru_agent_cycle_ext_access[MAX_RULE_COUNT];
    short  				ru_agent_sso_check_cycle[MAX_RULE_COUNT];
    short  				ru_agent_sso_keep_time[MAX_RULE_COUNT];
    unsigned char		ru_alarm_type[MAX_RULE_COUNT];
    unsigned long long	ru_alarm_mn_sq[MAX_RULE_COUNT];
    unsigned char   	ru_flag[MAX_RULE_COUNT];
    unsigned char		*dp_print_port;

    unsigned char       ru_cpu_alarm[MAX_RULE_COUNT];
    unsigned char       ru_cpu_ctrl[MAX_RULE_COUNT];
    unsigned int        ru_cpu_alarm_rate[MAX_RULE_COUNT]; // 전체 CPU 알람 기준 사용률( 1 - 100% )
    unsigned int        ru_cpu_alarm_sustained_time[MAX_RULE_COUNT]; // 전체 CPU 알람 기준 지속시간(1 - 3600초 )
    unsigned int        ru_cpu_ctrl_rate[MAX_RULE_COUNT]; // 전체 CPU 통제 기준 사용률 ( 1 - 100% )
    unsigned int        ru_cpu_ctrl_sustained_time[MAX_RULE_COUNT]; // 전체 CPU 통제 기준 지속시간( 1 - 3600초 )
    unsigned int        ru_cpu_ctrl_limit_rate[MAX_RULE_COUNT]; // 전체 CPU 통제 목표 사용률( 1 - 100 )
    char                ru_str_agent_cpu[MAX_RULE_COUNT][512];
    short               ru_agent_self_protect[MAX_RULE_COUNT];  //Agent 자기보호기능 활성화 여부
} dbRule;

typedef struct
{
    unsigned int    	tot_cnt;
    unsigned long long 	ua_sq[MAX_UPGRADE_COUNT];
    unsigned char		ua_name[MAX_UPGRADE_COUNT][32+1];
    unsigned char		ua_version[MAX_UPGRADE_COUNT][15+1];
    unsigned char		ua_target_type[MAX_UPGRADE_COUNT];
    unsigned char		ua_target_value[MAX_UPGRADE_COUNT][31+1];
    unsigned char		ua_date[MAX_UPGRADE_COUNT][8+1];
    unsigned char		ua_time[MAX_UPGRADE_COUNT][6+1];
} dbUgd;

typedef struct
{
    unsigned int    	tot_cnt;
    unsigned long long	ru_sq[MAX_RULE_COUNT];
    unsigned char		rs_name[MAX_RULE_COUNT][32+1];
    unsigned char		rs_type[MAX_RULE_COUNT];
    unsigned char		rs_weekofday[MAX_RULE_COUNT][20];
    unsigned char		rs_dayofweek[MAX_RULE_COUNT][30];
    unsigned char		rs_day[MAX_RULE_COUNT][100];
    unsigned char		rs_date[MAX_RULE_COUNT][17+1];
    unsigned char		rs_time[MAX_RULE_COUNT][13+1];
    unsigned char		rs_exception[MAX_RULE_COUNT];
} dbSchd;

typedef struct
{
    unsigned char	sg_class_ip[11+1];
    unsigned char	sg_default_mac[128];
} dbGw;


typedef struct
{
    unsigned int    	tot_cnt;
    unsigned long long	ru_sq[MAX_RULE_COUNT];
    unsigned char		*rd_value[MAX_RULE_COUNT];
    unsigned char		rd_type[MAX_RULE_COUNT];
} dbDetect;

typedef struct
{
    unsigned char	*dp_process_white;
    unsigned char	*dp_process_black;
    unsigned char	*dp_process_block;
    unsigned char	*ws_ipaddr;
    unsigned char	*ws_url;

    unsigned char   *dp_tot_alarm_cpu_exception;
    unsigned char   *dp_alarm_cpu_exception;
    unsigned char   *dp_tot_ctrl_cpu_exception;
    unsigned char   *dp_ctrl_cpu_exception;
} dbDetectSel;

typedef struct
{
    unsigned long long	mn_sq;
    int             	mn_level;
    unsigned char   	mn_id[32+1];
    unsigned char   	mn_pw[255+1];
    unsigned char   	mn_ipaddr[50+1];
    unsigned char   	mn_login_status;
    unsigned char   	mn_flag;
    int             	mn_fail_count;
    unsigned char   	mn_event_noti;
    unsigned short  	mn_conn_pid;
    short  				mn_conn_fq; // default -1
} dbManager;

typedef struct
{
    unsigned long long	us_sq;
    unsigned char   	us_ip[128+1];
    unsigned char		us_sno[32+1];
    unsigned char		us_db_router;
    unsigned char		us_log_level;
    unsigned char   	us_auto_sync;
    unsigned char   	us_flag;
    unsigned long long	ug_sq;
    unsigned char   	ug_name[100+1];
    //unsigned char us_ip_chain[31+1];
} dbUser;


typedef struct
{
    unsigned long long	ug_sq_p;
    unsigned long long	ug_sq_c;
} dbGroupLink;


typedef struct
{
    unsigned long long	hb_sq;
    unsigned long long	us_sq;
    unsigned char   	hb_unq[20+1];
    unsigned char   	hb_access_ip[15+1];
    unsigned char   	hb_access_mac[17+1];
    unsigned char   	hb_sock_ip[15+1];
    unsigned char   	hb_agent_ver[16+1];
    unsigned char   	hb_first_time[19+1];
    unsigned char   	hb_access_time[19+1];
    unsigned char   	hb_del;
    unsigned char       hb_external;
} dbBase;

typedef struct
{
    unsigned char   cfname[64+1];
    unsigned char   cfvalue[512+1];
    unsigned char   cfflag;
} dbConfig;

typedef struct
{
    int  time_out;
    int	 cfgRetryAckCount;
    int  retryAckSleep;
    int	 cfPrmonInterval;
    char pcPort[10];
    int  nListenPort;
    int  cfgQCount;
    int  cfgCheckNotiInterval;
    int  cfgCheckEventInterval;
    int	 cfgUseGetArpMac;
    int	 cfgLoadDetectMem;
    int	 cfgUseDebugRealIp;
    int  cfgThreadCnt;
    int	 cfgForkCnt;
    int	 cfgKeepSession;
    int	 cfgFileDownDelayTime;
    char cfgDropL4SwIp[128];
    char cfgReportPath[128];
    long last_send_time;
    long last_accept_time;
    long ing_job_time;

    char certPath[256];
    char caFile[30];
    char certFile[30];
    char keyFile[30];
    char caFullPath[286+1];
    char certFullPath[286+1];
    char keyFullPath[286+1];
    char cfgThreadHangCheck[1 +1];

    /** File 처리 변수 **/

    char szDtFilePath[256 +1];
    char szPolicyFilePath[256 +1];
    char szFileName[32+1];
    char szFileFullPath[256 +1];

    int   nLastFileIdx;
    int   nRsltQsize;
    unsigned int   nCfgMaxFileSize;
    int   nCfgMaxFileIndex;
    FILE* Fp;
    /**********************************/

    int nTotRule;
    int nTotSchd;
    int nTotManager;
    int nTotUser;
    int nTotGroupLink;
    int nTotBase;
    int nTotGw;
    int nTotDetect;
    int nTotUpgrade;
    int nTotConfig;
    time_t nConfigLastModify;
    size_t nThreadStackSize;

//    SSL_CTX *ctx;

    int	 listenSock;
    int	 parentPid;
    int* ptrChildPid;

    int  nAcceptNonBlockFlag; // 0 : Accept Block Mode, 1 : Accept Non-Block Mode

}_DAP_PROC_PCIF_INFO;

typedef struct _ThreadInfo
{
    int		count;
    int		pid;
    int		socket;
    char	ipaddr[15+1];
} ThreadInfo;

struct proxy_conn
{
    int         cli_thread;
    int         cli_sock;
    char        cli_ip[15+1];

    short threadStepStatus;
    int   threadMsgCode;
};


typedef struct
{
    unsigned char   	na_rule;
    unsigned long long  na_rusq;
    unsigned char   	wf_rule;
    unsigned long long  wf_rusq;
    unsigned char   	bt_rule;
    unsigned long long  bt_rusq;
    unsigned char   	nc_rule;
    unsigned long long  nc_rusq;
    unsigned char   	dk_rule;
    unsigned long long  dk_rusq;
    unsigned char   	nd_rule;
    unsigned long long  nd_rusq;
    unsigned char   	sf_rule;
    unsigned long long  sf_rusq;
    unsigned char   	id_rule;
    unsigned long long  id_rusq;
    unsigned char   	rt_rule;
    unsigned long long  rt_rusq;
    unsigned char   	np_rule;
    unsigned long long  np_rusq;
    unsigned char   	st_rule_vm;
    unsigned long long  st_rusq_vm;
    unsigned char   	na_over_rule;
    unsigned long long  na_over_rusq;
    unsigned char   	na_dupip_rule;
    unsigned long long  na_dupip_rusq;
    unsigned char   	na_dupmac_rule;
    unsigned long long  na_dupmac_rusq;
    unsigned char   	dk_reg_rule;
    unsigned long long  dk_reg_rusq;
    unsigned char   	dk_hidden_rule;
    unsigned long long  dk_hidden_rusq;
    unsigned char   	dk_new_rule;
    unsigned long long  dk_new_rusq;
    unsigned char   	dk_mobile_rule;
    unsigned long long  dk_mobile_rusq;
    unsigned char   	dk_mobile_read_rule;
    unsigned long long  dk_mobile_read_rusq;
    unsigned char   	dk_mobile_write_rule;
    unsigned long long  dk_mobile_write_rusq;
    unsigned char   	ps_black_rule;			// RULE_DETECT_TB RD_TYPE=0
    unsigned long long  ps_black_rusq;
    int		ps_black_idx;
    unsigned char   	ps_white_rule;			// RULE_DETECT_TB RD_TYPE=1
    unsigned long long  ps_white_rusq;
    int		ps_white_idx;
    unsigned char   	ps_accessmon_rule;		// RULE_DETECT_TB RD_TYPE=2
    unsigned long long  ps_accessmon_rusq;
    unsigned char   	ps_accessmon_exp;
    //unsigned long long  ps_accessmon_exp_rusq; nouse
    int		ps_accessmon_idx;
    unsigned char   	ps_block_rule;			// RULE_DETECT_TB RD_TYPE=4
    unsigned long long  ps_block_rusq;
    int		ps_block_idx;
    unsigned char   	ce_rule;			// RULE_DETECT_TB RD_TYPE=3
    unsigned long long  ce_rusq;
    int		connect_ext_idx;
    unsigned char   	ps_detailinfo_rule;
    unsigned long long  ps_detailinfo_rusq;
    unsigned char   	ps_forcekill_rule;
    unsigned long long  ps_forcekill_rusq;
    unsigned char   	na_mulip_rule;
    unsigned long long  na_mulip_rusq;
    unsigned char   	sc_rule;
    unsigned long long  sc_rusq;
//    unsigned char   	win_drv_rule;
//    unsigned long long  win_drv_rusq;
    unsigned char   	rdp_session_rule;
    unsigned long long  rdp_session_rusq;
    unsigned char   	rdp_block_copy_rule;
    unsigned long long  rdp_block_copy_rusq;

    unsigned char   	cpu_alarm_rule;
    unsigned long long  cpu_alarm_rusq;
    unsigned int        cpu_alarm_rate;
    unsigned int        cpu_alarm_sustained_time;
    unsigned int        total_cpu_alarm_except_idx; //전체 프로세스 알람 예외 IDX
    unsigned int        cpu_alarm_idx;  //프로세스 알람 예외 IDX

    unsigned char   	cpu_ctrl_rule;
    unsigned long long  cpu_ctrl_rusq;
    unsigned int        cpu_ctrl_rate; // 전체 CPU 통제 기준 사용률 ( 1 - 100% )
    unsigned int        cpu_ctrl_sustained_time; // 전체 CPU 통제 기준 지속시간( 1 - 3600초 )
    unsigned int        cpu_ctrl_limit_rate; // 전체 CPU 통제 목표 사용률( 1 - 100 )
    unsigned int        cpu_ctrl_idx; // 개별 프로세스 통제 IDX
    unsigned int        cpu_ctrl_except_idx; // 프로세스 통제 예외 IDX
    char                agent_str_cpu_limit[512];
} cpRule;

typedef struct
{
    short	agent_process;
    short 	agent_process_access;
    short 	agent_net_printer;
    short 	agent_net_scan;
    short 	agent_router;
    short 	agent_ext_access;
    short 	agent_sso_check_cycle;
    short 	agent_sso_keep_time;
} cpCycle;

typedef struct
{
    char   	win_drv_rule;   //Windows 드라이버 수집여부
    unsigned long long  win_drv_rusq;   //Windows 드라이버 Rule SQ번호.
    char    agent_self_protect; // agent 자기보호기능 활성화여부
}cpFlag;


_DAP_PROC_PCIF_INFO g_stProcPcifInfo;
_CONFIG_NOTI* g_pstNotiEvent;

dbRule		    ruleInfo;
dbRule*		    pRuleInfo;
dbUgd		    ugdInfo;
dbUgd*		    pUgdInfo;
dbSchd		    schdInfo;
dbSchd*		    pSchdInfo;
dbGw            gwInfo[MAX_GW_COUNT];
dbGw*           pGwInfo;
dbDetect		detectInfo;
dbDetect*		pDetectInfo;
dbDetectSel		detectSelInfo;
dbDetectSel*	pDetectSelInfo;

dbManager		managerInfo[MAX_MANAGER_COUNT];
dbManager*		pManagerInfo;
dbConfig        configInfo[MAX_CONFIG_COUNT];
dbConfig*       pConfigInfo;

/* stack -> heap 변경 */
//dbBase			baseInfo[MAX_BASE_COUNT];
//dbUser			userInfo[MAX_USER_COUNT];
//dbGroupLink     groupLink[MAX_GROUP_COUNT];
dbBase*			pBaseInfo;
dbGroupLink*    pGroupLink;
dbUser*			pUserInfo;

struct proxy_conn* g_stThread_arg;

/** Pcif DB처리 구별을 위한 Fork Index **/
int                g_nPcifForkIdx;

struct _STLOGTAIL
{
    SSL*   ssl;
    int    sock;
    int    mgwid;
    char  FilePath[256+1];
    char  FlagFilePath[256+1];
    char*  cpip;
};
struct _DistTailInfo
{
    SSL_Connection *sslcon;
    SSL*            p_manager_ssl; // Manager SSL
    int             manager_sock; // Manager Sock

    char*           cpip; //LogIp
    char  FlagFilePath[256+1];
};

typedef struct _TAIL
{
    FILE *fp;            // 파일 스트림
    char filename[256];  // 오픈한 파일 이름
    int fd;              // 파일 스트림에 대한 파일 지정자
    int revsize;         // 최근에 읽었던 파일 크기
} TAIL;
pthread_t pthreadtail;
pthread_t pthreadDisttail;
pthread_attr_t	attr2;

pthread_mutex_t mutexsum;
pthread_mutex_t mutexFile;
pthread_rwlock_t mutexpolicy;

pthread_mutex_t g_pthread_Mutex;

/* Thread Use / Unuse Flag */
char    thrFlag[MAX_THREAD];
/* Accept Thread Timer */
time_t  thrTimer[MAX_THREAD];
/* Fork Restart Flag */
//char    g_ForkExitFlag;
char    g_ForkHangFlag;

int		mngFQNum[MAX_MANAGER_COUNT];
int		gFQNum;
int		fqIdx;
int		g_ChildP[MAX_CHILD];
char    g_pthread_stop_sign[MAX_THREAD];
/* Fork Child Exit Flag */
char    g_ChildExitFlag;

/* 0x00 : 초기화  * 0x01 : 읽기가능  * 0x02 : 접근금지 */
char UpgradestFlag;
/* 0x00 : 초기화  * 0x01 : 로드완료  * 0x03 : 접근금지 */
char LoadConfigFlag;

char WrMutexFlag;
/* HW_BASE_TB dap.cfg Max Count Value */
int cfgMaxBase;

/* Fork 종료 Flag */
char g_ChildExitFlag;

int fpcif_PcifInit(void);

int fpcif_AgentRcvTask(int DetectType, int* buffLen, int* firstItem, char* tmpArrTokenKey,char* cpip, _DAP_QUEUE_BUF* QBuf, _DAP_DETECT_DATA* DetectData);
int fpcif_ManagerWork(
        int     thrNum,
        int     sock,
        char*   cpip,
        char*   msgType,
        int     msgCode,
        int     msgLeng,
        int     fqNum,
        char*   Buffer
);
int fpcif_GetForkIdx(pid_t param_FindPid);
void fpcif_DelAllMnq(void);
int fpcif_CheckNotiFile(void);
void* fpcif_ThreadLoop(void* arg);
int	fpcif_ThreadGetIdx( int nThreadCnt );
void fpcif_PrintThreadStatus(int nCfgThreadCnt);
int fpcif_ThreadHangCheck(void);
int fpcif_ForkTask(void);
void fpcif_SigchldHandler(int s);
void fpcif_ParentExit(int sid);
void fpcif_ChildExit(int sid);
void fpcif_KillZombieCp(void);
int fpcif_ReloadConfig(void);
void fpcif_Cleanup(int socketFd);
int fpcif_ForkWork();
int fpcif_CheckEventNotiFile(void);
void	fpcif_HandleSignal(int		sid);
void fpcif_SigHandler(void);
void fpcif_KillTail(char *logip, char* clientIp, char* clientPath);
int fpcif_CheckPid(char *logip, int pid, char *proc);
//int fpcif_ChkUpgradeFlag(char p);
int fpcif_ChkUpgradeFlag(void);
//int fpcif_ChkLoadFlag(char p);
int fpcif_ChkLoadFlag(void);
void fpcif_DistKillTail(char *logip, char* clientIp, char* clientPath);
int	fpcif_DistServerLog(
        int		sockMng,
        char*	p_mngIp,
        char*	p_distServerIp,
        int		p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize);
int fpcif_CopyServerLog(
        char 	*logPath,
        char	*cat,
        char 	*sDate,
        char	*eDate,
        char	*p_tmpLogDir,
        char	*findIp,
        char	*findStr,
        int		*findCnt,
        char	*p_logip);
int fpcif_GetFileName(json_t* rootLogList,
                      char* LogPath,
                      char* category,
                      char* StartDate,
                      char* EndDate,
                      char* FindIp,
                      char* FindStr,
                      int*   param_FindCnt,
                      char* LogIP);
int fpcif_ReqServerLogList(json_t** jsonRootLogList,
                           char* LogPath,
                           char* category,
                           char* StartDate,
                           char* EndDate,
                           char* FindIp,
                           int* FindCnt,
                           char* LogIP);
int fpcif_ReqServerLogFile(char* LogPath,
                           char* TarHome ,
                           char* category,
                           char* StartDate,
                           char* EndDate,
                           char* FindIp,
                           char* FindStr,
                           int* FindCnt,
                           char* LogIP);
int fpcif_GetFileNameCopy(char* LogPath,
                          char* TarHome,
                          char* category,
                          char* StartDate,
                          char* EndDate,
                          char* FindIp,
                          char* FindStr,
                          int* FindCnt,
                          char* LogIP);
int	fpcif_DistServerTail(
        char*	logip,
        char*	p_distServerIp,
        int		p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize,
        SSL_Connection**    p_sslcon);

int fpcif_CidrToIpAndMask(const char *cidr, uint32_t *ip, uint32_t *mask);
int fpcif_SetRuleInit(cpRule* pCpRuleInfo);
int fpcif_SetFlagInit(cpFlag* pCpFlagInfo);
int fpcif_SetRuleSt(int idx, cpRule* pCpRuleInfo, int p_agtFlag, char *p_logip);
void fpcif_SetCycleInit(cpCycle* pCpCycleInfo);
void fpcif_SetCycleSt(char *logip, int idx, cpCycle* pCpCycleInfo);
void fpcif_SetFlagSt(char* logip, int idx, cpFlag* pCpFlagInfo);
void fpcif_SetCycleLast(char *logip, cpCycle* pCpCycleInfo);
void fpcif_SetFlagLast(char* logip, cpFlag *  pCpFlagInfo);
int fpcif_GetFileToBin(char *fname, char **resData);
int fpcif_GetRuleSt(
        char*		p_realip,
        char*		p_logip,
        char*		p_sno,
        cpRule*		pCpRuleInfo,
        cpCycle*	pCpCycleInfo,
        cpFlag*     pCpFlag,
        int			agtFlag);
int fpcif_GetUpgradeSt(char* p_realip, char *p_logip, char* p_cpVer);
int fpcif_ValidDate(char *logip, char* stDate, char* stTime, int stType, int stExp);
int fpcif_ValidDayofWeek(char *logip, char* stDayOfWeek, char* stTime, int stExp);
int fpcif_ValidDay(char *logip, char* stDay, char* stTime, int stExp);
int fpcif_ValidWeekofDay(char *logip, char* stWeek, char* stTime, int stExp);
int fpcif_SetBinToFile(char* temp, char* wr_file_path, int filesize);
int fpcif_ValidDate(char *logip, char* stDate, char* stTime, int stType, int stExp);
int fpcif_SetAgentByBbaseKey(_DAP_AGENT_INFO *p_AI);
int fpcif_SetLogLevel(char *cpip);
int fpcif_ChkAllowSchedule(char *logip, unsigned long long p_sq);
int fpcif_ChkAllowRule(char *logip, DETECT_ITEM DetectItem, _DAP_DETECT_DATA* pDetectData, cpRule *pCpRuleInfo);
int fpcif_ScheduleInit(dbSchd *pSchdInfo, int start, int end);
int fpcif_UpgradeInit(dbUgd *pUgdInfo, int start, int end);
//int fpcif_DetectInit(dbDetect *pDetectInfo, int start, int end);
int fpcif_RuleInit(dbRule *pRuleInfo, int start, int end);
//void fpcif_MainLoop(void);
void* fpcif_MainThread(void* param_ThreadArg);
int fpcif_ChkFindStr(char *dName, char *findStr);
int fpcif_ChkFindIp(char *dName, char *findIp);
void fpcif_ReloadCfgFile(void);
void* fpcif_PcifThread(void *);
int	fpcif_AgentRecv(
        int     thrNum,
        int		sock,
        char*	cpip,
        char*	msgType,
        int		msgCode,
        int		msgLeng,
        char*   Buffer
);
int	fpcif_MngRecv(
        int		sock,
        char*	cpip,
        int		fqSelf);

int	fpcif_PthreadMemoryReadLockCheck( int pre_sec );
void fpcif_PthreadMemoryWriteLockBegin(void);
void fpcif_PthreadMemoryWriteLockEnd(void);
/* ------------------------------------------------------------------- */
int fpcif_LoadRule(dbRule *pRuleInfo);
int fpcif_LoadDpPrintPort(dbRule *rule);

int fpcif_LoadScheduleSt(dbSchd *pSchdInfo);
int fpcif_LoadDetectSt(dbDetect *pDetectInfo);
int fpcif_LoadDetectProcess(int* param_RuleCnt,
                            dbDetect *pDetectInfo);
int fpcif_LoadDetectUrl(int* param_RuleCnt,
                        dbDetect *pDetectInfo);
int fpcif_LoadDetectVirtualMachine(int* param_RuleCnt,
                        dbDetect *pDetectInfo);
int fpcif_LoadUpgradeSt(dbUgd *pUgdInfo);
int fpcif_LoadGwSt(dbGw *pGwInfo);
int fpcif_LoadManager(dbManager *pMI);
int fpcif_LoadGroupLink(dbGroupLink *pGL);
int fpcif_LoadConfig(dbConfig *pConfigInfo);
//int fpcif_LoadDetectSel(unsigned long p_ruSq, dbDetectSel *pDS);
int fpcif_LoadDetectIdx(unsigned long p_ruSq, int maxRuleCnt, dbDetectSel *pDS);
int fpcif_UpdateConnManager(char *p_userId, int pid, int fqNum);
int fpcif_UpdateSockIpManager(char *p_id, char *p_ip);
int fpcif_UpdateTerminateManager(char *p_userId);
int fpcif_UpdateFailCountManager(char *p_userId, int cnt);
int fpcif_UpdateConfigChange(
        char*	p_debugName,	// process name
        char*	p_changeName);	// path of touch file;
int fpcif_CheckLoginManagerSt(
        char *p_userId,
        char *p_userPw,
        int *p_userLevel,
        char *p_userIp,
        int p_msgCode,
        int p_fqNum);
int fpcif_LoadBase(void);
int fpcif_LoadUser(dbUser *pUI);


/* ------------------------------------------------------------------- */

struct proxy_conn* fpcif_ProcAccept(int lsn_sock, int *error);
struct proxy_conn* fpcif_GetConnByEvptr(int *evptr);
struct proxy_conn* fpcif_AllocProxyConn(void);
void fpcif_ReleaseProxyConn(struct proxy_conn *conn,
                            struct epoll_event *pending_evs,
                            int pending_fds);
void fpcif_SetConnEpollFds(struct proxy_conn *conn, int epfd);
int fpcif_MakeJsonCmd(char *logip, int p_msgCode, int p_cmdCode, char** p_resJson);
void fpcif_SetJsonStr(json_t* jsonObject, char* ruleValue, const char* GetkeyName,const char* SetkeyName);
void fpcif_SetJsonInt(json_t* jsonObject, int ruleValue, const char* GetkeyName, const char* SetkeyName);
int  fpcif_MakeJsonForward(
        char	*logip,
        int		p_msgCode,
        int		*p_convMsgCode,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	*p_findIp,
        char	*p_findStr,
        char	**p_resJson);
int fpcif_MakeJsonTail(int p_serverId, char* p_fileName, char* p_content, char** p_resJson);
int fpcif_MakeJsonUrgentReq(_DAP_AGENT_INFO* p_AI, char *p_cpIp, char** p_resJson);
int fpcif_MakeJsonManagerInfo(char *logip, int p_userLevel, char** p_resJson, char* param_RecvManagerVersion);
int fpcif_MakeJsonEvent(char *logip, _DAP_EventParam *EP, char** p_resJson);
int fpcif_MakeJsonRequestCfg(_DAP_AGENT_INFO *p_AI, char *p_cpIp, char** p_resJson, int p_baseReq);
int fpcif_MakeJsonRequestCfgOld(_DAP_AGENT_INFO *p_AI, char** p_resJson, int p_baseReq);
int fpcif_MakeJsonServerLoglist(
        json_t*		root,
        char*		p_strMgwId,
        char*		p_dir,
        char*		p_strDate,
        int*		p_loop1,
        int*		p_loop2,
        char*		p_logip);
int fpcif_ParseReqLogin(
        int sock,
        json_t*	p_element,
        _DAP_MANAGER_INFO* p_ManagerInfo,
        char*	p_cpIp,
        int		p_msgCode,
        int		p_fqNum);
int fpcif_ParseReqAgentLog(char *logip, json_t *p_element, char *p_agentIp, char** p_resJson);
int fpcif_ParseReqServerTail(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerId,
        char**	resFile);
int fpcif_ParseServerLoglist(
        char	*logip,
        json_t 	*p_element,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	**resFindIp
);
int fpcif_ParseUrgentReq(int sock, char *p_cpIp, json_t *p_element, _DAP_AGENT_INFO* p_AgentInfo);
int fpcif_ParseRequestCfg(int sock,
                          char *p_cpIp,
                          json_t *p_element,
                          _DAP_AGENT_INFO* p_AgentInfo,
                          int thrNum );
int fpcif_ParseRequestAgentLog( char* param_CpIp,
                                json_t* param_Element,
                                _DAP_AGENTLOG_INFO* param_AgentLogInfo);
int fpcif_ParseServiceStatus(int sock,
                             char*              param_CpIp,
                             json_t*            param_Element,
                             _DAP_AGENT_INFO*   param_AgentInfo,
                             char*              param_ServiceStatus,
                             int thrNum);
int fpcif_ParseReqUploadFile(char *logip, int sock, json_t *p_element, char* p_managerId);
int	fpcif_ProcReqServerReport(
        int sock,
        char*	mngIp,
        char*	mngId,
        char*	sDate,
        char*	eDate);
int fpcif_ParseServerLogfile(
        char	*logip,
        json_t 	*p_element,
        char	*p_managerId,
        char	*p_beginDate,
        char	*p_endDate,
        char	*p_category,
        char	**resFindIp,
        char	**resFindStr
);
int fpcif_ParseNotifyReport(char *logip, json_t *p_element, _DAP_REPORT_INFO* p_ReportInfo);
int fpcif_ParseExternalRequestCfg(int sock, char *p_cpIp, json_t *p_element, _DAP_AGENT_INFO *p_AgentInfo);
int fpcif_ParseForwardServerLogtail(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerIp,
        int 	*p_managerFq,
        char**	resFile);
int fpcif_ParseForwardServerTerm(
        char* 	logip,
        json_t*	p_element,
        char* 	p_managerIp,
        char** 	resFile);
int fpcif_ParseJsonCmd(char *logip, int sock,  int p_msgCode);
int fpcif_ParseCollectServerTail(
        char*	logip,
        json_t*	p_element,
        int* 	p_serverId,
        char*	p_managerIp,
        int* 	p_managerFq,
        char*	p_fileName,
        char**	resData);
int	fpcif_MngLoginRecv(
        int		sock,
        char*	cpip,
        char*	msgType,
        int		msgCode,
        int		msgLeng,
        int     fqnum,
        char*	p_realip,
        char*	p_loginid,
        char*   Buffer
);
int fpcif_ParseReqShutdown(char *logip, json_t *p_element, char* p_managerId);
int fpcif_ParseMatchMac(char *logip, int sock, json_t* p_element);
void fpcif_ParseDuplicateCheck(_DAP_AGENT_INFO* p_AgentInfo, int idx);
int fpcif_GetIdxByUserSno(char *p_sno);
int fpcif_GetIdxByManagerIpIdSt(char *p_userIp, char* p_userId);
int fpcif_GetIdxByManagerSt(char *p_userId);
int fpcif_GetIdxByGroupLinkSt(int p_targetValue, int p_userGroup);
int fpcif_GetIdxByBaseIp(char *p_ip);
int fpcif_GetIdxByBaseKey(char *p_userKey, int ext);
int fpcif_GetIdxByScheduleSt(unsigned long long p_sq);
int fpcif_GetIdxByDetectSt(unsigned long long p_sq, int p_type);
int fpcif_GetIdxByConfigSt(char *p_cfName);
int fpcif_GetIdxByUserIp(char *p_userIp);
int fpcif_SendJsonToServer(int sock,  int code, char *jsonData, int jsonSize);
int fpcif_SendJsonToManager(int sock, int code, char *jsonData, int jsonSize);
int fpcif_SendJsonToAgent(int sock, char *type, int code, char *jsonData, int jsonSize);
int fpcif_SendNotiByFq(char *cpip, int fqn, char *logip);
int fpcif_SendToBinary(int sock, char *binData, int binSize);
int	fpcif_ProcReqAgentLog(
        int		sockMng,
        char*	agentIp,
        char*	mngIp,
        char*	p_msgType,
        int     p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize);

int	fpcif_ProcReqServerLog(
        int 	msgCode,
        int		sockMng,
        char*	mngIp,
        char*	sDate,
        char*	eDate,
        char*	tarHome,
        int		findCnt,
        json_t* ProcessListroot
);
int	fpcif_ProcForwardServerLog(
        int 	msgCode,
        int		sock,
        char*	cpip,
        char*	sDate,
        char*	eDate,
        char*	tarHome,
        int		findCnt,
        json_t* ProcessListroot);
int fpcif_SendToFw(int param_PackType,
                   char* param_UserKey,
                   char* param_Buffer,
                   int param_BufferSize);
int fpcif_UdpSendToFw(int param_PackType, char* param_UserKey, char* param_Buffer, int param_BufferSize);

int fpcif_SetTerminateByManager_st(char *p_userId);
int fpcif_CheckUrgentFile(char* AgentIp);
int fpcif_CheckCommandZip(char* logip);
int fpcif_GetDefaultMac(char *cpip, char *res);
int fpcif_GetIpChain(char *agentIp, char *res);

void* fpcif_ReqServerLogTail(void* arg);
void* fpcif_DistServerLogTail(void* arg);
int fpcif_ReqForwardLogTail(struct _STLOGTAIL* pstTail);
int fpcif_Readtail(TAIL *LTAIL, char *buf, size_t size, int usec, char* FlagFilePath);
TAIL* fpcif_OpenTail(char *fname);

SSL_CTX* fpcif_CertInit(void);
//int fpcif_OpensslNonBlockAccet(SSL* paramSsl, int localSocket);
int fpcif_OpensslNonBlockAccept(SSL* paramSsl, int localSocket);
int fpcif_OpensslAcceptNew(SSL* paramSsl, int localSocket);
void fpcif_OpensslLibLoadCertificates(SSL_CTX* ctx);
int fpcif_FileWriteInit(int param_ForkIndex);
int fpcif_GetLastFileIndex( char* param_FileName );
int fpcif_FileWriteWrLock(pthread_rwlock_t* param_Mutex_t);
int fpcif_FileWriteRdLock(pthread_rwlock_t* param_Mutex_t);
int fpcif_FileMutexLock(pthread_mutex_t* param_mutex_t);
int fpcif_GetLastFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize);
int fpcif_WriteDataFile(int param_ForkIdx, _DAP_QUEUE_BUF* param_Buffer, int param_BufferSize, FILE* param_Fp);
int fpcif_GetNextFileSize(int param_ForkIdx);

static unsigned char ENC_AESKEY[] =
        {
                0x31, 0xf7, 0xe5, 0xab, 0x43, 0xd2,
                0x43, 0x9f, 0x87, 0xf1, 0x13, 0x6f,
                0x14, 0xcd, 0x96, 0x5f,	0xff, 0x95,
                0x10, 0x7c, 0x6b, 0xc8, 0x4d, 0x8c,
                0x93, 0xbb, 0x0e, 0x88, 0xd8, 0xe8,
                0xe9, 0xf5
        };

static unsigned char ENC_IV[] =
        {
                0x32, 0xf8, 0xe6, 0xac, 0x44,
                0xd3, 0x44, 0xa0, 0x88, 0xf2,
                0x14, 0x70, 0x15, 0xce, 0x07, 0x60,
                0x11, 0x45, 0x35, 0xce, 0x17, 0x50,
                0x12, 0x46, 0x36, 0xc6, 0x18, 0x51,
                0x13, 0x42, 0x31, 0xc6
        };


#endif //PCIF_H
