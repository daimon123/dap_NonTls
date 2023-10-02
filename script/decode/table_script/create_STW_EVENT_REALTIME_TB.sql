create table STW_EVENT_REALTIME_TB
(
STWR_SQ          bigint unsigned zerofill auto_increment comment '고유 번호' primary key,
STWR_RECORD_TIME varchar(10)   not null comment '데이터 검출 시간(WEEK)',
STWR_IPADDR      varchar(15)   not null comment '이벤트발생 IP',
STWR_US_SQ       bigint        not null comment '이벤트발생 US_SQ',
STWR_UG_SQ       bigint        not null comment '이벤트발생 UG_SQ',
STWR_TYPE        smallint(3)   not null comment '이벤트 유형',
STWR_LEVEL       smallint(1)   not null comment '이벤트 레벨',
STWR_EXIST       smallint(1)   not null comment '0:해제,1:발생,2:중복,3:강제확인,4:강제삭제',
STWR_COUNT       int default 0 null comment '합계',
constraint STWR_RECORD_TIME unique (STWR_RECORD_TIME, STWR_IPADDR, STWR_US_SQ, STWR_TYPE, STWR_LEVEL, STWR_EXIST)
) engine = Aria charset = utf8;
create index STWR_EVENT_TB_IDX1 on STW_EVENT_REALTIME_TB (STWR_UG_SQ);
