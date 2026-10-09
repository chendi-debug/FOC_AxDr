#include "adc.h"
#include "StateMachine.h"

ADC_HandleTypeDef hadc1;
volatile uint16_t CurrentSense_Ic_Raw;
volatile uint16_t CurrentSense_Ib_Raw;
volatile uint16_t CurrentSense_Ic_Offset;
volatile uint16_t CurrentSense_Ib_Offset;
volatile int32_t CurrentSense_Ic_Count;
volatile int32_t CurrentSense_Ib_Count;
volatile int32_t CurrentSense_Ia_Count;
volatile uint16_t CurrentSense_CalibrationSamples;
volatile uint8_t CurrentSense_Calibrated;
volatile uint8_t CurrentSense_Valid;
volatile uint32_t CurrentSense_SequenceCount;

#define CURRENT_SENSE_OFFSET_SAMPLE_COUNT 1024U

static uint32_t CurrentSense_Ic_OffsetSum;
static uint32_t CurrentSense_Ib_OffsetSum;
static uint8_t CurrentSense_DiscardFirstSequence;

void MX_ADC1_Init(void)
{
    ADC_InjectionConfTypeDef injected = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.GainCompensation = 0U;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1U;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) Error_Handler();

    injected.InjectedSamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    injected.InjectedSingleDiff = ADC_SINGLE_ENDED;
    injected.InjectedOffsetNumber = ADC_OFFSET_NONE;
    injected.InjectedOffset = 0U;
    injected.InjectedNbrOfConversion = 2U;
    injected.InjectedDiscontinuousConvMode = DISABLE;
    injected.AutoInjectedConv = DISABLE;
    injected.QueueInjectedContext = DISABLE;
    injected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_T1_CC4;
    injected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_FALLING;

    injected.InjectedRank = ADC_INJECTED_RANK_1;
    injected.InjectedChannel = ADC_CHANNEL_1;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &injected) != HAL_OK) Error_Handler();

    injected.InjectedRank = ADC_INJECTED_RANK_2;
    injected.InjectedChannel = ADC_CHANNEL_2;
    if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &injected) != HAL_OK) Error_Handler();

}

HAL_StatusTypeDef CurrentSense_Start(void)
{
    CurrentSense_Valid = 0U;
    CurrentSense_Calibrated = 0U;
    CurrentSense_CalibrationSamples = 0U;
    CurrentSense_SequenceCount = 0U;
    CurrentSense_Ic_OffsetSum = 0U;
    CurrentSense_Ib_OffsetSum = 0U;
    CurrentSense_Ic_Count = 0;
    CurrentSense_Ib_Count = 0;
    CurrentSense_Ia_Count = 0;
    CurrentSense_DiscardFirstSequence = 1U;

    return HAL_ADCEx_InjectedStart_IT(&hadc1);
}
void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle)
{
    GPIO_InitTypeDef gpio = {0};
    if (adcHandle->Instance == ADC1)
    {
        __HAL_RCC_ADC12_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        gpio.Mode = GPIO_MODE_ANALOG;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio);
        HAL_NVIC_SetPriority(ADC1_2_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *adcHandle)
{
    if (adcHandle->Instance == ADC1)
    {
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
        HAL_NVIC_DisableIRQ(ADC1_2_IRQn);
        __HAL_RCC_ADC12_CLK_DISABLE();
    }
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC1)
    {
        return;
    }

    CurrentSense_SequenceCount++;
    CurrentSense_Ic_Raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_1);
    CurrentSense_Ib_Raw = (uint16_t)HAL_ADCEx_InjectedGetValue(
        hadc, ADC_INJECTED_RANK_2);

    /* ES0431 2.5.9: discard the first sequence following calibration. */
    if (CurrentSense_DiscardFirstSequence != 0U)
    {
        CurrentSense_DiscardFirstSequence = 0U;
        return;
    }

    if (CurrentSense_Calibrated == 0U)
    {
        CurrentSense_Ic_OffsetSum += CurrentSense_Ic_Raw;
        CurrentSense_Ib_OffsetSum += CurrentSense_Ib_Raw;
        CurrentSense_CalibrationSamples++;

        if (CurrentSense_CalibrationSamples >= CURRENT_SENSE_OFFSET_SAMPLE_COUNT)
        {
            CurrentSense_Ic_Offset = (uint16_t)((CurrentSense_Ic_OffsetSum +
                (CURRENT_SENSE_OFFSET_SAMPLE_COUNT / 2U)) /
                CURRENT_SENSE_OFFSET_SAMPLE_COUNT);
            CurrentSense_Ib_Offset = (uint16_t)((CurrentSense_Ib_OffsetSum +
                (CURRENT_SENSE_OFFSET_SAMPLE_COUNT / 2U)) /
                CURRENT_SENSE_OFFSET_SAMPLE_COUNT);
            CurrentSense_Calibrated = 1U;
        }
        return;
    }

    CurrentSense_Ic_Count = (int32_t)CurrentSense_Ic_Raw -
        (int32_t)CurrentSense_Ic_Offset;
    CurrentSense_Ib_Count = (int32_t)CurrentSense_Ib_Raw -
        (int32_t)CurrentSense_Ib_Offset;
    CurrentSense_Ia_Count = -(CurrentSense_Ib_Count + CurrentSense_Ic_Count);
    CurrentSense_Valid = 1U;

    /*
     * Run the 20 kHz current loop only after both injected conversions have
     * completed, so Clarke/Park and the PI controllers consume this PWM
     * period's freshly sampled Ib/Ic values rather than the previous period.
     */
    Motor_StateMachine_Run(&MotorSystem);
}
