/* Copyright Abandoned 1996, 1999, 2001 MySQL AB
   This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifndef _mariadb_version_h_
#define _mariadb_version_h_

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define PROTOCOL_VERSION		10
#define MARIADB_CLIENT_VERSION_STR	"10.2.16"
#define MARIADB_BASE_VERSION		"mariadb-10.2"
#define MARIADB_VERSION_ID		100216
#define MYSQL_VERSION_ID		100216
#define MARIADB_PORT	        	3306
#define MARIADB_UNIX_ADDR               "/var/lib/mysql/mysql.sock"
#define MYSQL_CONFIG_NAME		"my"

#define MARIADB_PACKAGE_VERSION "3.0.4"
#define MARIADB_PACKAGE_VERSION_ID 30004
#define MARIADB_SYSTEM_TYPE "Linux"
#define MARIADB_MACHINE_TYPE "x86_64"
#define MARIADB_PLUGINDIR "/usr/lib64/mysql/plugin"

/* mysqld compile time options */
#ifndef MYSQL_CHARSET
#define MYSQL_CHARSET			""
#endif
#endif

/* Source information */
#define CC_SOURCE_REVISION ""

#endif /* _mariadb_version_h_ */
