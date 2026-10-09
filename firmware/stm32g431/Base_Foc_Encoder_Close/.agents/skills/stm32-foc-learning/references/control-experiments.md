# Control Experiments and Evidence

## Scheduling

| Activity | Initial rate | Notes |
|---|---:|---|
| PWM/SVPWM | 20 kHz | Existing TIM1 carrier |
| Current sampling/current PI | 20 kHz | Add only with synchronous ADC data |
| SMO/PLL | 20 kHz | Same coherent current/voltage sample interval |
| Encoder acquisition | 1 kHz initially | Keep current blocking SPI outside fast ISR |
| Speed estimation/speed PI | 1 kHz | Use measured elapsed time if jittery |
| Position loop | 100-500 Hz | Based on mechanics/speed bandwidth |
| VOFA | about 100 Hz | Decimate; never stream from fast ISR |

Record actual `Ts` in each discrete controller. Never use the 50 us V/F interval for a 1 ms speed PI.

## PI state and angle errors

Expose `kp`, `ki`, `ts`, reference, feedback, error, P term, integrator, unlimited output, limited output, limits, saturation, and enable/reset state. Document whether `ki` is continuous-time and internally multiplied by `Ts`.

Use:

`electrical_angle_error = wrap_pm_pi(theta_est - theta_encoder)`

`mechanical_speed_error = omega_est - omega_encoder`

Never directly subtract wrapped angles. Report speed-filter time constant/cutoff and delay.

## Current-loop sequence

Before closing PI control: validate ADC offset at zero current; validate signs with controlled excitation; confirm transforms; command small references under limits; record bus voltage and saturation. A step response must include reference, feedback, PI output, saturation, and electrical speed.

## Sensorless experiment sequence

Develop in parallel with encoder commutation: validate current-model signs; expose back-EMF; compare its angle with encoder convention; tune PLL while encoder commutates; map errors across direction, speed, dynamics, load, and safe bus-voltage conditions; define lock criteria before takeover.

## Failure diagnosis order

1. Units, sample periods, numeric ranges, stale data.
2. Encoder/current validity and offsets.
3. Phase order and signal signs.
4. Pole pairs and electrical offset.
5. Transform conventions.
6. Saturation and integrator windup.
7. Timing among PWM, ADC, angle, observer.
8. Motor parameters and applied-voltage reconstruction.
9. Controller/observer gains and filters.

Change one hypothesis at a time and capture evidence.

## Definition of done

A coding task is software-complete when it has strongest available static/build verification, defined initialization/reset, stated rate/units, telemetry, bench procedure, and expected result. A motor-control milestone remains experimentally incomplete until hardware evidence meets its acceptance criteria.
