CREATE TABLE {RULE_DETECT_HISTORY} (
RDH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
RD_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT '고유번호',
RU_SQ BIGINT(20) UNSIGNED ZEROFILL DEFAULT NULL COMMENT '정책 고유번호',
RD_VALUE TEXT DEFAULT NULL COMMENT 'PROCESS or IP',
RD_TYPE TINYINT(2) DEFAULT '0' COMMENT '0:BLACK, 1:WHITE, 2:ACCESS, 3:CONEXT',
RD_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러 작업 상태',
RDH_RECORD_TIME datetime DEFAULT NULL COMMENT '기록 시간',
PRIMARY KEY(RDH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {RULE_DETECT_HISTORY}_IDX1 ON {RULE_DETECT_HISTORY} (RU_SQ,RD_TYPE);
