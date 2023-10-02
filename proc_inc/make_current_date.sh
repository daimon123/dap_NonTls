#!/bin/sh

VAL=`date`
echo $VAL
echo "#ifndef _DAP_CURRENT_DATE_H" > dap_current_date.h
echo "#define _DAP_CURRENT_DATE_H" >> dap_current_date.h
echo "#ifndef	VERSION_DF_compile_time" >> dap_current_date.h
echo "#define		VERSION_DF_compile_time							\""$VAL"\"" >> dap_current_date.h
echo "#endif" >> dap_current_date.h
echo "#endif //_DAP_CURRENT_DATE_H" >> dap_current_date.h
