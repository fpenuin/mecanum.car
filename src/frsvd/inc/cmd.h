/*
 * cmd.h
 *
 *  Created on: 2014-12-9
 *      Author: daiqingquan502
 *  Update: 2015-05-29, dqq
 *  增加辅助函数cmdGetArgD和cmdGetArgU，方便从命令中提取整型参数。
 *  Update: 2015-07-28, dqq
 *  增加辅助函数cmdGetArgF，方便从命令中提取浮点型参数。
 *  Update: 2016-4-26, dqq
 *  修改cmdExec无返回值为返回命令执行结果。
 */

#ifndef CMD_H_
#define CMD_H_

#define CMD_SUCCESS	(0)
#define CMD_FAILURE	(-1)
#define CMD_ERRARG	(-2)	//argument error
#define CMD_ERRMEM	(-3)	//memory error
#define CMD_ERRTO	(-4)	//time out
#define CMD_ERREXC	(0xff)

typedef int(* CmdFun_t)(char *);

void cmdInit(void);

int  cmdRegister(const char * name, CmdFun_t fun, const char * doc);
void cmdUnregister(const char * name);

void cmdPrompt(const char * prompt);
const char * cmdGet(void);
int cmdExec(const char * line);

int cmdGetArg(char * arg, ...);

#endif /* CMD_H_ */
