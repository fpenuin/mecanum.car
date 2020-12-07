/*
 * cmd.c
 *
 *  Created on: 2014-12-9
 *      Author: daiqingquan502
 *  Update: 2015-05-08, dqq
 *  修改cmdExec命令执行结果判断中r>0时执行asm(" NOP")为打印r的值。
 *  Update: 2015-05-29, dqq
 *  增加辅助函数cmdGetArgD和cmdGetArgU，方便从命令中提取整型参数。
 *  Update: 2015-07-28, dqq
 *  增加辅助函数cmdGetArgF，方便从命令中提取浮点型参数。
 *  Update: 2015-08-20, dqq
 *  支持的命令格式改为：
 *  command argument1,argument2,argument3 ....
 *  支持整数、浮点和字符串类型的参数，字符串必须使用''或者""标出。
 *  命令格式的更改，修改了cmdGet函数，增加了辅助函数cmdGetArg，可自动识别参数类型。
 *  Update: 2015-3-1, dqq
 *  修改cmdGetArg对输入参数arg为NULL或者strlen（arg）为0时的返回值为0。
 *  Update: 2016-4-26, dqq
 *  修改cmdExec无返回值为返回命令执行结果。
 */


#include "../inc/cmd.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "list.h"

#define PBF_SIZE    (16)
#define LBF_SIZE    (256)

typedef struct _CmdItem_t{
    struct list_head    list;
    const char *        name;
    CmdFun_t            fun;
    const char *        doc;
}Cmd_t;

static struct list_head __cmd_list;
static char __line_buf[LBF_SIZE]={0};
static char __prompt[PBF_SIZE]={'>', 0};

static inline Cmd_t * __find_cmd(const char * name)
{
    struct list_head *pos, *n;
    Cmd_t * cmd;

    list_for_each_safe(pos, n, &__cmd_list)
    {
        cmd = list_entry(pos, Cmd_t, list);
        if(0==strcmp (name, cmd->name))
            return cmd;
    }

    return NULL;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static inline int __help(char * arg)
{
    struct list_head *pos, *n;
    Cmd_t * cmd;

    list_for_each_safe(pos, n, &__cmd_list)
    {
        cmd = list_entry(pos, Cmd_t, list);
        printf("%-*s - %s\r\n", 16, cmd->name, cmd->doc);
    }

    return 0;
}

#pragma GCC diagnostic pop

void cmdInit(void)
{
    INIT_LIST_HEAD(&__cmd_list);
    cmdRegister("?", __help, "[none]: Display all supported commands.");
}

int cmdRegister(const char * name, CmdFun_t fun, const char * doc)
{
    Cmd_t * cmd;

    cmd = malloc(sizeof(Cmd_t));
    if(cmd)
    {
        memset(cmd, 0, sizeof(Cmd_t));
        cmd->name = name;
        cmd->fun = fun;
        cmd->doc = doc;
        list_add_tail(&cmd->list, &__cmd_list);

        return 0;
    }
    else
    {
        return -1;
    }
}
void cmdUnregister(const char * name)
{
    struct list_head *pos, *n;
    Cmd_t * cmd;

    list_for_each_safe(pos, n, &__cmd_list)
    {
        cmd = list_entry(pos, Cmd_t, list);
        if(0==strcmp (name, cmd->name))
        {
            list_del(&cmd->list);
            free(cmd);
        }
    }
}

void cmdPrompt(const char * prompt)
{
    if(prompt)
    {
        size_t len = strlen(prompt);
        memcpy(__prompt, prompt, len>(PBF_SIZE-1) ? (PBF_SIZE-1) : len);
    }
}
const char * cmdGet(void)
{
    char * line = fgets(__line_buf, LBF_SIZE, stdin);

    if(line)
    {
        size_t i,len;

        len = strlen(line);
        /* Check undispalyable contorl ascii */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
        for(i=0;i<len;i++)
            if(line[i]<32 || line[i]==127 || line[i]>255) line[i]=' ';
#pragma GCC diagnostic pop

        /* Strip left & right white space */
        while(*line == ' ') line++;
        len = strlen(line);
        for(i=len-1; (line[i]==' ')&&(i>0); i--)
            line[i]='\0';

        /* Check command again */
        len = strlen(line);
        if(len==0)
        {
            printf("%s", __prompt);
            return NULL;
        }

        /* Format command and argument */
        i=0;
        while(line[i]!=' ' && line[i]!='\0') i++;
        if(line[i]==' ')
        {
            size_t j, flag=0;

            for(i++; (line[i]==' ') && (line[i-1]==' '); i++)
            {
                for(j=i;j<len;j++)
                    line[j]=line[j+1];
                i--;
            }

            for(;i<len;i++)
            {
                if(line[i]=='\"')
                {
                    if(flag==0) flag=1;
                    else if(flag==1) flag=0;
                    else {;}
                }
                if(line[i]=='\'')
                {
                    if(flag==0) flag=2;
                    else if(flag==2) flag=0;
                    else {;}
                }

                if((flag==0) && (line[i]==' '))
                {
                    for(j=i;j<len;j++)
                        line[j]=line[j+1];
                    i--;
                }
            }
        }
    }
    else
        clearerr(stdin);

    return line;
}
int cmdExec(const char * line)
{
    int r=CMD_ERREXC;

    do
    {
        unsigned len = strlen(line);
        if(len==0) break;

        char * line_cp = malloc(len+1);
        if(!line_cp)
        {
            printf("%s: malloc = NULL\r\n", __func__);
            break;
        }

        strcpy(line_cp, line);

        /* Identify command name and truncate it */
        unsigned i=0;
        while(line_cp[i]!=' ' && line_cp[i]!='\0') i++;
        line_cp[i] = '\0';

        /* Check whether the command is supported or not */
        Cmd_t * cmd = __find_cmd(line_cp);
        if(!cmd)
        {
            free(line_cp);
            printf("Unrecognized command. try \"?\".\r\n\r\n");
            break;
        }

        char *arg = NULL;
        if(len!=i) arg = line_cp+i+1;
        r = cmd->fun(arg);
        free(line_cp);

        if(r>0)
            printf("\r\nCommand execution RETURN: %d.\r\n\r\n", r);
        else if(r==CMD_SUCCESS)
            printf("\r\nCommand execution SUCCEED.\r\n\r\n");
        else if(r==CMD_FAILURE)
            printf("\r\nCommand execution FAIL.\r\n\r\n");
        else if(r==CMD_ERRARG)
            printf("\r\nCommand execution err: illegal argument.\r\n\r\n");
        else if(r==CMD_ERRMEM)
            printf("\r\nCommand execution err: memory allocation fail.\r\n\r\n");
        else if(r==CMD_ERRTO)
            printf("\r\nCommand execution err: operation time out.\r\n\r\n");
        else
            printf("\r\nCommand execution err: unknown err.\r\n\r\n");

    }while(0);

    printf("%s", __prompt);

    return r;
}

int cmdGetArg(char * arg, ...)
{
    if(!arg || 0==strlen(arg))
        return 0;

    /* count the argument number */
    int i,argc=1, flag=0;
    for(i=0;arg[i]!='\0';i++)
    {
        if(arg[i]=='\"')
        {
            if(flag==0) flag=1;
            else if(flag==1) flag=0;
            else {;}
        }
        if(arg[i]=='\'')
        {
            if(flag==0) flag=2;
            else if(flag==2) flag=0;
            else {;}
        }
        if(flag==0 && arg[i]==',' )
            argc++;
    }
    if(flag!=0) return -1;

    double *arg_f=NULL;
    int *arg_d=NULL;
    char *arg_s=NULL;

    char *end, *start = arg;
    int argc_valid;

    va_list ap;
    va_start(ap, arg);

    /* extract the arguments */
    for(argc_valid=0,i=0; argc_valid<argc; )
    {
        /* string check */
        if((arg[i]=='\'') || (arg[i]=='\"'))
        {
            i++;
            start=arg+i;
            while(arg[i]!='\'' && arg[i]!='\"') i++;
            arg_s=va_arg(ap, char *);
            memcpy(arg_s, start, (size_t)(arg+i-start));    /* CAUTION: These may cause memory leak!!! */
            argc_valid++;

            i++;
            if(arg[i]=='\0') break;
            else i++;
        }
        else
        {
            /* float check */
            start=arg+i;
            int is_float=0;
            for(; arg[i]!=',' && arg[i]!='\0'; i++)
                if(arg[i]=='.')
                {
                    is_float=1;
                    break;
                }

            if(is_float)
            {
                arg_f=va_arg(ap, double *);
                *arg_f=strtod(start, &end);
                if(*end==',')
                {
                    i=end-arg+1;
                    argc_valid++;
                    if(arg[i]=='\0') break;
                }
                else if(*end=='\0')
                {
                    argc_valid++;
                    break;
                }
                else
                    break;
            }
            else
            {
                arg_d=va_arg(ap, int *);
                *arg_d=strtol(start, &end, 0);
                if(*end==',')
                {
                    i=end-arg+1;
                    argc_valid++;
                    if(arg[i]=='\0') break;
                }
                else if(*end=='\0')
                {
                    argc_valid++;
                    break;
                }
                else
                    break;
            }
        }

    }

    va_end(ap);

    return argc_valid;
}
