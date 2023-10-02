CREATE TABLE {AGENT_LOG_HISTORY}
(
	HB_SQ BIGINT(20) COMMENT '장치번호',
	HB_UNQ CHAR(24) COMMENT '유저키',
	IP   CHAR(15) COMMENT 'Server IP',	
	PROCESS VARCHAR(20) COMMENT '프로세스명',
	LOGDATE DATETIME COMMENT '로그시간',
	LOGLEVEL VARCHAR(10) COMMENT '에러레벨',
	LOGMSG VARCHAR(1024) COMMENT '로그메시지'	
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {AGENT_LOG_HISTORY}_IDX1 ON {AGENT_LOG_HISTORY} (IP,LOGDATE);
CREATE INDEX {AGENT_LOG_HISTORY}_IDX2 ON {AGENT_LOG_HISTORY} (HB_SQ);
CREATE INDEX {AGENT_LOG_HISTORY}_IDX3 ON {AGENT_LOG_HISTORY} (HB_UNQ); 