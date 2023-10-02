CREATE VIEW VW_IP_CHANGE AS SELECT 
A.HB_SQ AS HB_SQ, 
A.US_SQ AS US_SQ, 
A.HB_UNQ AS HB_UNQ,
A.HB_ACCESS_IP AS CURR_IP, 
B.HB_ACCESS_IP AS BEF1_IP, 
C.HB_ACCESS_IP AS BEF2_IP 
FROM DAP_DB.HW_BASE_TB A 
LEFT JOIN DAP_HISTORY_2018.HW_BASE_HISTORY_09 B ON A.HB_UNQ = B.HB_UNQ
LEFT JOIN DAP_HISTORY_2018.HW_BASE_HISTORY_08 C ON A.HB_UNQ = C.HB_UNQ
GROUP BY A.HB_UNQ;
