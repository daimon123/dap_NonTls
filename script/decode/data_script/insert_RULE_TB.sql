TRUNCATE TABLE RULE_TB;
ALTER TABLE RULE_TB AUTO_INCREMENT=1;
INSERT INTO DAP_DB.RULE_TB 
(RU_MODIFY_CNT,RU_ORDER,MN_SQ,RU_TARGET_TYPE,RU_TARGET_VALUE,RU_NET_ADAPTER,RU_WIFI,
RU_BLUETOOTH,RU_ROUTER,RU_PRINTER,RU_DISK,RU_NET_DRIVE,RU_SHARE_FOLDER,RU_INFRARED_DEVICE,
RU_VIRTUAL_MACHINE,RU_EXT_NET_DETECT_TYPE,RU_ALARM_TYPE,RU_ALARM_MN_SQ,RU_USE,RU_FLAG,
RU_NET_CONNECTION,RU_NET_ADAPTER_OVER,RU_NET_ADAPTER_DUPIP,RU_NET_ADAPTER_DUPMAC,RU_DISK_REG,
RU_DISK_MOBILE,RU_DISK_HIDDEN,RU_DISK_NEW,RU_DISK_MOBILE_WRITE,RU_DISK_MOBILE_READ,
RU_PROCESS_WHITE,RU_PROCESS_BLACK,RU_PROCESS_ACCESSMON,RU_PROCESS_DETAILINFO,RU_WIN_DRV) 
VALUES
( 0, 0, 1, 3, '0', 1, 1, 1, 1, 1, 1 , 1 , 1 , 1 , 1 , 1, 1, 0, 1, 'A', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1);
COMMIT;
