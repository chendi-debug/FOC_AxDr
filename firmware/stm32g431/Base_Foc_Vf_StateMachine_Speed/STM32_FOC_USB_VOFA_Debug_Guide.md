# STM32 电机控制中 USB 虚拟串口卡死问题的排查

## 一、问题现象

项目使用 STM32G431，实现以下功能：

- TIM6 以 20 kHz 频率执行 VF/FOC 控制；
- TIM1 输出三相互补 PWM；
- 主循环通过 USB CDC 向 VOFA+ 发送 JustFloat 数据；
- VOFA+ 用于观察角度、三相电压和 SVPWM 波形。

程序运行后发现：

- TIM6 计数器一直变化；
- 电机控制变量和 PWM 比较值也在更新；
- 但 VOFA+ 只能收到少量数据，随后波形停止刷新；
- USB 有时表现为一直处于 `BUSY` 状态。

一开始看起来像是 USB CDC 出了问题，但最终发现，真正的根因在 20 kHz 控制中断中。

## 二、先划分软件的执行层次

这个程序可以分成四层：

```text
TIM6 控制中断
    ↓
系统时基 SysTick
    ↓
主循环中的 Vofa 发送任务
    ↓
USB CDC 发送与完成中断
```

上位机卡住时，不能直接从 USB 驱动开始查，而应该从前往后确认：CPU 是否还有时间执行到 USB 发送代码。

## 三、第一步：确认主循环是否还能运行

首先在主循环中临时增加一个计数器：

```c
volatile uint32_t MainLoopCount = 0;

while (1)
{
    MainLoopCount++;
    Vofa_Send_Task();
}
```

让程序连续运行一段时间，然后暂停 CPU，观察 `MainLoopCount`。

判断方法：

- 持续快速增加：主循环正常；
- 只增加少量数值后基本不动：CPU 大部分时间被中断占用；
- 完全不增加：程序可能停在初始化、异常处理或某个死循环中。

本次调试中，`MainLoopCount` 在数秒内只增加了约 255 次。这说明程序并未真正卡死，而是主循环几乎得不到执行时间。

## 四、第二步：确认 SysTick 是否正常执行

VOFA 发送任务使用 `HAL_GetTick()` 控制发送周期：

```c
if ((HAL_GetTick() - last_send_tick) < 10U)
{
    return;
}
```

因此，只要 `uwTick` 不增加，发送任务就会一直提前返回。

先检查 SysTick 寄存器：

```text
SysTick->CTRL
SysTick->LOAD
SysTick->VAL
```

正常情况下，`SysTick->CTRL` 应满足：

```text
ENABLE    = 1
TICKINT   = 1
CLKSOURCE = 1
```

本项目中读取到：

```text
SysTick->CTRL = 0x00010007
SysTick->LOAD = 0x0002980F
```

这说明 SysTick 硬件已经正确启动，并按 170 MHz 系统时钟配置为约 1 ms 周期。

但是，硬件计数器工作并不代表中断函数一定得到执行。因此在 `SysTick_Handler()` 中临时加入计数：

```c
volatile uint32_t SysTickIrqCount = 0;

void SysTick_Handler(void)
{
    SysTickIrqCount++;
    HAL_IncTick();
}
```

运行约 20 秒后却发现：

```text
SysTickIrqCount ≈ 6
uwTick          ≈ 6
```

由此可以判断：

> SysTick 外设本身配置正确，但 SysTick 中断几乎没有获得 CPU 执行时间。

## 五、第三步：检查高频控制中断是否占满 CPU

接着在 TIM6 回调中增加计数器：

```c
volatile uint32_t Tim6IrqCount = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        Tim6IrqCount++;
        Motor_StateMachine_Run(&MotorSystem);
    }
}
```

测试结果表现为：

```text
Tim6IrqCount     持续快速增加
SysTickIrqCount  几乎不增加
MainLoopCount    增长极慢
```

这说明 TIM6 中断一直在运行，但它占用了绝大部分 CPU 时间。

此时再检查中断优先级：

```c
HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
```

而 HAL 默认的 SysTick 优先级为：

```c
#define TICK_INT_PRIORITY 15UL
```

数值越小，抢占优先级越高。因此 TIM6 为最高优先级，而 SysTick 为最低优先级。当 TIM6 中断执行时间接近或超过 50 us 时，SysTick 和主循环就会被严重挤压。

## 六、找到 TIM6 中断负载过高的原因

TIM6 的频率为：

```text
170 MHz / 8500 = 20 kHz
```

所以每次中断之间只有：

```text
1 / 20000 = 50 us
```

控制代码中原本使用了：

```c
cos(state->Theta);
sin(state->Theta);
fmod(state->Theta_Integrator, _2PI);
```

虽然输入和输出变量是 `float`，但 `cos()`、`sin()` 和 `fmod()` 默认执行双精度运算。它们在 20 kHz 中断中被频繁调用，造成了明显的计算负担。

将其修改为单精度版本：

```c
cosf(state->Theta);
sinf(state->Theta);
fmodf(state->Theta_Integrator, _2PI);
```

同时确保浮点常量带有 `f` 后缀：

```c
-0.5f
0.86f
```

修改后，主循环计数从数秒内几百次提升到数百万次，说明 TIM6 中断的执行时间显著下降，CPU 重新有时间处理后台任务。

## 七、调整中断优先级

为了防止控制中断再次阻塞系统时基和 USB，将优先级调整为：

```text
USB_LP_IRQn    优先级 0
SysTick_IRQn   优先级 1
TIM6_DAC_IRQn  优先级 2
```

对应修改：

```c
#define TICK_INT_PRIORITY 1UL
```

以及：

```c
HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 2, 0);
```

调整后观察到：

```text
uwTick 与实际运行时间基本一致
SysTickIrqCount 按约 1 kHz 增长
Tim6IrqCount 按约 20 kHz 增长
MainLoopCount 持续快速增加
```

至此，系统调度恢复正常。

## 八、最后检查 USB 发送状态

在确认主循环能够正常执行之后，再检查 USB。

USB 正常枚举时应满足：

```text
hUsbDeviceFS.dev_state  = 0x03
hUsbDeviceFS.pClassData != NULL
```

其中：

```text
0x01：默认状态
0x02：已分配地址
0x03：配置完成
0x04：挂起状态
```

本项目每帧发送 9 个浮点数和 4 字节帧尾：

```text
9 × 4 + 4 = 40 字节
```

原程序存在一个数据拷贝错误：

```c
memcpy(Byte_Data_Array, Send_Data_Array, data_len);
```

`Send_Data_Array` 实际只有 36 字节，但这里读取了 40 字节，而且两个数组位于同一结构体中，可能产生越界读取和重叠访问。

正确写法为：

```c
memcpy(Byte_Data_Array,
       Send_Data_Array,
       data_num * sizeof(float));
```

然后单独填充 4 字节 JustFloat 帧尾。

同时不能在主循环中无间隔发送，应限制发送频率，例如 10 ms 一帧：

```c
if ((HAL_GetTick() - last_send_tick) < 10U)
{
    return;
}
```

这样 VOFA+ 的接收频率约为 100 Hz，已经足够观察控制波形。

## 九、最终故障链路

这次问题的完整因果关系是：

```text
TIM6 以 20 kHz 运行
    ↓
中断中执行双精度 sin、cos 和 fmod
    ↓
TIM6 中断执行时间过长
    ↓
低优先级 SysTick 和主循环得不到执行
    ↓
uwTick 几乎不增加
    ↓
VOFA 发送任务一直被 10 ms 判断提前返回
    ↓
USB 发送完成处理也受到影响
    ↓
VOFA+ 表现为波形停止或卡住
```

最终解决措施包括：

1. 将 `sin/cos/fmod` 改为 `sinf/cosf/fmodf`；
2. 调整 USB、SysTick 和 TIM6 的中断优先级；
3. 修正 VOFA 数据拷贝长度；
4. 将 VOFA 发送频率限制为约 100 Hz；
5. 发送前检查 USB 是否处于配置完成状态。

## 十、针对这个问题的最短排查路径

以后再次遇到类似现象，只需要按下面顺序判断：

```text
VOFA+ 卡住
    ↓
主循环是否运行？
    ├─ 否：检查高频中断是否占满 CPU
    └─ 是
        ↓
uwTick 是否增加？
        ├─ 否：检查 SysTick 配置、优先级和中断占用
        └─ 是
            ↓
是否进入 CDC_Transmit_FS？
            ├─ 否：检查发送周期判断
            └─ 是
                ↓
USB 是否为 CONFIGURED 状态？
                ├─ 否：检查枚举和 USB 连接
                └─ 是
                    ↓
TxState 是否长期为 1？
                    ├─ 是：检查 USB 发送完成中断
                    └─ 否：检查数据格式和 VOFA+ 配置
```

这次调试最重要的经验是：

> 外设仍在计数，不代表 CPU 有时间执行主循环；USB 显示 `BUSY`，也不一定是 USB 驱动故障。面对“上位机卡住”，应先确认实时控制中断是否侵占了系统的全部执行时间。
