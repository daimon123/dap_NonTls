CREATE TABLE {CTRL_PROCESS_CPU_HISTORY} (
CPH_SQ bigint(20) unsigned zerofill  NOT NULL AUTO_INCREMENT COMMENT '고유번호',
CP_SQ bigint(20) unsigned zerofill  NOT NULL COMMENT 'TB 고유번호',
CP_ALARM_TYPE char(1) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '0:전체 알람, 1:프로세스 알람, 2:전체 통제, 3:프로세스 통제',
CP_STATUS tinyint(1) DEFAULT 0 COMMENT 'ControlExecute : 2, ControlExit : 3, AlarmOn : 5, AlarmOff : 6',
CP_STATUS_NAME varchar(32) DEFAULT NULL COMMENT 'Status 이름',
CP_NEW_DATA_FLAG tinyint(1) DEFAULT 0 COMMENT '0 : 기존 데이터, 1 : 신규 변경 데이터',
CP_IS_DAP_FLAG tinyint(1) DEFAULT 0 COMMENT '0 : 일반 프로세스, 1 : DAP 프로세스',
CP_PROCESS_ID varchar(12) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '프로세스 ID',
CP_PNAME varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '프로세스명',
CP_VALUE varchar(8) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'CPU 사용률',
CP_VALUE_CONDITION varchar(4) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'CPU 사용 정책 값 ',
CP_VALUE_LIMIT varchar(4) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'CPU 사용 제한 값 ',
CP_DURATION_TIME varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'CPU 지속시간',
CP_DURATION_TIME_CONDITION varchar(16) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT 'CPU 지속시간 조건 값 ',
CP_DETECT_TIME varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '데이터 검출 시간',
CP_PREV_HIST_SQ text CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT 'NULL' COMMENT '과거 히스토리 고유번호 묶음',
HB_SQ bigint(20) unsigned NOT NULL COMMENT 'HW_BASE_TB 고유번호',
US_SQ bigint(20) unsigned NOT NULL DEFAULT 0 COMMENT 'USER_TB 고유번호',
CPH_RECORD_TIME datetime DEFAULT NULL COMMENT '기록시간',
CP_START_TIME varchar(32) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '데이터 검출 시작 시간',
PRIMARY KEY (CPH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {CTRL_PROCESS_CPU_HISTORY}_IDX1 ON {CTRL_PROCESS_CPU_HISTORY} (CPH_SQ);
CREATE INDEX {CTRL_PROCESS_CPU_HISTORY}_IDX2 ON {CTRL_PROCESS_CPU_HISTORY} (HB_SQ);
CREATE INDEX {CTRL_PROCESS_CPU_HISTORY}_IDX3 ON {CTRL_PROCESS_CPU_HISTORY} (US_SQ);
CREATE INDEX {CTRL_PROCESS_CPU_HISTORY}_IDX4 ON {CTRL_PROCESS_CPU_HISTORY} (CP_DETECT_TIME);