#ifndef __ADC_H
#define __ADC_H
#include "main.h"
extern ADC_HandleTypeDef hadc1;
/* PA0=IC、PA1=IB；故障的PA2不再采样，IA由KCL重构。 */
extern volatile uint16_t CurrentSense_Ic_Raw;
extern volatile uint16_t CurrentSense_Ib_Raw;
extern volatile uint16_t CurrentSense_Ic_Offset;
extern volatile uint16_t CurrentSense_Ib_Offset;
extern volatile int32_t CurrentSense_Ic_Count;
extern volatile int32_t CurrentSense_Ib_Count;
extern volatile int32_t CurrentSense_Ia_Count;
extern volatile uint16_t CurrentSense_CalibrationSamples;
extern volatile uint8_t CurrentSense_Calibrated;
extern volatile uint8_t CurrentSense_Valid;
extern volatile uint32_t CurrentSense_SequenceCount;
void MX_ADC1_Init(void);
HAL_StatusTypeDef CurrentSense_Start(void);
#endif
