/* ======================================================================
 * Vofa+配置文件
 *======================================================================*/
 
#include "Vofa.h"
#include "StateMachine.h"
#include "main.h"

#define VOFA_SEND_PERIOD_MS 1U

VofaSend_TypeDef VofaSend;
 /**
   * @brief Vofa+发送函数
   * @param  
   * @retval 
   */
void Vofa_Send_Task(void)
 {
	 static uint32_t last_send_tick = 0; /* 上一次遥测发送时刻，单位：ms */
	 uint32_t current_tick = HAL_GetTick(); /* 当前HAL时基，单位：ms */

	 if ((current_tick - last_send_tick) < VOFA_SEND_PERIOD_MS)
	 {
		 return;
	 }
	 last_send_tick = current_tick;

	/* M2零位校准观测；状态3表示成功，状态4表示样本不足失败。 */
	 /* I0~I3临时观察去零偏后的Ib/Ic ADC计数统计。 */
	 VofaSend.Send_Data_Array[0] = MotorSystem.IbCount_Average;
	 VofaSend.Send_Data_Array[1] = MotorSystem.IcCount_Average;
	 VofaSend.Send_Data_Array[2] = MotorSystem.IbCount_PeakToPeak;
	 VofaSend.Send_Data_Array[3] = MotorSystem.IcCount_PeakToPeak;
	 VofaSend.Send_Data_Array[4] = MotorSystem.Iq_Reference;                             /* 速度环输出，A */
	 VofaSend.Send_Data_Array[5] = MotorSystem.Encoder.Theta_Electrical;                 /* 校准电角度，rad */
	 VofaSend.Send_Data_Array[6] = MotorSystem.Encoder.Speed_rad_s;                      /* 滤波机械速度，rad/s */
	 /* I7~I9观察dq电压及限幅；I10~I13发送20kHz电流的1ms统计量。 */
	 VofaSend.Send_Data_Array[7] = MotorSystem.Foc.Vd;
	 VofaSend.Send_Data_Array[8] = MotorSystem.Foc.Vq;
	 VofaSend.Send_Data_Array[9] = CURRENT_LOOP_VOLTAGE_LIMIT_V;
	 VofaSend.Send_Data_Array[10] = MotorSystem.Id_Average;
	 VofaSend.Send_Data_Array[11] = MotorSystem.Iq_Average;
	 VofaSend.Send_Data_Array[12] = MotorSystem.Id_PeakToPeak;
	 VofaSend.Send_Data_Array[13] = MotorSystem.Iq_PeakToPeak;

	 Vofa_Data_Process();
 }
 /**
   * @brief Vofa+数据处理函数
   * @param  
   * @retval 
   */
 
void Vofa_Data_Process(void)
 {
	/*通过拷贝将浮点数据转换为单字节数据*/
	 memcpy(VofaSend.Byte_Data_Array,
	        VofaSend.Send_Data_Array,
	        data_num * sizeof(float));
 
	 /* 填补通信协议的帧尾*/
	 VofaSend.Byte_Data_Array[data_len - 4] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 3] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 2] = 0x80;
	 VofaSend.Byte_Data_Array[data_len - 1] = 0x7f;	 
	 
	 /* 发送串口数据*/
	 CDC_Transmit_FS((uint8_t*)VofaSend.Byte_Data_Array,data_len);
 }
 
