/*
 * mecanum.c
 *
 *  Created on: 2020Äê10ÔÂ27ÈÕ
 *      Author: fpenguin
 */

#include "mecanum.h"

#include <math.h>

#include "stm32f4xx_hal.h"
#include "arm_math.h"


#define PHI_2_MAXR_TABLE_SIZE (360U)

static float __tbl_max_r[PHI_2_MAXR_TABLE_SIZE];
static float __tbl_phi[PHI_2_MAXR_TABLE_SIZE];
static float __tbl_phi_step=PI/(float)PHI_2_MAXR_TABLE_SIZE;
static float __tbl_phi_half_step=PI/(float)PHI_2_MAXR_TABLE_SIZE/2.0;


void mPolar2Cart(Coord_t * dot)
{
    float _sin, _cos;
    // arm_sin_cos_f32(dot->p, &_sin, &_cos);
    _sin=sinf(dot->p);
    _cos=cosf(dot->p);
    dot->x = dot->r * _cos;
    dot->y = dot->r * _sin;
}
void mCart2Polar(Coord_t * dot)
{
    static float double_pi=PI*2;

    float sum = dot->x*dot->x + dot->y*dot->y;
    // arm_sqrt_f32(sum, &dot->r); 
    dot->r = sqrtf(sum);

    float phi = asinf(dot->y/dot->r);
    if( (dot->x >= 0) && (dot->y >= 0) )
    {
        dot->p = phi;
    }    
    else if( (dot->x < 0) && (dot->y >= 0) )
    {
        dot->p = PI-phi;
    }
    else if( (dot->x < 0) && (dot->y < 0) )
    {
        dot->p = PI-phi;
    }
    else if( (dot->x >= 0) && (dot->y < 0) )
    {
        dot->p = phi+double_pi;
    }
    else
    {
        printf("%s, line%u: This is not suppose to happen.\r\n", \
            __func__, __LINE__);
    }
    

}

void mPhi2MaxRTabInit(void)
{
    float allowed_error = 0.00001F;

    uint32_t i=0;
    float phi=0;
    while(i<PHI_2_MAXR_TABLE_SIZE && phi<PI)
    {
        if( (phi-0.0<allowed_error) || \
            (phi-PI*0.5<allowed_error) || \
            (phi-PI*1.0<allowed_error) || \
            (phi-PI*1.5<allowed_error) )
        {
            __tbl_max_r[i]=1.0;
        }
        else if( phi>PI*1.75 )
        {
            __tbl_max_r[i] = 1/cosf(phi);
        }
        else if( phi>PI*1.25 )
        {
            __tbl_max_r[i] = 1/sinf(phi);
        }
        else if( phi>PI*0.75 )
        {
            __tbl_max_r[i] = 1/cosf(phi);
        }
        else if( phi>PI*0.25 )
        {
            __tbl_max_r[i] = 1/sinf(phi);
        }
        else 
        {
            __tbl_max_r[i] = 1/cosf(phi);
        }

        __tbl_phi[i]=phi;
        i++, phi+=__tbl_phi_step;
    }

}
float mPhi2MaxR(float phi)
{
    uint32_t idx=0;
    uint32_t mid=PHI_2_MAXR_TABLE_SIZE>>1;

    while(mid>1)
    {
        if( phi-__tbl_phi[idx] < __tbl_phi_half_step )
        {
            break;
        }
        else if(phi>__tbl_phi[idx])
        {
            mid = mid>>1;
            idx = idx+mid;
        }
        else if(phi<__tbl_phi[idx])
        {
            mid = mid>>1;
            idx = idx-mid;
        }
        else
        {
            printf("%s, line%u: This is not suppose to happen.\r\n", \
                __func__, __LINE__);
        }
    }

    printf("phi idx=%lu, phi=%f, max_r=%f.\r\n", \
                idx, __tbl_phi[idx], __tbl_max_r[idx]);
    
    return __tbl_max_r[idx];
}

