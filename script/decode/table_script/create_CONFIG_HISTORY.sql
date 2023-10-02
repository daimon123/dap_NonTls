CREATE TABLE {CONFIG_HISTORY} (
CFH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
CF_NAME VARCHAR(64) NOT NULL COMMENT '환경설정명',
CF_VALUE VARCHAR(512) DEFAULT '' COMMENT '설정값',
CF_FLAG CHAR(1) NOT NULL COMMENT '스케줄러 작업 상태',
CFH_RECORD_TIME DATETIME DEFAULT NULL COMMENT '기록시간',
PRIMARY KEY (CFH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {CONFIG_HISTORY}_IDX1 ON {CONFIG_HISTORY} (CF_NAME);
