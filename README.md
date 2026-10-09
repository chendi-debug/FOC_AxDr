# FOC_CxDr

FOC（磁场定向控制）学习仓库，集中保存 Simulink 仿真模型、硬件选型资料和 STM32G431 嵌入式工程。

## 仓库结构

```text
FOC_CxDr/
├─ simulation/
│  └─ simulink/          # FOC、SVPWM、观测器、参数辨识等仿真模型
├─ hardware/
│  └─ datasheets/        # MOSFET、驱动器、传感器、电源与运放手册
└─ firmware/
   └─ stm32g431/         # STM32CubeMX + Keil MDK 工程与学习笔记
```

## 内容导航

- [Simulink 仿真模型](simulation/simulink/README.md)
- [硬件选型与数据手册](hardware/datasheets/README.md)
- [STM32G431 软件工程](firmware/stm32g431/README.md)

## 使用说明

1. 克隆仓库：`git clone https://github.com/chendi-debug/FOC_CxDr.git`
2. 仿真模型使用 MATLAB/Simulink 打开 `.slx` 文件。
3. 固件工程可先用 STM32CubeMX 打开 `.ioc` 文件检查外设配置，再用 Keil MDK 打开 `MDK-ARM` 下的 `.uvprojx` 工程。
4. 编译产物和工具缓存不纳入版本控制；本地重新生成即可。

> 本仓库中的器件手册及 ST 官方驱动保留其原始版权和许可，仅用于学习与选型参考。
