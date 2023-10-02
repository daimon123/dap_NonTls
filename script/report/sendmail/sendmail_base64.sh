#!/bin/bash

# Desc:a shell script for sending mail with sendmail command
# Author:by Jason.z
# Mail:ccnuzxg@gmail.com
# Date:2014-03-31
# Copyright: Jasonz (http://www.jason-z.com)
# We use mime protocel,you can get more info from http://en.wikipedia.org/wiki/MIME

ret=$(which sendmail)

if [ "$ret" == "" ];then
	yum -y install sendmail
fi 

# set the encode of your mail 
encode="UTF-8" 

# base64 string
# params:$1 string
#        $2 way B/Q
function base64_string()
{
	if [ "$encode" ] && [ "$1" ] && [ "$2" ];then
		echo "=?"$encode"?"$2"?"$(echo $1 | base64)"?="
	fi
}

DEST=$HOME/dap/script/report/html
IMG=$HOME/dap/script/report/img
Year=$(date +%Y)
Now=$(date +%Y%m%d)

from=$(base64_string $1 "B")
to=$2
subject=$(base64_string "DAP_REPORT" "B")
boundary="_Part_189_619193260.1384275896069"
body="This email is a report about DAP and should not be read by anyone other than the person concerned."

declare -a attachments
#attachments=("/home/intent/dap/script/report/20181112_DAP_REPORT.html" "b.txt")
attachments=("${DEST}/${Year}/${Now}_DAP_REPORT.html")

# get file mimetype by file command
# params:$1 filename
function get_mimetype()
{
	ret=$(which file)

	if [ "$ret" == "" ];then
		yum -y install file
	fi 

	if [ $1 ];then
		echo $(file --mime-type $1 | cut -d: -f2 | sed s/[[:space:]]//g)
	fi

}

# Build headers
{
 
printf '%s\n' "From: $from
To: $to
Subject: $subject
Mime-Version: 1.0
Content-Type: multipart/mixed; boundary=\"$boundary\"
 
--${boundary}
Content-Type: text/html; charset=\"$encode\"
Content-Transfer-Encoding: 7bit
Content-Disposition: inline

<html>
<head>
<title>DAP Report</title>
</head>
<body>
<div id=\"preloadDiv\">
<span style='font-family:\"Segoe UI WestEuropean;font-weight:100;\"'></span>
<span style='font-family:\"Segoe UI WestEuropean;font-weight:200;\"'></span>
<span style='font-family:\"Segoe UI WestEuropean;font-weight:400;\"'></span>
<span style='font-family:\"Segoe UI WestEuropean;font-weight:600;\"'></span>
</div>
<div id=\"app\"></div>
<div id=\"warn-title\">
<span style='font-family:\"Segoe UI WestEuropean;font-weight:400;\"'>◈ Warning ◈</span>
</div>
<div id=\"warn-content\" style=\"background-color:#FFD8D8;\">
<ul>
<li>
<span style='font-family:\"Segoe UI WestEuropean;font-weight:600;\"'>$body</span>
</li>
</ul>
<ul>
<li>
<span style='font-family:\"Segoe UI WestEuropean;font-weight:200;\"'>This mail is for testing purposes.</span>
</li>
</ul>
</div>
</body>
</html>

"
 
# now loop over the attachments, guess the type
# and produce the corresponding part, encoded base64
for file in "${attachments[@]}"; do
 
  [ ! -f "$file" ] && echo "Warning: attachment $file not found, skipping" >&2 && continue
 
  mimetype=$(get_mimetype "$file") 

  if [ "$mimetype" = "text/html" ];then
    sendname=`echo $file | sed "s/\/home\/intent\/dap\/script\/report\/html\/${Year}//g"`
  elif [ "$mimetype" = "image/png" ];then
    sendname=`echo $file | sed "s/\/home\/intent\/dap\/script\/report\/img//g"`
  else
    sendname=`echo $file | sed "s/\/home\/intent\/dap\/script\/report\/img//g"`
  fi
 
  printf '%s\n' "--${boundary}
Content-Type: $mimetype
Content-Transfer-Encoding: base64
Content-Disposition: attachment; filename=\"$(base64_string $sendname "Q")\"
"
 
  base64 "$file"
  echo
done
 
# print last boundary with closing --
printf '%s\n' "--${boundary}--"
 
} | sendmail -t -oi -v  # you can give more arguments here ,like -v to debug

ERROR=$?
if [ $ERROR -ne 0 ]
then
    echo -ne "\nError"
    [ $ERROR -eq 67 ] && echo ": Fail in sendmail"
    echo
else
    echo -e "\nSuccess"
fi

exit $ERROR
