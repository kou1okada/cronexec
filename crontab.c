
/*
 * crontab.c - crontab parser program.
 *
 * $Id: crontab.c,v 0.3 2001/06/13 03:39:49 tosihisa Exp $
 *
 * Self-Test mode compile: gcc -o crontab -D__SELFTEST__ crontab.c
 */

#include <stdio.h>
#include <time.h>

#include "crontab.h"

int crontab_parse_kind = -1;

char *crontab_getword(char *crontabstr,int *wordtype,int *number)
{
	char *crontabptr;


	crontabptr = crontabstr;

	*wordtype = IS_NONE;

	switch(*crontabptr)
	{
		case ',':
			*wordtype = IS_LIST;
			crontabptr++;
			break;
		case '*':
			*wordtype = IS_WILD;
			crontabptr++;
			break;
		case '-':
			*wordtype = IS_RANGE;
			crontabptr++;
			break;
		case '/':
			*wordtype = IS_STEP;
			crontabptr++;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			*wordtype = IS_NUMBER;
			*number = 0;
			while(isdigit(*crontabptr))
			{
				*number = *number * 10;
				*number = *number + (*crontabptr - (char)('0'));
				crontabptr++;
			}
			break;
		case ' ':
		case '\t':
			*wordtype = IS_TAKEN;
			crontabptr++;
			break;
		case ('\0'):
			*wordtype = IS_EOS;
			break;
	}

	return crontabptr;
}

void crontab_setval(char *flagarray,int min,int max,int step)
{
	int i;


	if(max == -1)
	{
		*(flagarray + min) = 1;
	}
	else
	{
		for(i = min;i <= max;i += step)
			*(flagarray + i) = 1;
	}
}

/*
      cjbst       1          2             3        4            5       6         0
             +----------------------+<-----------------+----------------------+
             V                      |                  |                      |
   ->[taken]-+->[num][*]-+->[,]-----+                  |                      |
                         |                             |                      |
                         |                     +->[,]--+                      |
                         |                     |                              |
                         +->[-]--------->[num]-+->[taken]--------------------------+
                         |                     |                              |    |
                         |                     +->[/]-------+                 |    |
                         |                                  |                 |    |
                         |                                  V                 |    |
                         +->[/]------------------------------->[num]-+-->[,]--+    |
                         |                                           |             V
                         |                                           +->[taken]----+
                         |                                                         |
                         |                                                         V
                         +->[taken]----------------------------------------------> End.
*/
char *crontab_parse_1(int range_min,int range_max,char *crontabstr,char *flagarray,int *status)
{
	int cjbst = 0;
	char *crontabptr;
	int wordtype;
	int number;
	int val_min,val_max,val_step;


	crontabptr = crontabstr;

	val_min = -1;
	val_max = -1;
	val_step = 1;
	cjbst = 1;

	while(cjbst > 0)
	{
		crontabptr = crontab_getword(crontabptr,&wordtype,&number);

		if(cjbst == 1)
		{
			switch(wordtype)
			{
				case IS_NUMBER:
					if((number < range_min) || (number > range_max))
					{
						cjbst = CRONTAB_OUT_OF_RANGE;	/* Out Of Range */
					}
					else
					{
						val_min = number;
						cjbst = 2;
					}
					break;
				case IS_WILD:
					val_min = range_min;
					val_max = range_max;
					cjbst = 2;
					break;
				case IS_TAKEN:
					/* Read Continue */
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
		else if(cjbst == 2)
		{
			switch(wordtype)
			{
				case IS_EOS:
				case IS_TAKEN:
					crontab_setval(flagarray,val_min,val_max,val_step);
					cjbst = 0;	/* End */
					break;
				case IS_LIST:
					crontab_setval(flagarray,val_min,val_max,val_step);
					val_min = -1;
					val_max = -1;
					val_step = 1;
					cjbst = 1;	/*  */
					break;
				case IS_RANGE:
					cjbst = 3;
					break;
				case IS_STEP:
					cjbst = 5;
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
		else if(cjbst == 3)
		{
			switch(wordtype)
			{
				case IS_NUMBER:
					if((val_min > number) || (number > range_max))
					{
						cjbst = CRONTAB_OUT_OF_RANGE;
					}
					else
					{
						val_max = number;
						cjbst = 4;
					}
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
		else if(cjbst == 4)
		{
			switch(wordtype)
			{
				case IS_STEP:
					cjbst = 5;	/* End */
					break;
				case IS_LIST:
					crontab_setval(flagarray,val_min,val_max,val_step);
					val_min = -1;
					val_max = -1;
					val_step = 1;
					cjbst = 1;	/*  */
					break;
				case IS_EOS:
				case IS_TAKEN:
					crontab_setval(flagarray,val_min,val_max,val_step);
					cjbst = 0;	/* End */
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
		else if(cjbst == 5)
		{
			switch(wordtype)
			{
				case IS_NUMBER:
					val_step = number;
					cjbst = 6;	/* End */
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
		else if(cjbst == 6)
		{
			switch(wordtype)
			{
				case IS_LIST:
					crontab_setval(flagarray,val_min,val_max,val_step);
					val_min = -1;
					val_max = -1;
					val_step = 1;
					cjbst = 1;	/*  */
					break;
				case IS_EOS:
				case IS_TAKEN:
					crontab_setval(flagarray,val_min,val_max,val_step);
					cjbst = 0;	/* End */
					break;
				default:
					cjbst = CRONTAB_SYNTAX_ERROR;	/* Syntax Error */
					break;
			}
		}
	}

	*status = cjbst;

	return crontabptr;
}

char *crontab_parse(int parse_mode,char *crontabstr,st_crontab *crontabdat,int *status)
{
	char *crontabptr;


	*status = 0;
	crontabptr = crontabstr;

	crontab_parse_kind = CRONTAB_PARSE_SEC;
	if(parse_mode)
		crontabptr = crontab_parse_1(SEC_MIN,SEC_MAX,crontabptr,crontabdat->sec,status);
	else
		(void)crontab_parse_1(SEC_MIN,SEC_MAX,"*",crontabdat->sec,status);
	if(*status != 0)
		return crontabptr;

	crontab_parse_kind = CRONTAB_PARSE_MIN;
	crontabptr = crontab_parse_1(MIN_MIN,MIN_MAX,crontabptr,crontabdat->min,status);
	if(*status != 0)
		return crontabptr;

	crontab_parse_kind = CRONTAB_PARSE_HOUR;
	crontabptr = crontab_parse_1(HOUR_MIN,HOUR_MAX,crontabptr,crontabdat->hour,status);
	if(*status != 0)
		return crontabptr;

	crontab_parse_kind = CRONTAB_PARSE_DAY;
	crontabptr = crontab_parse_1(DAY_MIN,DAY_MAX,crontabptr,crontabdat->day,status);
	if(*status != 0)
		return crontabptr;

	crontab_parse_kind = CRONTAB_PARSE_MON;
	crontabptr = crontab_parse_1(MON_MIN,MON_MAX,crontabptr,crontabdat->mon,status);
	if(*status != 0)
		return crontabptr;

	crontab_parse_kind = CRONTAB_PARSE_WEEK;
	crontabptr = crontab_parse_1(WEEK_MIN,WEEK_MAX,crontabptr,crontabdat->week,status);
	if(*status != 0)
		return crontabptr;

	if(crontabdat->week[WEEK_MAX] != 0)
		crontabdat->week[WEEK_MIN] = crontabdat->week[WEEK_MAX];
	else if(crontabdat->week[WEEK_MIN] != 0)
		crontabdat->week[WEEK_MAX] = crontabdat->week[WEEK_MIN];

	return crontabptr;
}

int crontab_init_data(st_crontab *crontabdat)
{
	int i;
	char	*clearptr;


	clearptr = (char *)crontabdat;

	for(i = 0;i < sizeof(st_crontab);i++)
		*(clearptr + i) = 0x00;

	return 0;
}

int crontab_check_time(st_crontab *crontabdat,struct tm *tm_ptr)
{
	int i_retval;

	i_retval = 1;
	i_retval &= crontabdat->sec[ tm_ptr->tm_sec ];
	i_retval &= crontabdat->min[ tm_ptr->tm_min ];
	i_retval &= crontabdat->hour[ tm_ptr->tm_hour ];
	i_retval &= crontabdat->day[ tm_ptr->tm_mday ];
	i_retval &= crontabdat->mon[ tm_ptr->tm_mon + 1 ];
	i_retval &= crontabdat->week[ tm_ptr->tm_wday ];

	return i_retval;
}

#ifdef __SELFTEST__
int main(int argc,char *argv[])
{
  int status,i;
  char *crontabstr;
  char *crontabptr;
  st_crontab crontabdat;
  time_t nowtime;


  crontabstr = argv[1];

  crontab_init_data(&crontabdat);

  crontabptr = crontab_parse(0,crontabstr,&crontabdat,&status);

  printf("crontab perser = %d\n",status);

  if(status == 0)
  {
    printf("-----sec-----\n");
    for(i = SEC_MIN;i <= SEC_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.sec[i]));
    printf("\n");

    printf("-----min-----\n");
    for(i = MIN_MIN;i <= MIN_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.min[i]));
    printf("\n");

    printf("-----hour-----\n");
    for(i = HOUR_MIN;i <= HOUR_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.hour[i]));
    printf("\n");

    printf("-----day-----\n");
    for(i = DAY_MIN;i <= DAY_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.day[i]));
    printf("\n");

    printf("-----mon-----\n");
    for(i = MON_MIN;i <= MON_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.mon[i]));
    printf("\n");

    printf("-----week-----\n");
    for(i = WEEK_MIN;i <= WEEK_MAX;i++)
      printf("%02d:[%d]\n",i,(int)(crontabdat.week[i]));
    printf("\n");
  }

  printf("=>[%02x]\n",(int)*crontabptr);

  time(&nowtime);
  printf("Now Time %s",asctime(localtime(&nowtime)));
  if(crontab_check_time(&crontabdat,localtime(&nowtime)) != 0)
  {
    printf("Cron JOB Do!!\n");
  }
  else
  {
    printf("Cron JOB Wait\n");
  }

  return 0;

}
#endif /* __SELFTEST__ */

