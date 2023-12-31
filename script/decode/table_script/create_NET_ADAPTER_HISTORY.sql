CREATE TABLE {NET_ADAPTER_HISTORY} ( 
NAH_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL AUTO_INCREMENT COMMENT '고유번호',
NA_SQ BIGINT(20) UNSIGNED ZEROFILL NOT NULL COMMENT 'TB 고유번호',
NA_NAME	VARCHAR(128) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NOT NULL COMMENT '네트워크 어댑터 이름',
NA_PN VARCHAR(128) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '네트워크 어댑터 제품명',
NA_DESC	VARCHAR(256) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '네트워크 어댑터 설명',
NA_DEVICE_TYPE TINYINT(2) DEFAULT '0' COMMENT '네트워크어댑터연결방식',
NA_IPV4	VARCHAR(512) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT 'IPv4 주소',
NA_IPV6	VARCHAR(640) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT 'IPv6 주소',
NA_MAC VARCHAR(32) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT 'MAC 주소',
NA_SUBNET VARCHAR(16) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '서브넷 마스크',
NA_DEFAULT_GW VARCHAR(256) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '기본 게이트웨이 IP 주소',
NA_DEFAULT_GW_MAC VARCHAR(256) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '기본 게이트웨이 MAC 주소',
NA_PREF_DNS	VARCHAR(16) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '기본 설정 DNS 서버',
NA_ALTE_DNS	VARCHAR(16) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '대체 DNS 서버',
NA_NET_CONNECTION_ID VARCHAR(64) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '네트워크 연결 ID',
NA_NET_CONNECTION_STATUS SMALLINT(3) UNSIGNED NULL DEFAULT '0' COMMENT '네트워크에 대한 네트워크 어댑터 연결 상태',
NA_NET_ENABLED BOOL NULL DEFAULT '0' COMMENT '네트워크 활성화 여부',
NA_PHYSICAL_ADAPTER BOOL NULL DEFAULT '0' COMMENT '물리적 장비 어댑터 정보 여부',
NA_PNP_DEVICE_ID VARCHAR(128) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '논리 장치의 Windows PnP 장치 식별자',
NA_SERVICE_NAME VARCHAR(32) CHARACTER SET UTF8 COLLATE UTF8_GENERAL_CI NULL DEFAULT '' COMMENT '네트워크 어댑터 서비스 이름',
NA_DETECT_TIME DATETIME DEFAULT NULL COMMENT '데이터 검출 시간',
NA_PREV_HIST_SQ TEXT DEFAULT NULL COMMENT '과거 히스토리 고유번호 묶음',
NA_SUMMARY TEXT DEFAULT NULL COMMENT '변동사항',
HB_SQ BIGINT(20) UNSIGNED NOT NULL COMMENT 'HW_BASE_TB 고유번호',
US_SQ BIGINT(20) UNSIGNED NOT NULL DEFAULT 0 COMMENT 'USER_TB 고유번호',
NAH_RECORD_TIME DATETIME NULL COMMENT '기록시간',
PRIMARY KEY(NAH_SQ) 
) ENGINE=Aria AUTO_INCREMENT=1 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci PAGE_CHECKSUM=1 ROW_FORMAT=Page;
CREATE INDEX {NET_ADAPTER_HISTORY}_IDX1 ON {NET_ADAPTER_HISTORY} (NA_SQ);
CREATE INDEX {NET_ADAPTER_HISTORY}_IDX2 ON {NET_ADAPTER_HISTORY} (HB_SQ);
CREATE INDEX {NET_ADAPTER_HISTORY}_IDX3 ON {NET_ADAPTER_HISTORY} (US_SQ);
CREATE INDEX {NET_ADAPTER_HISTORY}_IDX4 ON {NET_ADAPTER_HISTORY} (NA_DETECT_TIME);
