CREATE TABLE {DETECT_PRINTER_PORT_HISTORY} (
PPH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
PP_PORT SMALLINT(3) NOT NULL COMMENT '접속 검사대상 프린터 포트',
PP_DESC VARCHAR(128) DEFAULT '' COMMENT '포트 정보에 대한 추가 설명',
PP_USE TINYINT(1) DEFAULT '1' COMMENT '등록정보 사용 여부',
PP_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러플래그',
PPH_RECORD_TIME DATETIME DEFAULT NULL COMMENT '기록시간',
PRIMARY KEY (PPH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page; 
CREATE INDEX {DETECT_PRINTER_PORT_HISTORY}_IDX1 ON {DETECT_PRINTER_PORT_HISTORY} (PP_PORT);
