#ifndef __ENCONDER_H
#define __ENCONDER_H
#include "stdint.h"

/* 编码器状态。角度单位为rad，计数位置允许跨越单圈边界。 */
typedef struct
{
	uint8_t Dir;                  /* 机械正方向：0同原始计数，1反向 */
	uint32_t OneTurn_Count;          /* 方向修正后的当前单圈计数，范围0~Line-1；每次1kHz采样更新 */
	uint16_t Raw_Count;           /* MT6816原始计数，范围0~16383 */
	int16_t Delta_Count;          /* 相邻有效帧的最短有符号增量，单位：count */
	int32_t Position_Count;       /* 连续多圈位置，单位：count */
	float Theta;                  /* 单圈机械角，单位：rad */
	float Theta_Electrical_Raw;   /* 未加零偏的单圈电角度，范围[0,2pi)，单位：rad */
	float Theta_Electrical;       /* 加入零偏后的单圈电角度，范围[0,2pi)，单位：rad */
	float Electrical_Offset_rad;  /* 电角度零偏，当前观测阶段设为0，单位：rad */
	float Speed_rad_s;   /* 一阶低通后的机械角速度，单位：rad/s */
	float Speed_Filter_Tau_s;     /* 速度一阶低通时间常数，单位：s */
	uint8_t Speed_Valid;          /* 连续有效帧且时间差非零时为1 */
	uint8_t Valid;                /* 本次采样有效为1，失效为0 */
	uint8_t Initialized;          /* 前一有效帧的差分基准已建立为1 */
	uint8_t Position_Initialized; /* 首次连续位置已经建立为1，失联时保留 */
	uint32_t Last_Valid_Tick_ms;  /* 最近有效帧的HAL时基，单位：ms */
	uint16_t Line;                /* 每机械圈计数，本项目为16384 */
	uint8_t Pole_Pairs;           /* 电机极对数；DJI 2312S按7极对配置 */
}Enconder_TypeDef;

/** 初始化状态；除计数、方向和速度滤波外，pole_pairs为极对数，electrical_offset_rad为电角度零偏(rad)。 */
void Enconder_Init(Enconder_TypeDef *state,uint16_t line,uint8_t dir,float speed_filter_tau_s,uint8_t pole_pairs,float electrical_offset_rad);
/** 输入MT6816原始计数Data(0~line-1)及HAL时基tick_ms(ms)，更新机械角与连续位置。 */
void Enconder_Update(Enconder_TypeDef *state,uint32_t Data,uint32_t tick_ms);
/** 标记采样无效并更新年龄；tick_ms为当前HAL时基(ms)，保留连续位置。 */
void Enconder_MarkInvalid(Enconder_TypeDef *state,uint32_t tick_ms);
/** 设置电角度零偏并立即刷新校准电角度；offset_rad单位为rad。 */
void Enconder_SetElectricalOffset(Enconder_TypeDef *state,float offset_rad);

#endif
