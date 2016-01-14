
/*
 * crontab.h - crontab parser program header.
 *
 * $Id: crontab.h,v 0.1 2001/06/11 07:30:45 tosihisa Exp tosihisa $
 */

#ifndef __CRONTAB_H
#define __CRONTAB_H

#define SEC_MIN		0
#define	SEC_MAX		60
#define	MIN_MIN		0
#define	MIN_MAX		59
#define	HOUR_MIN	0
#define	HOUR_MAX	23
#define	DAY_MIN		0
#define	DAY_MAX		31
#define	MON_MIN		0
#define	MON_MAX		12
#define	WEEK_MIN	0
#define	WEEK_MAX	7

#define	DEFARY(min,max)	(((max) - (min))+1)

typedef struct _s_crontab {
	char sec[ DEFARY(SEC_MIN,SEC_MAX) ];	/* second */
	char min[ DEFARY(MIN_MIN,MIN_MAX) ];	/* min    ( 0 - 59) */
	char hour[ DEFARY(HOUR_MIN,HOUR_MAX) ];	/* hour   ( 0 - 23) */
	char day[ DEFARY(DAY_MIN,DAY_MAX) ];	/* day    ( 0 - 31) */
	char mon[ DEFARY(MON_MIN,MON_MAX) ];	/* mon    ( 0 - 12) */
	char week[ DEFARY(WEEK_MIN,WEEK_MAX) ];	/* week   ( 0 -  7) */
} st_crontab;

#define	IS_NUMBER	0	/* "0123456789" */
#define	IS_LIST		1	/* ',' */
#define	IS_WILD		2	/* '*' */
#define	IS_RANGE	3	/* '-' */
#define	IS_STEP		4	/* '/' */
#define IS_TAKEN	10	/* blank */
#define IS_EOS		11	/* End Of String */
#define IS_NONE		99	/* else */

#define	CRONTAB_SYNTAX_ERROR	-1
#define	CRONTAB_OUT_OF_RANGE	-2

#define	CRONTAB_PARSE_SEC	0
#define	CRONTAB_PARSE_MIN	1
#define	CRONTAB_PARSE_HOUR	2
#define	CRONTAB_PARSE_DAY	3
#define	CRONTAB_PARSE_MON	4
#define	CRONTAB_PARSE_WEEK	5

char *crontab_parse_1(int,int,char *,char *,int *);
char *crontab_parse(int ,char *,st_crontab *,int *);
int crontab_init_data(st_crontab *);
int crontab_check_time(st_crontab *,struct tm *);

#endif /* __CRONTAB_H */

