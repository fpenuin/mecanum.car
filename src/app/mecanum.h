/*
 * mecanum.h
 *
 *  Created on: 2020Äê10ÔÂ27ÈÕ
 *      Author: fpenguin
 */

#ifndef APP_MECANUM_H_
#define APP_MECANUM_H_

#include <stdint.h>

typedef struct __Coord_t{

    /*??(????Cartesian)??*/
    float x;
    float y;
    /*???*/
    float p;
    float r;

}Coord_t;

void mCart2Polar(Coord_t * dot);
void mPolar2Cart(Coord_t * dot);

void mPhi2MaxRTabInit(void);
float mPhi2MaxR(float phi);


#endif /* APP_MECANUM_H_ */
