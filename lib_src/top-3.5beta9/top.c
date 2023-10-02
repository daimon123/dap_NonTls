
#include "os.h"
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/time.h>

/* includes specific to top */
#include "top.h"
#include "top.local.h"
#include "boolean.h"
#include "machine.h"
#include "utils.h"
#include	<common.h>


t_systemLoad	systemload;

/* internal routines */
void quit();

main(argc, argv)

int  argc;
char *argv[];

{
    register int i;
    register int active_procs;

    struct system_info system_info;
    struct statics statics;
    caddr_t processes;

    int delay = Default_DELAY;
	int loop;

#if Default_TOPN == Infinity
#endif
    char ch;
    char *iptr;
    struct process_select ps;
#ifdef ORDER
    char *order_name = NULL;
    int order_index = 0;
#endif

	connect_db();

	/* kim */
	delay	=	10;

    /* initialize the kernel memory interface */
    if (machine_init(&statics) == -1)
    {
		perror("mainchin init");
		exit(1);
    }

	i = 0;
    while (1)
    {

	printf(" %x \n", time(0) );
	/* get the current stats */
	get_system_info(&system_info);

	p_memory(system_info.memory,&systemload );

	if(i > 0)
	p_cpustaus(system_info.cpustates, &systemload);
	else
		i = 1;

	sleep(5);
	systemload.mgwid = 1; 
	update_sysInfo(&systemload);

    }

    quit(0);
    /*NOTREACHED*/
}

void quit(status)		/* exit under duress */
int status;

{
    exit(status);
;
    /*NOTREACHED*/
}

p_memory(stats, data)
int *stats;
t_systemLoad *data;
{

	data->mem_tot = atoi(format_k(stats[0]));
	data->mem_free = atoi(format_k(stats[2]));
	data->mem_used = data->mem_tot - data->mem_free;
/*

	printf(" MEMORY REAL = [%05s] FREE =[%05s] SWIN = [%05s] SWFE = [%05s]\n", 
	format_k(stats[0]),
	format_k(stats[2]),
	format_k(stats[3]),
	format_k(stats[4]) );
*/

}

p_cpustaus(states, data)
register int *states;
t_systemLoad *data;
{
	data->cpu_idle = states[0] / 10;
	data->cpu_user = states[1] / 10;
	data->cpu_sys = states[2] / 10;
/*
	printf("CPU IDLE =[%4.1f] USER = [[%4.1f] KER = [%4.1f] iowait = [%4.1f] \n",

	((float)states[0])/10.,
	((float)states[1])/10.,
	((float)states[2])/10.,
	((float)states[3])/10.,
	((float)states[4])/10. );

	printf("%d %d %d %d \n", states[0], states[1],states[2],states[3],states[4]);
*/

}

