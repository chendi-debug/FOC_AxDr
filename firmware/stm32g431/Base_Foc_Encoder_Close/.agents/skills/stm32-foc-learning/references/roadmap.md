# FOC-to-Sensorless Roadmap

This is a dependency graph, not a calendar. A milestone is complete only after its acceptance criteria pass on hardware or in an explicitly scoped simulation.

## M1: Encoder state and speed estimation

Learn modular angle arithmetic, differentiation noise, continuous versus one-turn position, and low-pass filtering.

Implement raw validity and sample age; mechanical angle; wrap-safe signed delta; multi-turn position; raw/filtered mechanical speed in rad/s and RPM; explicit direction.

Observe raw count, angle, delta, continuous angle, raw/filtered speed, age, validity. Accept when manual cross-zero rotation has no false speed spike, continuous angle changes exactly one turn per revolution, directions are consistent, and stopped noise/filter delay are measured.

## M2: Electrical angle and sensored commutation

Learn pole pairs, electrical offset, phase order, signs, and rotor alignment. Implement configurable pole pairs, low-voltage alignment, averaged offset capture, calibrated electrical angle, and a mode that substitutes encoder angle for V/F angle. Observe angle, offset, `Vd/Vq`, mode, and direction. Accept smooth low-speed bidirectional operation, `pole_pairs` electrical cycles per mechanical revolution, and repeatable alignment.

## M3: Voltage-mode speed loop

Learn PI discretization, saturation, anti-windup, command ramps, and voltage-mode limitations. Implement a 1 kHz speed task, target ramp, speed PI producing bounded `Vq_ref`, `Vd_ref = 0`, and reset behavior. Observe target/ramped/measured speed, error, P/I terms, unlimited/limited output, and saturation. Accept repeatable steps/ramps with measured settling/error and controlled saturation recovery.

## M4: Synchronized current acquisition

Learn shunt topology, PWM-synchronous sampling, ADC offset/gain/noise/polarity. Implement TIM1-triggered ADC sampling, DMA or deterministic injected conversion, offset calibration, count-to-amp conversion, phase-sign/current-sum checks, and validity flags. Accept repeatable zero offsets, verified magnitude/sign, consistent current sum, and valid sampling across intended duty range.

## M5: Sensored current-loop FOC

Learn Clarke/Park transforms, `Id/Iq`, current bandwidth, voltage saturation. Implement current transforms with encoder angle, 20 kHz `Id/Iq` PI, vector voltage limiting and anti-windup, initial `Id_ref = 0`, and speed PI output changed to bounded `Iq_ref`. Observe phase current, alpha-beta, `Id/Iq`, references, PI terms, `Vd/Vq`, and saturation. Accept controlled `Id/Iq` steps, correct torque sign, stable speed loop, and clean saturation recovery.

## M6: Position loop

Implement continuous position feedback, a limited position-to-speed controller, and trajectory limits if needed. Accept repeatable position steps/trajectories with deliberate multi-turn and wrap semantics.

## M7: Parameter and voltage-model validation

Measure and document phase `Rs`, `Ld/Lq` or equivalent inductance convention, DC bus scaling, applied-voltage reconstruction, and flux/back-EMF constant where practical. Accept parameter accuracy sufficient to initialize and assess the observer.

## M8: Parallel sensorless SMO plus PLL

Learn stationary-frame voltage model, back-EMF observability, sliding current correction, chattering/filter delay, and PLL phase tracking. Implement alpha-beta observer at 20 kHz, bounded correction, estimated back-EMF, PLL angle/speed, encoder parallel comparison, angle/speed error, and validity/reset states.

Observe current prediction, back-EMF alpha/beta, encoder and observer angle, wrapped angle error, encoder and PLL speed, PLL error/state, and observer saturation. Accept bounded error only above a documented minimum speed, then characterize acceleration, deceleration, reversal, and load response. Do not claim universal sensorless operation.

## M9: Sensorless transition

Implement an alignment/open-loop or sensored startup path, lock criteria with dwell/hysteresis, controlled handover, and fallback on excessive angle error or observer unlock. Accept repeatable transitions without unacceptable current/torque transients in a defined envelope.

## M10: Optional zero-speed research

Back-EMF SMO cannot solve zero speed. Study HFI only after M8/M9, after confirming usable motor saliency, and include injection, demodulation, acoustic effects, inverter nonlinearity, and blending with the back-EMF estimate.
