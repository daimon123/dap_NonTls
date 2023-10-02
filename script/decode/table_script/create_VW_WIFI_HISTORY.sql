CREATE OR REPLACE VIEW {VW_WIFI_HISTORY} as 
select 
wf.*,
info.HB_ACCESS_IP,info.UG_NAME,
ifnull(ev.US_SQ, evh.US_SQ) as US_SQ,
ifnull(ev.EV_SQ, evh.EV_SQ) as EV_SQ,
ifnull(ev.EV_IPADDR, evh.EV_IPADDR) as EV_IPADDR,
ifnull(ev.EV_TYPE, evh.EV_TYPE) as EV_TYPE,
ifnull(ev.EV_LEVEL, evh.EV_LEVEL) as EV_LEVEL,
ifnull(ev.EV_EXIST, evh.EV_EXIST) as EV_EXIST,
ifnull(ev.EV_EVENT_CONTEXT, evh.EV_EVENT_CONTEXT) as EV_CONTEXT,
ifnull(ev.EV_DETECT_TIME, evh.EV_DETECT_TIME) as EV_DETECT_TIME,
ifnull(ev.EV_DUP_DETECT_TIME, evh.EV_DUP_DETECT_TIME) as EV_DUP_DETECT_TIME,
ifnull(ev.EV_RECORD_TIME, evh.EV_RECORD_TIME) as EV_RECORD_TIME,
evh.EVH_SQ,
evh.EVH_RECORD_TIME 
from {WIFI_HISTORY} as wf 
    left join (
        select hw.HB_SQ, hw.HB_ACCESS_IP, ug.UG_NAME 
        from DAP_DB.HW_BASE_TB as hw 
            join DAP_DB.USER_LINK_TB as ul on hw.US_SQ = ul.US_SQ 
            join DAP_DB.USER_GROUP_TB as ug on ul.UG_SQ = ug.UG_SQ 
    ) as info on wf.HB_SQ = info.HB_SQ 
left join DAP_DB.EVENT_TB as ev on wf.HB_SQ = ev.HB_SQ 
	and ev.EV_TYPE in ({EV_TYPE}) 
	and ev.EV_EXIST = 1 
	and wf.WF_DETECT_TIME = ev.EV_DETECT_TIME 
left join {EVENT_HISTORY} as evh on wf.HB_SQ = evh.HB_SQ 
	and evh.EV_TYPE in ({EV_TYPE}) 
	and ((evh.EV_EXIST = 2 and wf.WF_DETECT_TIME = evh.EV_DETECT_TIME) 
		or (evh.EV_EXIST = 0 and wf.WF_DETECT_TIME = evh.EV_DUP_DETECT_TIME)) 
	and isnull(ev.EV_EXIST);
