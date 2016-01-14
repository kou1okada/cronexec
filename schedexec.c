
/*
 * schedexec - process running on schedule.
 */

/* $Id: schedexec.c,v 0.85 2001/06/16 16:29:31 tosihisa Exp $ */

#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <unistd.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

#include "crontab.h"

#define HAVE_TIMES	/* need times(2) system call. */

#ifdef HAVE_TIMES
#include <sys/times.h>
#ifndef HZ
#	define	HZ	100
#endif
#define	clock_t2seconds(s)	((s) / HZ)
#endif /* HAVE_TIMES */

#define	SCHEDEXEC_PAUSE_FILE	".schedexec_pause"

char *RCSid = "$Id: schedexec.c,v 0.85 2001/06/16 16:29:31 tosihisa Exp $";

char *progname = NULL;
int print_time_flag = 0;

int sleep_justZEROsecond(int,time_t,int,struct tm **);

void print_time(void);

int exist_schedexec_pause_file( void )
{
	FILE *fp;
	int i_retval;

	i_retval = 1;

	fp = fopen(SCHEDEXEC_PAUSE_FILE,"rb");
	if(fp == NULL)
	{
		i_retval = 0;
	}
	else
	{
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
	fp = fopen(SCHEDEXEC_PAUSE_FILE,"w+b");
	if(fp != NULL)
	{
		fclose(fp);
		printf("schedule_job_create_pause_file\n");
		i_retval = 1;
	}
	else
	{
		printf("schedule_job_make_pause_file_error : %s (%d)\n",strerror(errno),errno);
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
		default:
		case 1:
			timestr = asctime(localtime(&lval));
			break;
		case 2:
			timestr = asctime(gmtime(&lval));
			break;
	}

	*(timestr + strlen(timestr) - 1) = (char)('\0');
	printf("%s : ",timestr);
}

void print_help(void)
{
	printf("%s [OPTIONS] -- \"CRONTAB_STRING\" arg(s)...\n",progname);
}

pid_t cron_job(char **argv,char **environ)
{
	int i;
	pid_t pid;


	print_time();
	printf("schedule_job_do : ");
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

clock_t wait_proc(int pid,int exitcode_flag,int exitcode,int *exitcode_pid)
{
	int status;
	clock_t proc_seconds,wait_timer_start,wait_timer_end;
	struct tms tms_buf;

	wait_timer_start = times(&tms_buf);

	print_time();
	printf("schedule_job_wait_pid %d\n",pid);
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
			printf("schedule_job_pid_termination_status %d\n",WEXITSTATUS(status));
			break;
		}
	}

	if(exitcode_pid != NULL)
		*exitcode_pid = WEXITSTATUS(status);

	if((exitcode_flag != 0) && (*exitcode_pid != exitcode))
	{
		print_time();
		printf("schedule_job_exitcode_nomatch : %d %d\n",exitcode,*exitcode_pid);
		make_pause_file();
	}

	wait_timer_end = times(&tms_buf);

	proc_seconds = 0;

	if((wait_timer_start >= 0) && (wait_timer_end >= wait_timer_start))
		proc_seconds = wait_timer_end - wait_timer_start;

	return proc_seconds;
}

int main(int argc,char **argv,char **environ)
{
	clock_t proc_seconds;
	char **cmd_argv = NULL;
	pid_t  child_pid = 0;
	int  job_count_exit = 0;
	long job_counter;
	int c;
	int exitcode_flag = 0;
	int exitcode,exitcode_pid;
	st_crontab crontabdat;
	int status;
	int i_wakeup;
	time_t beforetime;
	struct tm *tm_ptr;
	int i;
	int local_flag;
	char *crontabptr;
	extern char *optarg;
	extern int optind, opterr, optopt;



	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stdin,NULL,_IONBF,0);

	progname = *(argv + 0);

	exitcode_flag = 0;
	exitcode = exitcode_pid = 0;
	job_counter = -1;
	print_time_flag = 0;
	local_flag = 1;

	crontab_init_data(&crontabdat);

	while(1)
	{
		c = getopt(argc,argv,"e:c:C:vhlLG");

		if(c == EOF)
			break;

		switch(c)
		{
			case 'e':
				exitcode_flag = 1;
				exitcode = strtoul(optarg, (char **)NULL, 10);
				break;
			case 'C':
				job_count_exit = 1;
			case 'c':
				job_counter = strtol(optarg, (char **)NULL, 10);
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
			case 'G':
				local_flag = 0;
			default :
				return 127;
		}
	}

	if (optind < argc)
	{
		crontabptr = argv[optind];

		crontab_parse(0,argv[optind],&crontabdat,&status);

		if(status != 0)
		{
			if(status == CRONTAB_SYNTAX_ERROR)
				printf("%s : crontab-string Syntax error.\n",progname);
			else if(status == CRONTAB_OUT_OF_RANGE)
				printf("%s : crontab-string Out of range.\n",progname);
			else
			{
				printf("%s : !!! CRONTAB PARSER INTERNAL ERROR!!!.\n",progname);
				printf("%s : Please E-Mail <tosihisa@netfort.gr.jp>\n",progname);
			}
			return 1;
		}
		optind++;
	}
	else
	{
		print_help();
		return 1;
	}

	cmd_argv = NULL;

	if (optind < argc)
	{
		cmd_argv = argv + optind;
	}

	if(cmd_argv == NULL)
	{
		print_help();
		return 1;
	}

	print_time();
	printf("schedule_job_start : [%s]\n",crontabptr);
	print_time();
	printf("schedule_job_count : %ld %d\n",job_counter,job_count_exit);
	print_time();
	printf("schedule_job : ");
	for(i = 0;cmd_argv[i] != NULL;i++)
	{
		printf("[%s]",cmd_argv[i]);
	}
	printf("\n");

	time(&beforetime);

	/* main loop */
	while((job_counter != 0) || (job_count_exit == 0))
	{
		i_wakeup = sleep_justZEROsecond(local_flag,beforetime+5,0,&tm_ptr);

		time(&beforetime);

		if((i_wakeup != 0) || ((crontab_check_time(&crontabdat,tm_ptr) != 0) && (job_counter != 0)) )
		{
			if(!exist_schedexec_pause_file())
			{
				child_pid = cron_job(cmd_argv,environ);

				if(child_pid > 0)
				{
					proc_seconds = wait_proc(child_pid,exitcode_flag,exitcode,&exitcode_pid);
					print_time();
					printf("schedule_job_time : %d.%02d seconds.\n",proc_seconds / HZ,proc_seconds % HZ);
				}

				if(job_counter > 0)
					job_counter--;
			}
			else
			{
				print_time();
				printf("schedule_job_cancel : found_pause_file\n");
			}
		}
	}

	print_time();
	printf("schedule_job_end\n");

	return 0;
}
