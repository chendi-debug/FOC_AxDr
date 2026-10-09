/* ======================================================================
 * Vofa+配置文件
 *======================================================================*/
 
#include "Vofa.h"
#include "StateMachine.h"
#include "main.h"

VofaSend_TypeDef VofaSend;
 
 /**
   * @brief Vofa+发送函数
   * @param  
   * @retval 
   */
void Vofa_Send_Task(void)
 {
	 static uint32_t last_send_tick = 0;
	 uint32_t current_tick = HAL_GetTick();

	 if ((current_tick - last_send_tick) < 10U)
	 {
		 return;
	 }
	 last_send_tick = current_tick;

	/* 填入需要发送的数据*/
	 VofaSend.Send_Data_Array[0] = MotorSystem.Foc.Theta;
	 VofaSend.Send_Data_Array[1] = MotorSystem.Vf.Traget_Speed;
	 VofaSend.Send_Data_Array[2] = MotorSystem.Vf.Speed;
	 VofaSend.Send_Data_Array[3] = MotorSystem.Vf.theta;
	 
	 
	 
	 VofaSend.Send_Data_Array[4] = MotorSystem.Foc.Vv;	 
	 VofaSend.Send_Data_Array[5] = MotorSystem.Foc.Vw;
	 VofaSend.Send_Data_Array[6] = MotorSystem.Foc.Vu_Mod;
	 VofaSend.Send_Data_Array[7] = MotorSystem.Foc.Vv_Mod;
	 VofaSend.Send_Data_Array[8] = MotorSystem.Foc.Vw_Mod;	 	 
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
 
