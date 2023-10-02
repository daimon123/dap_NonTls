#!/bin/bash
if [ $# == 1 ]
then
	FILE="$1"
else
    echo Usage: make_table [file]
    exit 1
fi

## MySql 실행파일 경로
MYSQL="/usr/bin/mysql"
## MySql 계정명
MYUSER="intent"
## MySql 계정 패스워드
MYPASS="intent00"
## MySql 접속 IP
MYHOST="10.20.20.44"
## MySql 데이터베이스명
MYDBNAME="test"
## MySql 접속포트
MYPORT="50205"

$MYSQL -h $MYHOST -u$MYUSER -p$MYPASS -P$MYPORT $MYDBNAME < $FILE

