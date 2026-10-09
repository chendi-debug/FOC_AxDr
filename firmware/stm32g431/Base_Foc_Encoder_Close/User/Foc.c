#include "Foc.h"
#include "math.h"

#define ONE_OVER_SQRT3 0.57735026919f

static float Foc_Clamp(float value, float limit)
{
	if (value > limit)
	{
		return limit;
	}
	if (value < -limit)
	{
		return -limit;
	}
	return value;
}

void Foc_PIInit(Foc_PIControllerTypeDef *pi, float kp, float ki, float ts,
	float output_limit)
{
	pi->Kp = kp;
	pi->Ki = ki;
	pi->Ts = ts;
	pi->Integrator = 0.0f;
	pi->OutputLimit = output_limit;
}

void Foc_PIReset(Foc_PIControllerTypeDef *pi)
{
	pi->Integrator = 0.0f;
}

float Foc_PIUpdate(Foc_PIControllerTypeDef *pi, float reference,
	float feedback)
{
	float error = reference - feedback;
	float proportional = pi->Kp * error;
	float candidate_integrator = pi->Integrator + pi->Ki * error * pi->Ts;
	float candidate_output = proportional + candidate_integrator;
	float limited_output = Foc_Clamp(candidate_output, pi->OutputLimit);

	/* 条件积分抗饱和：饱和加深时暂停积分，误差反向时允许退出饱和。 */
	if ((candidate_output == limited_output) ||
		((limited_output >= pi->OutputLimit) && (error < 0.0f)) ||
		((limited_output <= -pi->OutputLimit) && (error > 0.0f)))
	{
		pi->Integrator = Foc_Clamp(candidate_integrator, pi->OutputLimit);
	}

	return Foc_Clamp(proportional + pi->Integrator, pi->OutputLimit);
}

void Foc_CurrentAngleUpdate(Foc_Typedef *state, float theta)
{
	state->Theta = theta;
	state->SinTheta = sinf(theta);
	state->CosTheta = cosf(theta);
}

void Foc_CurrentTransform(Foc_Typedef *state, float ia, float ib, float ic)
{
	state->Ia = ia;
	state->Ib = ib;
	state->Ic = ic;
	state->Ialpha = ia;
	state->Ibeta = (ia + 2.0f * ib) * ONE_OVER_SQRT3;
	state->Id = state->Ialpha * state->CosTheta +
		state->Ibeta * state->SinTheta;
	state->Iq = -state->Ialpha * state->SinTheta +
		state->Ibeta * state->CosTheta;
}

/**
  * @brief 反Park变换
  * @param  Foc结构体指针
  * @retval 
  */
void Rev_Park_Tranf(Foc_Typedef *state)
{
	/* 与Park共用20kHz预测角度，避免正反变换使用不同时间点的theta。 */
	state->Valpha = state->Vd * state->CosTheta - state->Vq * state->SinTheta;
	state->Vbeta  = state->Vd * state->SinTheta + state->Vq * state->CosTheta;	
}

/**
  * @brief 反Clark变换
  * @param  
  * @retval 
  */
void Rev_Clark_Tranf(Foc_Typedef *state)
{
	state->Vu = state->Valpha;
	state->Vv = -0.5f * state->Valpha + 0.86f * state->Vbeta;
	state->Vw = -0.5f * state->Valpha - 0.86f * state->Vbeta;
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
	state->Tcmp1 = ((state->Vu_Mod / state->Udc) + 0.5f) * state->Tpwm;
	state->Tcmp2 = ((state->Vv_Mod / state->Udc) + 0.5f) * state->Tpwm;
	state->Tcmp3 = ((state->Vw_Mod / state->Udc) + 0.5f) * state->Tpwm;

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
/**
  * @brief Vf速度控制参数初始化
  * @param  
  * @retval 
  */
void Vf_SpeedControl_Param_Init(Vf_SpeedControl_TypeDef *state,float acc,float target_speed,float dead_zone,float ts)
{
	state->Acc = acc;
	state->Traget_Speed = target_speed;
	state->Dead_Zone = dead_zone;
	state->Ts = ts;
	
}
/**
  * @brief VF速度控制周期函数
  * @param  
  * @retval 
  */
void Vf_SpeedControl_Updata(Vf_SpeedControl_TypeDef *state,float *Target_Theta_Add)
{
	state->Speed_Error = state->Traget_Speed - state->Speed_Integrator;
	
	/* 死区处理 */
	if(fabs(state->Speed_Error) < state->Dead_Zone)
	{
		state->Speed_Error_Dead = 0;
	}
	else 
	{
		state->Speed_Error_Dead = state->Speed_Error;
	}
	
	/* 加速度方向判断 */
	if(state->Speed_Error_Dead > 0)
	{
		state->Acc_Dir = 1;
	}
	else if(state->Speed_Error_Dead < 0)
	{
		state->Acc_Dir = -1;
	}
	else
	{
		state->Acc_Dir = 0;
	}
	
	/* 加速度积分得到速度 */
	state->Speed_Integrator += state->Acc * state->Acc_Dir * state->Ts;
	
	/* 速度积分得到角度*/
	state->Theta_Integrator += state->Speed_Integrator * state->Ts;
	
	/* 输出角度 */
	state->theta = fmodf(state->Theta_Integrator,_2PI);
	if(state->theta < 0)
	{
		state->theta += _2PI;
	}
	/* 输出速度 */
	state->Speed = state->Speed_Integrator;
	
	/* 将计算的Theta使用指针传出去 */
	*Target_Theta_Add = state->theta;
	
}

