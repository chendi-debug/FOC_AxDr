#include "StateMachine.h"


Motor_Typedef MotorSystem;

void Motor_StateMachine_Init(Motor_Typedef *state)
{
	Motor_HardWare_Init();
	
	/*Set Motor Udc and PWM */
	Set_Udc_Tpwm(&state->Foc,UDC,TPWM);
	
	
}

void Motor_StateMachine_Run(Motor_Typedef *state)
{
	/*Vd = 0 Vq = 1 Theta */
	static float Theta = 0;
	Theta += 0.01f;
	if(Theta >= _2PI)
	{
		Theta -= _2PI;
	}
	state->Foc.Theta = Theta;
	state->Foc.Vd = 0;
	state->Foc.Vq = 1;
	
	Foc_VoltageUpdate(&state->Foc);

	/*ÉèÖÃPWM zhankongbi*/
	Motor_SetPWM(state->Foc.Tcmp1,state->Foc.Tcmp2,state->Foc.Tcmp3);
}