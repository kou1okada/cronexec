
/*
 * cronexec - process running on interval
 */

/* $Id: cronexec.c,v 0.90 2001/06/16 16:28:06 tosihisa Exp $ */

#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <unistd.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

#define HAVE_TIMES	/* need times(2) system call. */

#ifdef HAVE_TIMES
#include <sys/times.h>
#ifndef HZ
#	define	HZ	100
#endif
#define	clock_t2seconds(s)	((s) / HZ)
#endif /* HAVE_TIMES */

#define	CRONEXEC_PAUSE_FILE	".cronexec_pause"

char *RCSid = "$Id: cronexec.c,v 0.90 2001/06/16 16:28:06 tosihisa Exp $";

int do_job = 0;
int do_job_force = 0;
char *progname = NULL;
int print_time_flag = 0;

void print_time(void);

int check_cronexec_do(int logexpr)
{
	FILE *fp;
	int i_retval;

	i_retval = 0;

	fp = fopen(CRONEXEC_PAUSE_FILE,"rb");
	if(fp == NULL)
	{
		i_retval = 1;
	}
	else
	{
		if(logexpr)
		{
			print_time();
			printf("cron_job_cancel : found_pause_file\n");
		}
		fclose(fp);
	}

	return i_retval;
}

int make_pause_file(void)
{
	FILE *fp;
	int i_retval;

	i_retval = 0;

	print_time();
	fp = fopen(CRONEXEC_PAUSE_FILE,"w+b");
	if(fp != NULL)
	{
		fclose(fp);
		printf("cron_job_create_pause_file\n");
		i_retval = 1;
	}
	else
	{
		printf("cron_job_make_pause_file_error : %s (%d)\n",strerror(errno),errno);
	}

	return i_retval;
}

void print_time(void)
{
	time_t lval;
	char *timestr;

	time(&lval);

	
	switch(print_time_flag)
	{
		case 0:
			return;
		case 1:
			timestr = asctime(localtime(&lval));
			break;
		default:
		case 2:
			timestr = asctime(gmtime(&lval));
			break;
	}

	*(timestr + strlen(timestr) - 1) = (char)('\0');
	printf("%s : ",timestr);
}

void print_help(void)
{
	printf("%s [OPTIONS] -- wait-sec arg(s)...\n",progname);
}

void alarm_signal(int signum)
{
}

void hup_signal(int signum)
{
	print_time();
	printf("cron_job_recv_HUP_signal\n");

	do_job = 1;
	do_job_force = 1;
}

int timer_wait(int wait_mode,unsigned int seconds)
{
	time_t starttim,endtim;
	unsigned int wait_seconds;

	endtim = time(NULL) + seconds;

	while(do_job == 0)
	{
		if(wait_mode == 0)
		{
			wait_seconds = seconds;
			do_job = 1;
		}
		else
		{
			starttim = time(NULL);
			if(starttim >= endtim)
			{
				do_job = 1;
				break;
			}

			wait_seconds = endtim - starttim;
			if(wait_seconds > 5)
				wait_seconds = 5;
		}

		alarm(wait_seconds);
		pause();
	}

	return do_job;
}

int cron_job(char **argv,char **environ)
{
	int i;
	int pid;


	print_time();
	printf("cron_job_do : ");
	for(i = 0;argv[i] != NULL;i++)
	{
		printf("[%s]",argv[i]);
	}

	pid = fork();
	if (pid == -1)
	{
		printf(" : fork_error : %s (%d)\n",strerror(errno),errno);
	}
	else if (pid == 0)
	{
		execve(argv[0], argv, environ);
		printf(" : exec_error : %s (%d)\n",strerror(errno),errno);
		exit(127);
	}
	else
	{
		printf(" : OK pid %d\n",pid);
	}

	return pid;
}

clock_t wait_proc(int pid,int exitcode_flag,int exitcode,int *exitcodewk)
{
	int status;
	clock_t proc_seconds,wait_timer_start,wait_timer_end;
	struct tms tms_buf;

	wait_timer_start = times(&tms_buf);

	print_time();
	printf("cron_job_wait_pid %d\n",pid);
	while(1)
	{
		if (waitpid(pid, &status, 0) == -1)
		{
			if (errno != EINTR)
			{
				print_time();
				printf("waitpid() : %s (%d)\n",strerror(errno),errno);
				break;
			}
		}
		else
		{
			print_time();
			printf("cron_job_pid_termination_status %d\n",WEXITSTATUS(status));
			break;
		}
	}

	if(exitcodewk != NULL)
		*exitcodewk = WEXITSTATUS(status);

	if((exitcode_flag != 0) && (*exitcodewk != exitcode))
	{
		print_time();
		printf("cron_job_exitcode_nomatch : %d %d\n",exitcode,*exitcodewk);
		make_pause_file();
	}

	wait_timer_end = times(&tms_buf);

	proc_seconds = 0;

	if((wait_timer_start >= 0) && (wait_timer_end >= wait_timer_start))
		proc_seconds = wait_timer_end - wait_timer_start;

	return proc_seconds;
}

unsigned int get_second(char *timestr)
{
	unsigned int ui_retval;
	unsigned int ui_timeofs;
	char *timestr_end;
	char *timestr_tmp;

	ui_retval = 0;
	ui_timeofs = 1;

	if((timestr == NULL) || (*timestr == (char)('\0')))
		return 0;

	timestr_tmp = (char *)malloc(strlen(timestr) * sizeof(char));
	if(timestr_tmp == NULL)
		return 0;

	strcpy(timestr_tmp,timestr);

	timestr_end = timestr_tmp + strlen(timestr_tmp) - 1;

	switch(*timestr_end)
	{
		/* second */
		case 's':
		case 'S':
			*timestr_end = (char)('\0');
		default:
			ui_timeofs = 1;
			break;
		/* min */
		case 'm':
		case 'M':
			*timestr_end = (char)('\0');
			ui_timeofs = 1 * 60;
			break;

		/* hour */
		case 'h':
		case 'H':
			*timestr_end = (char)('\0');
			ui_timeofs = 1 * 60 * 60;
			break;

		/* day */
		case 'd':
		case 'D':
			*timestr_end = (char)('\0');
			ui_timeofs = 1 * 60 * 60 * 24;
			break;

		/* week */
		case 'w':
		case 'W':
			*timestr_end = (char)('\0');
			ui_timeofs = 1 * 60 * 60 * 24 * 7;
			break;
	}

	ui_retval = (unsigned int)strtoul(timestr_tmp, (char **)NULL, 10) * ui_timeofs;

	free(timestr_tmp);

	return ui_retval;
}

int main(int argc,char **argv,char **environ)
{
	unsigned int seconds = 0;
	int seconds_flag = 0;
	unsigned int wait_seconds;
	unsigned int proc_seconds;
	int wait_mode = 0;
	int proc_wait = 0;
	char **cmd_argv = NULL;
	int  child_pid = 0;
	int  job_count_flag = 0;
	int  job_count_exit = 0;
	unsigned long job_counter    = 1;
	int c;
	int first_do_flag = 0;
	int exitcode_flag = 0;
	int exitcode,exitcodewk;
	int init_sec_flag = 0;
	unsigned int init_seconds = 0;
	extern char *optarg;
	extern int optind, opterr, optopt;

	
	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stdin,NULL,_IONBF,0);

	progname = *(argv + 0);

	seconds_flag  = 0;
	first_do_flag = 0;
	exitcode_flag = 0;
	exitcode = exitcodewk = 0;
	job_counter   = 1;
	init_sec_flag = 0;
	init_seconds = 0;

	while(1)
	{
		c = getopt(argc,argv,"ae:TWc:C:vhlLi:");

		if(c == EOF)
			break;

		switch(c)
		{
			case 'a':
				first_do_flag = 1;
				init_sec_flag = 0;
				break;
			case 'e':
				exitcode_flag = 1;
				exitcode = strtoul(optarg, (char **)NULL, 10);
				break;
			case 'T':
				wait_mode = 1;
				break;
			case 'W':
				proc_wait = 1;
				break;
			case 'C':
				job_count_exit = 1;
			case 'c':
				job_count_flag = 1;
				job_counter = strtoul(optarg, (char **)NULL, 10);
				break;
			case 'v':
				printf("%s\n",RCSid);
			case 'h':
				print_help();
				return 0;
				break;
			case 'l':
				print_time_flag = 1;
				break;
			case 'L':
				print_time_flag = 2;
				break;
			case 'i':
				init_sec_flag = 1;
				first_do_flag = 0;
				init_seconds = get_second(optarg);
				break;
			default :
				return 127;
		}
	}

	if (optind < argc)
	{
		seconds = get_second(argv[optind]);
		optind++;
	}
	if (optind < argc)
	{
		cmd_argv = argv + optind;
	}

	if((seconds == 0) || (cmd_argv == NULL))
	{
		print_help();
		exit(0);
	}

#if 1
	{
		int i;

		print_time();
		printf("cron_job_start : %u %d %d %d\n",seconds,wait_mode,proc_wait,first_do_flag);
		print_time();
		printf("cron_job_count : %u %d %d\n",job_counter,job_count_flag,job_count_exit);
		print_time();
		printf("cron_job_exit  : %d %d\n",exitcode_flag,exitcode);

		print_time();
		printf("cron_job_args  : ");
		for(i = 0;cmd_argv[i] != NULL;i++)
		{
			printf("[%s]",cmd_argv[i]);
		}
		printf("\n");
	}
#endif

	wait_seconds = seconds;

	/* main loop */
	while((job_counter != 0) || (job_count_exit == 0))
	{
		do_job = 0;
		do_job_force = 0;
		proc_seconds = 0;

		if(init_sec_flag != 0)
		{
			wait_seconds = time(NULL);
			if(init_seconds <= wait_seconds)
				wait_seconds = seconds;
			else
				wait_seconds = init_seconds - wait_seconds;

			print_time();
			printf("cron_job_init_time  : %u\n",wait_seconds);
		}
		init_sec_flag = 0;

		signal(SIGALRM, alarm_signal);
		signal(SIGHUP,  hup_signal);

		if((first_do_flag != 0) || (timer_wait(wait_mode,wait_seconds) != 0))
		{

			signal(SIGALRM, SIG_IGN);
			signal(SIGHUP,  SIG_IGN);

			if(check_cronexec_do( ((do_job_force != 0) || (job_counter > 0)) ) != 0)
			{
				if((do_job_force != 0) || (job_counter > 0))
				{
					child_pid = cron_job(cmd_argv,environ);

					if(child_pid > 0)
						proc_seconds = wait_proc(child_pid,exitcode_flag,exitcode,&exitcodewk);

					if((job_count_flag != 0) && (job_counter > 0))
						job_counter--;
				}
			}
		}

		first_do_flag = 0;

		wait_seconds = seconds;
		if(proc_wait != 0)
			wait_seconds -= (clock_t2seconds(proc_seconds) % seconds);
	}
 
	print_time();
	printf("cron_job_end\n");

	return 0;
}

