#ifndef __HARDWARE_DRIVER_H
#define __HARDWARE_DRIVER_H

#include "stdint.h"
#include "tim.h"
#include "spi.h"

void Motor_HardWare_Init(void);
void Motor_EnablePWM(void);
void Motor_DisEnablePWM(void);
void Motor_SetPWM(uint16_t tcmp1,uint16_t tcmp2,uint16_t tcmp3);
uint16_t MT6816_ReadData(void);


#endif

