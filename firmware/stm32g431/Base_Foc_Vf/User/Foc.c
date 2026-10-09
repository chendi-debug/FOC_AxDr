#include "Foc.h"

Foc_Typedef Foc;

/**
  * @brief 反Park变换
  * @param  Foc结构体指针
  * @retval 
  */
void Rev_Park_Tranf(Foc_Typedef *state)
{
	state->Valpha = state->Vd * cosf(state->Theta) - state->Vq * sinf(state->Theta);
	state->Vbeta  = state->Vd * sinf(state->Theta) + state->Vq * cosf(state->Theta);	
}

/**
  * @brief 反Clark变换
  * @param  
  * @retval 
  */
void Rev_Clark_Tranf(Foc_Typedef *state)
{
	state->Vu = state->Valpha;
	state->Vv = -0.5f * state->Valpha + 0.866f * state->Vbeta;
	state->Vw = -0.5f * state->Valpha - 0.866f * state->Vbeta;
}

/**
  * @brief 基于零序分量注入的SVPWM
	* @param  state:FOC结构体指针
  * @retval 
  */
void SVPWM_ZeroSqInject(Foc_Typedef *state)
{
	/* 寻找三相最大电压 */
	float Max = (state->Vu > state->Vv) ? (state->Vu > state->Vw ? state->Vu : state->Vw) : (state->Vv > state->Vw ? state->Vv : state->Vw);
	/* 寻找三相最小电压 */
	float Min = (state->Vu < state->Vv) ? (state->Vu < state->Vw ? state->Vu : state->Vw) : (state->Vv < state->Vw ? state->Vv : state->Vw);
	float V0 = -0.5f*(Max+Min);
	
	/* 计算三相马鞍波电压*/
	state->Vu_Mod = state->Vu + V0;
	state->Vv_Mod = state->Vv + V0;
	state->Vw_Mod = state->Vw + V0;	
	
	/* 将三相马鞍波电压转换成 定时器PWM的比较值*/
	state->Tcmp1 = ((state->Vu_Mod / UDC) + 0.5f) * TPWM;
	state->Tcmp2 = ((state->Vv_Mod / UDC) + 0.5f) * TPWM;
	state->Tcmp3 = ((state->Vw_Mod / UDC) + 0.5f) * TPWM;

	TIM1->CCR1 = state->Tcmp1;
	TIM1->CCR2 = state->Tcmp2;
	TIM1->CCR3 = state->Tcmp3;
}

/**
  * @brief 设置母线电压与pwm的周期
  * @param  
  * @retval 
  */
void Set_Udc_Tpwm(Foc_Typedef *state,float Udc_temp,float Tpwm_temp)
{
	state->Udc = Udc_temp;
	state->Tpwm = Tpwm_temp;
}

/************************** 以上的函数计算完之后要更新的 ********************************/

/**
  * @brief 坐标变换电压更新函数 给3个函数打包成1个函数，再送到TIM的定时器中断里面
  * @param  
  * @retval 
  */
void Foc_VoltageUpdate(Foc_Typedef *state)
{
	/* 反Park变换 */
	Rev_Park_Tranf(state);
	/* 反Clark变换 */
	Rev_Clark_Tranf(state);
	/* SVPWM */
	SVPWM_ZeroSqInject(state);
}

