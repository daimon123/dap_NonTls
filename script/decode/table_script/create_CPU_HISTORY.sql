CREATE TABLE {CPU_HISTORY} ( 
CUH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
CU_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT 'TB 고유번호',
CU_NAME	VARCHAR(128) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI	NULL DEFAULT '' COMMENT '이름',
CU_MF VARCHAR(128) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '제작사',
CU_DESC	VARCHAR(256) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI	NULL DEFAULT '' COMMENT '설명',
CU_PID VARCHAR(32) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '프로세서 ID',
CU_DETECT_TIME DATETIME DEFAULT NULL COMMENT '데이터 검출 시간',
CU_PREV_HIST_SQ TEXT DEFAULT NULL COMMENT '과거 히스토리 고유번호 묶음',
CU_SUMMARY TEXT DEFAULT NULL COMMENT '변동사항',
HB_SQ BIGINT(20) UNSIGNED NOT NULL COMMENT 'HW_BASE_TB 고유번호',
US_SQ BIGINT(20) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'USER_TB 고유번호',
CUH_RECORD_TIME DATETIME NULL COMMENT '기록시간',
PRIMARY KEY(CUH_SQ)  
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {CPU_HISTORY}_IDX1 ON {CPU_HISTORY} (CU_SQ);
CREATE INDEX {CPU_HISTORY}_IDX2 ON {CPU_HISTORY} (HB_SQ);
CREATE INDEX {CPU_HISTORY}_IDX3 ON {CPU_HISTORY} (US_SQ);
CREATE INDEX {CPU_HISTORY}_IDX4 ON {CPU_HISTORY} (CU_DETECT_TIME);
