#ifndef __VOFA_H
#define __VOFA_H

#include "stdint.h"
#include "string.h"
#include "usbd_cdc_if.h"

/* =========== 宏定义 ==========*/
#define data_num 5												/*定义发送数量*/
#define data_len (data_num * 4 + 4)				/*定义发送长度*/


/* =========== 发送结构体 ==========*/
typedef struct
{
	float Send_Data_Array[data_num];			/* 定义发送数量 */
	uint8_t Byte_Data_Array[data_len];		/* 定义发送长度 */
}VofaSend_TypeDef;


/*==============变量声明===================*/
//在进行变量声明的时候需要用extern进行修饰。在.h中使用extern 的本质，是为了给其他源文件提供一个“外部引用声明”，
//告诉编译器“这个符号在其他地方有定义，你先别分配空间，链接时再去找”，以此保证整个程序只有一个内存实体，满足C/C++的单一定义规则（ODR）。

extern VofaSend_TypeDef VofaSend;

/*==============函数声明===================*/
void Vofa_Send_Task(void);
void Vofa_Data_Process(void);

#endif

