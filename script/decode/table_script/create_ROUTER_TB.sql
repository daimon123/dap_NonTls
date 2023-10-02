CREATE TABLE ROUTER_TB (
RT_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
RT_DETECT_TYPE SMALLINT(3) UNSIGNED DEFAULT '0' COMMENT '공유기 검출 유형',
RT_IPADDR VARCHAR(16) DEFAULT '' COMMENT '공유기 IP 주소',
RT_MAC_ADDR VARCHAR(32) DEFAULT '' COMMENT '공유기 MAC 주소',
RT_WEB_TEXT VARCHAR(128) DEFAULT '' COMMENT 'HTTP, HTTPS로 접속 후 수신한 문자열에서 검출한 키워드 문자열(EX. IPTIME, ETC)',
RT_CAPTION VARCHAR(128) DEFAULT '' COMMENT '공유기 정보',
RT_DETECT_TIME DATETIME DEFAULT NULL COMMENT '데이터 검출 시간',
RT_CURR_HIST_SQ BIGINT(20) UNSIGNED DEFAULT NULL COMMENT '현재 히스토리 고유번호',
RT_CURR_HIST_DATE VARCHAR(8) DEFAULT NULL COMMENT '현재 히스토리 날짜',
RT_PREV_HIST_SQ TEXT DEFAULT NULL COMMENT '과거 히스토리 고유번호 묶음',
RT_SUMMARY TEXT DEFAULT NULL COMMENT '변동사항',
HB_SQ BIGINT(20) UNSIGNED NOT NULL COMMENT 'HW_BASE_TB 고유번호',
US_SQ BIGINT(20) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'USER_TB 고유번호',
PRIMARY KEY (RT_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX ROUTER_TB_IDX1 ON ROUTER_TB (HB_SQ);
CREATE INDEX ROUTER_TB_IDX2 ON ROUTER_TB (US_SQ);
