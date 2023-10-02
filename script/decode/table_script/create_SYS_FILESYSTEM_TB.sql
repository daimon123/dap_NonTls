CREATE TABLE SYS_FILESYSTEM_TB (
MGWID INTEGER(4) NOT NULL COMMENT '시스템ID',
TOTAL INTEGER DEFAULT 0 COMMENT '총 용량',
USED INTEGER DEFAULT 0 COMMENT '사용중인 용량',
AVAIL INTEGER DEFAULT 0 COMMENT '사용가능한 용량',
CAPACITY INTEGER DEFAULT 0 COMMENT '사용가능한 용량%',
FILESYSTEM varchar(80) NOT NULL COMMENT '파티션',
PRIMARY KEY(MGWID,FILESYSTEM)
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;