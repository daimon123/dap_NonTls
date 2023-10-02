CREATE TABLE WATCH_SERVER_TB (
WS_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유 번호',
WS_SVR VARCHAR(64) NOT NULL COMMENT '접속 감시대상 IP OR URL',
WS_DESC VARCHAR(128) DEFAULT '' COMMENT '서버에 대한 추가 설명',
WS_USE TINYINT(1) DEFAULT '1' COMMENT '등록정보 사용여부',
WS_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러플래그',
PRIMARY KEY (WS_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX WATCH_SERVER_TB_IDX1 ON WATCH_SERVER_TB (WS_SVR);
CREATE INDEX WATCH_SERVER_TB_IDX2 ON WATCH_SERVER_TB (WS_FLAG);
