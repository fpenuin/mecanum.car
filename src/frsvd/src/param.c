/*
 * param.c
 *
 *  Created on: 2014-12-9
 *      Author: daiqingquan502
 *  Update: 2015-05-29, dqq
 *  修改paramSave出错时的返回值。(原来所有错误都返回-1)
 *  增加头文件string.h。
 *  Update: 2015-12-21, dqq
 *  兼容性改善，修改ParamsBin_t成员数据类型，解决28335 char型位宽为16的问题。
 *  paramRegister在参数不正确的情况下返回 -2。
 *  Update: 2015-12-24, dqq
 *  增加crc校验，参数载入时校验不正确返回-2。
 *  Upadate: 2016-4-8, dqq
 *  增加paramListEx， 使用strstr实现查询参数名称查询功能。
 */

#include "../inc/param.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "list.h"

#define PARAM_MAGIC	(0x41524150)	//"PARA"

/*single parameter describe */
typedef struct _ParamEx_t{
	struct list_head 	list;
	void *				buf;	//data loaded form rom;
	Param_t				param;
}ParamEx_t;

/*struct to be saved on flash*/
typedef struct _ParamsBin_t{
	uint32_t	magic;
	uint32_t	crc;
	uint32_t	len;
	uint32_t	buf[1];		//make sure the memory continuance.
}ParamsBin_t;

static struct list_head __param_ex_list;
static unsigned __max_bin=0;
static SaveFun_t __save_fun;
static LoadFun_t __load_fun;

static ParamEx_t * __find(const char * name)
{
	struct list_head *pos, *n;
	ParamEx_t * param_ex;

	if(!name) return NULL;
	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		if(0==strcmp(name, param_ex->param.name))
			return param_ex;
	}

	return NULL;
}
static uint16_t __crc(void * buf, unsigned len)
{

	unsigned char * ptr=buf;
	int crc, i;

	crc=0;
	while(len--)
	{
		crc = crc^ (unsigned short)*ptr++ << 8;
		for(i=0; i<8; i++)
			if(crc & 0x8000)
				crc=crc<<1^0x1021;
			else
				crc=crc<<1;
	}

	return (uint16_t)(crc & 0xFFFF);
}

void paramInit(unsigned max_bin_in_bytes, SaveFun_t save, LoadFun_t load)
{
	INIT_LIST_HEAD(&__param_ex_list);
	__max_bin = max_bin_in_bytes;
	__save_fun = save;
	__load_fun = load;
}
Param_t * paramFind(const char * name)
{
	struct list_head *pos, *n;
	ParamEx_t * param_ex;

	if(!name) return NULL;
	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		if(0==strcmp(name, param_ex->param.name))
			return &param_ex->param;
	}

	return NULL;
}
Param_t * paramList(void)
{
	static struct list_head *__pos = NULL;
	static struct list_head *__n = NULL;
	ParamEx_t * param_ex;

	if((NULL==__pos) || (NULL==__n))
	{
		__pos = __param_ex_list.next;
		__n = __pos->next;
	}

	if(__pos != &__param_ex_list)
	{
		param_ex = list_entry(__pos, ParamEx_t, list);
		__pos = __n;
		__n = __pos->next;
		return &param_ex->param;
	}

	__pos = NULL;
	__n = NULL;
	return NULL;
}
Param_t * paramListEx(const char * name)
{
	static struct list_head *__pos = NULL;
	static struct list_head *__n = NULL;
	ParamEx_t * param_ex;

	if(!name) return NULL;

	if((NULL==__pos) || (NULL==__n))
	{
		__pos = __param_ex_list.next;
		__n = __pos->next;
	}

	while(__pos != &__param_ex_list)
	{
		param_ex = list_entry(__pos, ParamEx_t, list);
		__pos = __n;
		__n = __pos->next;

		if(NULL!=strstr(param_ex->param.name, name))
			return &param_ex->param;
	}

	__pos = NULL;
	__n = NULL;
	return NULL;
}
/*
 * After paramLoad and paramRegister, this will delete any parameter form list
 * that loaded but not registered.
 * */
void paramCleanup(void)
{
	struct list_head * pos, *n;
	ParamEx_t * param_ex;

	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		if(param_ex->buf == param_ex->param.data)
		{
			list_del(&param_ex->list);
			free(param_ex->param.data);
			free(param_ex);
		}
	}
}

int paramRegister(const char * name, unsigned int type, unsigned int len, void * p)
{
	if((!p) || (!name)) return -2;
	if(strlen(name)+1>PARAM_NAME_LEN) return -2;

	ParamEx_t * param_ex = __find(name);
	if(param_ex)
	{
		if(param_ex->param.len==len && param_ex->param.type==type)
		{
			param_ex->param.data = p;
			memcpy(param_ex->param.data, param_ex->buf, param_ex->param.len);
			return 0;
		}
		else
		{
			list_del(&param_ex->list);
			free(param_ex); param_ex=NULL;
		}
	}

	param_ex = (ParamEx_t *)malloc(sizeof(ParamEx_t));
	param_ex->param.data = malloc(len);
	if(!param_ex || !param_ex->param.data) return -1;

	strcpy(param_ex->param.name, name);
	param_ex->param.type = type;
	param_ex->param.len = len;
	param_ex->param.data = p;
	list_add_tail(&param_ex->list, &__param_ex_list);

	return 0;
}
int paramUnregister(const char * name)
{
	if(!name) return -2;
	if(strlen(name)+1>PARAM_NAME_LEN) return -2;

	ParamEx_t * param_ex = __find(name);
	if(param_ex)
	{
		list_del(&param_ex->list);
		free(param_ex->param.data); param_ex->param.data=NULL;
		free(param_ex); param_ex=NULL;
		return 0;
	}
	else
		return -1;
}

int paramSave(void)
{
	struct list_head * pos, *n;
	ParamEx_t * param_ex;
	unsigned len=0;

	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		len += sizeof(param_ex->param.name) + sizeof(param_ex->param.type) + sizeof(param_ex->param.len);
		len += param_ex->param.len;
	}

	ParamsBin_t * bin;
	unsigned char * buf;
	unsigned bin_len;

	bin_len = sizeof(ParamsBin_t)+len-sizeof(bin->buf);
	if(bin_len>__max_bin)
	{
		printf("bin_len=%u, __max_bin=%u.\r\n", bin_len, __max_bin);
		return -2;
	}

	bin = malloc(bin_len);
	if(!bin) return -3;

	buf = (unsigned char *)bin->buf;
	bin->magic = PARAM_MAGIC;
	bin->len = len;

	unsigned idx=0;
	unsigned name_len=sizeof(param_ex->param.name);
	unsigned type_len=sizeof(param_ex->param.type);
	unsigned len_len=sizeof(param_ex->param.len);

	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		memcpy(buf+idx, param_ex->param.name, name_len); idx+=name_len;
		memcpy(buf+idx, &param_ex->param.type, type_len); idx+=type_len;
		memcpy(buf+idx, &param_ex->param.len, len_len); idx+=len_len;
		memcpy(buf+idx, param_ex->param.data, param_ex->param.len); idx+=param_ex->param.len;
	}
	bin->crc = __crc(bin->buf, bin->len);

	int ret;
	ret = __save_fun(bin, bin_len);

	free(bin);

	if(ret!=0)
		return ret;
	else
		return 0;
}
int paramLoad(void)
{
	struct list_head * pos, *n;
	ParamEx_t * param_ex;

	list_for_each_safe(pos, n, &__param_ex_list)
	{
		param_ex = list_entry(pos, ParamEx_t, list);
		list_del(&param_ex->list);
		free(param_ex->param.data); param_ex->param.data=NULL;
		free(param_ex); param_ex=NULL;
	}

	ParamsBin_t * bin;
	int ret;

	bin = malloc(__max_bin);
	if(!bin) return -1;

	ret = __load_fun(bin, __max_bin);
	if(ret!=0) {free(bin);return ret;}

	if(bin->magic != PARAM_MAGIC) {free(bin);return 1;}
	if(bin->crc != __crc(bin->buf, bin->len)) {free(bin);return -2;}

	unsigned char * buf = (unsigned char *)bin->buf;

	unsigned idx=0;
	unsigned name_len=sizeof(param_ex->param.name);
	unsigned type_len=sizeof(param_ex->param.type);
	unsigned len_len=sizeof(param_ex->param.len);

	char name[PARAM_NAME_LEN];
	unsigned type, len;

	while(idx < bin->len)
	{
		memcpy(&name, buf+idx, name_len); idx+=name_len;
		memcpy(&type, buf+idx, type_len); idx+=type_len;
		memcpy(&len, buf+idx, len_len); idx+=len_len;

		param_ex = (ParamEx_t *)malloc(sizeof(ParamEx_t));
		if(!param_ex){free(bin);return -1;}
		param_ex->buf = malloc(len);
		if(!param_ex->buf)
		{
			free(bin);
			free(param_ex);
			return -1;
		}

		strcpy(param_ex->param.name, name);
		param_ex->param.type = type;
		param_ex->param.len = len;
		param_ex->param.data = param_ex->buf;
		memcpy(param_ex->buf, buf+idx, len);idx+=len;
		list_add_tail(&param_ex->list, &__param_ex_list);
	}

	return 0;
}

