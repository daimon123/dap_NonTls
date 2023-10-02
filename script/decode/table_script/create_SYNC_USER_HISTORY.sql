CREATE TABLE {SYNC_USER_HISTORY} (
SUH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
EMN VARCHAR(20) NOT NULL COMMENT '직원번호',
AEMP_NM VARCHAR(20) DEFAULT NULL COMMENT '직원명',
BJURIS_BRCD VARCHAR(10) DEFAULT NULL COMMENT '관할부점코드',
CBENM VARCHAR(20) DEFAULT NULL COMMENT '관할부점명',
PRS_BE_YMD CHAR(8) DEFAULT NULL COMMENT '현재소속년월일',
MWK_BRCD VARCHAR(10) DEFAULT NULL COMMENT '실근무지부점코드',
GBENM VARCHAR(20) DEFAULT NULL COMMENT '실근무지명',
BE_TEAM_CD VARCHAR(10) DEFAULT NULL COMMENT '소속팀코드',
BBENM VARCHAR(20) DEFAULT NULL COMMENT '소속명',
BJURIS_HQCD VARCHAR(10) DEFAULT NULL COMMENT '관할본부코드',
EBENM VARCHAR(20) DEFAULT NULL COMMENT '관할본부명',
MNDT_CD VARCHAR(10) DEFAULT NULL COMMENT '보임코드',
JBCL_CD VARCHAR(10) DEFAULT NULL COMMENT '직급코드',
JBTT_CD VARCHAR(10) DEFAULT NULL COMMENT '직위코드',
ABNM_JTM VARCHAR(10) DEFAULT NULL COMMENT '약칭직위명',
DUCD CHAR(4) DEFAULT NULL COMMENT '직책코드',
ETBN_DCD VARCHAR(10) DEFAULT NULL COMMENT '입행구분코드',
EMP_CPN VARCHAR(30) DEFAULT NULL COMMENT '전화번호',
EAD VARCHAR(40) DEFAULT NULL COMMENT '이메일',
RSNO CHAR(6) DEFAULT NULL COMMENT '주민등록번호(생년월일)',
ENBK_YMD CHAR(8) DEFAULT NULL COMMENT '입행년월일',
RETI_YMD CHAR(8) DEFAULT NULL COMMENT '퇴직년월일',
SEUMU CHAR(1) DEFAULT NULL COMMENT '서무구분',
CHUL VARCHAR(10) DEFAULT NULL COMMENT '조직속성코드',
SUH_RECORD_TIME DATETIME NULL COMMENT '기록 시간',
PRIMARY KEY(SUH_SQ)
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {SYNC_USER_HISTORY}_IDX1 ON {SYNC_USER_HISTORY} (EMN);