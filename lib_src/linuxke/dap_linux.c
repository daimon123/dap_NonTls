/*
 * top - a top users display for Unix
 *
 * SYNOPSIS:  Linux 1.2.x, 1.3.x, using the /proc filesystem
 *
 * DESCRIPTION:
 * This is the machine-dependent module for Linux 1.2.x or 1.3.x.
 *
 * LIBS:
 *
 * CFLAGS: -DHAVE_GETOPT
 *
 * AUTHOR: Richard Henderson <rth@tamu.edu>
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ifaddrs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "linuxke/dap_linux.h"
#include "linuxke/machine.h"
#include "linuxke/utils.h"
#include "com/dap_com.h"



static char *state_abbrev[NPROCSTATES+1] =
{
        "", "run", "sleep", "disk", "zomb", "stop", "swap",
        NULL
};

static char *procstatenames[NPROCSTATES+1] =
{
        "", " running, ", " sleeping, ", " uninterruptable, ",
        " zombie, ", " stopped, ", " swapping, ",
        NULL
};


static char *cpustatenames[NCPUSTATES+1] =
{
        "user", "nice", "system", "idle",
        NULL
};


static char *memorynames[NMEMSTATS+1] =
{
        "K used, ", "K free, ", "K shd, ", "K buf  Swap: ",
        "K used, ", "K free",
        NULL
};

static char fmt_header[] =
        "  PID X        PRI NICE  SIZE   RES STATE   TIME   WCPU    CPU COMMAND";



static inline char* skip_ws(const char *p);
static inline char* skip_token(const char *p);


static inline char* skip_ws(const char *p)
{
    while (isspace(*p)) p++;
    return (char *)p;
}

static inline char* skip_token(const char *p)
{
    while (isspace(*p)) p++;

    while (*p && !isspace(*p)) p++;

    return (char *)p;
}

char *strecpy(register char *to,register char * from)
{
    while ((*to++ = *from++) != '\0');
    return(--to);
}

int machine_init(struct statics *statics)
{
    WRITE_INFO(CATEGORY_DEBUG,"Function Start " );
    /* make sure the proc filesystem is mounted */
    {
        struct statfs sb;
        if (statfs(PROCFS, &sb) < 0 || sb.f_type != PROC_SUPER_MAGIC)
        {
            fprintf(stderr, "linux: proc filesystem not mounted on " PROCFS "\n");
            return -1;
        }
    }

    /* chdir to the proc filesystem to make things easier */
    chdir(PROCFS);

    /* initialize the process hash table */
    {
        int i;
        for (i = 0; i < HASH_SIZE; ++i)
            ptable[i].pid = -1;
    }
    /* fill in the statics information */
    statics->procstate_names = procstatenames;
    statics->cpustate_names = cpustatenames;
    statics->memory_names = memorynames;
    WRITE_INFO(CATEGORY_DEBUG,"Function End " );
    /* all done! */
    return 0;
}

void get_system_info(struct system_info *info)
{
    char buffer[4096+1];
    int fd, len;
    char *p;

    chdir("/proc");
    /* get load averages */
    {
        fd = open("loadavg", O_RDONLY);
        len = read(fd, buffer, sizeof(buffer)-1);

        close(fd);
        buffer[len] = '\0';

        info->load_avg[0] = strtod(buffer, &p);
        info->load_avg[1] = strtod(p, &p);
        info->load_avg[2] = strtod(p, &p);
        p = skip_token(p);			/* skip running/tasks */
        p = skip_ws(p);
        if (*p)
            info->last_pid = atoi(p);
        else
            info->last_pid = -1;
    }

    /* get the cpu time info */
    {
        fd = open("stat", O_RDONLY);
        len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);
        buffer[len] = '\0';

        p = skip_token(buffer);			/* "cpu" */
        cp_time[0] = strtoul(p, &p, 0);
        cp_time[1] = strtoul(p, &p, 0);
        cp_time[2] = strtoul(p, &p, 0);
        cp_time[3] = strtoul(p, &p, 0);

        /* convert cp_time counts to percentages */
        Percentages(4, cpu_states, cp_time, cp_old, cp_diff);
    }

    /* get system wide memory usage */
    {
        char *p;

        fd = open("meminfo", O_RDONLY);
        len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);
        buffer[len] = '\0';

        p = strstr(buffer,"MemTotal:");
        p = skip_token(p);
        memory_stats[0] = strtoul(p, &p, 10);   /* MemTotal */

        p = strstr(buffer,"MemFree:");
        p = skip_token(p);
        memory_stats[1] = strtoul(p, &p, 10);  /* MemFree */

        p = strstr(buffer,"MemAvailable:");
        p = skip_token(p);
        memory_stats[2] = strtoul(p, &p, 10);  /* MemFree */

        p = strstr(buffer,"SwapTotal:");
        p = skip_token(p);          /* SwapTotal */
        memory_stats[3] = strtoul(p, &p, 10);

        p = strstr(buffer,"SwapFree:");
        p = skip_token(p);          /* SwapFree */
        memory_stats[4] = strtoul(p, &p, 10);

        memory_stats[0] = bytetok(memory_stats[0]);
        memory_stats[1] = bytetok(memory_stats[1]);
        memory_stats[2] = bytetok(memory_stats[2]);
        memory_stats[3] = bytetok(memory_stats[3]);
        //memory_stats[4] = bytetok(memory_stats[4]);
        //memory_stats[5] = bytetok(memory_stats[5]);
    }

    /* set arrays and strings */
    info->cpustates = cpu_states;
    info->memory = memory_stats;
}


int read_one_proc_stat(pid_t pid, struct top_proc *proc)
{
    char buffer[4096], *p;
    int loop;
    char filebuf[4096];

    /* grab the proc stat info in one go */
    {
        int fd, len;

        /* proc args resd */
        memset(filebuf, 0x00, sizeof(filebuf));
        sprintf(filebuf, "%d/cmdline", pid);
        fd = open(filebuf, O_RDONLY);
        if(fd == (-1))
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"File pid(%d) Open Failed [%d:%s] ",pid,errno,strerror(errno));
            return 0;
        }
        memset(buffer, 0x00, sizeof(buffer));
        len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);

        for(loop =0; loop < len; loop++)
        {
            if(buffer[loop] == 0x00 )
            {

                buffer[loop] = ' ';
            }
        }

        if( len  < 1 )
            return 0;
        buffer[len-1] = '\0';
        strcpy(proc->arg,buffer);

        sprintf(buffer, "%d/stat", pid);
        fd = open(buffer, O_RDONLY);
        memset(buffer, 0x00, sizeof(buffer));
        len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);

        buffer[len] = '\0';
    }

    proc->uid = proc_owner(pid);

    /* parse out the status */
    p = buffer;

    if (strlen((char *)p) < 1)
        return RET_SUCC;

    p = strchr(p, '(')+1;			/* skip pid */
    {
        char *q = strrchr(p, ')');
        int len = q-p;
        if (len >= (int)sizeof(proc->name))
            len =  (int)sizeof(proc->name)-1;
        memcpy(proc->name, p, len);
        proc->name[len] = 0;
        p = q+1;
    }

    p = skip_ws(p);
    switch (*p++)
    {
        case 'R': proc->state = 1; break;
        case 'S': proc->state = 2; break;
        case 'D': proc->state = 3; break;
        case 'Z': proc->state = 4; break;
        case 'T': proc->state = 5; break;
        case 'W': proc->state = 6; break;
    }

    proc->ppid = strtol(p, &p, 10);

    p = skip_token(p);				/* skip pgid */
    p = skip_token(p);				/* skip sid */
    p = skip_token(p);				/* skip ttynr */
    p = skip_token(p);				/* skip tty pgrp */
    p = skip_token(p);				/* skip flags */
    p = skip_token(p);				/* skip min flt */
    p = skip_token(p);				/* skip cmin flt */
    p = skip_token(p);				/* skip maj flt */
    p = skip_token(p);				/* skip cmaj flt */

    proc->time = strtoul(p, &p, 10);		/* utime */
    proc->time += strtoul(p, &p, 10);		/* stime */


    p = skip_token(p);				/* skip cutime */
    p = skip_token(p);				/* skip cstime */

    proc->pri = strtol(p, &p, 10);		/* priority */
    proc->nice = strtol(p, &p, 10);		/* nice */


    p = skip_token(p);				/* skip num_threads */
    p = skip_token(p);				/* skip it_real_val */

    proc->stime = strtoul(p, &p, 10);


    proc->size = bytetok(strtoul(p, &p, 10));	/* vsize */

    proc->rss = pagetok(strtoul(p, &p, 10));	/* rsslim */


#if 0
    /* for the record, here are the rest of the fields */
    p = skip_token(p);				/* skip rlim */
    p = skip_token(p);				/* skip start_code */
    p = skip_token(p);				/* skip end_code */
    p = skip_token(p);				/* skip start_stack */
    p = skip_token(p);				/* skip sp */
    p = skip_token(p);				/* skip pc */
    p = skip_token(p);				/* skip signal */
    p = skip_token(p);				/* skip sigblocked */
    p = skip_token(p);				/* skip sigignore */
    p = skip_token(p);				/* skip sigcatch */
    p = skip_token(p);				/* skip wchan */
#endif

    return 1;
}


caddr_t get_process_info(struct system_info *si,
                         struct process_select *sel,
                         int (*compare)())
{
    struct timeval thistime;
    double timediff, alpha, beta;


    /* calculate the time difference since our last check */
    gettimeofday(&thistime, 0);
    if (lasttime.tv_sec)
    {
        timediff = ((thistime.tv_sec - lasttime.tv_sec) +
                    (thistime.tv_usec - lasttime.tv_usec) * 1e-6);
    }
    else
        timediff = 1e9;
    lasttime = thistime;

    /* calculate constants for the exponental average */
    if (timediff < 30.0)
    {
        alpha = 0.5 * (timediff / 30.0);
        beta = 1.0 - alpha;
    }
    else
        alpha = beta = 0.5;
    timediff *= HZ;  /* convert to ticks */

    /* mark all hash table entries as not seen */
    {
        int i;
        for (i = 0; i < HASH_SIZE; ++i)
            ptable[i].state = 0;
    }

    /* read the process information */
    {
        DIR *dir = opendir(".");
        struct dirent *ent;
        int total_procs = 0;
        struct top_proc **active = pactive;

        int show_idle = sel->idle;
        int show_uid = sel->uid != -1;

        memset(process_states, 0, sizeof(process_states));

        while ((ent = readdir(dir)) != NULL)
        {
            struct top_proc *proc;
            pid_t pid;
            unsigned long otime;

            if (!isdigit(ent->d_name[0]))
                continue;


            pid = atoi(ent->d_name);

            /* look up hash table entry */
            proc = &ptable[HASH(pid)];


            while (proc->pid != pid && proc->pid != -1)
            {
                if (++proc == ptable+HASH_SIZE)
                    proc = ptable;
            }

            otime = proc->time;

            read_one_proc_stat(pid, proc);

            if (proc->state == 0)
                continue;

            total_procs++;
            process_states[proc->state]++;

            if (proc->pid == -1)
            {
                proc->pid = pid;
                proc->wcpu = proc->pcpu = proc->time / timediff;
            }
            else
            {
                proc->pcpu = (proc->time - otime) / timediff;
                proc->wcpu = proc->pcpu * alpha + proc->wcpu * beta;
            }

            if ((show_idle || proc->state == 1 || proc->pcpu) &&
                (!show_uid || (int)proc->uid == sel->uid))
            {
                *active++ = proc;
            }
        }
        closedir(dir);

        si->p_active = total_procs;
        si->p_total = total_procs;
        si->procstates = process_states;
    }

    /* flush old hash table entries */
    {
        int i;
        for (i = 0; i < HASH_SIZE; ++i)
            if (ptable[i].state == 0)
                ptable[i].pid = -1;
    }

    /* if requested, sort the "active" procs */
    if (compare && si->p_active)
        qsort(pactive, si->p_active, sizeof(struct top_proc *), compare);

    /* don't even pretend that the return value thing here isn't bogus */
    nextactive = pactive;
    return (caddr_t)0;
}


char* format_header(char *uname_field)
{
    int uname_len = strlen(uname_field);
    if (uname_len > 8)
        uname_len = 8;

    memcpy(strchr(fmt_header, 'X'), uname_field, uname_len);

    return fmt_header;
}


char* format_next_process(char* (*get_userid)())
{
    static char fmt[128];	/* static area where result is built */
    struct top_proc *p = *nextactive++;

    sprintf(fmt,
            "%5d %-8.8s %3d %4d %5s %5s %-5s %6s %5.2f%% %5.2f%% %.14s",
            p->pid,
            (*get_userid)(p->uid),
            p->pri,
            p->nice,
            format_k(p->size),
            format_k(p->rss),
            state_abbrev[p->state],
            format_time(p->time / HZ),
            p->wcpu * 100.0,
            p->pcpu * 100.0,
            p->name);

    /* return the result */
    return (fmt);
}


/*
 *  proc_compare - comparison function for "qsort"
 *	Compares the resource consumption of two processes using five
 *  	distinct keys.  The keys (in descending order of importance) are:
 *  	percent cpu, cpu ticks, state, resident set size, total virtual
 *  	memory usage.  The process states are ordered as follows (from least
 *  	to most important):  WAIT, zombie, sleep, stop, start, run.  The
 *  	array declaration below maps a process state index into a number
 *  	that reflects this ordering.
 */


int proc_compare (struct top_proc **pp1,struct top_proc **pp2)
{
    static unsigned char sort_state[] =
    {
            0,	/* empty */
            6, 	/* run */
            3,	/* sleep */
            5,	/* disk wait */
            1,	/* zombie */
            2,	/* stop */
            4	/* swap */
    };

    struct top_proc *p1, *p2;
    int result;
    double dresult;

    /* remove one level of indirection */
    p1 = *pp1;
    p2 = *pp2;

    /* compare percent cpu (pctcpu) */
    dresult = p2->pcpu - p1->pcpu;
    if (dresult != 0.0)
        return dresult > 0.0 ? 1 : -1;

    /* use cputicks to break the tie */
    if ((result = p2->time - p1->time) == 0)
    {
        /* use process state to break the tie */
        if ((result = (sort_state[p2->state] - sort_state[p1->state])) == 0)
        {
            /* use priority to break the tie */
            if ((result = p2->pri - p1->pri) == 0)
            {
                /* use resident set size (rssize) to break the tie */
                if ((result = p2->rss - p1->rss) == 0)
                {
                    /* use total memory to break the tie */
                    result = (p2->size - p1->size);
                }
            }
        }
    }

    return result == 0 ? 0 : result < 0 ? -1 : 1;
}


/*
 * proc_owner(pid) - returns the uid that owns process "pid", or -1 if
 *              the process does not exist.
 *              It is EXTREMLY IMPORTANT that this function work correctly.
 *              If top runs setuid root (as in SVR4), then this function
 *              is the only thing that stands in the way of a serious
 *              security problem.  It validates requests for the "kill"
 *              and "renice" commands.
 */

uid_t proc_owner(pid_t pid)
{
    struct stat sb;
    char buffer[32];
    sprintf(buffer, "%d", pid);

    if (stat(buffer, &sb) < 0)
        return -1;
    else
        return sb.st_uid;
}

long Percentages(int cnt,
                 int *out,
                 register long *new,
                 register long *old,
                 long*        diffs)
{
    register int i;
    register long change;
    register long total_change;
    register long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++)
    {
        if ((change = *new - *old) < 0)
        {
            /* this only happens when the counter wraps */
            change = (int)
                    ((unsigned long)*new-(unsigned long)*old);
        }
        total_change += (*dp++ = change);
        *old++ = *new++;
    }

    /* avoid divide by zero potential */
    if (total_change == 0)
    {
        total_change = 1;
    }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;
    for (i = 0; i < cnt; i++)
    {
        *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
    }

    /* return the total in case the caller wants to use it */
    return(total_change);
}

char *format_time(long seconds)
{
    static char result[10];

    /* sanity protection */
    if (seconds < 0 || seconds > (99999l * 360l))
    {
        strcpy(result, "   ???");
    }
    else if (seconds >= (1000l * 60l))
    {
        /* alternate (slow) method displaying hours and tenths */
        sprintf(result, "%5.1fH", (double)seconds / (double)(60l * 60l));

        /* It is possible that the sprintf took more than 6 characters.
           If so, then the "H" appears as result[6].  If not, then there
           is a \0 in result[6].  Either way, it is safe to step on.
         */
        result[6] = '\0';
    }
    else
    {
        /* standard method produces MMM:SS */
        /* we avoid printf as must as possible to make this quick */
        sprintf(result, "%3lu:%02lu", seconds / 60l, seconds % 60l);
    }
    return(result);
}

char *itoa(register int val)
{
    register char *ptr;
    static char buffer[16];	/* result is built here */
    /* 16 is sufficient since the largest number
   we will ever convert will be 2^32-1,
   which is 10 digits. */

    ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    if (val == 0)
    {
        *--ptr = '0';
    }
    else while (val != 0)
        {
            *--ptr = (val % 10) + '0';
            val /= 10;
        }
    return(ptr);
}


char *format_k(int amt)
{
    static char retarray[NUM_STRINGS][16];
    static int index = 0;
    register char *p;
    register char *ret;
    register char tag = 'K';

    p = ret = retarray[index];
    index = (index + 1) % NUM_STRINGS;

    if (amt >= 10000)
    {
        amt = (amt + 512) / 1024;
        tag = 'M';
        if (amt >= 10000)
        {
            amt = (amt + 512) / 1024;
            tag = 'G';
        }
    }

    p = strecpy(p, itoa(amt));
    *p++ = tag;
    *p = '\0';

    return(ret);
}

