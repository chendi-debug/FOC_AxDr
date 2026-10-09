/* ======================================================================
 * Vofa+配置文件
 *======================================================================*/
 
#include "Vofa.h"

VofaSend_TypeDef VofaSend;
 
 /**
   * @brief Vofa+发送函数
   * @param  
   * @retval 
   */
void Vofa_Send_Task(void)
 {
	/* 填入需要发送的数据*/
	 VofaSend.Send_Data_Array[0] = 1;
	 VofaSend.Send_Data_Array[1] = 2;
	 VofaSend.Send_Data_Array[2] = 3;
	 VofaSend.Send_Data_Array[3] = 4;
	 VofaSend.Send_Data_Array[4] = 5;	 
	 
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
	 memcpy(VofaSend.Byte_Data_Array,VofaSend.Send_Data_Array,data_len);
 
	 /* 填补通信协议的帧尾*/
	 VofaSend.Byte_Data_Array[data_len - 4] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 3] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 2] = 0x80;
	 VofaSend.Byte_Data_Array[data_len - 1] = 0x7f;	 
	 
	 /* 发送串口数据*/
	 CDC_Transmit_FS((uint8_t*)VofaSend.Byte_Data_Array,data_len);
 }
 
