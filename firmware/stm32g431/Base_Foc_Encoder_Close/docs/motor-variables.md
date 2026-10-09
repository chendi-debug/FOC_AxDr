# 电机变量速查

当前阶段是编码器有感电压运行，不是电流闭环。按下面的顺序读代码：

`MT6816原始计数 -> 方向修正的单圈计数 -> 电角度 -> Foc.Theta -> SVPWM`。

| 变量 | 含义及单位 | 更新位置 / 是否参与换相 |
| --- | --- | --- |
| `Encoder.Raw_Count` | MT6816原始计数，0~16383 count | 约1 kHz；仅输入/观测 |
| `Encoder.OneTurn_Count` | 按`Dir`修正后的单圈计数，0~16383 count | 约1 kHz；计算机械角和相邻增量 |
| `Encoder.Delta_Count` | 相邻两次有效读数的最短有符号差，count | 约1 kHz；累计位置和估计速度 |
| `Encoder.Position_Count` | 从启动起累计的多圈位置，count | 约1 kHz；不直接参与本次换相 |
| `Encoder.Theta` | 单圈机械角，0~2pi rad | 约1 kHz；乘极对数得到电角度 |
| `Encoder.Speed_rad_s` | 一阶低通后的机械角速度，rad/s | 约1 kHz；原始瞬时速度仅在函数内参与滤波 |
| `Encoder.Theta_Electrical_Raw` | 未加零偏的电角度，0~2pi rad | 约1 kHz；校准时取样 |
| `Encoder.Electrical_Offset_rad` | 校准获得的电角度零偏，rad | 每次校准结束更新一次 |
| `Encoder.Theta_Electrical` | 加零偏后的电角度，0~2pi rad | 约1 kHz；有感运行时供给`Foc.Theta` |
| `Encoder.Valid` | 当前编码器读数有效标志，1为有效 | 约1 kHz；运行保护 |
| `Encoder.Last_Valid_Tick_ms` | 最近有效样本的HAL时间戳，ms | 约1 kHz；样本年龄按`HAL_GetTick() - Last_Valid_Tick_ms`计算 |
| `Alignment.Status` | 校准状态：0禁用、1稳定、2采样、3成功、4失败 | 校准流程 |
| `Pulse_Request` | 保留的运行请求变量；当前由校准完成逻辑自动启动，不需要手动写入 | 20 kHz任务清零；仅作兼容/观测 |
| `Pulse_Status` | 运行状态：0等待、1持续输出、2保留、3编码器失效 | 20 kHz任务更新；持续运行直到编码器失效或系统复位 |
| `Foc.Theta` | 本次PWM采用的电角度，rad | 20 kHz；校准为0，运行取编码器电角度 |
| `Foc.Vd / Foc.Vq` | 定向轴 / 转矩轴电压命令，V | 20 kHz；不是实测相电压或电流 |

`Dir=1` 表示把原始编码器计数方向反转；当前 DJI 2312S 配置为7极对。电角度关系为
`wrap_0_2pi(7 * Theta + Electrical_Offset_rad)`。VOFA当前约100 Hz，不能捕获所有1 kHz采样瞬变。
