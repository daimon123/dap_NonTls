#!/bin/bash
if [ $# == 1 ]
then
	FILE="$1"
else
    echo Usage: make_data [file]
    exit 1
fi

MYSQL="/usr/bin/mysql"
MYUSER="intent"
MYPASS="intent00"
MYHOST="10.20.20.44"
MYDBNAME="DAP_DB"
MYPORT="50205"

$MYSQL -h $MYHOST -u$MYUSER -p$MYPASS -P$MYPORT $MYDBNAME < $FILE

