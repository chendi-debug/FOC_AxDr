#include "HardWare_Driver.h"


#define MT6816_READ_CMD          0x80U
#define MT6816_ANGLE_HIGH_REG    0x03U
#define MT6816_ANGLE_LOW_REG     0x04U
#define MT6816_ANGLE_MASK        0x3FFFU
#define MT6816_SPI_TIMEOUT_MS    10U
#define MT6816_READ_ERROR        0xFFFFU

/**
  * @brief  使用一个16位SPI帧读取MT6816寄存器
  * @note   命令字节按照MSB优先发送；两次寄存器访问之间必须先拉高CS，
  *         保证每条读命令都从一个新的通信帧开始。
  */
static HAL_StatusTypeDef MT6816_ReadRegister(uint8_t reg, uint8_t *data)
{
	uint16_t tx = (uint16_t)(MT6816_READ_CMD | reg) << 8;
	uint16_t rx = 0;
	HAL_StatusTypeDef status;

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
	status = HAL_SPI_TransmitReceive(&hspi1,
										 (uint8_t *)&tx,
										 (uint8_t *)&rx,
										 1,
										 MT6816_SPI_TIMEOUT_MS);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);

	if (status == HAL_OK)
	{
		/* MT6816将所选寄存器的数据返回到16位接收帧的低8位。 */
		*data = (uint8_t)(rx & 0x00FFU);
	}

	return status;
}

/**
  * @brief 电机硬件初始化
  * @param  
  * @retval 
  */
void Motor_HardWare_Init(void)
{
	/* 显式启动CH4，使内部比较边沿可以触发ADC1注入转换。 */
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);
	/* CH4仅作为ADC硬件触发源；快速环在ADC注入序列完成后执行。 */
	/* PWM使能 */
	Motor_EnablePWM();
}

/**
  * @brief PWM使能初始化
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
  * @brief PWM失能初始化
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

void Motor_SetPWM(uint16_t Tcmp1,uint16_t Tcmp2,uint16_t Tcmp3)
{
	/* 算出来的比较值赋值给三路的CCR*/
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,Tcmp1);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,Tcmp2);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,Tcmp3);
	
}


/**
  * @brief  读取MT6816的14位原始角度
  * @retval 0~16383为有效角度，0xFFFF表示SPI通信失败
  */
uint16_t MT6816_ReadData(void)
{
	uint8_t angle_high;
	uint8_t angle_low;
	uint16_t raw_angle;

	/* 0x03和0x04返回的16位数据中，位[15:2]为14位绝对角度。 */
	if (MT6816_ReadRegister(MT6816_ANGLE_HIGH_REG, &angle_high) != HAL_OK)
	{
		return MT6816_READ_ERROR;
	}

	if (MT6816_ReadRegister(MT6816_ANGLE_LOW_REG, &angle_low) != HAL_OK)
	{
		return MT6816_READ_ERROR;
	}

	raw_angle = ((uint16_t)angle_high << 8) | angle_low;
	return (raw_angle >> 2) & MT6816_ANGLE_MASK;
}
