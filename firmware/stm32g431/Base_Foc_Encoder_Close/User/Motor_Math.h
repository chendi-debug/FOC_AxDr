#ifndef __MOTOR_MATH_H
#define __MOTOR_MATH_H

#include "math.h"

#define MOTOR_TWO_PI 6.283185307179f

/** 将任意弧度角度限制到[0, 2pi)，供机械角、电角度和观测器统一使用。 */
static inline float Motor_Wrap_0_2pi(float angle_rad)
{
    float wrapped = fmodf(angle_rad, MOTOR_TWO_PI); /* 保留一圈内的余数，单位：rad */
    return wrapped < 0.0f ? wrapped + MOTOR_TWO_PI : wrapped;
}

#endif
