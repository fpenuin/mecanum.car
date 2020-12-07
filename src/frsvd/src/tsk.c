/*
 * tsk.c
 *
 *  Created on: 2015-9-6
 *      Author: daiqingquan502
 *  Update: 2016-3-1， dqq
 *  增加tskList与tskInfo函数用于查询正在运行的任务；
 *  将tskExec中任务超时提醒提前到任务函数调用之前。
 */

#include "../inc/tsk.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"

#define __STAT_RUNNING	(0)
#define __STAT_BLOCKED	(1)


typedef struct _Tsk_t{
	struct list_head	list;
	char				name[MAX_TSK_NAME];
	void *				fun;
	unsigned int		arg[2];

	unsigned int		stat;
	unsigned int		pd;
	unsigned long		tk;
}Tsk_t;

static struct list_head __tsk_list;

static unsigned long (* __tk_get_fun)(void);
static unsigned int __tk_pd_us=0;

void tskInit(unsigned int tick_period_us, unsigned long (* tick_get_fun)(void) )
{
	INIT_LIST_HEAD(&__tsk_list);
	__tk_get_fun=tick_get_fun;
	__tk_pd_us=tick_period_us;
}

TskHandle_t tskList(void)
{
	static struct list_head *__pos = NULL;
	static struct list_head *__n = NULL;
	Tsk_t * tsk;

	if((NULL==__pos) || (NULL==__n))
	{
		__pos = __tsk_list.next;
		__n = __pos->next;
	}

	if(__pos != &__tsk_list)
	{
		tsk = list_entry(__pos, Tsk_t, list);
		__pos = __n;
		__n = __pos->next;
		return (TskHandle_t)tsk;
	}

	__pos = NULL;
	__n = NULL;
	return NULL;
}
void tskInfo(TskHandle_t tsk)
{
	printf("0x%08lX: %s, 0x%08lX, %u, %u;\r\n",
			(unsigned long)tsk,
			((Tsk_t *)tsk)->name,
			(unsigned long)((Tsk_t *)tsk)->fun,
			((Tsk_t *)tsk)->stat,
			((Tsk_t *)tsk)->pd);
}

TskHandle_t tskAdd(char * name, void * fun,
		unsigned int arg0, unsigned int arg1)
{
	if((!fun) || (!name)) return NULL;
	if(strlen(name)+1>MAX_TSK_NAME) return NULL;

	Tsk_t * tsk;

	tsk = (Tsk_t *)malloc(sizeof(Tsk_t));
	if(!tsk) return NULL;

	strcpy(tsk->name, name);
	tsk->fun = fun;
	tsk->arg[0] = arg0;
	tsk->arg[1] = arg1;

	tsk->stat = __STAT_RUNNING;
	tsk->pd = 0;
	tsk->tk = 0;

	list_add_tail(&tsk->list, &__tsk_list);

	return (TskHandle_t)tsk;
}
void tskRemove(TskHandle_t tsk)
{
	if(tsk)
	{
		list_del(&((Tsk_t *)tsk)->list);
		free(tsk); tsk=NULL;
	}
}

void tskSuspend(TskHandle_t tsk)
{
	((Tsk_t *)tsk)->stat = __STAT_BLOCKED;
}
void tskResume(TskHandle_t tsk)
{
	((Tsk_t *)tsk)->stat = __STAT_RUNNING;
}
int tskSuspendEx(char * name)
{
	struct list_head *pos, *n;
	Tsk_t * tsk;

	list_for_each_safe(pos, n, &__tsk_list)
	{
		tsk = list_entry(pos, Tsk_t, list);
		if( 0==strcmp(name, tsk->name) )
		{
			tsk->stat = __STAT_BLOCKED;
			return 0;
		}
	}

	return -1;
}
int tskResumeEx(char * name)
{
	struct list_head *pos, *n;
	Tsk_t * tsk;

	list_for_each_safe(pos, n, &__tsk_list)
	{
		tsk = list_entry(pos, Tsk_t, list);
		if( 0==strcmp(name, tsk->name) )
		{
			tsk->stat = __STAT_RUNNING;
			return 0;
		}
	}

	return -1;
}

void tskPeriod(TskHandle_t tsk, unsigned int pd_ms)
{
	((Tsk_t *)tsk)->pd=pd_ms*1000L/__tk_pd_us;
}
int tskPeriodEx(char * name, unsigned int pd_ms)
{
	struct list_head *pos, *n;
	Tsk_t * tsk;

	list_for_each_safe(pos, n, &__tsk_list)
	{
		tsk = list_entry(pos, Tsk_t, list);
		if( 0==strcmp(name, tsk->name) )
		{
			tsk->pd=pd_ms*1000L/__tk_pd_us;
			return 0;
		}
	}

	return -1;
}

void tskExec(void)
{
	struct list_head *pos, *n;
	Tsk_t * tsk;

	list_for_each_safe(pos, n, &__tsk_list)
	{
		tsk = list_entry(pos, Tsk_t, list);
		if(tsk->stat == __STAT_RUNNING)
		{
			if(__tk_get_fun && tsk->pd)
			{
				unsigned long now=__tk_get_fun();
				unsigned long invl = now - tsk->tk;
				if( invl >= tsk->pd )
				{
					tsk->tk = now;
					if(invl>tsk->pd)
						printf("WARNNING: Task \"%s\" delay=%lu tick(s).\r\n", tsk->name, invl-tsk->pd);
					((void(*)(unsigned int, unsigned int))tsk->fun)(tsk->arg[0], tsk->arg[1]);
				}
			}
			else
				((void(*)(unsigned int, unsigned int))tsk->fun)(tsk->arg[0], tsk->arg[1]);
		}
	}
}
