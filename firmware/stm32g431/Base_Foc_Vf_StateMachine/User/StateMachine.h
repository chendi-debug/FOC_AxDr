#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include "Foc.h"
#include "HardWare_Driver.h"
#include "Motor_Config.h"


typedef struct
{
	
		Foc_Typedef     Foc;
}Motor_Typedef;

void Motor_StateMachine_Init(Motor_Typedef *state);
void Motor_StateMachine_Run(Motor_Typedef *state);

extern Motor_Typedef MotorSystem;


#endif

