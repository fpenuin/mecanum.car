/*
 * mdebug.c
 *
 *  Created on: 2015-1-4
 *      Author: daiqingquan502
 *  Update: 2015-12-21, dqq
 *  兼容性改善，解决28335 char型位宽为16的问题。
 */

#include "../inc/mdebug.h"

#include <stdio.h>
#include <string.h>

#include "memx.h"


void memDump(uint32_t start_addr, uint32_t len_in_byte)
{
	char line[16];
	uint32_t i,j;
	uint32_t start=start_addr>>4<<4;
	uint32_t end=(start_addr+len_in_byte)>>4<<4;

	if(end<start_addr+len_in_byte)
		end += 16;

	for(i=start;i<end;i+=16)
	{
		memcpy(line, (void *)i, 16);

#ifdef __TMS320C2000__
		printf("0x%08lX: ", i);
		for(j=0;j<16;j++)
		{
			if(i+j<start_addr || i+j>=start_addr+len_in_byte)
				printf("     ");
			else
				printf("%04X ", line[j]&0xffff);
		}
#else
		printf("0x%08lX: ", i);
		for(j=0;j<16;j++)
		{
			if(i+j<start_addr || i+j>=start_addr+len_in_byte)
				printf("   ");
			else
				printf("%02X ", line[j]&0xff);
		}
#endif
		printf("; ");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
		for(j=0;j<16;j++)
			if(i+j<start_addr || i+j>=start_addr+len_in_byte)
				putchar(' ');
			else if(line[j]<32 || line[j]==127 || line[j]>255)
				putchar('-');
			else
				putchar(line[j]);
#pragma GCC diagnostic pop

		printf("\r\n");
	}
}

int memChk(uint32_t start_addr, uint32_t len_in_byte)
{
	uint32_t i,v;
	uint32_t start=start_addr>>2<<2;
	uint32_t end=((start_addr+len_in_byte)>>2<<2)-4;

	printf("Memory test start at 0x%08lx, end at 0x%08lx...\r\n", start, end);

	uint32_t progress_now, progress;
	uint32_t len=end-start;

	printf("Part 1:\r\n");
	printf("Writing...\r\n");
	progress=progress_now=0;
	printf("%lu%%", progress);
	for(i=start;i<=end;i+=4)
	{
		MEM32(i) = i;
		progress_now=(uint32_t)((uint64_t)(i-start)*100L/len);
		if(progress_now!=progress)
		{
			progress=progress_now;
			if(progress<=10) printf("\b\b");
			else printf("\b\b\b");
			printf("%lu%%", progress);
		}
		if(0xFFFFFFFC==i)break;
	}
	printf("\r\n");

	printf("Checking...\r\n");
	progress=progress_now=0;
	printf("%lu%%", progress);
	for(i=start;i<=end;i+=4)
	{
		v=MEM32(i);
		if(v!=i)
		{
			printf("Memory check failed at address 0x%08lx: expect=0x%08lx, actual=0x%08lx.\r\n", i, i, v);
			return -1;
		}
		progress_now=(uint32_t)((uint64_t)(i-start)*100L/len);
		if(progress_now!=progress)
		{
			progress=progress_now;
			if(progress<=10) printf("\b\b");
			else printf("\b\b\b");
			printf("%lu%%", progress);
		}
		if(0xFFFFFFFC==i)break;
	}
	printf("\r\n");

	printf("Part 2:\r\n");
	printf("Writing...\r\n");
	progress=progress_now=0;
	printf("%lu%%", progress);
	for(i=start;i<=end;i+=4)
	{
		MEM32(i) = ~i;
		progress_now=(uint32_t)((uint64_t)(i-start)*100L/len);
		if(progress_now!=progress)
		{
			progress=progress_now;
			if(progress<=10) printf("\b\b");
			else printf("\b\b\b");
			printf("%lu%%", progress);
		}
		if(0xFFFFFFFC==i)break;
	}
	printf("\r\n");

	printf("Checking...\r\n");
	progress=progress_now=0;
	printf("%lu%%", progress);
	for(i=start;i<=end;i+=4)
	{
		v=MEM32(i);
		if(v!=(~i))
		{
			printf("Memory check failed at address 0x%08lx: expect=0x%08lx, actual=0x%08lx.\r\n", i, ~i, v);
			return -1;
		}
		progress_now=(uint32_t)((uint64_t)(i-start)*100L/len);
		if(progress_now!=progress)
		{
			progress=progress_now;
			if(progress<=10) printf("\b\b");
			else printf("\b\b\b");
			printf("%lu%%", progress);
		}
		if(0xFFFFFFFC==i)break;
	}
	printf("\r\n");

	printf("Memory test passed.\r\n");

	return 0;
}

