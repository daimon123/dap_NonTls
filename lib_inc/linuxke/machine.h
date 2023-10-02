/*
 *  This file defines the interface between top and the machine-dependent
 *  module.  It is NOT machine dependent and should not need to be changed
 *  for any specific machine.
 */
#ifndef MACHINE_H
#define MACHINE_H

#include <sys/param.h>		/* for HZ */


#include "linuxke/page.h"   /* for PAGE_SHIFT */
#include "linuxke/top.h"
/*
 * the statics struct is filled in by machine_init
 */
struct statics
{
    char **procstate_names;
    char **cpustate_names;
    char **memory_names;
#ifdef ORDER
    char **order_names;
#endif
};

/*
 * the system_info struct is filled in by a machine dependent routine.
 */

struct system_info
{
    int    last_pid;
    double load_avg[NUM_AVERAGES];
    int    p_total;
    int    p_active;     /* number of procs considered "active" */
    int    *procstates;
    int *cpustates;
    int    *memory;
};

/* cpu_states is an array of percentages * 10.  For example, 
   the (integer) value 105 is 10.5% (or .105).
 */

/*
 * the process_select struct tells get_process_info what processes we
 * are interested in seeing
 */

struct process_select
{
    int idle;		/* show idle processes */
    int system;		/* show system processes */
    int uid;		/* only this uid (unless uid == -1) */
    char *command;	/* only this command (unless == NULL) */
};


typedef struct
{
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    char name[64];
    //char arg[120];
    char arg[1024];
    int pri, nice;
    unsigned long size, rss;	
    int state;
    unsigned long time;
    unsigned long stime;
    double pcpu, wcpu;
}proc_info;

char *format_header();
/*char *format_next_process();*/
//void read_one_proc_stat(pid_t pid, struct top_proc *proc);

/* non-int routines typically used by the machine dependent module */
char *printable();

#endif
