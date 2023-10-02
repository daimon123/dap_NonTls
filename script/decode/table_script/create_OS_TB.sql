CREATE TABLE OS_TB (
OS_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
OS_NAME VARCHAR(128) NOT NULL COMMENT '운영체제명',
OS_VERSION VARCHAR(32) DEFAULT '' COMMENT '운영체제 버전',
OS_ARCHITECTURE VARCHAR(16) DEFAULT '' COMMENT '아키텍처',
OS_LANG INT(5) UNSIGNED DEFAULT '0' COMMENT '언어',
OS_TYPE INT(5) UNSIGNED DEFAULT '0' COMMENT '운영체제 유형',
OS_PORTABLE TINYINT(1) DEFAULT '0' COMMENT '외부 USB 장치에서 부팅되었는지 여부',
OS_SP_MAJOR_VER INT(5) UNSIGNED DEFAULT '0' COMMENT '서비스팩의 주버전 번호',
OS_SP_MINOR_VER INT(5) UNSIGNED DEFAULT '0' COMMENT '서비스팩의 부버전 번호',
OS_DETECT_TIME DATETIME DEFAULT NULL COMMENT '데이터 검출 시간',
OS_CURR_HIST_SQ BIGINT(20) UNSIGNED DEFAULT NULL COMMENT '현재 히스토리 고유번호',
OS_CURR_HIST_DATE VARCHAR(8) DEFAULT NULL COMMENT '현재 히스토리 날짜',
OS_PREV_HIST_SQ TEXT DEFAULT NULL COMMENT '과거 히스토리 고유번호 묶음',
OS_SUMMARY TEXT DEFAULT NULL COMMENT '변동사항',
HB_SQ BIGINT(20) UNSIGNED NOT NULL COMMENT 'HW_BASE_TB 고유번호',
US_SQ BIGINT(20) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'USER_TB 고유번호',
PRIMARY KEY (OS_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX OS_TB_IDX1 ON OS_TB (HB_SQ);
CREATE INDEX OS_TB_IDX2 ON OS_TB (US_SQ);
