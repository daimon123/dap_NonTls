CREATE TABLE ALARM_TB (
AL_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
MN_SQ BIGINT(20) UNSIGNED DEFAULT '0' COMMENT '관리자',
AL_DETECT_TYPE TINYINT(2) DEFAULT '0' COMMENT '알람정책',
AL_DETECT_LEVEL TINYINT(2) UNSIGNED DEFAULT '0' COMMENT '알람정책레벨',
AL_USE TINYINT(2) UNSIGNED DEFAULT '0' COMMENT '알람 종류 0:미사용,1:SMS,2:MAIL,3:BOTH',
AL_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러 작업 상태',
AL_RECORD_TIME DATETIME DEFAULT NULL COMMENT '기록시간',
PRIMARY KEY (AL_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page; 
CREATE INDEX ALARM_TB_IDX1 ON ALARM_TB (MN_SQ,AL_USE);
CREATE INDEX ALARM_TB_IDX2 ON ALARM_TB (AL_FLAG);
