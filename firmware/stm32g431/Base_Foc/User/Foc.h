#ifndef __FOC_H
#define __FOC_H

#include "math.h"
typedef struct
{
	float Udc;
	float Tpwm;
	float Vd;
	float Vq;
	float Theta;
	
	float Valpha;
	float Vbeta;
	
	float Vu;
	float Vv;
	float Vw;
	
	float Vu_Mod;
	float Vv_Mod;
	float Vw_Mod;
	
	int Tcmp1;
	int Tcmp2;
	int Tcmp3;
	
}Foc_Typedef;
/******************* 函数声明 **************************/
void Rev_Park_Tranf(Foc_Typedef *state);
void Rev_Clark_Tranf(Foc_Typedef *state);
void SVPWM_ZeroSqInject(Foc_Typedef *state);
void Foc_VoltageUpdate(Foc_Typedef *state);
void Set_Udc_Tpwm(Foc_Typedef *state,float Udc_temp,float Tpwm_temp);
/******************* 变量声明 **************************/
extern Foc_Typedef Foc;

#endif

