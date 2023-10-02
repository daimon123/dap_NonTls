CREATE TABLE {MANAGER_HISTORY} (
MNH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
MN_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT 'TB 고유번호',
MN_LEVEL SMALLINT(3) UNSIGNED DEFAULT '2' COMMENT '관리자 등급',
MN_ID VARCHAR(32) NOT NULL COMMENT '관리자 ID',
MN_PW VARCHAR(255) DEFAULT '' COMMENT '관리자 비밀번호',
MN_NAME VARCHAR(32) DEFAULT '' COMMENT '관리자명',
MN_IPADDR VARCHAR(50) NOT NULL COMMENT '접속 IP주소',
MN_SOCKET_IP VARCHAR(15) DEFAULT NULL COMMENT '접속SOCKET IP주소',
MN_EMAIL VARCHAR(64) DEFAULT '' COMMENT '이메일 주소',
MN_CELL_PHONE VARCHAR(64) DEFAULT '' COMMENT '핸드폰 번호',
MN_DESC VARCHAR(128) DEFAULT '' COMMENT '계정 추가 설명',
MN_FLAG CHAR(1) NOT NULL COMMENT '스케줄러 작업 상태',
MN_LOGIN_STATUS TINYINT(1) DEFAULT NULL COMMENT '사용자로그인 상태',
MN_FAIL_COUNT INT(11) DEFAULT NULL COMMENT '로그인 실패 카운트',
MN_EVENT_NOTI TINYINT(1) NOT NULL DEFAULT '0' COMMENT '이벤트 NOTI 방법',
MN_CONN_PID INT(8) UNSIGNED DEFAULT '0' COMMENT '로그인PID',
MN_CONN_FQ INT(8) DEFAULT '-1' COMMENT '로그인FQ',
MN_CREATE_TIME DATETIME DEFAULT NULL COMMENT '생성일자',
MN_MODIFY_TIME DATETIME DEFAULT NULL COMMENT '수정일자',
MNH_RECORD_TIME DATETIME DEFAULT NULL COMMENT '기록시간',
PRIMARY KEY (MNH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {MANAGER_HISTORY}_IDX1 ON {MANAGER_HISTORY} (MN_SQ);
CREATE INDEX {MANAGER_HISTORY}_IDX2 ON {MANAGER_HISTORY} (MN_ID);
