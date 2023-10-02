//
// Created by KimByoungGook on 2020-06-22.
//

#ifndef DAP_REQ_H
#define DAP_REQ_H

#define MAX_COMMON_COUNT		10
#define MAX_PORT_COUNT			30
#define MAX_NS_OPEN_PORT_COUNT	50
#define MAX_PROCESS_COUNT		100
#define MAX_NETSCAN_COUNT		254 //1~254
#define MAX_WINDRV_COUNT		10

//
// DAP PROTOCOL
//
#define		DAP_AGENT		"INTENT-DAP"
#define		DAP_MANAGE		"DAP-MANAGE"
#define		DAP_FORWARD		"DAP-FRWARD"

// MANAGE 로그 실시간 요청
#define		MANAGE_REQ_SERVER_LOGTAIL	71

#define		START_MANAGER_FQ		20
// MANAGE 로그인
#define		MANAGE_REQ_LOGIN		50



// MANAGE 타 AP서버로 전달
#define		MANAGE_FORWARD_SERVER_LOGTAIL	57
//vwlog/tailutil.c에서 TCP -> FQPut으로 변경하여 사용안함
#define		MANAGE_COLLECT_SERVER_LOGTAIL	58
#define		MANAGE_FORWARD_SERVER_TERM		59
#define		MANAGE_FORWARD_SHUTDOWN			65
// MANAGE 리턴 코드
#define     MANAGE_RTN_SUCCESS          91
#define     MANAGE_RTN_FAIL             92
#define		MANAGE_LOGIN_NOTFOUND_USER	93
#define		MANAGE_LOGIN_DUPLICATE_USER	94
#define		MANAGE_LOGIN_INVALID_IP		95
#define		MANAGE_LOGIN_INVALID_PASSWORD	96
#define		MANAGE_LOGIN_OVER_FAILCOUNT	97

#define     MANAGE_RSP_EVENT            98
#define		MANAGE_PING					99

//Agent 긴급패치 전문
#define     AGENT_URGENT_PATCH          119

// AGENT 리턴 코드
#define     DATACODE_RTN_SUCCESS    31      // AGENT 수신 성공
#define     DATACODE_RTN_FAIL       32      // AGENT 수신 실패
#define     DATACODE_RTN_DELETE		33      // AGENT 삭제(userkey제외)
#define     DATACODE_RTN_DELETE_ALL	34      // AGENT 삭제(모두삭제)
#define     DATACODE_RTN_RESTART	35      // AGENT 서비스재시작
#define     DATACODE_RTN_BASE_REQ	36      // AGENT main_board정보요청
// AGENT 로그파일 리스트  수신
#define		SERVER_CMD_LOG_LIST		13
#define		DATACODE_LOG_LIST		23
// AGENT 로그파일 수신
#define		SERVER_CMD_LOG_FILE		14
#define		DATACODE_LOG_FILE		24
// AGENT WIN 이벤트 로그파일 수신
#define		SERVER_CMD_WIN_LOGFILE	15
#define		DATACODE_WIN_LOGFILE	25



// MANAGE 접속 강제 끊기 요청
#define		MANAGE_FORCED_LOGIN		52
#define		MANAGE_FORCED_KILL		53
#define		MANAGE_FQ_KILL			54		// NOTI 전달 FQ

// MANAGE 다운
#define		MANAGE_RSP_TERM     	66
#define		MANAGE_REQ_SHUTDOWN		67
#define		MANAGE_NOTI_RESTART		68
#define		MANAGE_NOTI_SHUTDOWN	69
#define		MANAGE_RSP_LOGTAIL		72

// MANAGE 로그 실시간 요청
#define     MANAGE_REQ_AGENT_LOGTAIL    70
#define     MANAGE_REQ_SERVER_LOGTAIL   71
#define     MANAGE_RSP_LOGTAIL          72
#define     MANAGE_REQ_LOGTAIL_TERM 	73
// MANAGE 로그파일 리스트 요청
#define     MANAGE_REQ_AGENT_LOGLIST    74
#define     MANAGE_REQ_SERVER_LOGLIST   75
#define     MANAGE_RSP_LOGLIST          76
// MANAGE 로그파일 요청
#define     MANAGE_REQ_AGENT_LOGFILE    77
#define     MANAGE_REQ_SERVER_LOGFILE   78
#define     MANAGE_RSP_LOGFILE          79
#define     MANAGE_RSP_REPORTFILE       49
// MANAGE 리턴 코드
#define     MANAGE_RTN_SUCCESS          91
#define     MANAGE_RTN_FAIL             92
#define     MANAGE_RSP_EVENT            98
// MANAGE 로그인 계정
#define		MANAGE_REQ_ACCOUNT		    51

// MANAGE 버전정보
#define     MANAGE_REQ_VERSION          80

// WINDOWS 이벤트 로그파일 요청
#define		MANAGE_REQ_WIN_LOGFILE	64
// Agent update 파일 수신
#define		MANAGE_REQ_UPLOAD_FILE		90

// MANAGE 정책 알림
#define		MANAGE_NOTIFY_POLICY	60
// MANAGE 리포트 설정
#define		MANAGE_NOTIFY_REPORT	61
// MANAGE 환경 설정
#define		MANAGE_NOTIFY_PREFER	62
// MANAGE 관리자 설정
#define		MANAGE_NOTIFY_ACCOUNT_PREFER	63
// MANAGE 타 AP서버로 전달
#define		MANAGE_FORWARD_SERVER_LOGLIST	55
#define		MANAGE_FORWARD_SERVER_LOGFILE	56
#define		MANAGE_FORWARD_SERVER_LOGTAIL	57


// AGENT 정책 갱신
#define     DATACODE_REQUEST_TEST   0
#define     DATACODE_REQUEST_CFG    1

// AGENT 검출 데이터 전송
#define     DATACODE_DETECT_DATA    2
#define     EXTERNAL_DETECT_DATA    3
#define     EXTERNAL_REQUEST_CFG    4

// Agent Service Status 데이터 전송
#define     DATACODE_SERVICE_STATUS 5

// Agent Log 데이터 전송
#define     DATACODE_AGENTLOG_DATA  6

// AGENT 동일 네트워크 SCAN
#define     SERVER_CMD_ADDRESS_LIST 11
#define     DATACODE_ADDRESS_LIST   21
// AGENT 데이터 검출 명령
#define     SERVER_CMD_DATA_REFRESH 10
#define     DATACODE_DETECT_DATA_S  20

// RETRY EVENT ALARM
#define		RETRY_EVENT_ALARM			100
// SMS & MAIL ALARM
#define     EVENT_ALARM					101

// AGENT MAC주소 검사 명령
#define     SERVER_CMD_MATCH_MAC    12
#define     DATACODE_MATCH_MAC      22

/* Definition Data Structure */
/* ------------------------------------------------------------------- */
/* VWLOG Process */
/* ------------------------------------------------------------------- */
typedef	struct
{
    char	server_ip[15+1];
    char	manager_ip[15+1];
    int		manager_fq;
    char	tail_path[512];
}_DAP_TAIL_INFO;
/* ------------------------------------------------------------------- */

#pragma pack(push,2)
typedef	struct
{
    char	msgtype[10];
    int  	msgleng;
    short	msgcode;
    //char	*jsondata;
}CRhead;
#pragma pack(pop)

/* ------------------------------------------------------------------- */
/* Alarm Process */
/* ------------------------------------------------------------------- */
typedef struct
{
    unsigned char 		user_key[40]; // vwlog filename값이 들어가서 40byte로 잡음
    unsigned char 		user_ip[15+1];
    unsigned long long 	user_seq;
    unsigned char 		private_ip[15+1];
    unsigned char 		detect_time[19+1];
    int  				ev_type;
    int	 				ev_level;
    unsigned char 		ev_context[512];
    unsigned char 		prefix[6+1];
    unsigned char 		postfix[4+1];
    unsigned long long 	ru_sq;
} _DAP_EventParam;
/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */
/* Report Process */
/* ------------------------------------------------------------------- */
typedef	struct
{
    unsigned char	manager_id[20]; //ex) 'intent'
    unsigned char	begin_date[10+1]; //ex) '2018-10-15'
    unsigned char	end_date[10+1]; //ex) '2018-10-15'
    unsigned long long	group_sq;
    int				view_type;
    unsigned char	mail_type[10];		//ex) day:일간,week:주간,month:월간, today:당일
    unsigned char	mail_lang[2+1];		//ex) kr, en
    unsigned char	mail_from[30];      //ex) 'intent@intentsecure.com'
    unsigned char	mail_to[512+1];     //ex) 'intent@intentsecure.com,intent2@intentsecure.com'
}_DAP_REPORT_INFO;
/* ------------------------------------------------------------------- */


typedef	struct
{
    unsigned char		agent_ver[16+1];    //ex) 1.0.0.1
    unsigned char		user_key[20+1];     //ex) 18052811472488213494
    unsigned char		user_mac[128];      //ex) 90:9F:33:3B:21:C6 ','로구분
    unsigned char		user_ip[15+1];      //ex) 127.0.0.1
    unsigned char		user_sno[32+1];     //ex)
    unsigned long long	user_seq;
    unsigned char		sock_ip[15+1];      // NAT IP 즉, socket에서 가져온 IP
    unsigned char		access_time[19+1];  // HW_BASE_TB(hb_access_time) 업데이트하기위함
    unsigned int        hb_del;             /* HW_BASE 삭제유무 */
    unsigned int        hb_external;        /* 외부망 AGENT 유무 */
}_DAP_AGENT_INFO;

typedef struct
{
    unsigned char	forwarder_ver[16+1]; //ex) 1.0.0.1
    unsigned char	forwarder_id[32+1]; //ex) id
    unsigned char	forwarder_pw[255+1]; //ex) pw
}_DAP_FORWARDER_INFO;

typedef	struct
{
    unsigned char	manager_ver[16+1]; 	//ex) 1.0.0.1
    unsigned char	manager_id[32+1]; 	//ex) id
    unsigned char	manager_pw[255+1]; 	//ex) pw
    unsigned char	manager_mac[128];
    unsigned char	manager_ip[15+1]; 	//ex) 127.0.0.1
    unsigned char	sock_ip[15+1]; 		// NAT IP 즉, socket에서 가져온 IP
}_DAP_MANAGER_INFO;

typedef struct
{
    unsigned char   hb_sq[20 +1];
    unsigned char	user_key[24 +1];
    unsigned char	user_ip[15 +1];
    char            process[10 +1];
    char            log_date[20 +1];
    char            log_level[10 +1];
    char            log_msg[1023 +1];
}_DAP_AGENTLOG_INFO;


/* ------------------------------------------------------------------- */
/* Define DAP Detect Data */
/* ------------------------------------------------------------------- */
typedef	struct
{
    char	hb_mb_mf[64];
    char	hb_mb_pn[64];
    char	hb_mb_sn[64];
    //char	hb_summary[1024]; ����
}_DAP_MAINBOARD;

typedef struct
{
    char		st_bootup_state[64]; //ex) Fail-safe with network boot
    //int			st_connect_ext_svr; //ex) 포트번호 들어옴, 없으면: 0
    char		st_dns_host_name[64];
    char		st_domain[64];
    int			st_domain_role;
    int			st_memory;
    char		st_vga[32];
    int			st_installed_vm_size;
    char		st_installed_vm[MAX_COMMON_COUNT][32];
    char		st_name[64];
    int			st_time_zone;
    char		st_vm_name[64];
    int			st_wakeup_type;
    char		st_work_group[256];
    unsigned char	st_rule_vm;
    unsigned long long	st_rusq_vm;
    char		st_summary[1024]; //20190725 null값이 들어와 합의후 사용안하기로함
}_DAP_SYSTEM;

typedef struct
{
    int		size;

    struct	_ConnectExtValue
    {
        char	ce_url[64];
        int		ce_connected;
    } ConnectExtValue[MAX_COMMON_COUNT];

    char			ce_summary[1024];
    unsigned char	ce_rule;
    unsigned long long	ce_rusq;
}_DAP_CONNECT_EXT;

typedef struct
{
    char		os_architecture[16];
    int			os_lang;
    char		os_name[128];
    int			os_portable;
    int			os_sp_major_ver;
    int			os_sp_minor_ver;
    int			os_type;
    char		os_version[32];
    char		os_summary[1024];
}_DAP_OPERATING_SYSTEM;

typedef struct
{
    int		size;

    struct	_CpuValue
    {
        char	cu_desc[256];
        char	cu_mf[128];
        char	cu_name[128];
        char	cu_pid[32];
    } CpuValue[MAX_COMMON_COUNT];

    char 	cu_summary[1024];
}_DAP_CPU;

typedef struct
{
    int		size;
    int		physical_nic;
    struct	_NetAdapterValue
    {
        char	na_alte_dns[15+1];
        char	na_default_gw[256+1];
        char	na_default_gw_mac[256+1];
        char	na_desc[256+1]; //ex) Intel(R) Ethernet Connection (2) I219-V
        int 	na_device_type;
        char	na_ipv4[512+1];
        char	na_ipv6[640+1]; //ex) fe80::5938:f54d:316a:6ce4
        char	na_mac[32+1];
        char	na_name[128]; //ex) Intel(R) Ethernet Connection (2) I219-V
        char	na_net_connection_id[64]; //ex) Bluetooth 네트워크 연결
        int		na_net_connection_status;
        int 	na_net_enabled;
        int		na_physical_adapter;
        char	na_pn[128]; //ex) Intel(R) Ethernet Connection (2) I219-V
        char	na_pnp_device_id[128]; //ex) PCI\\VEN_8086&DEV_15B8&SUBSYS_15B81849&REV_00\\3&11583659&0&FE
        char	na_pref_dns[15+1];
        char	na_service_name[32]; //ex) e1iexpress
        char	na_subnet[15+1];
        //int		na_blocked;
    } NetAdapterValue[MAX_COMMON_COUNT];

    char 	na_summary[1024];
    unsigned char		na_rule;
    unsigned long long	na_rusq;
    unsigned char		na_over_rule;
    unsigned long long	na_over_rusq;
    unsigned char		na_dupip_rule;
    unsigned long long	na_dupip_rusq;
    unsigned char		na_dupmac_rule;
    unsigned long long	na_dupmac_rusq;
    unsigned char		na_mulip_rule;
    unsigned long long	na_mulip_rusq;
    unsigned char	    na_except_rule;

}_DAP_NETADAPTER;

typedef struct
{
    int		size;
    struct	_WifiValue
    {
        int		wf_8021x;
        unsigned int	wf_auth_algo;
        int		wf_bss_network_type;
        unsigned int 	wf_cipher_algo;
        int		wf_connection_mode;
        char	wf_interface_desc[256];
        int		wf_interface_status;
        char	wf_mac_addr[17+1];
        unsigned int	wf_phy_network_type;
        char	wf_profile_name[64];
        int		wf_security;
        char	wf_ssid[64];
        //int	wf_blocked;
    } WifiValue[MAX_COMMON_COUNT];
    char 	wf_summary[1024];
    unsigned char wf_rule;
    unsigned long long 	wf_rusq;
}_DAP_WIFI;

typedef struct
{
    int		size;
    struct	_BluetoothValue
    {
        char	bt_instance_name[128];
        char	bt_mac_addr[17+1];
        char	bt_device[128];
        char	bt_minor_device[64];
        int 	bt_danger;
        int		bt_connected;
        int		bt_authenticated;
        int		bt_remembered;
        //int		bt_blocked;
    } BluetoothValue[MAX_COMMON_COUNT];
    char 	bt_summary[1024];
    unsigned char	bt_rule;
    unsigned long long	bt_rusq;
}_DAP_BLUETOOTH;

typedef struct
{
    int		size;
    struct	_NetConnectionValue
    {
        char	nc_local_name[128];
        int		nc_connection_state;
        int		nc_connection_type;
        char	nc_desc[256];
        char	nc_provider_name[128];
        char	nc_remote_name[128];
        char	nc_remote_path[128];
        char	nc_display_type[16];
        char	nc_resource_type[16];
        //int		nc_blocked;
    } NetConnectionValue[MAX_COMMON_COUNT];
    char 	nc_summary[1024];
    unsigned char	nc_rule;
    unsigned long long	nc_rusq;
}_DAP_NETCONNECTION;

typedef struct
{
    int		size;
    struct	_DiskValue {
        char	dk_desc[256]; //ex) 로컬 고정 디스크
        int		dk_drive_type;
        char	dk_file_system[32];
        char	dk_interface_type[32];
        char	dk_mf[128]; //ex) (Standard disk drives)
        char	dk_model[64]; //ex) WDC WDS240G1G0A-00SS50
        char	dk_name[128];
        int		dk_access;
        char	dk_physical_sn[64]; //ex) 1729AS802564
        char	dk_volume_name[32];
        char	dk_volume_sn[16]; //ex) E602E411
        //int	dk_blocked;
    } DiskValue[MAX_COMMON_COUNT];
    char 	dk_summary[1024];
    unsigned char		dk_rule;
    unsigned long long 	dk_rusq;
    unsigned char		dk_reg_rule;
    unsigned long long 	dk_reg_rusq;
    unsigned char		dk_hidden_rule;
    unsigned long long 	dk_hidden_rusq;
    unsigned char		dk_new_rule;
    unsigned long long 	dk_new_rusq;
    unsigned char		dk_mobile_rule;
    unsigned long long 	dk_mobile_rusq;
    unsigned char		dk_mobile_read_rule;	//0:비활성,1:활성
    unsigned long long 	dk_mobile_read_rusq;
    unsigned char		dk_mobile_write_rule;	//0:비활성,1:활성
    unsigned long long 	dk_mobile_write_rusq;
}_DAP_DISK;

typedef struct
{
    int		size;
    struct	_NetDriveValue
    {
        char	nd_drive_name[128];
        char	nd_user_name[32];
        int		nd_connection_type;
        int		nd_defer_flags;
        char	nd_provider_name[128];
        int 	nd_provider_type;
        char	nd_remote_path[128];
        //int		nd_blocked;
    } NetDriveValue[MAX_COMMON_COUNT];
    char 	nd_summary[1024];
    unsigned char	nd_rule;
    unsigned long long 	nd_rusq;
}_DAP_NETDRIVE;

typedef struct
{
    int		size;
    struct	_OSAccountValue
    {
        char	oa_caption[64];
        char	oa_desc[256];
        int 	oa_disabled;
        int		oa_local;
        char	oa_name[64];
        char	oa_sid[64]; //ex) S-1-5-21-3351612032-1919284535-3612176967-500
        int		oa_sid_type;
        char	oa_status[32];
        int		oa_type;
        //int		oa_blocked;
    } OSAccountValue[MAX_COMMON_COUNT];
    char 	oa_summary[1024];
}_DAP_OSACCOUNT;

typedef struct
{
    int		size;
    struct	_ShareFolderValue
    {
        char	sf_name[128];
        char	sf_path[256];
        char	sf_status[10+1];
        int		sf_type;
        //int		sf_blocked;
    } ShareFolderValue[MAX_COMMON_COUNT];
    char 	sf_summary[1024];
    unsigned char	sf_rule;
    unsigned long long	sf_rusq;
}_DAP_SHARE_FOLDER;

typedef struct
{
    int		size;
    struct	_InfraredDeviceValue
    {
        char	id_name[128];
        char	id_mf[128];
        int		id_protocol_supported;
        char	id_status[10+1];
        int		id_status_info;
        //int		id_blocked;
    } InfraredDeviceValue[5];
    char 	id_summary[1024];
    unsigned char	id_rule;
    unsigned long long	id_rusq;
}_DAP_INFRARED_DEVICE; /* 적외선 */

typedef struct
{
    int		size;
    struct	_ProcessValue {
        char	ps_file_name[128];
        int		ps_type;
        char	ps_file_path[256];
        char	ps_original_file_name[128];
        char	ps_company_name[64];
        char	ps_file_desc[256];
        char	ps_file_ver[32];
        char	ps_copy_right[64];
        int		ps_running;
        int 	ps_connected_svr_addr_size;
        char 	ps_connected_svr_addr[MAX_COMMON_COUNT][15+1];
    } ProcessValue[MAX_PROCESS_COUNT];
    char 	ps_summary[4096];
    unsigned char		ps_white_rule;
    unsigned long long	ps_white_rusq;
    unsigned char		ps_black_rule;
    unsigned long long	ps_black_rusq;
    unsigned char		ps_accessmon_rule;
    unsigned long long	ps_accessmon_rusq;
    unsigned char		ps_detailinfo_rule;
    unsigned long long	ps_detailinfo_rusq;
}_DAP_PROCESS;

typedef struct
{
    int		size;
    struct	_RouterValue
    {
        int		rt_detect_type;
        char	rt_ipaddr[15+1];
        char	rt_mac_addr[17+1];
        char	rt_web_text[128];
        char	rt_caption[128];
        //int		rt_blocked;
    } RouterValue[MAX_COMMON_COUNT];
    char 	rt_summary[1024];
    unsigned char		rt_rule;
    unsigned long long	rt_rusq;
}_DAP_ROUTER;

typedef struct
{
    int		size;
    struct	_NetPrinterValue
    {
        int		np_connected;
        int		np_discordance;
        //int		np_lpr_protocol;
        char	np_host_name[128];
        int		np_wsd_printer_device;
        char	np_wsd_location[64];
        int		np_web_connect;
        char	np_web_text[128];
        int		np_open_port_size;
        int		np_open_port[MAX_PORT_COUNT];
        int		np_printer_port_size;
        int		np_printer_port[MAX_PORT_COUNT];
        //int		np_blocked;
    } NetPrinterValue[MAX_COMMON_COUNT];
    char 	np_summary[1024];
    unsigned char		np_rule;
    unsigned long long	np_rusq;
}_DAP_NETPRINTER;

typedef	struct
{
    int		size;
    int		unchanged; //0:변경된데이터(tb + history), 1:변경안됨(only tb)
    struct	_NetScanValue {
        int		ns_dap_agent;
        char	ns_ip[15+1];
        char	ns_mac[17+1];
        int		ns_mac_match;
        char	ns_web_text[128];
        int		ns_open_port_size;
        int		ns_open_port[MAX_NS_OPEN_PORT_COUNT];
    } NetScanValue[MAX_NETSCAN_COUNT];
    //unsigned char	ns_rule; //no use
}_DAP_NETSCAN;

typedef	struct
{
    int		size;
    struct	_ArpValue
    {
        int		bDAPAgent;
        char	strIP[15+1];
        char	strMAC[17+1];
        int		strOpenPortSize;
        int		strOpenPort[MAX_PORT_COUNT];
    } ArpValue[MAX_COMMON_COUNT];
    char 	ap_summary[1024];
}_DAP_ARP;

typedef struct
{
    int				sc_uncertified;
    unsigned char	sc_rule;
    unsigned char	sc_rusq;
}_DAP_SSO_CERT;

// WIN_DRV는 단순수집용으로 RULE이 없고 DB에만 기록
// 추후 추가될수도 있어 Event값 36으로 미리 정의함
typedef struct
{
    int		size;
    struct	_WinDrvValue
    {
        char	dv_class[15];
        char	dv_class_desc[128];
        char	dv_desc[128];
        char	dv_driver[128];
        char	dv_enum[50];
        char	dv_file_company[30];
        char	dv_file_copy_right[128];
        char	dv_file_desc[128];
        char	dv_file_path[128];
        char	dv_file_product[30];
        char	dv_file_ver[15];
        char	dv_location[128];
        char	dv_mfg[128];
        char	dv_name[128];
        char	dv_service[30];
        char	dv_start[15];
        char	dv_status[15];
        char	dv_type[30];
        int     dv_class_reset; // 0(Skip, 삭제 없이 처리) 또는 1(동일 Class 삭제)
        int		dv_data_type;   // 검출데이터 분류. 0:기존데이터 , 1:추가된 드라이버, 2:제거된 드라이버, 3:변경된 드라이버
    } WinDrvValue[MAX_WINDRV_COUNT];
}_DAP_WINDRV;
/* ------------------------------------------------------------------- */

typedef struct
{
    int		size;
    struct	_RdpSessionValue
    {
        char	rdp_client_ip[15+1];
        char	rdp_client_name[128+1];
        char	rdp_connect_time[19+1];
        char	rdp_user_id[30+1];
    } RdpSessionValue[MAX_COMMON_COUNT];
    char 			rdp_summary[1024];
    unsigned char	rdp_session_rule;
    unsigned char	rdp_session_rusq;
}_DAP_RDP_SESSION;

typedef struct
{
    int size;
    int historysize;
    struct	_CpuUsageValue
    {
        char cpu_usage_rate[20+1];
        char cpu_usage_rate_condition[4+1];
        char cpu_usage_rate_limit[4+1];
        char cpu_usage_detect_time[20+1];
        char cpu_usage_duration_time[20+1];
        char cpu_usage_duration_time_condition[4+1];
        char cpu_usage_status[3+1];
        char cpu_usage_status_name[30+1];
        char cpu_usage_type[3+1];
        char cpu_usage_process_name[30+1];
        char cpu_usage_process_id[5+1];
        int  is_dap_agent;
    } CpuUsageValue[MAX_PROCESS_COUNT];

    struct	_CpuHistoryValue {
        char cpu_usage_rate[20+1];
        char cpu_usage_rate_condition[4+1];
        char cpu_usage_rate_limit[4+1];
        char cpu_usage_detect_time[20+1];
        char cpu_usage_duration_time[20+1];
        char cpu_usage_duration_time_condition[4+1];
        char cpu_usage_status[3+1];
        char cpu_usage_status_name[30+1];
        char cpu_usage_type[3+1];
        char cpu_usage_type_name[30+1];
        char cpu_usage_process_name[30+1];
        char cpu_usage_process_id[5+1];
        int  is_dap_agent;
    } CpuHistoryValue[MAX_PROCESS_COUNT];

    unsigned char		cpu_alarm_rule;
    unsigned long long	cpu_alarm_rusq;
    unsigned char		cpu_ctrl_rule;
    unsigned long long	cpu_ctrl_rusq;
//    char cpu_usage_summary[1024+1];
}_DAP_CPU_USAGE;

typedef struct
{
    _DAP_AGENT_INFO 	AgentInfo;
    char				detect_time[19+1];
    char				change_item[256];
} _DAP_DETECT;


typedef struct
{
    _DAP_SYSTEM System;
    _DAP_CONNECT_EXT ConnectExt;
    _DAP_OPERATING_SYSTEM OperatingSystem;
    _DAP_CPU Cpu;
    _DAP_NETADAPTER NetAdapter; //na_rule
    _DAP_WIFI Wifi; //wf_rule
    _DAP_BLUETOOTH Bluetooth; //bt_rule
    _DAP_NETCONNECTION NetConnection;
    _DAP_DISK Disk; //dk_rule
    _DAP_NETDRIVE NetDrive; //nd_rule
    _DAP_OSACCOUNT OSAccount;
    _DAP_SHARE_FOLDER ShareFolder; //sf_rule
    _DAP_INFRARED_DEVICE InfraredDevice; //id_rule
    _DAP_PROCESS Process; //ps_rule
    _DAP_ROUTER Router; //rt_rule
    _DAP_NETPRINTER NetPrinter; //np_rule
    _DAP_NETSCAN NetScan; //ns_rule
    //_Arp				Arp;
    _DAP_SSO_CERT SsoCert;
    _DAP_WINDRV WinDrv;
    _DAP_MAINBOARD MainBoard;
    _DAP_RDP_SESSION RdpSession;
    _DAP_CPU_USAGE   CpuUsage;
}_DAP_DETECT_DATA;


#endif //DAP_REQ_H
