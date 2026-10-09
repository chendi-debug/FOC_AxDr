#include "HardWare_Driver.h"
#include "tim.h"

/**
  * @brief 电机硬件初始化
  * @param  
  * @retval 
  */
void Motor_HardWare_Init(void)
{
		Motor_EnablePWM();
	
	/* 开启中断 */
		HAL_TIM_Base_Start_IT(&htim6);
}
/**
  * @brief 使能PWM输出
  * @param  
  * @retval 
  */
void Motor_EnablePWM(void)
{
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
	
	HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_3);
	
}
/**
  * @brief 失能PWM输出
  * @param  
  * @retval 
  */
void Motor_DisEnablePWM(void)
{
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_3);
	
	HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_3);
	
}


/**
  * @brief 设置PWM占空比
  * @param  
  * @retval 
  */
void Motor_SetPWM(uint16_t tcmp1,uint16_t tcmp2,uint16_t tcmp3)
{
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,tcmp1);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,tcmp2);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,tcmp3);
}

