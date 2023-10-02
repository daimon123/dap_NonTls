CREATE TABLE {HW_BASE_HISTORY} (
HBH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
HB_SQ BIGINT(20) UNSIGNED NOT NULL COMMENT 'TB 고유번호',
US_SQ BIGINT(20) UNSIGNED DEFAULT '0' COMMENT '유저고유번호',
HB_UNQ CHAR(24) NOT NULL DEFAULT '' COMMENT '서버에서 부여한 AGENT 고유값',
HB_MB_PN VARCHAR(128) DEFAULT '' COMMENT '메인보드 제품명',
HB_MB_MF VARCHAR(128) DEFAULT '' COMMENT '메인보드 제작사',
HB_MB_SN VARCHAR(64) DEFAULT '' COMMENT '메인보드 시리얼번호',
HB_FIRST_TIME DATETIME DEFAULT NULL COMMENT '처음 변경(등록) 시간',
HB_ACCESS_IP VARCHAR(15) DEFAULT NULL COMMENT '최근 접속한 IP주소',
HB_ACCESS_MAC CHAR(17) DEFAULT NULL COMMENT '최근 접속한 MAC주소',
HB_SOCKET_IP VARCHAR(15) DEFAULT NULL COMMENT '최근 접속한 SOCKET IP주소',
HB_AGENT_VER CHAR(16) DEFAULT NULL COMMENT '마지막 접속에 사용한 AGENT 버전',
HB_EXTERNAL TINYINT(1) DEFAULT '0' COMMENT '외부에서접속유무',
HB_ACCESS_TIME DATETIME DEFAULT NULL COMMENT 'AGENT 정책요청 시간',
HB_RECORD_TIME DATETIME DEFAULT NULL COMMENT '마지막 변경 시간',
HB_PREV_HIST_SQ TEXT DEFAULT NULL COMMENT '과거 히스토리 고유번호 묶음',
HB_DEL TINYINT(2) DEFAULT '0' COMMENT '1:DEL(EXCEPT.KEY),2:DEL(ALL),3:RESTART,4:REQBASE,9:MANUAL DEL',
HBH_RECORD_TIME DATETIME DEFAULT NULL COMMENT '마지막 변경 시간',
PRIMARY KEY (HBH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {HW_BASE_HISTORY}_IDX1 ON {HW_BASE_HISTORY} (HB_SQ);
CREATE INDEX {HW_BASE_HISTORY}_IDX2 ON {HW_BASE_HISTORY} (HB_UNQ);
CREATE INDEX {HW_BASE_HISTORY}_IDX3 ON {HW_BASE_HISTORY} (HB_ACCESS_IP);
