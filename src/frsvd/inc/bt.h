/*
 * bt.h
 *
 *  Created on: 2015-12-28
 *      Author: daiqingquan502
 */

#ifndef BT_H_
#define BT_H_

#include <stdio.h>
#include <string.h>

static inline const char * bt(void)
{
	static char __build_time_str[16]={0};

	int day, month, year, hour, minute, second;
	char mon_str[4];
	const char * mon_strs[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep", "Oct", "Nov","Dec"};

	sscanf(__DATE__, "%s %d %d", mon_str, &day, &year);
	sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

	for(month=1;month<=12;month++)
		if(0==strcmp(mon_strs[month-1], mon_str))
			break;

	sprintf(__build_time_str, "%04d%02d%02d.%02d%02d", year, month, day, hour, minute);

	return __build_time_str;
}

#endif /* BT_H_ */
