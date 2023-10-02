CREATE TABLE SYS_SYSTEM_TB (
MGWID INTEGER(4) NOT NULL COMMENT '시스템ID',
MEM_USED INTEGER DEFAULT 0 COMMENT '메모리 사용중인 용량',
MEM_TOT INTEGER DEFAULT 0 COMMENT '메모리 총 용량',
MEM_FREE INTEGER DEFAULT 0 COMMENT '메모리 사용가능한 용량',
MEM_PERUSED INTEGER DEFAULT 0 COMMENT '메모리 % 사용중인 용량',
CPU_USER INTEGER DEFAULT 0 COMMENT 'CPU USER 사용량',
CPU_SYS INTEGER DEFAULT 0 COMMENT 'CPU SYS 사용량',
CPU_IDLE INTEGER DEFAULT 0 COMMENT 'CPU IDEL',
PRIMARY KEY(MGWID)
) ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
