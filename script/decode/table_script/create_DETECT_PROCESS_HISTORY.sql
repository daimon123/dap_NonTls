CREATE TABLE {DETECT_PROCESS_HISTORY} (
DPH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
DP_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT 'TB 고유번호',
DP_PROCESS_NAME VARCHAR(32) NOT NULL COMMENT '프로세스 파일명',
DP_DETAIL_INFO TINYINT(1) DEFAULT '0' COMMENT '0 or 1',
DP_HASH CHAR(32) DEFAULT '' COMMENT '프로세스 파일 HASH',
DP_DESC VARCHAR(128) DEFAULT '' COMMENT '프로세스에 대한 추가 설명',
DP_USE TINYINT(1) DEFAULT '1' COMMENT '등록정보 사용 여부',
DP_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러작업',
DPH_RECORD_TIME DATETIME DEFAULT NULL COMMENT '기록시간',
PRIMARY KEY (DPH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {DETECT_PROCESS_HISTORY}_IDX1 ON {DETECT_PROCESS_HISTORY} (DP_SQ);
CREATE INDEX {DETECT_PROCESS_HISTORY}_IDX2 ON {DETECT_PROCESS_HISTORY} (DP_PROCESS_NAME);
