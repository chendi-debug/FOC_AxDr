#include "StateMachine.h"


Motor_Typedef MotorSystem;

void Motor_StateMachine_Init(Motor_Typedef *state)
{
	/* 硬件初始化 */
	Motor_HardWare_Init();
	
	/*设置电机母线电压和PWM周期 */
	Set_Udc_Tpwm(&state->Foc,UDC,TPWM);
	
	
	/* 初始化Vf参数 */
	Vf_SpeedControl_Param_Init(&state->Vf,50.0f,0.0f,0.0001,0.00005f);
	
}

void Motor_StateMachine_Run(Motor_Typedef *state)
{
	
	/* 设置电机角度 */
	Vf_SpeedControl_Updata(&state->Vf,&state->Foc.Theta);
	
	/*Vd = 0 Vq = 1 Theta */
	state->Foc.Vd = 0.0f;
//	state->Foc.Vq = 0.0f;
	
	/* 坐标变换更新函数 */
	Foc_VoltageUpdate(&state->Foc);

	/*设置PWM zhankongbi*/
	Motor_SetPWM(state->Foc.Tcmp1,state->Foc.Tcmp2,state->Foc.Tcmp3);
}


