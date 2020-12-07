/*
 * param.h
 *
 *  Created on: 2014-12-12
 *      Author: daiqingquan502
 *  Upadate: 2016-4-8, dqq
 *  增加paramListEx， 使用strstr实现查询参数名称查询功能。
 */

#ifndef PARAM_H_
#define PARAM_H_

#define PARAM_NAME_LEN		(16)

typedef struct _Param_t{
	char		name[PARAM_NAME_LEN];
	unsigned	type;
	unsigned	len;
	void *		data;
}Param_t;

/*Return zero means O.K.*/
typedef int(* SaveFun_t)(void *, unsigned int);
typedef int(* LoadFun_t)(void *, unsigned int);

void paramInit(unsigned max_bin_in_bytes, SaveFun_t save, LoadFun_t load);
Param_t * paramFind(const char * name);
Param_t * paramList(void);
Param_t * paramListEx(const char * name);
void paramCleanup(void);

int paramRegister(const char * name, unsigned int type, unsigned int len, void * p);
int paramUnregister(const char * name);

/*Return the SaveFun_t or LoadFun_t's return value.*/
int paramSave(void);
int paramLoad(void);

#endif /* PARAM_H_ */
