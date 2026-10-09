/* ======================================================================
 * Vofa+配置文件
 *======================================================================*/
 
#include "Vofa.h"
#include "Foc.h"
VofaSend_TypeDef VofaSend;
 
 /**
   * @brief Vofa+发送函数
   * @param  
   * @retval 
   */
void Vofa_Send_Task(void)
 {
	/* 填入需要发送的数据*/
	 VofaSend.Send_Data_Array[0] = Foc.Theta;
	 VofaSend.Send_Data_Array[1] = Foc.Valpha;
	 VofaSend.Send_Data_Array[2] = Foc.Vbeta;
	 VofaSend.Send_Data_Array[3] = Foc.Vu;
	 VofaSend.Send_Data_Array[4] = Foc.Vv;	 
	 VofaSend.Send_Data_Array[5] = Foc.Vw;
	 VofaSend.Send_Data_Array[6] = Foc.Vu_Mod;
	 VofaSend.Send_Data_Array[7] = Foc.Vv_Mod;
	 VofaSend.Send_Data_Array[8] = Foc.Vw_Mod;	 	 
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
	        sizeof(VofaSend.Send_Data_Array));
 
	 /* 填补通信协议的帧尾*/
	 VofaSend.Byte_Data_Array[data_len - 4] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 3] = 0x00;
	 VofaSend.Byte_Data_Array[data_len - 2] = 0x80;
	 VofaSend.Byte_Data_Array[data_len - 1] = 0x7f;	 
	 
	 /* 发送串口数据*/
	 CDC_Transmit_FS((uint8_t*)VofaSend.Byte_Data_Array,data_len);
 }
 
