create table STM_EVENT_REALTIME_TB
(
STMR_SQ          bigint unsigned zerofill auto_increment comment '고유 번호' primary key,
STMR_RECORD_TIME varchar(7)    not null comment '데이터 검출 시간(MONTH)',
STMR_IPADDR      varchar(15)   not null comment '이벤트발생 IP',
STMR_US_SQ       bigint        not null comment '이벤트발생 US_SQ',
STMR_UG_SQ       bigint        not null comment '이벤트발생 UG_SQ',
STMR_TYPE        smallint(3)   not null comment '이벤트 유형',
STMR_LEVEL       smallint(1)   not null comment '이벤트 레벨 주의(1) 카운트',
STMR_EXIST       smallint(1)   not null comment '0:해제,1:발생,2:중복,3:강제확인,4:강제삭제',
STMR_COUNT       int default 0 null comment '합계',
constraint STMR_RECORD_TIME unique (STMR_RECORD_TIME, STMR_IPADDR, STMR_US_SQ, STMR_TYPE, STMR_LEVEL, STMR_EXIST)
) engine = Aria charset = utf8;
create index STMR_EVENT_TB_IDX1 on STM_EVENT_REALTIME_TB (STMR_UG_SQ);
