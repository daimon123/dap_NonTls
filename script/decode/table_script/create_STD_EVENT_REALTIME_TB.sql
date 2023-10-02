create table STD_EVENT_REALTIME_TB
(
STDR_SQ          bigint unsigned zerofill auto_increment comment '고유 번호' primary key,
STDR_RECORD_TIME varchar(13)   not null comment '데이터 검출 시간(HOUR)',
STDR_IPADDR      varchar(15)   not null comment '이벤트발생 IP',
STDR_US_SQ       bigint        not null comment '이벤트발생 US_SQ',
STDR_UG_SQ       bigint        not null comment '이벤트발생 UG_SQ',
STDR_TYPE        smallint(3)   not null comment '이벤트 유형',
STDR_LEVEL       smallint(1)   not null comment '이벤트 레벨',
STDR_EXIST       smallint(1)   not null comment '0:해제,1:발생,2:중복,3:강제확인,4:강제삭제',
STDR_COUNT       int default 0 null comment '합계',
constraint STDR_RECORD_TIME unique (STDR_RECORD_TIME, STDR_IPADDR, STDR_US_SQ, STDR_TYPE, STDR_LEVEL, STDR_EXIST)
) engine = Aria charset = utf8; 
create index STDR_EVENT_TB_IDX1 on STD_EVENT_REALTIME_TB (STDR_UG_SQ);
