#include "Enconder.h"
#include "Motor_Math.h"

#define _2PI 6.283185307179f

/** 初始化编码器状态；pole_pairs为极对数，electrical_offset_rad为电角度零偏(rad)。 */
void Enconder_Init(Enconder_TypeDef *state,uint16_t line,uint8_t dir,float speed_filter_tau_s,uint8_t pole_pairs,float electrical_offset_rad)
{
	state->Line = line;
	state->Dir = dir;
	state->OneTurn_Count = 0U;
	state->Raw_Count = 0U;
	state->Delta_Count = 0;
	state->Position_Count = 0;
	state->Theta = 0.0f;
	state->Theta_Electrical_Raw = 0.0f;
	state->Theta_Electrical = 0.0f;
	state->Electrical_Offset_rad = electrical_offset_rad;
	state->Pole_Pairs = pole_pairs;
	state->Speed_rad_s = 0.0f;
	state->Speed_Filter_Tau_s = speed_filter_tau_s;
	state->Speed_Valid = 0U;
	state->Valid = 0U;
	state->Initialized = 0U;
	state->Position_Initialized = 0U;
	state->Last_Valid_Tick_ms = 0U;
}

/** 更新角度与位置；Data为原始计数，tick_ms为HAL时基(ms)。 */
void Enconder_Update(Enconder_TypeDef *state,uint32_t Data,uint32_t tick_ms)
{
	int32_t delta;               /* 相邻采样增量，单位：count */
	uint16_t pulse_data;         /* 方向修正后的本帧单圈计数 */
	uint32_t elapsed_ms;         /* 相邻有效采样实际间隔，单位：ms */
	uint8_t had_baseline;        /* 本帧之前已有连续有效采样基准 */
	float dt_s;                  /* 相邻有效采样实际间隔，单位：s */
	float speed_raw_rad_s;       /* 当前有效帧计算出的未滤波机械角速度，单位：rad/s */
	float alpha;                 /* 一阶低通更新权重，范围0~1 */

	if (state->Line == 0U || Data >= state->Line)
	{
		Enconder_MarkInvalid(state,tick_ms);
		return;
	}

	/* 先统一机械正方向，所有输出使用相同符号约定。 */
	if(state->Dir == 0U)
	{
		pulse_data = (uint16_t)Data;
	}
	else
	{
		pulse_data = (uint16_t)((state->Line - Data) % state->Line);
	}
	had_baseline = state->Initialized;
	elapsed_ms = tick_ms - state->Last_Valid_Tick_ms;
	state->Raw_Count = (uint16_t)Data;
	state->Valid = 1U;
	state->Last_Valid_Tick_ms = tick_ms;
	if (state->Initialized == 0U)
	{
		/* 首次采样或失联恢复首帧只建立差分基准，跳过未知时段位移。 */
		state->Initialized = 1U;
		state->Delta_Count = 0;
		if (state->Position_Initialized == 0U)
		{
			/* 上电后用第一帧建立连续位置的初始绝对位置。 */
			state->Position_Count = pulse_data;
			state->Position_Initialized = 1U;
		}
	}
	else
	{
		/* 取模line的最短增量，避免跨零时出现整圈假跳变。 */
		delta = (int32_t)pulse_data - (int32_t)state->OneTurn_Count;
		if (delta > ((int32_t)state->Line / 2))
		{
			delta -= state->Line;
		}
		else if (delta < -((int32_t)state->Line / 2))
		{
			delta += state->Line;
		}
		state->Delta_Count = (int16_t)delta;
		state->Position_Count += delta;
	}
	state->OneTurn_Count = pulse_data;
	state->Theta = state->OneTurn_Count / (float)state->Line * _2PI;
	state->Theta_Electrical_Raw = Motor_Wrap_0_2pi(
		(float)state->Pole_Pairs * state->Theta);
	state->Theta_Electrical = Motor_Wrap_0_2pi(
		state->Theta_Electrical_Raw + state->Electrical_Offset_rad);
	if (had_baseline != 0U && elapsed_ms != 0U)
	{
		dt_s = (float)elapsed_ms * 0.001f;
		speed_raw_rad_s = (float)state->Delta_Count * _2PI / ((float)state->Line * dt_s);
		/* 离散一阶低通：alpha=dt/(tau+dt)，tau<=0表示不过滤。 */
		alpha = state->Speed_Filter_Tau_s > 0.0f
			? dt_s / (state->Speed_Filter_Tau_s + dt_s) : 1.0f;
		state->Speed_rad_s += alpha *
			(speed_raw_rad_s - state->Speed_rad_s);
		state->Speed_Valid = 1U;
	}
	else
	{
		/* 首帧、失联恢复首帧或同一毫秒内重复采样不能估计速度。 */
		state->Speed_Valid = 0U;
	}
}

/** 设置并归一化电角度零偏，同时刷新当前校准电角度。 */
void Enconder_SetElectricalOffset(Enconder_TypeDef *state,float offset_rad)
{
	state->Electrical_Offset_rad = Motor_Wrap_0_2pi(offset_rad);
	state->Theta_Electrical = Motor_Wrap_0_2pi(
		state->Theta_Electrical_Raw + state->Electrical_Offset_rad);
}

/** 采样无效时保留最近角度和连续位置，恢复首帧仅重建差分基准。 */
void Enconder_MarkInvalid(Enconder_TypeDef *state,uint32_t tick_ms)
{
	(void)tick_ms;
	state->Valid = 0U;
	state->Speed_Valid = 0U;
	state->Speed_rad_s = 0.0f;
	state->Delta_Count = 0;
	state->Initialized = 0U;
}
