CREATE TABLE {DETECT_LINK_HISTORY} (
DLH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
DG_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT '감시 그룹 고유번호',
OBJECT_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT '감시 고유번호',
OBJECT_TYPE TINYINT(2) DEFAULT '0' COMMENT '0:PROCESS, 1:IP, 2:URL',
DL_FLAG CHAR(1) NOT NULL COMMENT '스케줄러 작업 상태',
DLH_RECORD_TIME DATETIME NULL COMMENT '기록시간',
PRIMARY KEY (DLH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page; 
CREATE INDEX {DETECT_LINK_HISTORY}_IDX1 ON {DETECT_LINK_HISTORY} (OBJECT_SQ,DG_SQ,OBJECT_TYPE);
