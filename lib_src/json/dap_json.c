//
// Created by KimByoungGook on 2020-06-26.
//
#include <stdio.h>
#include <string.h>
#include <openssl/des.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "json/dap_json.h"
#include "json/jansson.h"
#include "json/jansson_config.h"
#include "com/dap_req.h"
#include "com/dap_com.h"

void fjson_PrintAllAgentInfo(_DAP_AGENT_INFO *p_AgentInfo)
{
    WRITE_INFO(CATEGORY_INFO,"* [agent_info] ");
    WRITE_INFO(CATEGORY_INFO,"** agent_ver=[%s]", p_AgentInfo->agent_ver);
    WRITE_INFO(CATEGORY_INFO,"** user_key=[%s]", p_AgentInfo->user_key);
    WRITE_INFO(CATEGORY_INFO,"** user_mac=[%s]", p_AgentInfo->user_mac);
    WRITE_INFO(CATEGORY_INFO,"** user_ip=[%s]", p_AgentInfo->user_ip);
    WRITE_INFO(CATEGORY_INFO,"** user_sno=[%s]", p_AgentInfo->user_sno);
    WRITE_INFO(CATEGORY_INFO,"** user_seq=[%llu]", p_AgentInfo->user_seq);
    WRITE_INFO(CATEGORY_INFO,"** sock_ip=[%s]", p_AgentInfo->sock_ip);
    WRITE_INFO(CATEGORY_INFO,"** access_time=[%s]", p_AgentInfo->access_time);
    return;
}

void fjson_PrintAllManagerInfo(_DAP_MANAGER_INFO* p_ManagerInfo)
{
    WRITE_INFO(CATEGORY_INFO,"* [manager_info]");
    WRITE_INFO(CATEGORY_INFO,"** manager_ver=[%s]", p_ManagerInfo->manager_ver);
    WRITE_INFO(CATEGORY_INFO,"** manager_id=[%s]", p_ManagerInfo->manager_id);
    WRITE_INFO(CATEGORY_INFO,"** manager_pw=[%s]", p_ManagerInfo->manager_pw);
    WRITE_INFO(CATEGORY_INFO,"** manager_mac=[%s]", p_ManagerInfo->manager_mac);
    WRITE_INFO(CATEGORY_INFO,"** manager_ip=[%s]", p_ManagerInfo->manager_ip);
    WRITE_INFO(CATEGORY_INFO,"** sock_ip=[%s]", p_ManagerInfo->sock_ip);
    return;
}

void fjson_PrintAllMainBoard(_DAP_MAINBOARD *p_MainBoard)
{
    WRITE_INFO(CATEGORY_INFO,"* [main_board]");
    WRITE_INFO(CATEGORY_INFO,"** hb_mb_mf=[%s]", p_MainBoard->hb_mb_mf);
    WRITE_INFO(CATEGORY_INFO,"** hb_mb_pn=[%s]", p_MainBoard->hb_mb_pn);
    WRITE_INFO(CATEGORY_INFO,"** hb_mb_sn=[%s]", p_MainBoard->hb_mb_sn);
    return;
}

void fjson_PrintAllSystem(_DAP_SYSTEM *p_System)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [system]");
    WRITE_INFO(CATEGORY_INFO,"** st_bootup_state=[%s]", p_System->st_bootup_state);
    WRITE_INFO(CATEGORY_INFO,"** st_dns_host_name=[%s]", p_System->st_dns_host_name);
    WRITE_INFO(CATEGORY_INFO,"** st_domain=[%s]", p_System->st_domain);
    WRITE_INFO(CATEGORY_INFO,"** st_domain_role=[%d]", p_System->st_domain_role);
    WRITE_INFO(CATEGORY_INFO,"** st_memory=[%d]", p_System->st_memory);
    WRITE_INFO(CATEGORY_INFO,"** st_vga=[%s]", p_System->st_vga);
    WRITE_INFO(CATEGORY_INFO,"** st_installed_vm_size=[%d]", p_System->st_installed_vm_size);
    for(i=0; i<p_System->st_installed_vm_size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** st_installed_vm[%d]=[%s]", i, p_System->st_installed_vm[i]);
    }
    WRITE_INFO(CATEGORY_INFO,"** st_name=[%s]", p_System->st_name);
    WRITE_INFO(CATEGORY_INFO,"** st_time_zone=[%d]", p_System->st_time_zone);
    WRITE_INFO(CATEGORY_INFO,"** st_vm_name=[%s]", p_System->st_vm_name);
    WRITE_INFO(CATEGORY_INFO,"** st_wakeup_type=[%d]", p_System->st_wakeup_type);
    WRITE_INFO(CATEGORY_INFO,"** st_work_group=[%s]", p_System->st_work_group);
    WRITE_INFO(CATEGORY_INFO,"** st_rule_vm=[%c][%llu]", p_System->st_rule_vm,p_System->st_rusq_vm);
    WRITE_INFO(CATEGORY_INFO,"** st_summary=[%s]", p_System->st_summary);
    return;
}

void fjson_PrintAllConnectExt(_DAP_CONNECT_EXT *p_ConnectExt)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [connect_ext_svr],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_ConnectExt->size);
    for(i=0; i<p_ConnectExt->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ce_url=[%s],", i, (char *)p_ConnectExt->ConnectExtValue[i].ce_url);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ce_connected=[%d],", i, p_ConnectExt->ConnectExtValue[i].ce_connected);
    }
    WRITE_INFO(CATEGORY_INFO,"** ce_summary=[%s],", p_ConnectExt->ce_summary);
    WRITE_INFO(CATEGORY_INFO,"** ce_rule=[%c][%llu],", p_ConnectExt->ce_rule,p_ConnectExt->ce_rusq);
    return;
}
void fjson_PrintAllOperatingSystem(_DAP_OPERATING_SYSTEM* p_OperatingSystem)
{
    WRITE_INFO(CATEGORY_INFO,"* [operating_system]");
    WRITE_INFO(CATEGORY_INFO,"** os_architecture=[%s]", p_OperatingSystem->os_architecture);
    WRITE_INFO(CATEGORY_INFO,"** os_lang=[%d]", p_OperatingSystem->os_lang);
    WRITE_INFO(CATEGORY_INFO,"** os_name=[%s]", p_OperatingSystem->os_name);
    WRITE_INFO(CATEGORY_INFO,"** os_portable=[%d]", p_OperatingSystem->os_portable);
    WRITE_INFO(CATEGORY_INFO,"** os_sp_major_ver=[%d]", p_OperatingSystem->os_sp_major_ver);
    WRITE_INFO(CATEGORY_INFO,"** os_sp_minor_ver=[%d]", p_OperatingSystem->os_sp_minor_ver);
    WRITE_INFO(CATEGORY_INFO,"** os_type=[%d]", p_OperatingSystem->os_type);
    WRITE_INFO(CATEGORY_INFO,"** os_version=[%s]", p_OperatingSystem->os_version);
    WRITE_INFO(CATEGORY_INFO,"** os_summary=[%s]", p_OperatingSystem->os_summary);
    return;
}

void fjson_PrintAllCpu(_DAP_CPU *p_Cpu)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [cpu]");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d]", p_Cpu->size);
    for(i=0; i<p_Cpu->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cu_desc=[%s],", i, (char *)p_Cpu->CpuValue[i].cu_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cu_mf=[%s],", i, (char *)p_Cpu->CpuValue[i].cu_mf);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cu_name=[%s],", i, (char *)p_Cpu->CpuValue[i].cu_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cu_pid=[%s],", i, (char *)p_Cpu->CpuValue[i].cu_pid);
    }
    WRITE_INFO(CATEGORY_INFO,"** cu_summary=[%s]", p_Cpu->cu_summary);
    return;
}

void fjson_PrintAllNetAdapter(_DAP_NETADAPTER *p_NetAdapter)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [net_adapter]");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d]", p_NetAdapter->size);
    WRITE_INFO(CATEGORY_INFO,"** physical_nic=[%d]", p_NetAdapter->physical_nic);
    for(i=0; i<p_NetAdapter->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_alte_dns=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_alte_dns);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_default_gw=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_default_gw);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_default_gw_mac=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_default_gw_mac);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_desc=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_device_type=[%d]", i, p_NetAdapter->NetAdapterValue[i].na_device_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_ipv4=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_ipv4);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_ipv6=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_ipv6);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_mac=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_mac);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_name=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_net_connection_id=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_net_connection_id);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_net_connection_status=[%d]", i, p_NetAdapter->NetAdapterValue[i].na_net_connection_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_net_enabled=[%d]", i, p_NetAdapter->NetAdapterValue[i].na_net_enabled);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_physical_adapter=[%d]", i, p_NetAdapter->NetAdapterValue[i].na_physical_adapter);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_pn=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_pn);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_pnp_device_id=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_pnp_device_id);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_pref_dns=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_pref_dns);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_service_name=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_service_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] na_subnet=[%s]", i, p_NetAdapter->NetAdapterValue[i].na_subnet);
    }
    WRITE_INFO(CATEGORY_INFO,"** na_summary=[%s]", p_NetAdapter->na_summary);
    WRITE_INFO(CATEGORY_INFO,"** na_rule=[%c][%llu]", p_NetAdapter->na_rule,p_NetAdapter->na_rusq);
    WRITE_INFO(CATEGORY_INFO,"** na_over_rule=[%c][%llu]", p_NetAdapter->na_over_rule,p_NetAdapter->na_over_rusq);
    WRITE_INFO(CATEGORY_INFO,"** na_dupip_rule=[%c][%llu]", p_NetAdapter->na_dupip_rule,p_NetAdapter->na_dupip_rusq);
    WRITE_INFO(CATEGORY_INFO,"** na_dupmac_rule=[%c][%llu]", p_NetAdapter->na_dupmac_rule,p_NetAdapter->na_dupmac_rusq);
    WRITE_INFO(CATEGORY_INFO,"** na_mulip_rule=[%c][%llu]", p_NetAdapter->na_mulip_rule,p_NetAdapter->na_mulip_rusq);
    //WRITE_INFO(CATEGORY_INFO,"** na_except_rule=[%c],", p_NetAdapter->na_except_rule);
    return;
}

void fjson_PrintAllWifi(_DAP_WIFI *p_Wifi)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [wifi],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_Wifi->size);
    for(i=0; i<p_Wifi->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_8021x=[%d],", i, p_Wifi->WifiValue[i].wf_8021x);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_auth_algo=[%u],", i, p_Wifi->WifiValue[i].wf_auth_algo);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_bss_network_type=[%d],", i, p_Wifi->WifiValue[i].wf_bss_network_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_cipher_algo=[%u],", i, p_Wifi->WifiValue[i].wf_cipher_algo);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_connection_mode=[%d],", i, p_Wifi->WifiValue[i].wf_connection_mode);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_interface_desc=[%s],", i, p_Wifi->WifiValue[i].wf_interface_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_interface_status=[%d],", i, p_Wifi->WifiValue[i].wf_interface_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_mac_addr=[%s],", i, p_Wifi->WifiValue[i].wf_mac_addr);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_phy_network_type=[%u],", i, p_Wifi->WifiValue[i].wf_phy_network_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_profile_name=[%s],", i, p_Wifi->WifiValue[i].wf_profile_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_security=[%d],", i, p_Wifi->WifiValue[i].wf_security);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] wf_ssid=[%s],", i, p_Wifi->WifiValue[i].wf_ssid);
    }
    WRITE_INFO(CATEGORY_INFO,"** wf_summary=[%s],", p_Wifi->wf_summary);
    WRITE_INFO(CATEGORY_INFO,"** wf_rule=[%c][%llu],", p_Wifi->wf_rule,p_Wifi->wf_rusq);
    return;
}

void fjson_PrintAllBluetooth(_DAP_BLUETOOTH *p_Bluetooth)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [bluetooth]");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d]", p_Bluetooth->size);
    for(i=0; i<p_Bluetooth->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_instance_name=[%s]", i, p_Bluetooth->BluetoothValue[i].bt_instance_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_mac_addr=[%s]", i, p_Bluetooth->BluetoothValue[i].bt_mac_addr);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_device=[%s]", i, p_Bluetooth->BluetoothValue[i].bt_device);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_minor_device=[%s]", i, p_Bluetooth->BluetoothValue[i].bt_minor_device);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_danger=[%d]", i, p_Bluetooth->BluetoothValue[i].bt_danger);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_connected=[%d]", i, p_Bluetooth->BluetoothValue[i].bt_connected);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_authenticated=[%d]", i, p_Bluetooth->BluetoothValue[i].bt_authenticated);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] bt_remembered=[%d]", i, p_Bluetooth->BluetoothValue[i].bt_remembered);
    }
    WRITE_INFO(CATEGORY_INFO,"** bt_summary=[%s] ", p_Bluetooth->bt_summary);
    WRITE_INFO(CATEGORY_INFO,"** bt_rule=[%c][%llu]", p_Bluetooth->bt_rule,p_Bluetooth->bt_rusq);
    return;
}

void fjson_PrintAllNetConnection(_DAP_NETCONNECTION *p_NetConnection)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [net_connection]");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d]", p_NetConnection->size);
    for(i=0; i<p_NetConnection->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_local_name=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_local_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_connection_state=[%d]", i, p_NetConnection->NetConnectionValue[i].nc_connection_state);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_connection_type=[%d]", i, p_NetConnection->NetConnectionValue[i].nc_connection_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_desc=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_provider_name=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_provider_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_remote_name=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_remote_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_remote_path=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_remote_path);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_display_type=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_display_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nc_resource_type=[%s]", i, p_NetConnection->NetConnectionValue[i].nc_resource_type);
    }
    WRITE_INFO(CATEGORY_INFO,"** nc_summary=[%s],", p_NetConnection->nc_summary);
    WRITE_INFO(CATEGORY_INFO,"** nc_rule=[%c][%llu],", p_NetConnection->nc_rule,p_NetConnection->nc_rusq);
    return;
}

void fjson_PrintAllDisk(_DAP_DISK* p_Disk)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [disk],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_Disk->size);
    for(i=0; i<p_Disk->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_desc=[%s],", i, p_Disk->DiskValue[i].dk_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_drive_type=[%d],", i, p_Disk->DiskValue[i].dk_drive_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_file_system=[%s],", i, p_Disk->DiskValue[i].dk_file_system);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_interface_type=[%s],", i, p_Disk->DiskValue[i].dk_interface_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_mf=[%s],", i, p_Disk->DiskValue[i].dk_mf);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_model=[%s],", i, p_Disk->DiskValue[i].dk_model);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_name=[%s],", i, p_Disk->DiskValue[i].dk_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_access=[%d],", i, p_Disk->DiskValue[i].dk_access);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_physical_sn=[%s],", i, p_Disk->DiskValue[i].dk_physical_sn);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_volume_name=[%s],", i, p_Disk->DiskValue[i].dk_volume_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dk_volume_sn=[%s],", i, p_Disk->DiskValue[i].dk_volume_sn);
    }
    WRITE_INFO(CATEGORY_INFO,"** dk_summary=[%s],", p_Disk->dk_summary);
    WRITE_INFO(CATEGORY_INFO,"** dk_rule=[%c][%llu],", p_Disk->dk_rule,p_Disk->dk_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_reg_rule=[%c][%llu],", p_Disk->dk_reg_rule,p_Disk->dk_reg_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_hidden_rule=[%c][%llu],", p_Disk->dk_hidden_rule,p_Disk->dk_hidden_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_new_rule=[%c][%llu],", p_Disk->dk_new_rule,p_Disk->dk_new_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_mobile_rule=[%c][%llu],", p_Disk->dk_mobile_rule,p_Disk->dk_mobile_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_mobile_read_rule=[%c][%llu],", p_Disk->dk_mobile_read_rule,p_Disk->dk_mobile_read_rusq);
    WRITE_INFO(CATEGORY_INFO,"** dk_mobile_write_rule=[%c][%llu],", p_Disk->dk_mobile_write_rule,p_Disk->dk_mobile_write_rusq);
    return;
}

void fjson_PrintAllNetDrive(_DAP_NETDRIVE *p_NetDrive)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [net_drive],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_NetDrive->size);
    for(i=0; i<p_NetDrive->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_drive_name=[%s],", i, p_NetDrive->NetDriveValue[i].nd_drive_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_user_name=[%s],", i, p_NetDrive->NetDriveValue[i].nd_user_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_connection_type=[%d],", i, p_NetDrive->NetDriveValue[i].nd_connection_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_defer_flags=[%d],", i, p_NetDrive->NetDriveValue[i].nd_defer_flags);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_provider_name=[%s],", i, p_NetDrive->NetDriveValue[i].nd_provider_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_provider_type=[%d],", i, p_NetDrive->NetDriveValue[i].nd_provider_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] nd_remote_path=[%s],", i, p_NetDrive->NetDriveValue[i].nd_remote_path);
    }
    WRITE_INFO(CATEGORY_INFO,"** nd_summary=[%s],", p_NetDrive->nd_summary);
    WRITE_INFO(CATEGORY_INFO,"** nd_rule=[%c][%llu],", p_NetDrive->nd_rule,p_NetDrive->nd_rusq);
    return;
}

void fjson_PrintAllOsAccount(_DAP_OSACCOUNT *p_OA)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [os_account],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_OA->size);
    for(i=0; i<p_OA->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_caption=[%s],", i, p_OA->OSAccountValue[i].oa_caption);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_desc=[%s],", i, p_OA->OSAccountValue[i].oa_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_disabled=[%d],", i, p_OA->OSAccountValue[i].oa_disabled);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_local=[%d],", i, p_OA->OSAccountValue[i].oa_local);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_name=[%s],", i, p_OA->OSAccountValue[i].oa_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_sid=[%s],", i, p_OA->OSAccountValue[i].oa_sid);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_sid_type=[%d],", i, p_OA->OSAccountValue[i].oa_sid_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_status=[%s],", i, p_OA->OSAccountValue[i].oa_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] oa_type=[%d],", i, p_OA->OSAccountValue[i].oa_type);
    }
    WRITE_INFO(CATEGORY_INFO,"** oa_summary=[%s],", p_OA->oa_summary);
    return;
}

void fjson_PrintAllShareFolder(_DAP_SHARE_FOLDER *p_ShareFolder)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [share_folder],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_ShareFolder->size);
    for(i=0; i<p_ShareFolder->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] sf_name=[%s],", i, p_ShareFolder->ShareFolderValue[i].sf_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] sf_path=[%s],", i, p_ShareFolder->ShareFolderValue[i].sf_path);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] sf_status=[%s],", i, p_ShareFolder->ShareFolderValue[i].sf_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] sf_type=[%u],", i, p_ShareFolder->ShareFolderValue[i].sf_type);
    }
    WRITE_INFO(CATEGORY_INFO,"** sf_summary=[%s],", p_ShareFolder->sf_summary);
    WRITE_INFO(CATEGORY_INFO,"** sf_rule=[%c][%llu],", p_ShareFolder->sf_rule,p_ShareFolder->sf_rusq);
    return;
}

void fjson_PrintAllInfraredDevice(_DAP_INFRARED_DEVICE *p_InfraredDevice)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [infrared_device],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_InfraredDevice->size);
    for(i=0; i<p_InfraredDevice->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] id_name=[%s],", i, p_InfraredDevice->InfraredDeviceValue[i].id_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] id_mf=[%s],", i, p_InfraredDevice->InfraredDeviceValue[i].id_mf);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] id_protocol_supported=[%d],", i, p_InfraredDevice->InfraredDeviceValue[i].id_protocol_supported);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] id_status=[%s],", i, p_InfraredDevice->InfraredDeviceValue[i].id_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] id_status_info=[%d],", i, p_InfraredDevice->InfraredDeviceValue[i].id_status_info);
    }
    WRITE_INFO(CATEGORY_INFO,"** id_summary=[%s],", p_InfraredDevice->id_summary);
    WRITE_INFO(CATEGORY_INFO,"** id_rule=[%c][%llu],", p_InfraredDevice->id_rule,p_InfraredDevice->id_rusq);
    return;
}

void fjson_PrintAllProcess(_DAP_PROCESS *p_Process)
{
    int i, j;
    WRITE_INFO(CATEGORY_INFO,"* [process],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_Process->size);
    for(i=0; i<p_Process->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_file_name=[%s],", i, p_Process->ProcessValue[i].ps_file_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_type=[%d],", i, p_Process->ProcessValue[i].ps_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_file_path=[%s],", i, p_Process->ProcessValue[i].ps_file_path);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_original_file_name=[%s],", i, p_Process->ProcessValue[i].ps_original_file_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_company_name=[%s],", i, p_Process->ProcessValue[i].ps_company_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_file_desc=[%s],", i, p_Process->ProcessValue[i].ps_file_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_file_ver=[%s],", i, p_Process->ProcessValue[i].ps_file_ver);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_copy_right=[%s],", i, p_Process->ProcessValue[i].ps_copy_right);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_running=[%d],", i, p_Process->ProcessValue[i].ps_running);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ps_connected_svr_addr_size=[%d],", i, p_Process->ProcessValue[i].ps_connected_svr_addr_size);
        for(j=0; j<p_Process->ProcessValue[i].ps_connected_svr_addr_size; j++)
        {
            WRITE_INFO(CATEGORY_INFO,"**** idx=[%d] ps_connected_svr_addr=[%s],", j, p_Process->ProcessValue[i].ps_connected_svr_addr[j]);
        }
    }
    WRITE_INFO(CATEGORY_INFO,"** ps_summary=[%s],", p_Process->ps_summary);
    WRITE_INFO(CATEGORY_INFO,"** ps_white_rule=[%c][%llu],", p_Process->ps_white_rule,p_Process->ps_white_rusq);
    WRITE_INFO(CATEGORY_INFO,"** ps_black_rule=[%c][%llu],", p_Process->ps_black_rule,p_Process->ps_black_rusq);
    WRITE_INFO(CATEGORY_INFO,"** ps_accessmon_rule=[%c][%llu],", p_Process->ps_accessmon_rule,p_Process->ps_accessmon_rusq);
    return;
}

void fjson_PrintAllRouter(_DAP_ROUTER *p_Router)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [router],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_Router->size);
    for(i=0; i<p_Router->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rt_detect_type=[%d],", i, p_Router->RouterValue[i].rt_detect_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rt_ipaddr=[%s],", i, p_Router->RouterValue[i].rt_ipaddr);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rt_mac_addr=[%s],", i, p_Router->RouterValue[i].rt_mac_addr);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rt_web_text=[%s],", i, p_Router->RouterValue[i].rt_web_text);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rt_caption=[%s],", i, p_Router->RouterValue[i].rt_caption);
    }
    WRITE_INFO(CATEGORY_INFO,"** rt_summary=[%s],", p_Router->rt_summary);
    WRITE_INFO(CATEGORY_INFO,"** rt_rule=[%c][%llu],", p_Router->rt_rule,p_Router->rt_rusq);
    return;
}

void fjson_PrintAllNetPrinter(_DAP_NETPRINTER *p_NetPrinter)
{
    int i=0, j=0;
    WRITE_INFO(CATEGORY_INFO,"* [net_printer],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_NetPrinter->size);
    for(i=0; i<p_NetPrinter->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_connected=[%d],", i, p_NetPrinter->NetPrinterValue[i].np_connected);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_discordance=[%d],", i, p_NetPrinter->NetPrinterValue[i].np_discordance);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_host_name=[%s],", i, p_NetPrinter->NetPrinterValue[i].np_host_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_wsd_printer_device=[%d],", i, p_NetPrinter->NetPrinterValue[i].np_wsd_printer_device);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_wsd_location=[%s],", i, p_NetPrinter->NetPrinterValue[i].np_wsd_location);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_web_text=[%s],", i, p_NetPrinter->NetPrinterValue[i].np_web_text);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_open_port_size=[%d],", i, p_NetPrinter->NetPrinterValue[i].np_open_port_size);
        for(j=0; j<p_NetPrinter->NetPrinterValue[i].np_open_port_size; j++)
        {
            WRITE_INFO(CATEGORY_INFO,"**** np_open_port[%d]=[%d],", j, p_NetPrinter->NetPrinterValue[i].np_open_port[j]);
        }
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] np_printer_port_size=[%d],", i, p_NetPrinter->NetPrinterValue[i].np_printer_port_size);
        for(j=0; j<p_NetPrinter->NetPrinterValue[i].np_printer_port_size; j++)
        {
            WRITE_INFO(CATEGORY_INFO,"**** np_printer_port[%d]=[%d],", j, p_NetPrinter->NetPrinterValue[i].np_printer_port[j]);
        }
    }
    WRITE_INFO(CATEGORY_INFO,"** np_summary=[%s],", p_NetPrinter->np_summary);
    WRITE_INFO(CATEGORY_INFO,"** np_rule=[%c][%llu],", p_NetPrinter->np_rule,p_NetPrinter->np_rusq);
    return;
}

void fjson_PrintAllNetScan(_DAP_NETSCAN* p_NetScan)
{
    int i=0, j=0;
    WRITE_INFO(CATEGORY_INFO,"* [net_scan] " );
    WRITE_INFO(CATEGORY_INFO,"** size=[%d]", p_NetScan->size);
    WRITE_INFO(CATEGORY_INFO,"** unchanged=[%d]", p_NetScan->unchanged);
    for(i=0; i<p_NetScan->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ns_dap_agent=[%d]", i, p_NetScan->NetScanValue[i].ns_dap_agent);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ns_ip=[%s]", i, p_NetScan->NetScanValue[i].ns_ip);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ns_mac=[%s]", i, p_NetScan->NetScanValue[i].ns_mac);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ns_mac_match=[%d]", i, p_NetScan->NetScanValue[i].ns_mac_match);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] ns_open_port_size=[%d]", i, p_NetScan->NetScanValue[i].ns_open_port_size);
        for(j=0; j<p_NetScan->NetScanValue[i].ns_open_port_size; j++)
        {
            WRITE_INFO(CATEGORY_INFO,"**** ns_open_port[%d]=[%d],", j, p_NetScan->NetScanValue[i].ns_open_port[j]);
        }
    }
    return;
}

void fjson_PrintAllSsoCert(_DAP_SSO_CERT *p_SsoCert)
{
    WRITE_INFO(CATEGORY_INFO,"* [sso_cert]");
    WRITE_INFO(CATEGORY_INFO,"** sc_uncertified=[%d]", p_SsoCert->sc_uncertified);
    WRITE_INFO(CATEGORY_INFO,"** sc_rule=[%c][%llu]", p_SsoCert->sc_rule,p_SsoCert->sc_rusq);
    return;
}

void fjson_PrintAllWinDrv(_DAP_WINDRV* p_WinDrv)
{
    int i=0;
    WRITE_INFO(CATEGORY_INFO,"* [win_drv],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_WinDrv->size);
    for(i=0; i<p_WinDrv->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_class=[%s]", i, p_WinDrv->WinDrvValue[i].dv_class);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_class_desc=[%s]", i, p_WinDrv->WinDrvValue[i].dv_class_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_desc=[%s]", i, p_WinDrv->WinDrvValue[i].dv_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_driver=[%s]", i, p_WinDrv->WinDrvValue[i].dv_driver);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_enum=[%s]", i, p_WinDrv->WinDrvValue[i].dv_enum);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_company=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_company);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_copy_right=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_copy_right);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_desc=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_desc);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_path=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_path);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_product=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_product);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_file_ver=[%s]", i, p_WinDrv->WinDrvValue[i].dv_file_ver);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_location=[%s]", i, p_WinDrv->WinDrvValue[i].dv_location);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_mfg=[%s]", i, p_WinDrv->WinDrvValue[i].dv_mfg);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_name=[%s]", i, p_WinDrv->WinDrvValue[i].dv_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_service=[%s]", i, p_WinDrv->WinDrvValue[i].dv_service);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_start=[%s]", i, p_WinDrv->WinDrvValue[i].dv_start);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_status=[%s]", i, p_WinDrv->WinDrvValue[i].dv_status);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_type=[%s]", i, p_WinDrv->WinDrvValue[i].dv_type);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] dv_data_type=[%d]", i, p_WinDrv->WinDrvValue[i].dv_data_type);
    }
    return;
}

void fjson_PrintAllRdpSession(_DAP_RDP_SESSION *p_RdpSession)
{
    int i;
    WRITE_INFO(CATEGORY_INFO,"* [rdp_session],");
    WRITE_INFO(CATEGORY_INFO,"** size=[%d],", p_RdpSession->size);
    for(i=0; i<p_RdpSession->size; i++)
    {
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rdp_client_ip=[%s],", i, p_RdpSession->RdpSessionValue[i].rdp_client_ip);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rdp_client_name=[%s],", i, p_RdpSession->RdpSessionValue[i].rdp_client_name);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rdp_connect_time=[%s],", i, p_RdpSession->RdpSessionValue[i].rdp_connect_time);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] rdp_user_id=[%s],", i, p_RdpSession->RdpSessionValue[i].rdp_user_id);
    }
    WRITE_INFO(CATEGORY_INFO,"** rdp_summary=[%s],", p_RdpSession->rdp_summary);
    WRITE_INFO(CATEGORY_INFO,"** rdp_session_rule=[%c][%c],", p_RdpSession->rdp_session_rule,p_RdpSession->rdp_session_rusq);
    return;
}

void fjson_PrintAllDetectData(char *p_changeItem, _DAP_DETECT_DATA *p_DetectData)
{
    char *tokenKey = strtok(p_changeItem, ",");
    while(tokenKey != NULL)
    {
        if(strcmp(tokenKey, "system") == 0)
        {
            fjson_PrintAllSystem(&p_DetectData->System);
        }
        else if(strcmp(tokenKey, "operating_system") == 0)
        {
            fjson_PrintAllOperatingSystem(&p_DetectData->OperatingSystem);
        }
        else if(strcmp(tokenKey, "cpu") == 0)
        {
            fjson_PrintAllCpu(&p_DetectData->Cpu);
        }
        else if(strcmp(tokenKey, "net_adapter") == 0)
        {
            fjson_PrintAllNetAdapter(&p_DetectData->NetAdapter);
        }
        else if(strcmp(tokenKey, "wifi") == 0)
        {
            fjson_PrintAllWifi(&p_DetectData->Wifi);
        }
        else if(strcmp(tokenKey, "bluetooth") == 0)
        {
            fjson_PrintAllBluetooth(&p_DetectData->Bluetooth);
        }
        else if(strcmp(tokenKey, "net_connection") == 0)
        {
            fjson_PrintAllNetConnection(&p_DetectData->NetConnection);
        }
        else if(strcmp(tokenKey, "disk") == 0)
        {
            fjson_PrintAllDisk(&p_DetectData->Disk);
        }
        else if(strcmp(tokenKey, "net_drive") == 0)
        {
            fjson_PrintAllNetDrive(&p_DetectData->NetDrive);
        }
        else if(strcmp(tokenKey, "os_account") == 0)
        {
            fjson_PrintAllOsAccount(&p_DetectData->OSAccount);
        }
        else if(strcmp(tokenKey, "share_folder") == 0)
        {
            fjson_PrintAllShareFolder(&p_DetectData->ShareFolder);
        }
        else if(strcmp(tokenKey, "infrared_device") == 0)
        {
            fjson_PrintAllInfraredDevice(&p_DetectData->InfraredDevice);
        }
        else if(strcmp(tokenKey, "process") == 0)
        {
            fjson_PrintAllProcess(&p_DetectData->Process);
        }
        else if(strcmp(tokenKey, "router") == 0)
        {
            fjson_PrintAllRouter(&p_DetectData->Router);
        }
        else if(strcmp(tokenKey, "net_printer") == 0)
        {
            fjson_PrintAllNetPrinter(&p_DetectData->NetPrinter);
        }
        else if(strcmp(tokenKey, "net_scan") == 0)
        {
            fjson_PrintAllNetScan(&p_DetectData->NetScan);
        }
        else if(strcmp(tokenKey, "sso_cert") == 0)
        {
            fjson_PrintAllSsoCert(&p_DetectData->SsoCert);
        }
        else if(strcmp(tokenKey, "win_drv") == 0)
        {
            fjson_PrintAllWinDrv(&p_DetectData->WinDrv);
        }
        else if(strcmp(tokenKey, "rdp_session") == 0)
        {
            fjson_PrintAllRdpSession(&p_DetectData->RdpSession);
        }
        else if(strcmp(tokenKey, "main_board") == 0)
        {
            fjson_PrintAllMainBoard(&p_DetectData->MainBoard);
        }
        tokenKey = strtok(NULL, ",");
    } //while
    tokenKey = NULL;
    WRITE_INFO(CATEGORY_INFO,"**************************************,");

    return;
}

void fjson_PrintAllDetect(_DAP_DETECT *p_Detect)
{
    fjson_PrintAllAgentInfo(&p_Detect->AgentInfo);
    WRITE_INFO(CATEGORY_INFO,"* detect_time=[%s] ", p_Detect->detect_time );
    WRITE_INFO(CATEGORY_INFO,"* change_item=[%s] ", p_Detect->change_item);

    return;
}
void fjson_PrintAllCpuUsage(_DAP_CPU_USAGE* p_cpuUsage)
{
    int i = 0;
    WRITE_INFO(CATEGORY_INFO,"* [cpu usage] " );
    WRITE_INFO(CATEGORY_INFO,"** size=[%d] ", p_cpuUsage->size );
    for(i=0; i<p_cpuUsage->size; i++)
    {
        if(i > MAX_PROCESS_COUNT)
            return;

        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cpu_usage_rate : [%s] ",
               i,
               p_cpuUsage->CpuUsageValue[i].cpu_usage_rate);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cpu_usage_rate_condition : [%s] ",
               i,
               p_cpuUsage->CpuUsageValue[i].cpu_usage_rate_condition);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cpu_usage_duration_time : [%s] ",
               i,
               p_cpuUsage->CpuUsageValue[i].cpu_usage_duration_time);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cpu_usage_duration_time_condition : [%s] ",
               i,
               p_cpuUsage->CpuUsageValue[i].cpu_usage_duration_time_condition);
        WRITE_INFO(CATEGORY_INFO,"*** idx=[%d] cpu_usage_detect_time : [%s] ",
               i,
               p_cpuUsage->CpuUsageValue[i].cpu_usage_detect_time);
    }
    return;
}


int fjson_GetBlockcommonSummary(char *p_summary, char *detail)
{
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "block(", ")del", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "block(", ")del", resValue );
    if(strlen(resValue) == 0) // 없으면 차단이 완료된 데이터로 보고 해지
    {
        return 0;
    }

    // 하나라도 차단 or 차단 실패라면 이벤트 발생
    strcpy(detail, resValue);

    return 1;
}

int fjson_GetLengthMainBoard(_DAP_MAINBOARD* p_MainBoard)
{
    int len = 0;

    len += strlen(p_MainBoard->hb_mb_mf);
    len += strlen(p_MainBoard->hb_mb_pn);
    len += strlen(p_MainBoard->hb_mb_sn);

    return len;
}

int fjson_GetLengthSystem(_DAP_SYSTEM *p_System)
{
    int len = 0;

    len += strlen(p_System->st_bootup_state);
    len += strlen(p_System->st_dns_host_name);
    len += strlen(p_System->st_domain);
    len += p_System->st_domain_role;
    len += strlen(p_System->st_vga);
    len += p_System->st_installed_vm_size;
    len += strlen(p_System->st_name);
    len += p_System->st_time_zone;
    len += strlen(p_System->st_vm_name);
    len += p_System->st_wakeup_type;
    len += strlen(p_System->st_work_group);

    return len;
}

int fjson_GetLengthSystemVm(_DAP_SYSTEM* p_System)
{
    int len = 0;

    len += p_System->st_installed_vm_size;
    if(strlen(p_System->st_vm_name) > 0)
        len += 1;

    return len;
}

int fjson_GetLengthSystemVmSummary(char *p_summary)
{
    int		len = 0;
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "add(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "add(", ")", resValue );
    if(strstr(resValue, "vm") != NULL)
        len += 1;
    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "mod(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "mod(", ")", resValue );
    if(strstr(resValue, "vm") != NULL)
        len += 1;

    return len;
}


int fjson_GetLengthConnectExt(_DAP_CONNECT_EXT* p_ConnectExt)
{
    int len = 0;

    if(p_ConnectExt->size > 0)
        len = 1;

    return len;
}

int fjson_GetLengthConnectExtSummary(char *p_summary)
{
    int		len = 0;
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "add(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "add(", ")", resValue );

    if(strlen(resValue) > 0)
    {
        len += 1;
    }

    return len;
}

int fjson_GetLengthOperatingSystem(_DAP_OPERATING_SYSTEM* p_OperatingSystem)
{
    int len = 0;

    len += strlen(p_OperatingSystem->os_architecture);
    len += p_OperatingSystem->os_lang;
    len += strlen(p_OperatingSystem->os_name);
    len += p_OperatingSystem->os_portable;
    len += p_OperatingSystem->os_sp_major_ver;
    len += p_OperatingSystem->os_sp_minor_ver;
    len += p_OperatingSystem->os_type;
    len += strlen(p_OperatingSystem->os_version);

    return len;
}

int fjson_GetLengthCpu(_DAP_CPU *p_Cpu)
{
    int len = 0;

    len = p_Cpu->size;

    return len;
}

int fjson_GetLengthNetAdapter(_DAP_NETADAPTER *p_NetAdapter)
{
    int len = 0;

    len += p_NetAdapter->size;

    return len;
}

int fjson_GetLengthNetAdapterPhysicalNic(_DAP_NETADAPTER* p_NetAdapter)
{
    int len = 0;

    len += p_NetAdapter->physical_nic;

    return len;
}

int fjson_GetBlockNetAdapterSummary(char *p_summary, char *detail)
{
//    char	resValue[512];
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "block(", ")del", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "block(", ")del", resValue);
    if(strlen(resValue) == 0)  // 없으면 차단이 완료된 데이터로 보고 해지
    {
        return 1; // physical_nic가 1개(정상)라는 의미
    }

    // 하나라도 차단 or 차단 실패라면 이벤트 발생
    strcpy(detail, resValue);

    return 2;
}

int fjson_GetLengthNetAdapterMulip(_DAP_NETADAPTER* p_NetAdapter, char *detail)
{
    int i, len = 0;
    int	result = 0; //size2개이상일때 1개라도 들어오면 알람주기위해
    int	firstFlag = 1;

    for(i=0; i < p_NetAdapter->size; i++)
    {
        len = fcom_TokenCnt(p_NetAdapter->NetAdapterValue[i].na_ipv4, ",");
        // len == 0이면 ip 1개, len == 1이면 ip 2개
        if(len > 0)
        {
            if(firstFlag == 1)
            {
                firstFlag = 0;
            }
            else
            {
                strcat(detail, ",");
            }
            strcat(detail, "(");
            strcat(detail, p_NetAdapter->NetAdapterValue[i].na_ipv4);
            strcat(detail, ")");
        }
        result+=len;
    }

    return result;
}

int fjson_GetLengthWifi(_DAP_WIFI* p_Wifi)
{
    int len = 0;

    len = p_Wifi->size;

    return len;
}

int fjson_GetLengthBluetooth(_DAP_BLUETOOTH* p_Bluetooth)
{
    int len = 0;

    len = p_Bluetooth->size;

    return len;
}

int fjson_GetLengthNetConnection(_DAP_NETCONNECTION *p_NetConnection)
{
    int len = 0;

    len = p_NetConnection->size;

    return len;
}

int fjson_GetLengthDisk(_DAP_DISK* p_Disk)
{
    int len = 0;

    len = p_Disk->size;

    return len;
}

int fjson_GetLengthDiskRemovable(_DAP_DISK* p_Disk)
{
    int i, len = 0;

    for(i=0; i<p_Disk->size; i++)
    {
        switch(p_Disk->DiskValue[i].dk_drive_type)
        {
            case DK_REMOVABLE_DISK:
                len += 1;
                break;
        }
        if( !strcasecmp(p_Disk->DiskValue[i].dk_interface_type, "USB") )
            len += 1;
    }

    return len;
}

int fjson_GetLengthDiskHidden(_DAP_DISK *p_Disk)
{
    int i, len = 0;

    for(i=0; i<p_Disk->size; i++)
    {
        if(!strcasecmp(p_Disk->DiskValue[i].dk_name, "Not set"))
        {
            len = 1;
            break;
        }
    }

    return len;
}

int fjson_GetLengthDiskMobileRead(_DAP_DISK *p_Disk)
{
    int i, len = 0;

    for(i=0; i<p_Disk->size; i++)
    {
        if(p_Disk->DiskValue[i].dk_drive_type == DK_REMOVABLE_DISK)
        {
            if(p_Disk->DiskValue[i].dk_access == DK_READABLE)		len = 1;
            else if(p_Disk->DiskValue[i].dk_access == DK_READ_WRITE)len = 1;
        }
    }

    return len;
}

int fjson_GetLengthDiskMobileWrite(_DAP_DISK *p_Disk)
{
    int i, len = 0;

    for(i=0; i<p_Disk->size; i++)
    {
        if(p_Disk->DiskValue[i].dk_drive_type == DK_REMOVABLE_DISK)
        {
            if(p_Disk->DiskValue[i].dk_access == DK_WRITEABLE)		len = 1;
            else if(p_Disk->DiskValue[i].dk_access == DK_READ_WRITE)len = 1;
            else if(p_Disk->DiskValue[i].dk_access == DK_WRITE_ONCE)len = 1;
        }
    }

    return len;
}

int fjson_GetLengthNetDrive(_DAP_NETDRIVE *p_NetDrive)
{
    int len = 0;

    len = p_NetDrive->size;

    return len;
}

int fjson_GetLengthOsAccount(_DAP_OSACCOUNT *p_OA)
{
    int len = 0;

    len = p_OA->size;

    return len;
}

int fjson_GetLengthShareFolder(_DAP_SHARE_FOLDER *p_ShareFolder)
{
    int len = 0, i =0;

    for(i=0; i<p_ShareFolder->size; i++)
    {
        if(p_ShareFolder->ShareFolderValue[i].sf_type == 0)
        {
            len = 1;
            break;
        }
    }

    return len;
}

int fjson_GetLengthInfraredDevice(_DAP_INFRARED_DEVICE* p_InfraredDevice)
{
    int len = 0;

    len = p_InfraredDevice->size;

    return len;
}

int fjson_GetLengthProcess(_DAP_PROCESS* p_Process)
{
    int len = 0;

    len = p_Process->size;

    return len;
}

int fjson_GetLengthProcessWhite(_DAP_PROCESS* p_Process)
{
    int i = 0, len = 0;

    for(i=0; i<p_Process->size; i++)
    {
        if(	p_Process->ProcessValue[i].ps_type == 0 &&
               p_Process->ProcessValue[i].ps_running == 0) //실행이되어야하는프로세스
        {
            len = 1;
            break;
        }
    }

    return len;
}

int fjson_GetLengthProcessAccessSummary(char* p_summary)
{
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));

    fcom_GetTagValue(p_summary, "access(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "white(", ")", resValue );

    if ( resValue[0] == 0x00 )
    {
        return -1;
    }
    if(strstr(resValue, "[off]") != NULL)
        return 0;
    else
        return 1;


}

int fjson_GetLengthProcessWhiteSummary(char* p_summary)
{
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));

    fcom_GetTagValue(p_summary, "white(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "white(", ")", resValue );

    if(strstr(resValue, "[off]") != NULL)
        return 1;
    else
        return 0;

    return 0;
}

int fjson_GetLengthProcessBlack(_DAP_PROCESS* p_Process)
{
    int i = 0, len = 0;

    for(i=0; i<p_Process->size; i++)
    {
        if(	p_Process->ProcessValue[i].ps_type == 1 &&
               p_Process->ProcessValue[i].ps_running == 1) //실행이안되어야하는프로세스
        {
            len = 1;
            break;
        }
    }

    return len;
}

int fjson_GetLengthProcessBlackSummary(char *p_summary)
{
//    char	resValue[512];
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "black(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "black(", ")", resValue );
    if(strstr(resValue, "[on]") != NULL)
    {
        return 1;
    }
    else if(strstr(resValue, "[kill]") != NULL)
    {
        return -1;
    }
    else
    {
        return 0;
    }
    return 0;
}

int fjson_GetLengthProcessAccessmon(_DAP_PROCESS* p_Process)
{
    int i = 0, len = 2;

    // PS_TYPE : 0: white, 1: Black, 2:특정 ip주소로 접속, 3: white + 특정ip 접속, 4: black + 특정ip 접속, 5: black 프로세스 강제 종료
    for(i=0; i<p_Process->size; i++)
    {
        // White Process 또는 Black Process 에서 특정 IP 접속 상태의 프로세스인경우 2021.12.26 기준으로 access_mon summary 데이터가 안오므로 예외처리 시킨다.
        if ( p_Process->ProcessValue[i].ps_type == 3 ||
             p_Process->ProcessValue[i].ps_type == 4)
        {
            len = 2;
            break;
        }

        if ( p_Process->ProcessValue[i].ps_type == 2 )
        {
            if(p_Process->ProcessValue[i].ps_connected_svr_addr_size > 0)
            {
                len = 1;
                break;
            }
        }
    }

    return len;
}

// vm block
int fjson_GetLengthProcessBlockSummary(char *p_summary)
{
//    char	resValue[512];
    char	resValue[4096] = {0x00,};

    memset(resValue, 0x00, sizeof(resValue));
    fcom_GetTagValue(p_summary, "block(", ")", resValue, sizeof(resValue) );
//    fcom_GetTagValue(p_summary, "block(", ")", resValue );
    if(strstr(resValue, "[off]") != NULL) //실행되기전에차단
        return 1;
    else if(strstr(resValue, "[kill]") != NULL) //실행된후kill
        return 2;

    return 0;
}

int fjson_GetLengthRouter(_DAP_ROUTER* p_Router)
{
    int len = 0;

    len = p_Router->size;

    return len;
}

int fjson_GetLengthNetPrinter(_DAP_NETPRINTER* p_NetPrinter)
{
    int i = 0, len = 0;

    for(i=0; i<p_NetPrinter->size; i++)
    {
        if(p_NetPrinter->NetPrinterValue[i].np_discordance != 0)
        {
            len = 1;
            break;
        }
    }

    return len;
}

int fjson_GetLengthNetScan(_DAP_NETSCAN* p_NetScan)
{
    int len = 0;

    len = p_NetScan->size;

    return len;
}
int fjson_GetLengthSsoCert(_DAP_SSO_CERT* p_SsoCert)
{
    int len = 0;

    len = p_SsoCert->sc_uncertified;

    return len;
}

int fjson_GetLengthWinDrv(_DAP_WINDRV* p_WinDrv)
{
    int len = 0;

    len = p_WinDrv->size;

    return len;
}

int fjson_GetLengthRdpSession(_DAP_RDP_SESSION* p_RdpSession)
{
    int len = 0;

    len = p_RdpSession->size;

    return len;
}
int fjson_GetLengthAlarmCpuUsage(_DAP_CPU_USAGE* p_CpuUsage)
{
    int cnt = 0;
    int nRet = 0;
    int nStatus=0;

    /* Body Size가 0인경우 */
    if(p_CpuUsage->size <= 0)
    {
        for(cnt = 0; cnt < p_CpuUsage->historysize; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuHistoryValue[cnt].cpu_usage_status);

            if(nStatus == 5  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //Alarm On
                nRet = 1;
            if(nStatus == 6  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //Alarm Off
                nRet = (-1);// Event삭제.

            WRITE_INFO(CATEGORY_INFO,"-> CpuHistoryValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent);
        }
    }
    else //Body Size가 0이 아닌경우
    {
        for(cnt = 0; cnt < p_CpuUsage->size; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuUsageValue[cnt].cpu_usage_status);
            //Body에 Alarm On Status가 존재.
            if(nStatus == 5 && p_CpuUsage->CpuUsageValue[cnt].is_dap_agent == 0) //Alarm On
                nRet = 1;

            WRITE_INFO(CATEGORY_INFO,"-> CpuUsageValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuUsageValue[cnt].is_dap_agent);
        }

        for(cnt = 0; cnt < p_CpuUsage->historysize; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuHistoryValue[cnt].cpu_usage_status);
            //Body에 Alarm On이 없고
            if(nRet == 0)
            {
                //Summary에 Alarm Off인 경우
                if(nStatus == 6  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //ControlExit
                    nRet = (-1);// Event삭제.

                //Summary에 Alarm On인 경우
                if(nStatus == 5  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //Control Execute
                    nRet = 1;
            }
            WRITE_INFO(CATEGORY_INFO,"-> CpuHistoryValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent);
        }
    }

    return nRet;
}

int fjson_GetLengthControlCpuUsage(_DAP_CPU_USAGE* p_CpuUsage)
{
    int cnt = 0;
    int nRet = 0;
    int nStatus=0;

    /* Body Size가 0인경우 */
    if(p_CpuUsage->size <= 0)
    {
        for(cnt = 0; cnt < p_CpuUsage->historysize; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuHistoryValue[cnt].cpu_usage_status);

            if(nStatus == 2  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //ControlExecute
                nRet = 1;
            if(nStatus == 3  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //ControlExit
                nRet = (-1);// Event삭제.

            WRITE_INFO(CATEGORY_INFO,"-> CpuHistoryValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent );
        }
    }
    else //Body Size가 0이 아닌경우
    {
        for(cnt = 0; cnt < p_CpuUsage->size; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuUsageValue[cnt].cpu_usage_status);
            //Body에 Control Execute Status가 존재.
            if(nStatus == 2  && p_CpuUsage->CpuUsageValue[cnt].is_dap_agent == 0) //ControlExecute
                nRet = 1;

            WRITE_INFO(CATEGORY_INFO,"-> CpuUsageValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuUsageValue[cnt].is_dap_agent);
        }

        for(cnt = 0; cnt < p_CpuUsage->historysize; cnt++)
        {
            nStatus = atoi(p_CpuUsage->CpuHistoryValue[cnt].cpu_usage_status);
            //Body에 ControlExecute인게 없고
            if(nRet == 0)
            {
                //Summary에 Exit Status인 경우
                if(nStatus == 3  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //ControlExit
                    nRet = (-1);// Event삭제.

                //Summary에 Control Execute인 경우
                if(nStatus == 2  && p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent == 0) //Control Execute
                    nRet = 1;
            }
            WRITE_INFO(CATEGORY_INFO,"-> CpuHistoryValue nRet [%d] nStatus [%d] is dap [%d]",
                   nRet,nStatus,p_CpuUsage->CpuHistoryValue[cnt].is_dap_agent);
        }
    }

    return nRet;
}



int fjson_writeAttachFile(char* buffer, int bufferSize,char* fileName, char* savedFile)
{
//    struct tm *spTm;
    struct tm stTm;

    char fullPaht[256 +1] = {0x00,};
    int fd = 0;
    int rxt = 0;

    time_t tTime;
    tTime = time((time_t) 0);
//    spTm = (struct tm *) localtime(&tTime);
    localtime_r(&tTime, &stTm);

    memset(fullPaht, 0x00, sizeof(fullPaht));
    sprintf(fullPaht, "%s/%02d/%s", "/home1/msgpgw/pushdata", stTm.tm_hour, fileName);

    sprintf(savedFile, "%02d/%s", stTm.tm_hour, fileName);

    fd = open(fullPaht, O_WRONLY | O_CREAT, 0644);

    if (fd < 0)
    {
        WRITE_INFO(CATEGORY_INFO,"writePushContent : File Open Fail! = [%s].", fullPaht);

        return -1;
    }

    rxt = write(fd, buffer, bufferSize);

    if (rxt != bufferSize)
    {
        close(fd);
        WRITE_INFO(CATEGORY_INFO,"writePushContent : Write Fail= [%s],", fullPaht);
        return -1;
    }

    close(fd);

    WRITE_INFO(CATEGORY_INFO,"Push Content Save=[%s],", fullPaht);

    return 1;
}

char* fjson_GetFileExtenstion(char *file_name)
{
    int i = 0;
    char* file_ext = NULL;

    int file_name_len = strlen (file_name);
    file_name +=file_name_len ;

    for(i =0 ; i <file_name_len ; i ++)
    {
        if(* file_name == '.' )
        {
            file_ext = file_name +1 ;
            break;
        }
        file_name --;
    }
    return file_ext ;
}

DETECT_ITEM fjson_GetDetectItem(char *p_type)
{
    if(!strcmp(p_type, "main_board"))
    {
        return MAIN_BOARD;
    }
    else if(!strcmp(p_type, "system"))
    {
        return SYSTEM;
    }
    else if(!strcmp(p_type, "operating_system"))
    {
        return OPERATING_SYSTEM;
    }
    else if(!strcmp(p_type, "cpu"))
    {
        return CPU;
    }
    else if(!strcmp(p_type, "net_adapter"))
    {
        return NET_ADAPTER;
    }
    else if(!strcmp(p_type, "wifi"))
    {
        return WIFI;
    }
    else if(!strcmp(p_type, "bluetooth"))
    {
        return BLUETOOTH;
    }
    else if(!strcmp(p_type, "net_connection"))
    {
        return NET_CONNECTION;
    }
    else if(!strcmp(p_type, "disk"))
    {
        return DISK;
    }
    else if(!strcmp(p_type, "net_drive"))
    {
        return NET_DRIVE;
    }
    else if(!strcmp(p_type, "os_account"))
    {
        return OS_ACCOUNT;
    }
    else if(!strcmp(p_type, "share_folder"))
    {
        return SHARE_FOLDER;
    }
    else if(!strcmp(p_type, "infrared_device"))
    {
        return INFRARED_DEVICE;
    }
    else if(!strcmp(p_type, "process"))
    {
        return PROCESS;
    }
    else if(!strcmp(p_type, "router"))
    {
        return ROUTER;
    }
    else if(!strcmp(p_type, "net_printer"))
    {
        return NET_PRINTER;
    }
    else if(!strcmp(p_type, "net_scan"))
    {
        return NET_SCAN;
    }
    else if(!strcmp(p_type, "arp"))
    {
        return ARP;
    }
    else if(!strcmp(p_type, "virtual_machine"))
    {
        return VIRTUAL_MACHINE;
    }
    else if(!strcmp(p_type, "connect_ext_svr"))
    {
        return CONNECT_EXT_SVR;
    }
    else if(!strcmp(p_type, "sso_cert"))
    {
        return SSO_CERT;
    }
    else if(!strcmp(p_type, "win_drv"))
    {
        return WIN_DRV;
    }
    else if(!strcmp(p_type, "rdp_session"))
    {
        return RDP_SESSION;
    }
    else if(!strcmp(p_type, "cpu_usage"))
    {
        return CPU_USAGE;
    }
    else
    {
        return UNKNOWN;
    }
}