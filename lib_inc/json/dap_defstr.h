//
// Created by KimByoungGook on 2020-07-01.
//

#ifndef DAP_DEFSTR_H
#define DAP_DEFSTR_H

// COMMON
#define     STR_SIZE                        "size"
#define     STR_SUMMARY                     "summary"
#define		STR_ADD							"add"
#define		STR_MOD							"mod"
#define		STR_DEL							"del"
#define		STR_BLACK						"black"
#define		STR_WHITE						"white"
#define		STR_ACCESS						"access"
#define		STR_BLOCK						"block"
// ROOT
#define		STR_AGENT_VER					"agent_ver"
#define		STR_CHANGE_ITEM					"change_item"
#define		STR_DETECT_TIME					"detect_time"
#define		STR_USER_KEY					"user_key"
#define		STR_USER_SEQ					"user_seq"
#define		STR_DATA						"data"
// MAIN_BOARD(1)
#define		STR_MAIN_BOARD					"main_board"
#define		STR_HB_MB_MF					"hb_mb_mf"
#define		STR_HB_MB_PN					"hb_mb_pn"
#define		STR_HB_MB_SN					"hb_mb_sn"
// SYSTEM(2)
#define		STR_SYSTEM						"system"
#define		STR_ST_BOOTUP_STATE				"st_bootup_state"
#define		STR_ST_DNS_HOST_NAME			"st_dns_host_name"
#define		STR_ST_DOMAIN					"st_domain"
#define		STR_ST_DOMAIN_ROLE				"st_domain_role"
#define		STR_ST_MEMORY					"st_memory"
#define		STR_ST_VGA						"st_vga"
#define		STR_ST_INSTALLED_VM				"st_installed_vm"
#define		STR_ST_NAME						"st_name"
#define		STR_ST_TIME_ZONE				"st_time_zone"
#define		STR_ST_VM_NAME					"st_vm_name"
#define		STR_ST_WAKEUP_TYPE				"st_wakeup_type"
#define		STR_ST_WORK_GROUP				"st_work_group"
// OPERATING_SYSTEM(3)
#define		STR_OPERATING_SYSTEM			"operating_system"
#define		STR_OS_ARCHITECTURE				"os_architecture"
#define		STR_OS_LANG						"os_lang"
#define		STR_OS_NAME						"os_name"
#define		STR_OS_PORTABLE					"os_portable"
#define		STR_OS_SP_MAJOR_VER				"os_sp_major_ver"
#define		STR_OS_SP_MINOR_VER				"os_sp_minor_ver"
#define		STR_OS_TYPE						"os_type"
#define		STR_OS_VERSION					"os_version"
// CPU(4)
#define		STR_CPU							"cpu"
#define		STR_CU_DESC						"cu_desc"
#define		STR_CU_MF						"cu_mf"
#define		STR_CU_NAME						"cu_name"
#define		STR_CU_PID						"cu_pid"
// NET_ADAPTER(5)
#define     STR_NET_ADAPTER                 "net_adapter"
#define     STR_NA_ALTE_DNS                 "na_alte_dns"
#define     STR_NA_DEFAULT_GW               "na_default_gw"
#define     STR_NA_DEFAULT_GW_MAC           "na_default_gw_mac"
#define     STR_NA_DESC                     "na_desc"
#define     STR_NA_DEVICE_TYPE              "na_device_type"
#define     STR_NA_IPV4                     "na_ipv4"
#define     STR_NA_IPV6                     "na_ipv6"
#define     STR_NA_MAC                      "na_mac"
#define     STR_NA_NAME                     "na_name"
#define     STR_NA_NET_CONNECTION_ID        "na_net_connection_id"
#define     STR_NA_NET_CONNECTION_STATUS    "na_net_connection_status"
#define     STR_NA_NET_ENABLED              "na_net_enabled"
#define     STR_NA_PHYSICAL_ADAPTER         "na_physical_adapter"
#define     STR_NA_PN                       "na_pn"
#define     STR_NA_PNP_DEVICE_ID            "na_pnp_device_id"
#define     STR_NA_PREF_DNS                 "na_pref_dns"
#define     STR_NA_SERVICE_NAME             "na_service_name"
#define     STR_NA_SUBNET                   "na_subnet"
#define     STR_PHYSICAL_NIC				"physical_nic"
// WIFI(6)
#define		STR_WIFI						"wifi"
#define		STR_WF_8021X					"wf_8021x"
#define		STR_WF_AUTH_ALGO				"wf_auth_algo"
#define		STR_WF_BSS_NETWORK_TYPE			"wf_bss_network_type"
#define		STR_WF_CIPHER_ALGO				"wf_cipher_algo"
#define		STR_WF_CONNECTION_MODE			"wf_connection_mode"
#define		STR_WF_INTERFACE_DESC			"wf_interface_desc"
#define		STR_WF_INTERFACE_STATUS			"wf_interface_status"
#define		STR_WF_MAC_ADDR					"wf_mac_addr"
#define		STR_WF_PHY_NETWORK_TYPE			"wf_phy_network_type"
#define		STR_WF_PROFILE_NAME				"wf_profile_name"
#define		STR_WF_SECURITY					"wf_security"
#define		STR_WF_SSID						"wf_ssid"
// BLUETOOTH(7)
#define		STR_BLUETOOTH					"bluetooth"
#define		STR_BT_AUTHENTICATED			"bt_authenticated"
#define		STR_BT_CONNECTED				"bt_connected"
#define		STR_BT_DANGER					"bt_danger"
#define		STR_BT_DEVICE					"bt_device"
#define		STR_BT_INSTANCE_NAME			"bt_instance_name"
#define		STR_BT_MAC_ADDR					"bt_mac_addr"
#define		STR_BT_MINOR_DEVICE				"bt_minor_device"
#define		STR_BT_REMEMBERED				"bt_remembered"
// NET_CONNECTION(8)
#define		STR_NET_CONNECTION				"net_connection"
#define		STR_NC_CONNECTION_STATE			"nc_connection_state"
#define		STR_NC_CONNECTION_TYPE			"nc_connection_type"
#define		STR_NC_DESC						"nc_desc"
#define		STR_NC_DISPLAY_TYPE				"nc_display_type"
#define		STR_NC_LOCAL_NAME				"nc_local_name"
#define		STR_NC_PROVIDER_NAME			"nc_provider_name"
#define		STR_NC_REMOTE_NAME				"nc_remote_name"
#define		STR_NC_REMOTE_PATH				"nc_remote_path"
#define		STR_NC_RESOURCE_TYPE			"nc_resource_type"
// DISK(9)
#define		STR_DISK						"disk"
#define		STR_DK_ACCESS					"dk_access"
#define		STR_DK_DESC						"dk_desc"
#define		STR_DK_DRIVE_TYPE				"dk_drive_type"
#define		STR_DK_FILE_SYSTEM				"dk_file_system"
#define		STR_DK_INTERFACE_TYPE			"dk_interface_type"
#define		STR_DK_MF						"dk_mf"
#define		STR_DK_MODEL					"dk_model"
#define		STR_DK_NAME						"dk_name"
#define		STR_DK_PHYSICAL_SN				"dk_physical_sn"
#define		STR_DK_VOLUME_NAME				"dk_volume_name"
#define		STR_DK_VOLUME_SN				"dk_volume_sn"
// NET_DRIVE(10)
#define		STR_NET_DRIVE					"net_drive"
#define		STR_ND_CONNECTION_TYPE			"nd_connection_type"
#define		STR_ND_DEFER_FLAGS				"nd_defer_flags"
#define		STR_ND_DRIVE_NAME				"nd_drive_name"
#define		STR_ND_PROVIDER_NAME			"nd_provider_name"
#define		STR_ND_PROVIDER_TYPE			"nd_provider_type"
#define		STR_ND_REMOTE_PATH				"nd_remote_path"
#define		STR_ND_USER_NAME				"nd_user_name"
// OS_ACCOUNT(11)
#define		STR_OS_ACCOUNT					"os_account"
#define		STR_OA_CAPTION					"oa_caption"
#define		STR_OA_DESC						"oa_desc"
#define		STR_OA_DISABLED					"oa_disabled"
#define		STR_OA_LOCAL					"oa_local"
#define		STR_OA_NAME						"oa_name"
#define		STR_OA_SID						"oa_sid"
#define		STR_OA_SID_TYPE					"oa_sid_type"
#define		STR_OA_STATUS					"oa_status"
#define		STR_OA_TYPE						"oa_type"
// SHARE_FOLDER(12)
#define		STR_SHARE_FOLDER				"share_folder"
#define		STR_SF_NAME						"sf_name"
#define		STR_SF_PATH						"sf_path"
#define		STR_SF_STATUS					"sf_status"
#define		STR_SF_TYPE						"sf_type"
// INFRARED_DEVICE(13)
#define		STR_INFRARED_DEVICE				"infrared_device"
#define		STR_ID_MF						"id_mf"
#define		STR_ID_NAME						"id_name"
#define		STR_ID_PROTOCOL_SUPPORTED		"id_protocol_supported"
#define		STR_ID_STATUS					"id_status"
#define		STR_ID_STATUS_INFO				"id_status_info"
// PROCESS(14)
#define		STR_PROCESS						"process"
#define		STR_PS_COMPANY_NAME				"ps_company_name"
#define		STR_PS_CONNECTED_SVR_ADDR		"ps_connected_svr_addr"
#define		STR_PS_COPY_RIGHT				"ps_copy_right"
#define		STR_PS_FILE_DESC				"ps_file_desc"
#define		STR_PS_FILE_NAME				"ps_file_name"
#define		STR_PS_FILE_PATH				"ps_file_path"
#define		STR_PS_FILE_VER					"ps_file_ver"
#define		STR_PS_ORIGINAL_FILE_NAME		"ps_original_file_name"
#define		STR_PS_RUNNING					"ps_running"
#define		STR_PS_TYPE						"ps_type"
// ROUTER(15)
#define		STR_ROUTER						"router"
#define		STR_RT_CAPTION					"rt_caption"
#define		STR_RT_DETECT_TYPE				"rt_detect_type"
#define		STR_RT_IPADDR					"rt_ipaddr"
#define		STR_RT_MAC_ADDR					"rt_mac_addr"
#define		STR_RT_WEB_TEXT					"rt_web_text"
// NET_PRINTER(16)
#define		STR_NET_PRINTER					"net_printer"
#define		STR_NP_CONNECTED				"np_connected"
#define		STR_NP_DISCORDANCE				"np_discordance"
#define		STR_NP_HOST_NAME				"np_host_name"
#define		STR_NP_OPEN_PORT				"np_open_port"
#define		STR_NP_PRINTER_PORT				"np_printer_port"
#define		STR_NP_WEB_CONNECT				"np_web_connect"
#define		STR_NP_WEB_TEXT					"np_web_text"
#define		STR_NP_WSD_LOCATION				"np_wsd_location"
#define		STR_NP_WSD_PRINTER_DEVICE		"np_wsd_printer_device"
// NET_SCAN(17)
#define		STR_NET_SCAN					"net_scan"
#define		STR_NS_DAP_AGENT				"ns_dap_agent"
#define		STR_NS_IP						"ns_ip"
#define		STR_NS_MAC						"ns_mac"
#define		STR_NS_MAC_MATCH				"ns_mac_match"
#define		STR_NS_OPEN_PORT				"ns_open_port"
#define		STR_NS_WEB_TEXT					"ns_web_text"
#define		STR_UNCHANGED					"unchanged"


// ARP(18)
// VIRTUAL_MACHINE(19)


// CONNECT_EXT_SVR(20)
#define		STR_CONNECT_EXT_SVR				"connect_ext_svr"
#define		STR_CE_CONNECTED				"ce_connected"
#define		STR_CE_URL						"ce_url"


// NET_ADAPTER_OVER(21)
// NET_ADAPTER_DUPIP(22)
// NET_ADAPTER_DUPMAC(23)
// DISK_REG(24)
// DISK_HIDDEN(25)
// DISK_NEW(26)
// DISK_MOBILE(27)
// DISK_MOBILE_READ(28)
// DISK_MOBILE_WRITE(29)
// PROCESS_WHITE(30)
// PROCESS_BLACK(31)
// PROCESS_ACCESSMON(32)
// NET_ADAPTER_MULIP(33)
// EXTERNAL_CONN(34)


// SSO_CERT (35)
#define		STR_SSO_CERT					"sso_cert"
#define		STR_SC_UNCERTIFIED				"sc_uncertified"
// WIN_DRV (36)
#define		STR_WIN_DRV						"win_drv"
#define		STR_DV_CLASS					"dv_class"
#define		STR_DV_CLASS_DESC				"dv_class_desc"
#define		STR_DV_DESC						"dv_desc"
#define		STR_DV_DRIVER					"dv_driver"
#define		STR_DV_ENUM						"dv_enum"
#define		STR_DV_FILE_COMPANY				"dv_file_company"
#define		STR_DV_FILE_COPY_RIGHT			"dv_file_copy_right"
#define		STR_DV_FILE_DESC				"dv_file_desc"
#define		STR_DV_FILE_PATH				"dv_file_path"
#define		STR_DV_FILE_PRODUCT				"dv_file_product"
#define		STR_DV_FILE_VER					"dv_file_ver"
#define		STR_DV_LOCATION					"dv_location"
#define		STR_DV_MFG						"dv_mfg"
#define		STR_DV_NAME						"dv_name"
#define		STR_DV_SERVICE					"dv_service"
#define		STR_DV_START					"dv_start"
#define		STR_DV_STATUS					"dv_status"
#define		STR_DV_TYPE						"dv_type"
#define		STR_DV_DATA_TYPE				"dv_data_type"
#define     STR_DV_CLASS_RESET              "dv_class_reset"
// RDP_SESSION (38)
#define		STR_RDP_SESSION					"rdp_session"
#define		STR_RDP_CLIENT_IP				"rdp_client_ip"
#define		STR_RDP_CLIENT_NAME				"rdp_client_name"
#define		STR_RDP_CONNECT_TIME			"rdp_connect_time"
#define		STR_RDP_USER_ID					"rdp_user_id"
// CPU_USAGE (39)
#define		STR_CPU_USAGE					"cpu_usage"
#define     STR_CPU_USAGE_RATE              "cpu_usage_rate"
#define     STR_CPU_USAGE_RATE_LIMIT        "cpu_usage_rate_limit"
#define     STR_CPU_USAGE_CONDITION         "cpu_usage_rate_condition"
#define     STR_CPU_USAGE_DETECT_TIME       "detect_time"
#define     STR_CPU_USAGE_DURATION_TIME     "duration_time"
#define     STR_CPU_USAGE_DURATION_TIME_CONDITION   "duration_time_condition"
#define     STR_CPU_USAGE_PROCESS_ID        "process_id"
#define     STR_CPU_USAGE_PROCESS_NAME      "process_name"
#define     STR_CPU_USAGE_STATUS            "status"
#define     STR_CPU_USAGE_STATUS_NAME       "status_name"
#define     STR_CPU_USAGE_TYPE              "type"
#define     STR_CPU_USAGE_TYPE_NAME         "type_name"
#define     STR_CPU_USAGE_IS_DAP            "is_dap_agent"


#endif //DAP_DEFSTR_H
