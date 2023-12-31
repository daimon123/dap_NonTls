CREATE TABLE USER_LINK_TB (
UG_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT '사용자그룹 고유번호',
US_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT '사용자 고유번호',
UL_FLAG CHAR(1) DEFAULT 'A' COMMENT '스케줄러 작업 상태',
PRIMARY KEY(UG_SQ,US_SQ)
) ENGINE=Aria DEFAULT CHARSET=utf8 PAGE_CHECKSUM=1 ROW_FORMAT=PAGE;
CREATE INDEX USER_LINK_TB_IDX1 ON USER_LINK_TB (UL_FLAG);
