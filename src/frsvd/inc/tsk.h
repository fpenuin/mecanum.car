/*
 * tsk.h
 *
 *  Created on: 2015-9-6
 *      Author: daiqingquan502
 *  Update: 2016-3-1， dqq
 *  增加tskList与tskInfo函数用于查询正在运行的任务
 */

#ifndef TSK_H_
#define TSK_H_

#define MAX_TSK_NAME	(16)

typedef void * TskHandle_t;

void tskInit(unsigned int tick_period_us, unsigned long (* tick_get_fun)(void) );

TskHandle_t tskList(void);
void tskInfo(TskHandle_t tsk);

TskHandle_t tskAdd(char * name, void * fun,
		unsigned int arg0, unsigned int arg1);
void tskRemove(TskHandle_t tsk);

void tskSuspend(TskHandle_t tsk);
void tskResume(TskHandle_t tsk);
int tskSuspendEx(char * name);
int tskResumeEx(char * name);

void tskPeriod(TskHandle_t tsk, unsigned int pd_ms);
int tskPeriodEx(char * name, unsigned int pd_ms);

void tskExec(void);

#endif /* TSK_H_ */
