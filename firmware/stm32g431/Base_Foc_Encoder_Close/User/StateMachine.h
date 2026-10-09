#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include "Foc.h"
#include "HardWare_Driver.h"
#include "Motor_Config.h"
#include "Enconder.h"

/** 电角度校准阶段；数值直接用于VOFA观察。 */
typedef enum
{
	MOTOR_ALIGNMENT_DISABLED = 0, /* 未启用自动校准 */
	MOTOR_ALIGNMENT_SETTLING = 1, /* 正在施加固定D轴电压并等待转子稳定 */
	MOTOR_ALIGNMENT_SAMPLING = 2, /* 正在对编码器电角度做圆周平均 */
	MOTOR_ALIGNMENT_COMPLETE = 3, /* 零偏计算完成，电压输出已归零 */
	MOTOR_ALIGNMENT_FAILED = 4    /* 编码器有效样本不足，电压输出已归零 */
} Motor_AlignmentStatus;

/** 零位校准状态；慢速任务累加样本，20kHz任务只根据状态输出受限电压。 */
/** 有感试验状态：0等待校准，1正在输出，2已结束，3编码器失效。 */
typedef enum
{
	MOTOR_PULSE_WAITING = 0,
	MOTOR_PULSE_ACTIVE = 1,
	MOTOR_PULSE_DONE = 2,
	MOTOR_PULSE_INVALID = 3
} Motor_PulseStatus;

typedef struct
{
	uint32_t Start_Tick_ms;       /* 本次校准开始时刻，单位：ms */
	float Sin_Sum;                /* 原始电角度正弦累计值，用于跨零圆周平均 */
	float Cos_Sum;                /* 原始电角度余弦累计值，用于跨零圆周平均 */
	uint16_t Sample_Count;        /* 已参与平均的有效编码器样本数 */
	Motor_AlignmentStatus Status; /* 当前校准阶段 */
} Motor_AlignmentTypeDef;

typedef struct
{
	Foc_Typedef Foc;                 /* 20kHz电压变换和SVPWM状态 */
	Foc_PIControllerTypeDef Id_PI;
	Foc_PIControllerTypeDef Iq_PI;
	Foc_PIControllerTypeDef Speed_PI;
	float Id_Reference;
	float Iq_Reference;
	float Speed_Reference;
	volatile float Id_Average;       /* 20个20kHz样本的平均值，每1ms更新 */
	volatile float Iq_Average;       /* 20个20kHz样本的平均值，每1ms更新 */
	volatile float Id_PeakToPeak;    /* 20个样本内Id最大值减最小值 */
	volatile float Iq_PeakToPeak;    /* 20个样本内Iq最大值减最小值 */
	volatile float IbCount_Average;  /* 去零偏后Ib ADC计数的1ms平均值 */
	volatile float IcCount_Average;  /* 去零偏后Ic ADC计数的1ms平均值 */
	volatile float IbCount_PeakToPeak; /* 去零偏后Ib ADC计数的1ms峰峰值 */
	volatile float IcCount_PeakToPeak; /* 去零偏后Ic ADC计数的1ms峰峰值 */
	volatile uint8_t CurrentLoop_Enabled;
	volatile uint8_t CurrentLoop_Fault;
	float FaultSpeed_rad_s;
	float FaultIq_Reference;
	float FaultId;
	float FaultIq;
	float FaultIa;
	float FaultIb;
	float FaultIc;
	uint8_t FaultAlignmentStatus;
	volatile uint8_t SpeedLoop_Enabled;
	uint16_t OverspeedSamples;
	Vf_SpeedControl_TypeDef Vf;      /* 保留的开环V/F状态，本次校准不使用 */
	Enconder_TypeDef Encoder;         /* 1kHz编码器、电角度和速度状态 */
	Motor_AlignmentTypeDef Alignment; /* 电角度零位校准状态 */
	volatile uint8_t Pulse_Request; /* 手动触发：校准完成后写1，20kHz任务消费并清零；复位为0。 */
	volatile Motor_PulseStatus Pulse_Status; /* 单次低速有感运行状态；主循环启动，20kHz中断终止。 */
} Motor_Typedef;

/** 初始化硬件、编码器、母线参数和电角度校准状态。 */
void Motor_StateMachine_Init(Motor_Typedef *state);
/** 20kHz快速任务：校准期间输出固定D轴电压，结束后输出零电压。 */
void Motor_StateMachine_Run(Motor_Typedef *state);
/** 约1kHz慢速任务：更新编码器并完成电角度零偏圆周平均。 */
void Motor_EncoderTask_1kHz(Motor_Typedef *state);

extern Motor_Typedef MotorSystem; /* 全局电机控制状态，由快速和慢速任务共享 */

#endif
