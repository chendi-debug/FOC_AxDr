#include "StateMachine.h"
#include "Motor_Math.h"
#include "adc.h"

Motor_Typedef MotorSystem; /* 全局电机控制状态 */

#define CURRENT_STATISTICS_SAMPLE_COUNT 20U

static void Motor_CurrentStatistics_Update(Motor_Typedef *state)
{
	static float id_sum;
	static float iq_sum;
	static float id_min;
	static float id_max;
	static float iq_min;
	static float iq_max;
	static int32_t ib_count_sum;
	static int32_t ic_count_sum;
	static int32_t ib_count_min;
	static int32_t ib_count_max;
	static int32_t ic_count_min;
	static int32_t ic_count_max;
	static uint8_t sample_count;
	float id = state->Foc.Id;
	float iq = state->Foc.Iq;
	int32_t ib_count = CurrentSense_Ib_Count;
	int32_t ic_count = CurrentSense_Ic_Count;

	if (sample_count == 0U)
	{
		id_sum = id;
		iq_sum = iq;
		id_min = id;
		id_max = id;
		iq_min = iq;
		iq_max = iq;
		ib_count_sum = ib_count;
		ic_count_sum = ic_count;
		ib_count_min = ib_count;
		ib_count_max = ib_count;
		ic_count_min = ic_count;
		ic_count_max = ic_count;
	}
	else
	{
		id_sum += id;
		iq_sum += iq;
		if (id < id_min) id_min = id;
		if (id > id_max) id_max = id;
		if (iq < iq_min) iq_min = iq;
		if (iq > iq_max) iq_max = iq;
		ib_count_sum += ib_count;
		ic_count_sum += ic_count;
		if (ib_count < ib_count_min) ib_count_min = ib_count;
		if (ib_count > ib_count_max) ib_count_max = ib_count;
		if (ic_count < ic_count_min) ic_count_min = ic_count;
		if (ic_count > ic_count_max) ic_count_max = ic_count;
	}

	sample_count++;
	if (sample_count >= CURRENT_STATISTICS_SAMPLE_COUNT)
	{
		state->Id_Average = id_sum / (float)CURRENT_STATISTICS_SAMPLE_COUNT;
		state->Iq_Average = iq_sum / (float)CURRENT_STATISTICS_SAMPLE_COUNT;
		state->Id_PeakToPeak = id_max - id_min;
		state->Iq_PeakToPeak = iq_max - iq_min;
		state->IbCount_Average = (float)ib_count_sum /
			(float)CURRENT_STATISTICS_SAMPLE_COUNT;
		state->IcCount_Average = (float)ic_count_sum /
			(float)CURRENT_STATISTICS_SAMPLE_COUNT;
		state->IbCount_PeakToPeak = (float)(ib_count_max - ib_count_min);
		state->IcCount_PeakToPeak = (float)(ic_count_max - ic_count_min);
		sample_count = 0U;
	}
}

static void Motor_CurrentTransform_Update(Motor_Typedef *state)
{
	int32_t ib_count;
	int32_t ic_count;
	float ib;
	float ic;
	float ia;

	if (CurrentSense_Valid == 0U)
	{
		return;
	}

	ib_count = CurrentSense_Ib_Count;
	ic_count = CurrentSense_Ic_Count;
	/* 统一电流正方向：正Vq产生正Iq。 */
	ib = -(float)ib_count * CURRENT_AMPS_PER_COUNT;
	ic = -(float)ic_count * CURRENT_AMPS_PER_COUNT;
	ia = -(ib + ic);
	Foc_CurrentTransform(&state->Foc, ia, ib, ic);
}

static void Motor_CurrentAnglePredict_20kHz(Motor_Typedef *state)
{
	float theta_step;
	float theta_predict;

	if (state->Alignment.Status == MOTOR_ALIGNMENT_SETTLING ||
		state->Alignment.Status == MOTOR_ALIGNMENT_SAMPLING)
	{
		Foc_CurrentAngleUpdate(&state->Foc, 0.0f);
		return;
	}

	if (state->Encoder.Valid == 0U)
	{
		return;
	}

	if (state->Encoder.Speed_Valid == 0U)
	{
		Foc_CurrentAngleUpdate(&state->Foc,
			state->Encoder.Theta_Electrical);
		return;
	}

	/* 编码器1kHz给真值；其间按机械速度预测20kHz电角度。 */
	theta_step = state->Encoder.Speed_rad_s *
		(float)state->Encoder.Pole_Pairs * CURRENT_LOOP_TS_S;
	theta_predict = Motor_Wrap_0_2pi(state->Foc.Theta + theta_step);
	Foc_CurrentAngleUpdate(&state->Foc, theta_predict);
}

static uint8_t Motor_CurrentIsOverLimit(const Foc_Typedef *foc)
{
	return (fabsf(foc->Ia) > CURRENT_LOOP_OVERCURRENT_A) ||
		(fabsf(foc->Ib) > CURRENT_LOOP_OVERCURRENT_A) ||
		(fabsf(foc->Ic) > CURRENT_LOOP_OVERCURRENT_A);
}

static void Motor_CurrentLoopTrip(Motor_Typedef *state, uint8_t fault_code)
{
	/* 锁存故障发生瞬间的关键量，避免随后停机逻辑将现场清零。 */
	state->FaultSpeed_rad_s = state->Encoder.Speed_rad_s;
	state->FaultIq_Reference = state->Iq_Reference;
	state->FaultId = state->Foc.Id;
	state->FaultIq = state->Foc.Iq;
	state->FaultIa = state->Foc.Ia;
	state->FaultIb = state->Foc.Ib;
	state->FaultIc = state->Foc.Ic;
	state->FaultAlignmentStatus = (uint8_t)state->Alignment.Status;
	state->CurrentLoop_Fault = fault_code;
	state->CurrentLoop_Enabled = 0U;
	state->Pulse_Status = MOTOR_PULSE_INVALID;
	state->Foc.Vd = 0.0f;
	state->Foc.Vq = 0.0f;
	Foc_PIReset(&state->Id_PI);
	Foc_PIReset(&state->Iq_PI);
	__HAL_TIM_MOE_DISABLE(&htim1);
}

static void Motor_SpeedLoop_Update(Motor_Typedef *state)
{
	/* 超速是公共安全保护，不能依赖速度环是否启用。 */
	if ((SPEED_LOOP_OVERSPEED_PROTECTION_ENABLE != 0U) &&
		(state->CurrentLoop_Enabled != 0U) &&
		(state->CurrentLoop_Fault == 0U) &&
		(state->Encoder.Valid != 0U) &&
		(state->Encoder.Speed_Valid != 0U))
	{
		if (fabsf(state->Encoder.Speed_rad_s) > SPEED_LOOP_OVERSPEED_RAD_S)
		{
			if (state->OverspeedSamples < SPEED_LOOP_OVERSPEED_CONFIRM_SAMPLES)
			{
				state->OverspeedSamples++;
			}
			if (state->OverspeedSamples >= SPEED_LOOP_OVERSPEED_CONFIRM_SAMPLES)
			{
				Motor_CurrentLoopTrip(state, 4U);
				return;
			}
		}
		else
		{
			state->OverspeedSamples = 0U;
		}
	}
	else
	{
		state->OverspeedSamples = 0U;
	}

	/* 调试阶段关闭速度环时，保留已经验证过的固定Iq电流闭环。 */
	if (state->SpeedLoop_Enabled == 0U)
	{
		Foc_PIReset(&state->Speed_PI);
		state->Iq_Reference = (state->CurrentLoop_Fault == 0U) ?
			CURRENT_LOOP_IQ_REF_A : 0.0f;
		return;
	}

	if ((state->SpeedLoop_Enabled != 0U) &&
		(state->CurrentLoop_Enabled != 0U) &&
		(state->CurrentLoop_Fault == 0U) &&
		(state->Encoder.Valid != 0U))
	{
		state->Iq_Reference = SPEED_LOOP_OUTPUT_SIGN *
			Foc_PIUpdate(&state->Speed_PI, state->Speed_Reference,
				state->Encoder.Speed_rad_s);
	}
	else
	{
		Foc_PIReset(&state->Speed_PI);
		state->Iq_Reference = 0.0f;
		state->OverspeedSamples = 0U;
	}
}

/** 初始化硬件、编码器、母线参数和低电压零位校准状态。 */
void Motor_StateMachine_Init(Motor_Typedef *state)
{
	Enconder_Init(&state->Encoder, ENCODER_COUNTS_PER_REV, ENCODER_DIRECTION,
		ENCODER_SPEED_FILTER_TAU_S, MOTOR_POLE_PAIRS, ENCODER_ELECTRICAL_OFFSET_RAD);
	Set_Udc_Tpwm(&state->Foc, UDC, TPWM);
	Vf_SpeedControl_Param_Init(&state->Vf, 50.0f, 0.0f, 0.0001f, 0.00005f);

	state->Foc.Theta = 0.0f;
	state->Foc.Vd = 0.0f;
	state->Foc.Vq = 0.0f;
	state->Foc.SinTheta = 0.0f;
	state->Foc.CosTheta = 1.0f;
	Foc_PIInit(&state->Id_PI, CURRENT_LOOP_KP, CURRENT_LOOP_KI,
		CURRENT_LOOP_TS_S, CURRENT_LOOP_VOLTAGE_LIMIT_V);
	Foc_PIInit(&state->Iq_PI, CURRENT_LOOP_KP, CURRENT_LOOP_KI,
		CURRENT_LOOP_TS_S, CURRENT_LOOP_VOLTAGE_LIMIT_V);
	Foc_PIInit(&state->Speed_PI, SPEED_LOOP_KP, SPEED_LOOP_KI,
		SPEED_LOOP_TS_S, SPEED_LOOP_IQ_LIMIT_A);
	state->Id_Reference = CURRENT_LOOP_ID_REF_A;
	state->Iq_Reference = CURRENT_LOOP_IQ_REF_A;
	state->Speed_Reference = SPEED_LOOP_REFERENCE_RAD_S;
	state->Id_Average = 0.0f;
	state->Iq_Average = 0.0f;
	state->Id_PeakToPeak = 0.0f;
	state->Iq_PeakToPeak = 0.0f;
	state->IbCount_Average = 0.0f;
	state->IcCount_Average = 0.0f;
	state->IbCount_PeakToPeak = 0.0f;
	state->IcCount_PeakToPeak = 0.0f;
	state->CurrentLoop_Enabled = 0U;
	state->CurrentLoop_Fault = 0U;
	state->FaultSpeed_rad_s = 0.0f;
	state->FaultIq_Reference = 0.0f;
	state->FaultId = 0.0f;
	state->FaultIq = 0.0f;
	state->FaultIa = 0.0f;
	state->FaultIb = 0.0f;
	state->FaultIc = 0.0f;
	state->FaultAlignmentStatus = (uint8_t)MOTOR_ALIGNMENT_DISABLED;
	state->SpeedLoop_Enabled = 1U;
	state->OverspeedSamples = 0U;
	state->Alignment.Start_Tick_ms = HAL_GetTick();
	state->Alignment.Sin_Sum = 0.0f;
	state->Alignment.Cos_Sum = 0.0f;
	state->Alignment.Sample_Count = 0U;
	state->Pulse_Request = 0U;
	state->Pulse_Status = MOTOR_PULSE_WAITING;
#if MOTOR_ALIGNMENT_ENABLE
	state->Alignment.Status = MOTOR_ALIGNMENT_SETTLING;
#else
	state->Alignment.Status = MOTOR_ALIGNMENT_DISABLED;
#endif

	/* 状态全部初始化后再使能PWM，避免中断读取未准备的数据。 */
	Motor_HardWare_Init();
}

/** 约1kHz读取编码器，并在定向稳定后对原始电角度做圆周平均。 */
void Motor_EncoderTask_1kHz(Motor_Typedef *state)
{
	uint16_t raw_count = MT6816_ReadData(); /* MT6816原始14位计数或0xFFFF错误码 */
	uint32_t tick_ms = HAL_GetTick();       /* 当前系统时基，单位：ms */
	uint32_t elapsed_ms;                   /* 从校准开始累计的时间，单位：ms */
	float mean_angle;                      /* 圆周平均后的未校准电角度，单位：rad */

	if (raw_count != 0xFFFFU)
	{
		Enconder_Update(&state->Encoder, raw_count, tick_ms);
	}
	else
	{
		Enconder_MarkInvalid(&state->Encoder, tick_ms);
	}

	/* 每1ms用编码器真值校正预测角度；20kHz任务在两帧之间继续外推。 */
	Foc_CurrentAngleUpdate(&state->Foc, state->Encoder.Theta_Electrical);
	Motor_SpeedLoop_Update(state);

	if (state->Alignment.Status != MOTOR_ALIGNMENT_SETTLING &&
		state->Alignment.Status != MOTOR_ALIGNMENT_SAMPLING)
	{
		return;
	}

	elapsed_ms = tick_ms - state->Alignment.Start_Tick_ms;
	if (elapsed_ms < MOTOR_ALIGNMENT_SETTLE_MS)
	{
		state->Alignment.Status = MOTOR_ALIGNMENT_SETTLING;
		return;
	}

	if (elapsed_ms < (MOTOR_ALIGNMENT_SETTLE_MS + MOTOR_ALIGNMENT_SAMPLE_MS))
	{
		state->Alignment.Status = MOTOR_ALIGNMENT_SAMPLING;
		if (state->Encoder.Valid != 0U)
		{
			/* 直接平均角度会在0/2pi附近出错，因此分别累计正弦和余弦。 */
			state->Alignment.Sin_Sum += sinf(state->Encoder.Theta_Electrical_Raw);
			state->Alignment.Cos_Sum += cosf(state->Encoder.Theta_Electrical_Raw);
			if (state->Alignment.Sample_Count < 0xFFFFU)
			{
				state->Alignment.Sample_Count++;
			}
		}
		return;
	}

	if (state->Alignment.Sample_Count >= MOTOR_ALIGNMENT_MIN_SAMPLES)
	{
		mean_angle = atan2f(state->Alignment.Sin_Sum, state->Alignment.Cos_Sum);
		/* 固定定向角为0，因此offset = wrap(0 - rotor_electrical_angle)。 */
		Enconder_SetElectricalOffset(&state->Encoder, -mean_angle);
		state->Alignment.Status = MOTOR_ALIGNMENT_COMPLETE;
	}
	else
	{
		state->Alignment.Status = MOTOR_ALIGNMENT_FAILED;
	}
}

/** 20kHz快速任务；仅在校准期间输出受限D轴电压，结束后保持零电压。 */
void Motor_StateMachine_Run(Motor_Typedef *state)
{
	uint32_t alignment_elapsed_ms; /* 定向开始后的累计时间，单位：ms */

	/* 先预测本周期电角度，Park与反Park共用同一组正余弦。 */
	Motor_CurrentAnglePredict_20kHz(state);
	Motor_CurrentTransform_Update(state);
	Motor_CurrentStatistics_Update(state);
	if ((CURRENT_LOOP_OVERCURRENT_PROTECTION_ENABLE != 0U) &&
		(CurrentSense_Valid != 0U) &&
		(Motor_CurrentIsOverLimit(&state->Foc) != 0U))
	{
		Motor_CurrentLoopTrip(state, 2U);
		return;
	}

	alignment_elapsed_ms = HAL_GetTick() - state->Alignment.Start_Tick_ms;
	if ((state->Alignment.Status == MOTOR_ALIGNMENT_SETTLING ||
		 state->Alignment.Status == MOTOR_ALIGNMENT_SAMPLING) &&
		alignment_elapsed_ms >= (MOTOR_ALIGNMENT_SETTLE_MS + MOTOR_ALIGNMENT_SAMPLE_MS + 1U))
	{
		/* 即使1kHz编码器任务停滞，也不能持续施加定向电压。 */
		state->Alignment.Status = MOTOR_ALIGNMENT_FAILED;
	}

	state->Foc.Vq = 0.0f;
	if (state->Alignment.Status == MOTOR_ALIGNMENT_SETTLING ||
		state->Alignment.Status == MOTOR_ALIGNMENT_SAMPLING)
	{
		state->Foc.Vd = MOTOR_ALIGNMENT_VOLTAGE_V;
		Foc_PIReset(&state->Id_PI);
		Foc_PIReset(&state->Iq_PI);
	}
	else
	{
		state->Foc.Vd = 0.0f;
	}

	/* 仅校准成功且收到手动请求才触发；提前启动VOFA可捕获完整波形。 */
	/* 自动校准完成或使用固化零偏时，直接进入持续运行。 */
	if (state->Pulse_Status == MOTOR_PULSE_WAITING &&
		(state->Alignment.Status == MOTOR_ALIGNMENT_COMPLETE ||
		 state->Alignment.Status == MOTOR_ALIGNMENT_DISABLED) &&
		(CurrentSense_Calibrated != 0U) &&
		(CurrentSense_Valid != 0U))
	{
		state->Pulse_Request = 0U;
		state->Pulse_Status = MOTOR_PULSE_ACTIVE;
	}

	/* 校准后仅输出一次：角度来自最近的1kHz编码器帧，禁止在中断内阻塞读SPI。 */
	if (state->Pulse_Status == MOTOR_PULSE_ACTIVE)
	{
		uint32_t now_ms = HAL_GetTick(); /* 当前HAL时刻，单位ms。 */
		uint32_t age_ms = now_ms - state->Encoder.Last_Valid_Tick_ms; /* 最近有效帧年龄，单位ms。 */
		if (CurrentSense_Valid == 0U)
		{
			Motor_CurrentLoopTrip(state, 1U);
		}
		else if (state->Encoder.Valid == 0U || age_ms > MOTOR_SENSED_MAX_AGE_MS)
		{
			Motor_CurrentLoopTrip(state, 3U);
		}
		else
		{
			state->CurrentLoop_Enabled = 1U;
			state->Foc.Vd = Foc_PIUpdate(&state->Id_PI,
				state->Id_Reference, state->Foc.Id);
			state->Foc.Vq = Foc_PIUpdate(&state->Iq_PI,
				state->Iq_Reference, state->Foc.Iq);
		}
	}
	Foc_VoltageUpdate(&state->Foc);
	Motor_SetPWM(state->Foc.Tcmp1, state->Foc.Tcmp2, state->Foc.Tcmp3);
}
