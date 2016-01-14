
/*
 * Wait Timer Library.
 *
 * $Id: wait_time.c,v 0.2 2001/06/16 16:00:37 tosihisa Exp $
 *
 * Self-Test mode compile: gcc -o wait_time -D__SELFTEST__ wait_time.c
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

int force_wakeup = 0;

void alarm_signal(int signum)
{
	/* Nothing... */
}

void wakeup_signal(int signum)
{
	force_wakeup = 1;
}

int wait_alarm(struct timeval *timeout)
{
	struct itimerval itimeout;
	int i_retval;


	i_retval = 0;

	i_retval = getitimer(ITIMER_REAL,&itimeout);

	if(i_retval == 0)
	{
		itimeout.it_value = *timeout;
		i_retval = setitimer(ITIMER_REAL,&itimeout,NULL);
	}

	return i_retval;
}

#ifdef __SELFTEST__
struct timeval sleep_time;
#endif

int sleep_justZEROsecond(int localflag,time_t beforetime,int error_range,struct tm **tm_ptr)
{
	time_t nowtime;
	struct timeval timeout;

#ifdef __SELFTEST__
	sleep_time.tv_sec = sleep_time.tv_usec = 0;
#endif

	force_wakeup = 0;

	while(force_wakeup == 0)
	{
		signal(SIGALRM, alarm_signal);
		signal(SIGHUP, wakeup_signal);

		timeout.tv_sec = timeout.tv_usec = 0;

		time(&nowtime);
		*tm_ptr = (localflag) ? localtime(&nowtime) : gmtime(&nowtime);

		if( beforetime > nowtime)
			timeout.tv_sec = beforetime - nowtime;	/* reversed time !!! */
		else if((*tm_ptr)->tm_sec <= error_range)
			break;	/* LOOP END */
		else if((*tm_ptr)->tm_sec <= 45)
			timeout.tv_sec = 10;
		else if((*tm_ptr)->tm_sec <= 50)
			timeout.tv_sec = 5;
		else if((*tm_ptr)->tm_sec < 59)
			timeout.tv_sec = 1;
		else
			timeout.tv_usec = 200 * 1000;	/* 200 msec */

#if 0
		printf("Sleep %ld.%ld sec.\n",timeout.tv_sec,timeout.tv_usec);
#endif
#ifdef __SELFTEST__
		sleep_time.tv_sec  += timeout.tv_sec;
		sleep_time.tv_usec += timeout.tv_usec;
#endif

		wait_alarm(&timeout);
		pause();
	}

	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

#ifdef __SELFTEST__
	sleep_time.tv_sec  += (sleep_time.tv_usec / (1000 * 1000));
	sleep_time.tv_usec  = (sleep_time.tv_usec % (1000 * 1000));
#endif

	return force_wakeup;
}

#ifdef __SELFTEST__
int main(void)
{
	time_t beforetime;
	struct tm *tm_ptr;
	int i;


	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stdin, NULL,_IONBF,0);

	for(i = 0;i < (60 * 24 * 2);i++) /* between 2-days. */
	{
		time(&beforetime);

		sleep_justZEROsecond(1,beforetime+5,0,&tm_ptr);
		printf("%5d %04d.%06d %s",i,sleep_time.tv_sec,sleep_time.tv_usec,asctime(tm_ptr));
	}

	printf("End.\n");
	return 0;
}
#endif /* __SELFTEST__ */

