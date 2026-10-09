#ifndef __FOC_H
#define __FOC_H

#include "math.h"
#include "tim.h"
#include "Motor_Config.h"
typedef struct
{
	float Udc;					/* 母线电压 */
	float Tpwm;					/* 中断时间 */
	float Vd;						/* D轴电压 */
	float Vq;						/* Q轴电压 */
	float Theta;				/* 角度 */
	
	float Valpha;				/* Alpha轴电压 */
	float Vbeta;				/* Beta轴电压 */
	
	float Vu;						/* U相电压 */
	float Vv;						/* V相电压 */
	float Vw;						/* W相电压 */
	
	float Vu_Mod;				/* 调制后的U相电压 */
	float Vv_Mod;				/* 调制后的V相电压 */
	float Vw_Mod;				/* 调制后的W相电压 */
	
	int Tcmp1;					/* 调制后的U端电压 */
	int Tcmp2;					/* 调制后的V端电压 */
	int Tcmp3;					/* 调制后的W端电压 */
	
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

