# STM32G431 固件工程

本目录包含基于 STM32G431、STM32CubeMX 和 Keil MDK 的电机控制学习工程。

## 工程说明

| 工程 | 用途 |
| --- | --- |
| `Base_Project` | CubeMX 基础工程 |
| `Base_Vofa` | VOFA+ 通信/调试基础工程 |
| `Base_Foc` | FOC 基础实现 |
| `Base_Foc_Vf` | V/F 控制实验 |
| `Base_Foc_Vf_StateMachine` | 带状态机的 V/F/FOC 实验 |
| `Base_Foc_Vf_StateMachine_Speed` | 带速度控制的状态机实验 |
| `Base_Foc_Encoder_Close` | 编码器闭环控制实验及相关学习笔记 |

## 打开与构建

1. 使用 STM32CubeMX 打开各工程根目录中的 `.ioc` 文件。
2. 使用 Keil MDK 打开对应 `MDK-ARM` 目录下的 `.uvprojx` 文件。
3. `Core`、`User`、`Drivers`、`Middlewares` 和 `USB_Device` 保存源码及依赖。
4. `Objects`、`Listings`、`.axf`、`.o`、`.hex` 等构建产物已排除，可在本地重新生成。

工程内的 `docs`、`.agents` 和 `.spec-workflow` 中保留了已有学习笔记与工作流资料。
