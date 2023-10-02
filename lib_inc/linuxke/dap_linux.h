//
// Created by KimByoungGook on 2020-06-12.
//

#ifndef DAP_LINUX_H
#define DAP_LINUX_H
#include <stdio.h>
#include <unistd.h>

#include "linuxke/tasks.h"
#include "linuxke/machine.h"

#define PROC_SUPER_MAGIC 0x9fa0
#define PROCFS "/proc"

extern uid_t proc_owner(pid_t pid);

struct top_proc
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
};

/*=STATE IDENT STRINGS==================================================*/

#define NPROCSTATES 7


/*=SYSTEM STATE INFO====================================================*/


#define NCPUSTATES 4
#define NMEMSTATS 6
#define NUM_STRINGS 8


/* usefull macros */
#define bytetok(x)	(((x) + 512) >> 10)
#define pagetok(x)	((x) << (PAGE_SHIFT - 10))
#define HASH(x)		(((x) * 1686629713U) % HASH_SIZE)
#define HASH_SIZE	(NR_TASKS * 3 / 2)

/*======================================================================*/

/* these are for calculating cpu state percentages */

long cp_time[NCPUSTATES];
long cp_old[NCPUSTATES];
long cp_diff[NCPUSTATES];

/* for calculating the exponential average */

struct timeval lasttime;

/* these are for keeping track of processes */


struct top_proc ptable[HASH_SIZE];
struct top_proc *pactive[NR_TASKS];
struct top_proc **nextactive;

/* these are for passing data back to the machine independant portion */

int cpu_states[NCPUSTATES];
int process_states[NPROCSTATES];
int memory_stats[NMEMSTATS];

int machine_init(struct statics *statics);
void get_system_info(struct system_info *info);
int read_one_proc_stat(pid_t pid, struct top_proc *proc);
caddr_t get_process_info(struct system_info *si,
                         struct process_select *sel,
                         int (*compare)());
char* format_header(char *uname_field);
char* format_next_process(char* (*get_userid)());
int proc_compare (struct top_proc **pp1,struct top_proc **pp2);
uid_t proc_owner(pid_t pid);






#endif //DAP_LINUX_H
