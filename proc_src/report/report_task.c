//
// Created by KimByoungGook on 2020-06-24.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>

#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "db/dap_trandb.h"
#include "report.h"


int freport_SetDataLoop(char *jsbuff, char *jsonData)
{
    int  i,j,k,slen;
    char tmpbuff[1024*50];

    slen = strlen(jsbuff);

    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            strcat(tmpbuff, jsonData);

            j=strlen(tmpbuff);
            for (k=0;k<10;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);

    return strlen(jsbuff);
}

// 2022-02-05 param_param_cRealTimeFlag
// 0x00 : 정기보고서, 0x01 : 실시간보고서
int freport_MakeReportTemplate(
        char*				repPath,
        int					bDate,
        int					bCustom,
        char*				sDate,
        char*				eDate,
        unsigned long long 	groupSq,
        char*				mngId,
        char*				repHtmlPath,
        char                param_cRealTimeFlag)
{
    int				rxt;
    int             nRes = 0;
    int				nChartSize = 0;
    int				nTotChartSize = 0;
    int				nTotTableSize = 0;
    int				headerSize = 0;
    int				headerTotSize = 0;
    int				bodySize = 0;
    int				bodyTotSize = 0;
    int				footerSize = 0;
    int				footerTotSize = 0;
    int				chartMinJsSize = 0;
    int				utilsJsSize = 0;
    char            repFullPath[256 +1] = {0x00,};
    char			tmpBodyLoop[1024*3] = {0x00,};
    char			tmpFooterLoop[2048*3]= {0x00,};
    char			tmpRange[23+1]= {0x00,}; //최대치
    char			tmpTable[1024*50]= {0x00,};
    char			tmpChart[1024*10]= {0x00,};
    char			tmpPieAnaly[256]= {0x00,};
    char			tmpTimeAnaly[256]= {0x00,};
    char			tmpRankAnaly[256]= {0x00,};
    char			tmpTypeAnaly[256]= {0x00,};
    char			tmpGroupAnaly[256]= {0x00,};
    char			currYear[4+1]= {0x00,};
    char			currMonth[2+1]= {0x00,};
    char			currDate[10+1]= {0x00,};
    char			sDateRepl[8+1]= {0x00,};
    char			eDateRepl[8+1]= {0x00,};
    char			groupStr[30]= {0x00,};
    char			currLang[2+1]= {0x00,};
    char			tmpHtmlPath[256]= {0x00,};
    time_t			currTime;
    char            *repChartMinJsBuf = NULL;
    char            *repUtilsJsBuf = NULL;
    char            *repHeaderBuf = NULL; //header.html+js+여분
    char            *repBodyBuf = NULL; //body.html+chart+여분
    char            *repFooterBuf = NULL; //footer.html+title
    char            *repResultBuf = NULL;
    FILE			*fp = NULL;

    WRITE_INFO(CATEGORY_INFO,"1. Make js file " );
    memset(repFullPath,		0x00, sizeof(repFullPath));
    sprintf(repFullPath,	"%s/%s", repPath, "js");

    chartMinJsSize = fcom_GetFileSize(repFullPath, "Chart.min.js");
    if(fcom_malloc((void**)&repChartMinJsBuf,sizeof(char) * (chartMinJsSize + 1) ) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    nRes = fcom_GetReportFile(repFullPath, "Chart.min.js", repChartMinJsBuf);
    WRITE_INFO(CATEGORY_INFO, "1-1. Read 'Chart.min.js', nRes(%d)size(%d) ",
               nRes,
               chartMinJsSize);

    utilsJsSize = fcom_GetFileSize(repFullPath, "utils.js");
    if(fcom_malloc((void**)&repUtilsJsBuf, sizeof(char)*(utilsJsSize+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    nRes = fcom_GetReportFile(repFullPath, "utils.js", repUtilsJsBuf);
    WRITE_INFO(CATEGORY_INFO, "1-2. Read 'utils.js', nRes(%d)size(%d) ",
               nRes,
               utilsJsSize);


    WRITE_INFO(CATEGORY_INFO,"2. Make html file " );

    memset(repFullPath,		0x00, sizeof(repFullPath));
    sprintf(repFullPath,	"%s/%s", repPath, "template");

    headerSize = fcom_GetFileSize(repFullPath, "header.html");
    headerTotSize = headerSize + chartMinJsSize + utilsJsSize + 1;

    if(fcom_malloc((void**)&repHeaderBuf, sizeof(char)*headerTotSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    nRes = fcom_GetReportFile(repFullPath, "header.html", repHeaderBuf);
    WRITE_INFO(CATEGORY_INFO, "2-1. Read 'header.html', nRes(%d)size(%d) ",
               nRes,
               headerTotSize);

    nRes = 0;
    nTotChartSize = 0;

    memset(tmpBodyLoop, 0x00, sizeof(tmpBodyLoop));
    memset(tmpFooterLoop, 0x00, sizeof(tmpFooterLoop));

    if(bDate == 1) //day
    {
        memset(tmpRange, 0x00, sizeof(tmpRange));
        memset(tmpChart, 0x00, sizeof(tmpChart));
        memset(tmpTable, 0x00, sizeof(tmpTable));
        memset(tmpPieAnaly, 0x00, sizeof(tmpPieAnaly));
        memset(tmpTimeAnaly, 0x00, sizeof(tmpTimeAnaly));
        memset(tmpRankAnaly, 0x00, sizeof(tmpRankAnaly));
        memset(tmpTypeAnaly, 0x00, sizeof(tmpTypeAnaly));
        memset(tmpGroupAnaly, 0x00, sizeof(tmpGroupAnaly));
        nChartSize = 0;

        nRes += freport_DeployReportDailyBysq(repPath,
                                         tmpChart,
                                         tmpTable,
                                         tmpRange,
                                         tmpPieAnaly,
                                         tmpTimeAnaly,
                                         tmpRankAnaly,
                                         tmpTypeAnaly,
                                         tmpGroupAnaly,
                                         groupSq,
                                         &nChartSize, param_cRealTimeFlag);

        nTotChartSize += nChartSize;
        sprintf(tmpBodyLoop+strlen(tmpBodyLoop),
                "<form><fieldset>"
                "<legend>&nbsp;@subject#&nbsp;</legend>"
                "<h2>@range#</h2>"
                "<table width=\"100%%\" class=table6_3><tr>"
                "<td width=\"50%%\"><canvas id=\"pie-day\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"line-day\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td width=\"50%%\"><canvas id=\"rank-day\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"type-day\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td colspan=\"2\"><canvas id=\"group-day\" height=100></canvas></td>"
                "</tr><tr>"
                "<td colspan=\"2\">%s</td>"
                "</tr></table>"
                "@table#"
                "</fieldset></form>",
                tmpPieAnaly,tmpTimeAnaly,tmpRankAnaly,tmpTypeAnaly,tmpGroupAnaly);
        strcat(tmpFooterLoop,
               "var color = Chart.helpers.color;"
               "var ctxPieDay = document.getElementById('pie-day').getContext('2d');"
               "window.myPie = new Chart(ctxPieDay, varPieDay);"
               "var ctxLineDay = document.getElementById('line-day').getContext('2d');"
               "window.myLine = new Chart(ctxLineDay, varLineDay);"
               "var ctxRankDay = document.getElementById('rank-day').getContext('2d');"
               "window.myBar = new Chart(ctxRankDay, {"
               "type: 'horizontalBar',"
               "data: rankChartDay,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxTypeDay = document.getElementById('type-day').getContext('2d');"
               "window.myBar = new Chart(ctxTypeDay, {"
               "type: 'horizontalBar',"
               "data: typeChartDay,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxGroupDay = document.getElementById('group-day').getContext('2d');"
               "window.myBar = new Chart(ctxGroupDay, {"
               "type: 'bar',"
               "data: groupChartDay,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
        );
    }

    if(bDate == 2) //week
    {
        memset(tmpRange, 0x00, sizeof(tmpRange));
        memset(tmpChart, 0x00, sizeof(tmpChart));
        memset(tmpTable, 0x00, sizeof(tmpTable));
        memset(tmpPieAnaly, 0x00, sizeof(tmpPieAnaly));
        memset(tmpTimeAnaly, 0x00, sizeof(tmpTimeAnaly));
        memset(tmpRankAnaly, 0x00, sizeof(tmpRankAnaly));
        memset(tmpTypeAnaly, 0x00, sizeof(tmpTypeAnaly));
        memset(tmpGroupAnaly, 0x00, sizeof(tmpGroupAnaly));
        nChartSize = 0;


        nRes += freport_DeployReportWeeklyBysq(repPath,
                                          tmpChart,
                                          tmpTable,
                                          tmpRange,
                                          tmpPieAnaly,
                                          tmpTimeAnaly,
                                          tmpRankAnaly,
                                          tmpTypeAnaly,
                                          tmpGroupAnaly,
                                          groupSq,
                                          &nChartSize, param_cRealTimeFlag);


        nTotChartSize += nChartSize;
        sprintf(tmpBodyLoop+strlen(tmpBodyLoop),
                "<form><fieldset>"
                "<legend>&nbsp;@subject#&nbsp;</legend>"
                "<h2>@range#</h2>"
                "<table width=\"100%%\" class=table6_3><tr>"
                "<td width=\"50%%\"><canvas id=\"pie-week\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"line-week\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td width=\"50%%\"><canvas id=\"rank-week\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"type-week\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td colspan=\"2\"><canvas id=\"group-week\" height=100></canvas></td>"
                "</tr><tr>"
                "<td colspan=\"2\">%s</td>"
                "</tr></table>"
                "@table#"
                "</fieldset></form>",
                tmpPieAnaly,tmpTimeAnaly,tmpRankAnaly,tmpTypeAnaly,tmpGroupAnaly);
        strcat(tmpFooterLoop,
               "var color = Chart.helpers.color;"
               "var ctxPieWeek = document.getElementById('pie-week').getContext('2d');"
               "window.myPie = new Chart(ctxPieWeek, varPieWeek);"
               "var ctxLineWeek = document.getElementById('line-week').getContext('2d');"
               "window.myLine = new Chart(ctxLineWeek, varLineWeek);"
               "var ctxRankWeek = document.getElementById('rank-week').getContext('2d');"
               "window.myBar = new Chart(ctxRankWeek, {"
               "type: 'horizontalBar',"
               "data: rankChartWeek,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxTypeWeek = document.getElementById('type-week').getContext('2d');"
               "window.myBar = new Chart(ctxTypeWeek, {"
               "type: 'horizontalBar',"
               "data: typeChartWeek,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxGroupWeek = document.getElementById('group-week').getContext('2d');"
               "window.myBar = new Chart(ctxGroupWeek, {"
               "type: 'bar',"
               "data: groupChartWeek,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
        );
    }

    if(bDate == 3) //month
    {
        memset(tmpRange, 0x00, sizeof(tmpRange));
        memset(tmpChart, 0x00, sizeof(tmpChart));
        memset(tmpTable, 0x00, sizeof(tmpTable));
        memset(tmpPieAnaly, 0x00, sizeof(tmpPieAnaly));
        memset(tmpTimeAnaly, 0x00, sizeof(tmpTimeAnaly));
        memset(tmpRankAnaly, 0x00, sizeof(tmpRankAnaly));
        memset(tmpTypeAnaly, 0x00, sizeof(tmpTypeAnaly));
        memset(tmpGroupAnaly, 0x00, sizeof(tmpGroupAnaly));
        nChartSize = 0;

        nRes += freport_DeployReportMonthlyBysq(repPath,
                                           tmpChart,
                                           tmpTable,
                                           tmpRange,
                                           tmpPieAnaly,
                                           tmpTimeAnaly,
                                           tmpRankAnaly,
                                           tmpTypeAnaly,
                                           tmpGroupAnaly,
                                           groupSq,
                                           &nChartSize,
                                           param_cRealTimeFlag);


        nTotChartSize += nChartSize;
        sprintf(tmpBodyLoop+strlen(tmpBodyLoop),
                "<form><fieldset>"
                "<legend>&nbsp;@subject#&nbsp;</legend>"
                "<h2>@range#</h2>"
                "<table width=\"100%%\" class=table6_3><tr>"
                "<td width=\"50%%\"><canvas id=\"pie-month\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"line-month\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td width=\"50%%\"><canvas id=\"rank-month\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"type-month\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td colspan=\"2\"><canvas id=\"group-month\" height=100></canvas></td>"
                "</tr><tr>"
                "<td colspan=\"2\">%s</td>"
                "</tr></table>"
                "@table#"
                "</fieldset></form>",
                tmpPieAnaly,tmpTimeAnaly,tmpRankAnaly,tmpTypeAnaly,tmpGroupAnaly);
        strcat(tmpFooterLoop,
               "var color = Chart.helpers.color;"
               "var ctxPieMonth = document.getElementById('pie-month').getContext('2d');"
               "window.myPie = new Chart(ctxPieMonth, varPieMonth);"
               "var ctxLineMonth = document.getElementById('line-month').getContext('2d');"
               "window.myLine = new Chart(ctxLineMonth, varLineMonth);"
               "var ctxRankMonth = document.getElementById('rank-month').getContext('2d');"
               "window.myBar = new Chart(ctxRankMonth, {"
               "type: 'horizontalBar',"
               "data: rankChartMonth,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxTypeMonth = document.getElementById('type-month').getContext('2d');"
               "window.myBar = new Chart(ctxTypeMonth, {"
               "type: 'horizontalBar',"
               "data: typeChartMonth,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxGroupMonth = document.getElementById('group-month').getContext('2d');"
               "window.myBar = new Chart(ctxGroupMonth, {"
               "type: 'bar',"
               "data: groupChartMonth,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
        );
    }

    if(bCustom != 0) //custom
    {
        memset(tmpRange, 0x00, sizeof(tmpRange));
        memset(tmpChart, 0x00, sizeof(tmpChart));
        memset(tmpTable, 0x00, sizeof(tmpTable));
        memset(tmpPieAnaly, 0x00, sizeof(tmpPieAnaly));
        memset(tmpTimeAnaly, 0x00, sizeof(tmpTimeAnaly));
        memset(tmpRankAnaly, 0x00, sizeof(tmpRankAnaly));
        memset(tmpTypeAnaly, 0x00, sizeof(tmpTypeAnaly));
        memset(tmpGroupAnaly, 0x00, sizeof(tmpGroupAnaly));
        nChartSize = 0;


        nRes += freport_DeployReportCustomBysq(repPath,
                                          tmpChart,
                                          tmpTable,
                                          tmpRange,
                                          tmpPieAnaly,
                                          tmpTimeAnaly,
                                          tmpRankAnaly,
                                          tmpTypeAnaly,
                                          tmpGroupAnaly,
                                          &nChartSize,
                                          sDate,
                                          eDate,
                                          groupSq,
                                          bCustom, param_cRealTimeFlag);

        nTotChartSize += nChartSize;
        sprintf(tmpBodyLoop+strlen(tmpBodyLoop),
                "<form><fieldset>"
                "<legend>&nbsp;@subject#&nbsp;</legend>"
                "<h2>@range#</h2>"
                "<table width=\"100%%\" class=table6_3><tr>"
                "<td width=\"50%%\"><canvas id=\"pie-cust\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"line-cust\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td width=\"50%%\"><canvas id=\"rank-cust\" height=200></canvas></td>"
                "<td width=\"50%%\"><canvas id=\"type-cust\" height=200></canvas></td>"
                "</tr><tr>"
                "<td>%s</td><td>%s</td>"
                "</tr><tr>"
                "<td colspan=\"2\"><canvas id=\"group-cust\" height=100></canvas></td>"
                "</tr><tr>"
                "<td colspan=\"2\">%s</td>"
                "</tr></table>"
                "@table#"
                "</fieldset></form>",
                tmpPieAnaly,tmpTimeAnaly,tmpRankAnaly,tmpTypeAnaly,tmpGroupAnaly);
        strcat(tmpFooterLoop,
               "var color = Chart.helpers.color;"
               "var ctxPieCust = document.getElementById('pie-cust').getContext('2d');"
               "window.myPie = new Chart(ctxPieCust, varPieCust);"
               "var ctxLineCust = document.getElementById('line-cust').getContext('2d');"
               "window.myLine = new Chart(ctxLineCust, varLineCust);"
               "var ctxRankCust = document.getElementById('rank-cust').getContext('2d');"
               "window.myBar = new Chart(ctxRankCust, {"
               "type: 'horizontalBar',"
               "data: rankChartCust,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxTypeCust = document.getElementById('type-cust').getContext('2d');"
               "window.myBar = new Chart(ctxTypeCust, {"
               "type: 'horizontalBar',"
               "data: typeChartCust,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
               "var ctxGroupCust = document.getElementById('group-cust').getContext('2d');"
               "window.myBar = new Chart(ctxGroupCust, {"
               "type: 'bar',"
               "data: groupChartCust,"
               "options: {"
               "responsive: true,"
               "legend: {"
               "position: 'top',"
               "},"
               "title: {"
               "display: true,"
               "text: '@title#'"
               "}"
               "}"
               "});"
        );
    }

    //doughnut일때dataset항시표기
    strcat(tmpFooterLoop,
           "Chart.plugins.register({"
           "afterDatasetsDraw: function(chart) {"
           "var ctx = chart.ctx;"
           "chart.data.datasets.forEach(function(dataset, i) {"
           "var meta = chart.getDatasetMeta(i);"
           "if (!meta.hidden) {"
           "if(meta.type.indexOf('doughnut') != -1) {"
           "meta.data.forEach(function(element, index) {"
           "ctx.fillStyle = 'rgb(255, 255, 255)';"
           "var fontSize = 16;"
           "var fontStyle = 'normal';"
           "var fontFamily = 'Helvetica Neue';"
           "ctx.font = Chart.helpers.fontString(fontSize, fontStyle, fontFamily);"
           "if(dataset.data[index] != 0) {"
           "var dataString = dataset.data[index].toString();"
           "} else {"
           "var dataString = '';"
           "}"
           "ctx.textAlign = 'center';"
           "ctx.textBaseline = 'middle';"
           "var padding = 5;"
           "var position = element.tooltipPosition();"
           "ctx.fillText(dataString, position.x, position.y - (fontSize / 2) - padding);"
           "});"
           "}"
           "}"
           "});"
           "}"
           "});"
    );

    nTotTableSize = nRes;


    WRITE_INFO(CATEGORY_INFO,"2-2. Get range(%s)chart(%d)table(%d)tmpBodyLoop(%d) ",
            tmpRange,
            nTotChartSize,
            nTotTableSize,
            strlen(tmpBodyLoop));


    if(strlen(fcom_GetCustLang()) > 0)
    {
        strcpy(currLang, fcom_GetCustLang());
    }
    else
    {
        strcpy(currLang, g_stProcReportInfo.szConfMailLang);
    }

    if(!strcasecmp(currLang, "kr"))
    {
        bodySize = fcom_GetFileSize(repFullPath, "body.html");
    }
    else
    {
        bodySize = fcom_GetFileSize(repFullPath, "body_en.html");
    }
    bodyTotSize = bodySize + nTotTableSize + strlen(tmpBodyLoop) + 512; //body.html + table + range
    if(fcom_malloc((void**)&repBodyBuf, sizeof(char)*bodyTotSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    if(!strcasecmp(currLang, "kr"))
    {
        nRes = fcom_GetReportFile(repFullPath, "body.html", repBodyBuf);
        WRITE_INFO(CATEGORY_INFO,"2-3. Read 'body.html', nRes(%d)size(%d) ",nRes,bodyTotSize);
    }
    else
    {
        nRes = fcom_GetReportFile(repFullPath, "body_en.html", repBodyBuf);
        WRITE_INFO(CATEGORY_INFO,"2-3. Read 'body_en.html', nRes(%d)size(%d)",nRes, bodyTotSize);
    }

    nRes = freport_SetDataLoop(repBodyBuf, tmpBodyLoop);
    WRITE_INFO(CATEGORY_INFO,"2-4. Set body loop, nRes(%d) ",nRes );

    footerSize = fcom_GetFileSize(repFullPath, "footer.html");
    footerTotSize = footerSize + strlen(tmpFooterLoop) + 64; //footer.html + title

    if(fcom_malloc((void**)&repFooterBuf, sizeof(char)*footerTotSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    nRes = fcom_GetReportFile(repFullPath, "footer.html", repFooterBuf);

    WRITE_INFO(CATEGORY_INFO,"2-5. Read 'footer.html', nRes(%d)size(%d)",nRes,footerSize );

    nRes = freport_SetDataLoop(repFooterBuf, tmpFooterLoop);
    WRITE_INFO(CATEGORY_INFO,"2-6. Set footer loop, nRes(%d) ",nRes );

    WRITE_INFO(CATEGORY_INFO,"2. Make html file " );

    nRes = 0;


    nRes += fcom_ReportHeaderFilter(repHeaderBuf,
                                 repChartMinJsBuf,
                                 repUtilsJsBuf,
                                 headerTotSize);
    WRITE_INFO(CATEGORY_INFO,"          3-1. Put header js, nRes(%d) ", nRes );

    fcom_MallocFree((void**)&repChartMinJsBuf); //free
    fcom_MallocFree((void**)&repUtilsJsBuf); //free

    memset(groupStr, 0x00, sizeof(groupStr));
    if(groupSq != 0)
    {
        fdb_GetGroupName(groupSq, groupStr);
    }

    nRes += fcom_ReportBodyFilter( repBodyBuf,
                                tmpRange,
                                tmpTable,
                                bDate,
                                bCustom,
                                bodyTotSize,
                                groupStr,
                                g_stProcReportInfo.szConfMailLang
                                );
    WRITE_INFO(CATEGORY_INFO,"3-2. Put body data, nRes(%d) ",nRes );

    nRes += fcom_ReportFooterFilter(repFooterBuf, footerTotSize,g_stProcReportInfo.szConfMailLang);

    WRITE_INFO(CATEGORY_INFO,"3-3. Put footer data, nRes(%d) ",nRes );
    WRITE_INFO(CATEGORY_INFO,"3. Put html template " );

    WRITE_INFO(CATEGORY_INFO,"4. Make mail file " );

    if(fcom_malloc((void**)&repResultBuf, sizeof(char)*(nRes+nTotChartSize+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    strcat(repResultBuf, repHeaderBuf);
    fcom_MallocFree((void**)&repHeaderBuf);
    strcat(repResultBuf, repBodyBuf);
    fcom_MallocFree((void**)&repBodyBuf); //free
    if(bCustom != 0)
    {
        strcat(repResultBuf, tmpChart);
    }
    else
    {
        strcat(repResultBuf, tmpChart);
    }
    strcat(repResultBuf, repFooterBuf);

    fcom_MallocFree((void**)&repFooterBuf); //free
    WRITE_INFO(CATEGORY_INFO," repResultBuf(%s) ",repResultBuf);
    printf(" repResultBuf(%s) ",repResultBuf);

    currTime = time((time_t) 0);
    memset(currDate, 0x00, sizeof(currDate));
    fcom_time2str(currTime, currDate, "YYYYMMDD\0");
    memset(currYear, 0x00, sizeof(currYear));

    fcom_time2str(currTime, currYear, "YYYY\0");

    memset(currMonth, 0x00, sizeof(currMonth));
    fcom_time2str(currTime, currMonth, "MM\0");

    //폴더없으면 만들기
    memset(tmpHtmlPath, 0x00, sizeof(tmpHtmlPath));
    sprintf(tmpHtmlPath, "%s/html/%s/%s/", repPath,currYear,currMonth);
    if (access(tmpHtmlPath, W_OK) != 0)
    {

        rxt = fcom_MkPath(tmpHtmlPath, 0755);
        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in make path(%s) ",tmpHtmlPath );
//            fcom_BufferFree(repResultBuf);
            fcom_MallocFree((void**)&repResultBuf);
            return -1;
        }
        else
        {
            chmod(tmpHtmlPath, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
            WRITE_INFO(CATEGORY_INFO,"Succeed in make path(%s) ",tmpHtmlPath );
        }
    }

    if(bCustom != 0) //커스텀이라면
    {
        memset(sDateRepl, 0x00, sizeof(sDateRepl));
        memset(eDateRepl, 0x00, sizeof(eDateRepl));

        fcom_ReplaceAll(sDate, "-", "", sDateRepl);
        fcom_ReplaceAll(eDate, "-", "", eDateRepl);

        sprintf(repHtmlPath, "%s%s_%s_DAP_REPORT_%s-%s.html",
                tmpHtmlPath,mngId,currDate,sDateRepl,eDateRepl);
    }
    else
    {
        if(bDate == 1)
            sprintf(repHtmlPath, "%s%s_DAP_DAY_REPORT.html",
                    tmpHtmlPath,currDate);
        else if(bDate == 2)
            sprintf(repHtmlPath, "%s%s_DAP_WEEK_REPORT.html",
                    tmpHtmlPath,currDate);
        else if(bDate == 3)
            sprintf(repHtmlPath, "%s%s_DAP_MONTH_REPORT.html",
                    tmpHtmlPath,currDate);
    }
    fp = fopen(repHtmlPath, "wb");
    if(fp == NULL)
    {
        WRITE_INFO(CATEGORY_INFO,"Fail in fopen, path(%s)",repHtmlPath );
        fcom_MallocFree((void**)&repResultBuf);
        return -1;
    }

    nRes = fwrite(repResultBuf, strlen(repResultBuf), 1, fp);
    if(nRes < 0)
    {
        fclose(fp);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fwrite, path(%s)",repHtmlPath );

        if(repResultBuf != NULL)
        {
            fcom_MallocFree((void**)&repResultBuf);
        }

        return -1;
    }

    fcom_MallocFree((void**)&repResultBuf);

    fclose(fp);
    WRITE_INFO(CATEGORY_INFO,"4. Make mail file " );

    return 0;

}


//int freport_MakeTableDay(char *tmpRes, int sDayVal[][3], int uDayVal[][3])
int freport_MakeTableDay(char *tmpRes, int uDayVal[][3])
{
    int i;
//    int	 sDayTotSum = 0;
//    int	 sDayWarnSum = 0;
//    int	 sDayCritSum = 0;
//    int	 sDayBlokSum = 0;
    int	 uDayTotSum = 0;
    int	 uDayWarnSum = 0;
    int	 uDayCritSum = 0;
    int	 uDayBlokSum = 0;
    char tmpBuf[1024*50];
    char cTitle[64];
//    char sTitle[64];
    char uTitle[64];
    char strTime[10];
    char strWarning[10];
    char strCritical[10];
    char strBlock[10];
    char strSum[10];
    char strFontWarn[50];
    char strFontCrit[50];
    char strFontBlok[50];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        if(!strncmp(fcom_GetCustLang(), "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"시간");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle,		"Event Detail Info");
            strcpy(uTitle,		"Unsolved Event");
//            strcpy(sTitle,		"Solved Event");
            strcpy(strTime,		"TIME");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"시간");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle,		"Event Detail Info");
            strcpy(uTitle,		"Unsolved Event");
//            strcpy(sTitle,		"Solved Event");
            strcpy(strTime,		"TIME");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }

    memset(tmpBuf, 0x00, sizeof(tmpBuf));
//    sprintf(tmpBuf,
//            "<b>%s</b><br><br><table width=\"100%%\" "
//            "class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th><th colspan=4 class=\"center\">%s</th></tr><tr>"
//            "<th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th>"
//            "<th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th></tr>",
//            cTitle,uTitle,sTitle,strTime,strWarning,strCritical,strBlock,strSum,strWarning,strCritical,strBlock,strSum);
    sprintf(tmpBuf,
            "<b>%s</b><br><br><table width=\"100%%\" "
            "class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th></tr>"
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th>"
            "</tr>",
            cTitle, uTitle, strTime, strWarning, strCritical, strBlock, strSum);

    for(i=0; i<24; i++)
    {
        //시간
        if(i < 10)
            sprintf(tmpBuf+strlen(tmpBuf), "<tr><td class=\"center\">0%d</td>", i);
        else
            sprintf(tmpBuf+strlen(tmpBuf), "<tr><td class=\"center\">%d</td>", i);

        // 이벤트발생
        memset(strFontWarn, 0x00, sizeof(strFontWarn));
        if(uDayVal[i][0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uDayVal[i][0]);
        else
            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", uDayVal[i][0]);

        memset(strFontCrit, 0x00, sizeof(strFontCrit));
        if(uDayVal[i][1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uDayVal[i][1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uDayVal[i][1]);

        memset(strFontBlok, 0x00, sizeof(strFontBlok));
        if(uDayVal[i][2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uDayVal[i][2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uDayVal[i][2]);

        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", uDayVal[i][0]+uDayVal[i][1]+uDayVal[i][2]);

        // 이벤트해지
//        memset(strFontWarn, 0x00, sizeof(strFontWarn));
//        if(sDayVal[i][0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sDayVal[i][0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", sDayVal[i][0]);
//        memset(strFontCrit, 0x00, sizeof(strFontCrit));
//        if(sDayVal[i][1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sDayVal[i][1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sDayVal[i][1]);
//        memset(strFontBlok, 0x00, sizeof(strFontBlok));
//        if(sDayVal[i][2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sDayVal[i][2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sDayVal[i][2]);

//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);

//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", sDayVal[i][2]+sDayVal[i][1]+sDayVal[i][2]);

        strcat(tmpBuf, "</tr>");
        uDayWarnSum += uDayVal[i][0];
        uDayCritSum += uDayVal[i][1];
        uDayBlokSum += uDayVal[i][2];
//        sDayWarnSum += sDayVal[i][0];
//        sDayCritSum += sDayVal[i][1];
//        sDayBlokSum += sDayVal[i][2];
    }
    uDayTotSum = uDayWarnSum + uDayCritSum + uDayBlokSum;
//    sDayTotSum = sDayWarnSum + sDayCritSum + sDayBlokSum;
//    sprintf(tmpBuf+strlen(tmpBuf),
//            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th>"
//            "<th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\">"
//            "<font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th>"
//            "<th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th></tr>",
//            strSum,uDayWarnSum,uDayCritSum,uDayBlokSum,uDayTotSum,sDayWarnSum,sDayCritSum,sDayBlokSum,sDayTotSum);
    sprintf(tmpBuf+strlen(tmpBuf),
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\">"
            "<font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th>"
            "</tr>",
            strSum,
            uDayWarnSum,
            uDayCritSum,
            uDayBlokSum,
            uDayTotSum);

    strcat(tmpBuf, "</table>");

    strcpy(tmpRes, tmpBuf);

    return strlen(tmpRes);
}

//int freport_MakeTableWeek(char *tmpRes, char day[][11], int sWeekVal[][3], int uWeekVal[][3])
int freport_MakeTableWeek(char *tmpRes, char day[][11], int uWeekVal[][3])
{
    int i;
//    int	 sWeekTotSum = 0;
//    int	 sWeekWarnSum = 0;
//    int	 sWeekCritSum = 0;
//    int	 sWeekBlokSum = 0;
    int	 uWeekTotSum = 0;
    int	 uWeekWarnSum = 0;
    int	 uWeekCritSum = 0;
    int	 uWeekBlokSum = 0;
    char tmpBuf[1024*50];
    char cTitle[64];
//    char sTitle[64];
    char uTitle[64];
    char strTime[10];
    char strWarning[10];
    char strCritical[10];
    char strBlock[10];
    char strSum[10];
    char strFontWarn[50];
    char strFontCrit[50];
    char strFontBlok[50];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        if(!strncmp(fcom_GetCustLang(), "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"일자");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle, 		"Event Detail Info");
            strcpy(uTitle, 		"Unsolved Event");
//            strcpy(sTitle, 		"Solved Event");
            strcpy(strTime,		"DAY");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"일자");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle, 		"Event Detail Info");
            strcpy(uTitle, 		"Unsolved Event");
//            strcpy(sTitle, 		"Solved Event");
            strcpy(strTime,		"DAY");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }

    memset(tmpBuf, 0x00, sizeof(tmpBuf));
//    sprintf(tmpBuf, "<b>%s</b><br><br><table width=\"100%%\" class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th><th colspan=4 class=\"center\">%s</th></tr><tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th></tr>",
//            cTitle,uTitle,sTitle,strTime,strWarning,strCritical,strBlock,strSum,strWarning,strCritical,strBlock,strSum);
    sprintf(tmpBuf,
            "<b>%s</b><br><br><table width=\"100%%\" "
            "class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th></tr>"
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th>"
            "</tr>",
            cTitle, uTitle, strTime, strWarning, strCritical, strBlock, strSum);

    for(i=0; i<7; i++)
    {
        sprintf(tmpBuf+strlen(tmpBuf), "<tr><td class=\"center\">%s</td>", day[i]);
        // 이벤트발생
        memset(strFontWarn, 0x00, sizeof(strFontWarn));
        if(uWeekVal[i][0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uWeekVal[i][0]);
        else
            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", uWeekVal[i][0]);
        memset(strFontCrit, 0x00, sizeof(strFontCrit));
        if(uWeekVal[i][1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uWeekVal[i][1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uWeekVal[i][1]);
        memset(strFontBlok, 0x00, sizeof(strFontBlok));
        if(uWeekVal[i][2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uWeekVal[i][2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uWeekVal[i][2]);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", uWeekVal[i][0]+uWeekVal[i][1]+uWeekVal[i][2]);

        // 이벤트해지
//        memset(strFontWarn, 0x00, sizeof(strFontWarn));
//        if(sWeekVal[i][0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sWeekVal[i][0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", sWeekVal[i][0]);
//        memset(strFontCrit, 0x00, sizeof(strFontCrit));
//        if(sWeekVal[i][1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sWeekVal[i][1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sWeekVal[i][1]);
//        memset(strFontBlok, 0x00, sizeof(strFontBlok));
//        if(sWeekVal[i][2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sWeekVal[i][2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sWeekVal[i][2]);
//
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", sWeekVal[i][0]+sWeekVal[i][1]+sWeekVal[i][2]);
        strcat(tmpBuf, "</tr>");
        uWeekWarnSum += uWeekVal[i][0];
        uWeekCritSum += uWeekVal[i][1];
        uWeekBlokSum += uWeekVal[i][2];
//        sWeekWarnSum += sWeekVal[i][0];
//        sWeekCritSum += sWeekVal[i][1];
//        sWeekBlokSum += sWeekVal[i][2];
    }
    uWeekTotSum = uWeekWarnSum + uWeekCritSum + uWeekBlokSum;
//    sWeekTotSum = sWeekWarnSum + sWeekCritSum + sWeekBlokSum;
//    sprintf(tmpBuf+strlen(tmpBuf),
//            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th></tr>", strSum,uWeekWarnSum,uWeekCritSum,uWeekBlokSum,uWeekTotSum,sWeekWarnSum,sWeekCritSum,sWeekBlokSum,sWeekTotSum);
    sprintf(tmpBuf+strlen(tmpBuf),
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\">"
            "<font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th>"
            "</tr>",
            strSum,
            uWeekWarnSum,
            uWeekCritSum,
            uWeekBlokSum,
            uWeekTotSum);
    strcat(tmpBuf, "</table>");

    strcpy(tmpRes, tmpBuf);

    return strlen(tmpRes);
}

//int freport_MakeTableMonth(char *tmpRes, char day[][11], int sMonthVal[][3], int uMonthVal[][3], int monthNum)
int freport_MakeTableMonth(char *tmpRes, char day[][11], int uMonthVal[][3], int monthNum)
{
    int i;
//    int	 sMonthTotSum = 0;
//    int	 sMonthWarnSum = 0;
//    int	 sMonthCritSum = 0;
//    int	 sMonthBlokSum = 0;
    int	 uMonthTotSum = 0;
    int	 uMonthWarnSum = 0;
    int	 uMonthCritSum = 0;
    int	 uMonthBlokSum = 0;
    char tmpBuf[1024*50];
    char cTitle[64];
//    char sTitle[64];
    char uTitle[64];
    char strTime[10];
    char strWarning[10];
    char strCritical[10];
    char strBlock[10];
    char strSum[10];
    char strFontWarn[50];
    char strFontCrit[50];
    char strFontBlok[50];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        if(!strncmp(fcom_GetCustLang(), "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"일자");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle, 		"Event Detail Info");
            strcpy(uTitle, 		"Unsolved Event");
//            strcpy(sTitle, 		"Solved Event");
            strcpy(strTime,		"DAY");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            strcpy(strTime,		"일자");
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle, 		"Event Detail Info");
            strcpy(uTitle, 		"Unsolved Event");
//            strcpy(sTitle, 		"Solved Event");
            strcpy(strTime,		"DAY");
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }

    memset(tmpBuf, 0x00, sizeof(tmpBuf));
//    sprintf(tmpBuf, "<b>%s</b><br><br><table width=\"100%%\" class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th><th colspan=4 class=\"center\">%s</th></tr><tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th></tr>",
//            cTitle,uTitle,sTitle,strTime,strWarning,strCritical,strBlock,strSum,strWarning,strCritical,strBlock,strSum);
    sprintf(tmpBuf,
            "<b>%s</b><br><br><table width=\"100%%\" "
            "class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th></tr>"
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th>"
            "</tr>",
            cTitle, uTitle, strTime, strWarning, strCritical, strBlock, strSum);

    for(i=0; i<monthNum; i++)
    {
        sprintf(tmpBuf+strlen(tmpBuf), "<tr><td class=\"center\">%s</td>", day[i]);
        // 이벤트발생
        memset(strFontWarn, 0x00, sizeof(strFontWarn));
        if(uMonthVal[i][0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uMonthVal[i][0]);
        else
            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", uMonthVal[i][0]);
        memset(strFontCrit, 0x00, sizeof(strFontCrit));
        if(uMonthVal[i][1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uMonthVal[i][1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uMonthVal[i][1]);
        memset(strFontBlok, 0x00, sizeof(strFontBlok));
        if(uMonthVal[i][2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uMonthVal[i][2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uMonthVal[i][2]);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", uMonthVal[i][0]+uMonthVal[i][1]+uMonthVal[i][2]);
        // 이벤트해지
//        memset(strFontWarn, 0x00, sizeof(strFontWarn));
//        if(sMonthVal[i][0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sMonthVal[i][0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#fla836\">%d</font>", sMonthVal[i][0]);
//        memset(strFontCrit, 0x00, sizeof(strFontCrit));
//        if(sMonthVal[i][1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sMonthVal[i][1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sMonthVal[i][1]);
//        memset(strFontBlok, 0x00, sizeof(strFontBlok));
//        if(sMonthVal[i][2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sMonthVal[i][2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sMonthVal[i][2]);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", sMonthVal[i][0]+sMonthVal[i][1]+sMonthVal[i][2]);

        strcat(tmpBuf, "</tr>");
        uMonthWarnSum += uMonthVal[i][0];
        uMonthCritSum += uMonthVal[i][1];
        uMonthBlokSum += uMonthVal[i][2];
//        sMonthWarnSum += sMonthVal[i][0];
//        sMonthCritSum += sMonthVal[i][1];
//        sMonthBlokSum += sMonthVal[i][2];
    }
    uMonthTotSum = uMonthWarnSum + uMonthCritSum + uMonthBlokSum;
//    sMonthTotSum = sMonthWarnSum + sMonthCritSum + sMonthBlokSum;
//    sprintf(tmpBuf+strlen(tmpBuf),
//            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th></tr>", strSum,uMonthWarnSum,uMonthCritSum,uMonthBlokSum,uMonthTotSum,sMonthWarnSum,sMonthCritSum,sMonthBlokSum,sMonthTotSum);
    sprintf(tmpBuf+strlen(tmpBuf),
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\">"
            "<font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th>"
            "</tr>",
            strSum,
            uMonthWarnSum,
            uMonthCritSum,
            uMonthBlokSum,
            uMonthTotSum);

    strcat(tmpBuf, "</table>");

    strcpy(tmpRes, tmpBuf);

    return strlen(tmpRes);
}

//int freport_MakeTableCustom(char *tmpRes, char time[][14], int sCustVal[][3], int uCustVal[][3], int valueCnt, int bTime)
int freport_MakeTableCustom(char *tmpRes, char time[][14], int uCustVal[][3], int valueCnt, int bTime)
{
    int i;
//    int	 sCustTotSum = 0;
//    int	 sCustWarnSum = 0;
//    int	 sCustCritSum = 0;
//    int	 sCustBlokSum = 0;
    int	 uCustTotSum = 0;
    int	 uCustWarnSum = 0;
    int	 uCustCritSum = 0;
    int	 uCustBlokSum = 0;
    char tmpBuf[1024*50];
    char cTitle[64];
//    char sTitle[64];
    char uTitle[64];
    char strTime[10];
    char strWarning[10];
    char strCritical[10];
    char strBlock[10];
    char strSum[10];
    char strFontWarn[50];
    char strFontCrit[50];
    char strFontBlok[50];

    if(strlen(fcom_GetCustLang()) > 0)
    {
        if(!strncmp(fcom_GetCustLang(), "kr", 2))
        {
            strcpy(cTitle, "[ 이벤트 상세 정보 ]");
            strcpy(uTitle, "이벤트 발생");
//            strcpy(sTitle, "이벤트 해지");
            if(bTime == 1)
            {
                strcpy(strTime,		"시간");
            }
            else
            {
                strcpy(strTime,		"일자");
            }
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(cTitle, "Event Detail Info");
            strcpy(uTitle, "Unsolved Event");
//            strcpy(sTitle, "Solved Event");
            if(bTime == 1)
            {
                strcpy(strTime,		"TIME");
            }
            else
            {
                strcpy(strTime,		"DAY");
            }
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            strcpy(cTitle,		"[ 이벤트 상세 정보 ]");
            strcpy(uTitle,		"이벤트 발생");
//            strcpy(sTitle,		"이벤트 해지");
            if(bTime == 1)
            {
                strcpy(strTime,		"시간");
            }
            else
            {
                strcpy(strTime,		"일자");
            }
            strcpy(strWarning,	"경고");
            strcpy(strCritical,	"위험");
            strcpy(strBlock,	"차단");
            strcpy(strSum,		"합계");
        }
        else
        {
            strcpy(uTitle, "Unsolved Event");
//            strcpy(sTitle, "Solved Event");
            strcpy(cTitle, "Event Detail Info");
            if(bTime == 1)
            {
                strcpy(strTime,		"TIME");
            }
            else
            {
                strcpy(strTime,		"DAY");
            }
            strcpy(strWarning,	"WARNING");
            strcpy(strCritical,	"CRITICAL");
            strcpy(strBlock,	"BLOCK");
            strcpy(strSum,		"SUM");
        }
    }

    memset(tmpBuf, 0x00, sizeof(tmpBuf));
//    sprintf(tmpBuf,
//            "<b>%s</b><br><br><table width=\"100%%\" class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th><th colspan=4 class=\"center\">%s</th></tr><tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th><th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th></tr>",
//            cTitle,uTitle,sTitle,strTime,strWarning,strCritical,strBlock,strSum,strWarning,strCritical,strBlock,strSum);
    sprintf(tmpBuf,
            "<b>%s</b><br><br><table width=\"100%%\" "
            "class=table6_4><tr><th></th><th colspan=4 class=\"center\">%s</th></tr>"
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%s</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%s</font></th><th class=\"center\"><font color=\"#b150c5\">%s</font></th><th class=\"center\">%s</th>"
            "</tr>",
            cTitle, uTitle, strTime, strWarning, strCritical, strBlock, strSum);

    for(i=0; i<valueCnt; i++)
    {
        sprintf(tmpBuf+strlen(tmpBuf), "<tr><td class=\"center\">%s</td>", time[i]);
        // 이벤트발생
        memset(strFontWarn, 0x00, sizeof(strFontWarn));
        if(uCustVal[i][0] == 0)
            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", uCustVal[i][0]);
        else
            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", uCustVal[i][0]);
        memset(strFontCrit, 0x00, sizeof(strFontCrit));
        if(uCustVal[i][1] == 0)
            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", uCustVal[i][1]);
        else
            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", uCustVal[i][1]);
        memset(strFontBlok, 0x00, sizeof(strFontBlok));
        if(uCustVal[i][2] == 0)
            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", uCustVal[i][2]);
        else
            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", uCustVal[i][2]);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", uCustVal[i][0]+uCustVal[i][1]+uCustVal[i][2]);

        // 이벤트해지
//        memset(strFontWarn, 0x00, sizeof(strFontWarn));
//        if(sCustVal[i][0] == 0)
//            sprintf(strFontWarn, "<font color=\"#cccccc\">%d</font>", sCustVal[i][0]);
//        else
//            sprintf(strFontWarn, "<font color=\"#f1a836\">%d</font>", sCustVal[i][0]);
//        memset(strFontCrit, 0x00, sizeof(strFontCrit));
//        if(sCustVal[i][1] == 0)
//            sprintf(strFontCrit, "<font color=\"#cccccc\">%d</font>", sCustVal[i][1]);
//        else
//            sprintf(strFontCrit, "<font color=\"#ed2a5b\">%d</font>", sCustVal[i][1]);
//        memset(strFontBlok, 0x00, sizeof(strFontBlok));
//        if(sCustVal[i][2] == 0)
//            sprintf(strFontBlok, "<font color=\"#cccccc\">%d</font>", sCustVal[i][2]);
//        else
//            sprintf(strFontBlok, "<font color=\"#b150c5\">%d</font>", sCustVal[i][2]);

//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontWarn);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontCrit);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%s</td>", strFontBlok);
//        sprintf(tmpBuf+strlen(tmpBuf),"<td class=\"center\">%d</td>", sCustVal[i][0]+sCustVal[i][1]+sCustVal[i][2]);

        strcat(tmpBuf, "</tr>");
        uCustWarnSum += uCustVal[i][0];
        uCustCritSum += uCustVal[i][1];
        uCustBlokSum += uCustVal[i][2];

//        sCustWarnSum += sCustVal[i][0];
//        sCustCritSum += sCustVal[i][1];
//        sCustBlokSum += sCustVal[i][2];

    }
    uCustTotSum = uCustWarnSum + uCustCritSum + uCustBlokSum;
//    sCustTotSum = sCustWarnSum + sCustCritSum + sCustBlokSum;
//    sprintf(tmpBuf+strlen(tmpBuf),
//            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th><th class=\"center\"><font color=\"#fla836\">%d</font></th><th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\"><font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th></tr>", strSum,uCustWarnSum,uCustCritSum,uCustBlokSum,uCustTotSum,sCustWarnSum,sCustCritSum,sCustBlokSum,sCustTotSum);
    sprintf(tmpBuf+strlen(tmpBuf),
            "<tr><th class=\"center\">%s</th><th class=\"center\"><font color=\"#fla836\">%d</font></th>"
            "<th class=\"center\"><font color=\"#ed2a5b\">%d</font></th><th class=\"center\">"
            "<font color=\"#b150c5\">%d</font></th><th class=\"center\">%d</th>"
            "</tr>",
            strSum,
            uCustWarnSum,
            uCustCritSum,
            uCustBlokSum,
            uCustTotSum);


    strcat(tmpBuf, "</table>");
    strcpy(tmpRes, tmpBuf);

    return strlen(tmpRes);
}
