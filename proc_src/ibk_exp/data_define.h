#pragma once

enum EVENT_EQUIP_TYPE
{
    EVENT_TYPE_UNKNOWN = 0,
    EVENT_TYPE_MAIN_BOARD,
    EVENT_TYPE_SYSTEM,
    EVENT_TYPE_OPERATING_SYSTEM,
    EVENT_TYPE_CPU,
    EVENT_TYPE_NET_ADAPTER,
    EVENT_TYPE_WIFI,
    EVENT_TYPE_BLUETOOTH,
    EVENT_TYPE_NET_CONNECTION,
    EVENT_TYPE_DISK,
    EVENT_TYPE_NET_DRIVE,
    EVENT_TYPE_OS_ACCOUNT,
    EVENT_TYPE_SHARE_FOLDER,
    EVENT_TYPE_INFRARED_DEVICE,
    EVENT_TYPE_PROCESS,
    EVENT_TYPE_ROUTER,
    EVENT_TYPE_NET_PRINTER,
    EVENT_TYPE_NET_SCAN,
    EVENT_TYPE_ARP,
    EVENT_TYPE_VIRTUAL_MACHINE,
    EVENT_TYPE_CONNECT_EXT_NETWORK,
    EVENT_TYPE_NET_ADAPTER_OVER,
    EVENT_TYPE_NET_ADAPTER_DUP_IP,
    EVENT_TYPE_NET_ADAPTER_DUP_MAC,
    EVENT_TYPE_DISK_REG,
    EVENT_TYPE_DISK_HIDDEN,
    EVENT_TYPE_DISK_NEW,
    EVENT_TYPE_DISK_MOBILE,
    EVENT_TYPE_DISK_MOBILE_READ,
    EVENT_TYPE_DISK_MOBILE_WRITE,
    EVENT_TYPE_PROCESS_WHITE,
    EVENT_TYPE_PROCESS_BLACK,
    EVENT_TYPE_ACCESS_MON,
    EVENT_TYPE_NET_ADAPTER_MULTIPLE_IP,
    EVENT_TYPE_HW_BASE_CONNECT_EXT_DAP,
    EVENT_TYPE_SSO_CERT,
    EVENT_TYPE_WIN_DRV,
    EVENT_TYPE_PROCESS_BLOCK,
    EVENT_TYPE_RDP_SESSION,
    EVENT_TYPE_CPU_USAGE = 40,
    EVENT_TYPE_CPU_USAGE_ALARM = 41,
    EVENT_TYPE_CPU_USAGE_CONTROL = 42,
};

typedef struct
{
    unsigned int    ev_sq;
    unsigned int    hb_sq;
    unsigned int    us_sq;
    unsigned char   us_sno[32];
    unsigned short  ev_type;
    unsigned char   ev_ipaddr[16];
    unsigned char   ev_context[256];
    unsigned char   ev_detect_time[20];

}_IBK_EXP_EVENT_INFO;   /* 최근 발생 이벤트 정보 구조체 */

typedef struct
{
    unsigned int    hb_sq;
    unsigned int    us_sq;
    //unsigned char   us_ipaddr[16];
    //unsigned char   us_sno[32];
    unsigned char   rt_info[128];
    unsigned char   rt_ipaddr[16];
    unsigned char   rt_detect_time[20];

}_ROUTER_EVENT_DATA;    /* 검출된 공유기 정보 구조체 */

typedef struct
{
    unsigned char   dk_volume_sn[16+1];
    unsigned char   dk_physical_sn[64+1];
    unsigned char   dk_interface_type[32+1];
    unsigned short  dk_drive_type;
    unsigned char   dk_model[64+1];
    unsigned char   dk_summary[1024+1];
    unsigned char   dk_desc[128+1];
    unsigned char   dk_detect_time[20];

    // DK_VOLUME_SN, DK_PHYSICAL_SN, DK_INTERFACE_TYPE, DK_MODEL, DK_SUMMARY
    // 16, 64, 32, 64, 1024

}_DISK_DATA;

typedef struct
{
    unsigned char   wf_interface_desc[256+1];
    unsigned char   wf_mac_addr[32+1];
    unsigned char   wf_detect_time[20];

    // WF_INTERFACE_DESC, WF_MAC_ADDR, WF_DETECT_TIME

}_WIFI_EVENT_DATA;    /* 검출된 WIFI 정보 구조체 */

typedef struct
{
    unsigned char   bt_instance_name[128+1];
    unsigned char   bt_mac_addr[32+1];
    unsigned char   bt_minor_device[64+1];
    unsigned char   bt_detect_time[20];

    // BT_INSTANCE_NAME, BT_MAC_ADDR, BT_MINOR_DEVICE, BT_DETECT_TIME

}_BLUETOOTH_EVENT_DATA;    /* 검출된 Bluetooth 정보 구조체 */

typedef struct
{
    unsigned char   id_name[128+1];
    unsigned char   id_detect_time[20];

    // BT_INSTANCE_NAME, BT_MAC_ADDR, BT_MINOR_DEVICE, BT_DETECT_TIME

}_INFRARED_DEVICE_EVENT_DATA;    /* 검출된 적외선 장비 정보 구조체 */

typedef struct
{
    unsigned char   na_name[128+1];
    unsigned char   na_ip[512+1];
    unsigned char   na_mac[32+1];
    unsigned char   na_summary[1024+1];
    unsigned char   na_detect_time[20];

    // NA_NAME, NA_IPV4, NA_MAC, NA_DETECT_TIME
    // 128, 512, 32, 20

}_NET_ADAPTER_OVER_EVENT_DATA;    /* 검출된 다중 네트워크 아답터 정보 구조체 */

typedef struct
{
    unsigned int    mne_sq;
    unsigned char   mne_ipaddr[16];
    unsigned char   mn_id[32+1];
    unsigned short  mne_action_type;
    unsigned char   mne_record_time[20];

}_IBK_EXP_MANAGER_LOG_INFO;   /* 최근 발생 관리자 로그 정보 구조체 */