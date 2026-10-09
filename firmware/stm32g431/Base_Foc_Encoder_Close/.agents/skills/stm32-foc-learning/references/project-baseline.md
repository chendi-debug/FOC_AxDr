# Project Baseline

Verify this reference against the current repository because the project will evolve.

## Current architecture observed on 2026-09-29

### `User/Foc.c/.h`

- Stores DC bus voltage, PWM period, `Vd/Vq`, angle, alpha-beta voltage, phase voltage, modulated voltage, and timer compare values.
- Implements inverse Park, inverse Clarke, zero-sequence SVPWM, compare-value calculation, and a V/F angle integrator.
- Does not implement current transforms, measured currents, PI current control, voltage-vector limiting, or sensorless observation.

### `User/StateMachine.c/.h`

- Owns global `MotorSystem` with FOC and V/F state.
- Initializes hardware, `Udc/Tpwm`, and the V/F ramp.
- The 20 kHz run function gets `Foc.Theta` from the V/F integrator, applies inverse transforms/SVPWM, and writes TIM1 compare values.
- `Vd` is zero. `Vq` assignment is commented in the observed source, so verify its runtime source before depending on it.

### `User/Enconder.c/.h`

- Converts raw count to one-turn mechanical angle.
- Has no wrap-safe delta, multi-turn position, speed estimate, sample age, validity, pole-pair conversion, offset, or calibrated electrical angle.
- Is not integrated into `MotorSystem` control flow.

### `User/HardWare_Driver.c/.h`

- Starts TIM1 complementary PWM and writes three compare values.
- Reads MT6816 through two blocking 16-bit SPI transactions.
- Returns a 14-bit raw angle (`0..16383`) or `0xFFFF` on communication failure.

### `Core` and `User/Vofa.c/.h`

- STM32G431 system clock is about 170 MHz.
- TIM1 is center aligned with ARR 4249, giving about 20 kHz PWM and a 50 us callback interval.
- TIM1 channel 4 callback calls `Motor_StateMachine_Run()`.
- Main loop reads MT6816 about every 1 ms into global `Data`.
- VOFA sends nine float channels about every 10 ms, currently including raw encoder and V/F/SVPWM values only.

## Baseline classification

The controller is open-loop voltage-vector rotation plus independent encoder acquisition. It is not sensored closed-loop FOC: neither measured rotor angle nor current participates in the control law.

## Constraints and needed information

- Blocking MT6816 reading is unsuitable in the 50 us control ISR.
- No ADC phase-current acquisition is visible; true torque-controlled FOC and the default sensorless route need it.
- Retain the encoder as ground truth while developing sensorless control.
- Obtain motor construction, pole pairs, voltage/current rating, phase `Rs/Ld/Lq`, flux or back-EMF constant, current-sense topology/scaling, driver fault behavior, encoder direction, and mechanical load envelope when each milestone needs them.
