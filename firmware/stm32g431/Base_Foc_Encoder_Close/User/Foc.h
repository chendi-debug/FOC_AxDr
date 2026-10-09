#ifndef __FOC_H
#define __FOC_H

#include "math.h"
#include "tim.h"

#define  	 _2PI  								6.28318530f
typedef struct
{
	float Udc;					/* 母线电压 */
	float Tpwm;					/* 中断时间 */
	float Vd;						/* D轴电压 */
	float Vq;						/* Q轴电压 */
	float Theta;				/* 角度 */
	float Ia;						/* A相电流，A */
	float Ib;						/* B相电流，A */
	float Ic;						/* C相电流，A */
	float Ialpha;				/* alpha轴电流，A */
	float Ibeta;				/* beta轴电流，A */
	float Id;						/* d轴电流，A */
	float Iq;						/* q轴电流，A */
	float SinTheta;			/* 1kHz更新的电角度正弦 */
	float CosTheta;			/* 1kHz更新的电角度余弦 */
	
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

typedef struct
{
	float Kp;
	float Ki;
	float Ts;
	float Integrator;
	float OutputLimit;
} Foc_PIControllerTypeDef;

typedef struct
{
	/* 输入参数 */
	float Acc;							/* 速度的加速度 */
	float Traget_Speed;			/* 速度的目标值 */
	float Dead_Zone;				/* 死区值 */
	float Ts;								/* 每次进入中断的时间间隔 */
	
	
	/* 中间参数以及变量 */
	float Speed_Error;			/* 速度误差 */
	float Speed_Error_Dead;	/* 带有死区的速度误差 */
	float Acc_Dir;					/* 加速度方向 */
	float Speed_Integrator;	/* 速度积分器 */
	float Theta_Integrator;	/* 角度积分器 */
	
	
	/* 输出参数 */
	float Speed;						/* 实际速度 */
	float theta;						/* 实际角度 */
	
	
}Vf_SpeedControl_TypeDef;

/******************* 函数声明 **************************/
void Rev_Park_Tranf(Foc_Typedef *state);
void Rev_Clark_Tranf(Foc_Typedef *state);
void SVPWM_ZeroSqInject(Foc_Typedef *state);
void Foc_VoltageUpdate(Foc_Typedef *state);
void Foc_CurrentAngleUpdate(Foc_Typedef *state, float theta);
void Foc_CurrentTransform(Foc_Typedef *state, float ia, float ib, float ic);
void Foc_PIInit(Foc_PIControllerTypeDef *pi, float kp, float ki, float ts,
	float output_limit);
void Foc_PIReset(Foc_PIControllerTypeDef *pi);
float Foc_PIUpdate(Foc_PIControllerTypeDef *pi, float reference,
	float feedback);
void Set_Udc_Tpwm(Foc_Typedef *state,float Udc_temp,float Tpwm_temp);
void Vf_SpeedControl_Param_Init(Vf_SpeedControl_TypeDef *state,float acc,float target_speed,float dead_zone,float ts);
void Vf_SpeedControl_Updata(Vf_SpeedControl_TypeDef *state,float *Target_Theta_Add);
/******************* 变量声明 **************************/


#endif

