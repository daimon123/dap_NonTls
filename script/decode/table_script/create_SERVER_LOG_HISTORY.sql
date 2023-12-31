CREATE TABLE {SERVER_LOG_HISTORY}
(
	IP   CHAR(15) COMMENT 'Server IP',
	PID INT(8) COMMENT 'PID',
	PROCESS VARCHAR(20) COMMENT '프로세스명',
	LOGDATE DATETIME COMMENT '로그시간',
	LOGLEVEL VARCHAR(10) COMMENT '에러레벨',
	LOGMSG VARCHAR(1024) COMMENT '로그메시지'
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {SERVER_LOG_HISTORY}_IDX1 ON {SERVER_LOG_HISTORY} (IP,LOGDATE);
CREATE INDEX {SERVER_LOG_HISTORY}_IDX2 ON {SERVER_LOG_HISTORY} (PID); 