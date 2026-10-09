#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "Enconder.h"
#include "Motor_Math.h"

#define TEST_COUNTS_PER_REV 16384U
#define TEST_TWO_PI 6.283185307179f
#define FLOAT_TOLERANCE 0.0001f

/* 检查浮点计算误差是否处于编码器角度测试容差内。 */
static void assert_float_near(float actual, float expected)
{
    assert(fabsf(actual - expected) < FLOAT_TOLERANCE);
}

/* 验证16380到2跨零时得到+6 count，而不是大幅负跳变。 */
static void test_forward_wrap(void)
{
    Enconder_TypeDef encoder;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 16380U, 10U);
    assert(encoder.Delta_Count == 0);
    assert(encoder.Position_Count == 16380);

    Enconder_Update(&encoder, 2U, 11U);
    assert(encoder.Delta_Count == 6);
    assert(encoder.Position_Count == 16386);
}

/* 验证2到16380反向跨零时得到-6 count。 */
static void test_reverse_wrap(void)
{
    Enconder_TypeDef encoder;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 2U, 20U);
    Enconder_Update(&encoder, 16380U, 21U);
    assert(encoder.Delta_Count == -6);
    assert(encoder.Position_Count == -4);
}

/* 验证方向配置为1时，原始递增计数转换为机械负方向。 */
static void test_direction_configuration(void)
{
    Enconder_TypeDef encoder;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 1U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 100U, 30U);
    Enconder_Update(&encoder, 101U, 31U);
    assert(encoder.Delta_Count == -1);
}

/* 验证无效帧保留位置，恢复首帧不累计失联期间的未知位移。 */
static void test_invalid_and_recovery(void)
{
    Enconder_TypeDef encoder;
    int32_t position_before_failure;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 1000U, 100U);
    Enconder_Update(&encoder, 1005U, 101U);
    position_before_failure = encoder.Position_Count;

    Enconder_Update(&encoder, 0xFFFFU, 105U);
    assert(encoder.Valid == 0U);
    assert(encoder.Speed_Valid == 0U);
    assert_float_near(encoder.Speed_rad_s, 0.0f);
    assert(105U - encoder.Last_Valid_Tick_ms == 4U);
    assert(encoder.Position_Count == position_before_failure);

    Enconder_Update(&encoder, 1200U, 106U);
    assert(encoder.Valid == 1U);
    assert(encoder.Delta_Count == 0);
    assert(encoder.Speed_Valid == 0U);
    assert(encoder.Position_Count == position_before_failure);

    Enconder_Update(&encoder, 1203U, 107U);
    assert(encoder.Delta_Count == 3);
    assert(encoder.Speed_Valid == 1U);
    assert(encoder.Position_Count == position_before_failure + 3);
}

/* 同一毫秒没有可测的时间间隔，不能计算角速度。 */
static void test_zero_elapsed_time(void)
{
    Enconder_TypeDef encoder;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 1000U, 100U);
    Enconder_Update(&encoder, 1001U, 100U);
    assert(encoder.Speed_Valid == 0U);
    assert_float_near(encoder.Speed_rad_s, 0.0f);
    Enconder_Update(&encoder, 1000U, 101U);
    assert(encoder.Speed_Valid == 1U);
    assert(encoder.Speed_rad_s < 0.0f);
}

/* 验证速度使用真实采样间隔，并按20ms时间常数进行一阶低通。 */
static void test_speed_estimation_and_filter(void)
{
    Enconder_TypeDef encoder;
    float expected_raw;
    float expected_filtered;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    Enconder_Update(&encoder, 1000U, 100U);
    assert(encoder.Speed_Valid == 0U);

    Enconder_Update(&encoder, 2000U, 110U);
    expected_raw = 1000.0f * TEST_TWO_PI / (TEST_COUNTS_PER_REV * 0.01f);
    expected_filtered = expected_raw * (0.01f / (0.02f + 0.01f));
    assert(encoder.Speed_Valid == 1U);
    assert_float_near(encoder.Speed_rad_s, expected_filtered);
}

/* 2312S按7极对配置时，机械一圈电角度恰好经过7个周期。 */
static void test_seven_electrical_cycles(void)
{
    Enconder_TypeDef encoder;
    uint32_t step;

    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 0U, 0.02f, 7U, 0.0f);
    for (step = 0U; step < 7U; ++step)
    {
        Enconder_Update(&encoder, step * 2048U, step);
        assert_float_near(encoder.Theta_Electrical,
                          Motor_Wrap_0_2pi(step * 7.0f * TEST_TWO_PI / 8.0f));
    }
    Enconder_Update(&encoder, 8192U, 8U);
    assert_float_near(encoder.Theta_Electrical, TEST_TWO_PI / 2.0f);
    Enconder_Update(&encoder, 0U, 9U);
    assert_float_near(encoder.Theta_Electrical, 0.0f);
}

/* 零偏、反向配置和负角度都应落在[0,2pi)范围内。 */
static void test_electrical_offset_and_reverse(void)
{
    Enconder_TypeDef encoder;

    assert_float_near(Motor_Wrap_0_2pi(-TEST_TWO_PI / 4.0f),
                      3.0f * TEST_TWO_PI / 4.0f);
    Enconder_Init(&encoder, TEST_COUNTS_PER_REV, 1U, 0.02f, 7U,
                  TEST_TWO_PI / 4.0f);
    Enconder_Update(&encoder, 0U, 0U);
    assert_float_near(encoder.Theta_Electrical_Raw, 0.0f);
    assert_float_near(encoder.Theta_Electrical, TEST_TWO_PI / 4.0f);
    Enconder_Update(&encoder, 4096U, 1U);
    assert_float_near(encoder.Theta_Electrical_Raw, TEST_TWO_PI / 4.0f);
    assert_float_near(encoder.Theta_Electrical, TEST_TWO_PI / 2.0f);

    /* 运行时写入负零偏后，应归一化并立即刷新校准电角度。 */
    Enconder_SetElectricalOffset(&encoder, -TEST_TWO_PI / 4.0f);
    assert_float_near(encoder.Electrical_Offset_rad, 3.0f * TEST_TWO_PI / 4.0f);
    assert_float_near(encoder.Theta_Electrical, 0.0f);
}

int main(void)
{
    test_forward_wrap();
    test_reverse_wrap();
    test_direction_configuration();
    test_invalid_and_recovery();
    test_zero_elapsed_time();
    test_speed_estimation_and_filter();
    test_seven_electrical_cycles();
    test_electrical_offset_and_reverse();
    return 0;
}
