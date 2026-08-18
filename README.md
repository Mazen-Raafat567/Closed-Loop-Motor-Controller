# Closed-Loop DC Motor Controller

An Arduino-based closed-loop DC motor speed controller using encoder feedback, PI control, safety interlocks, and real-time MATLAB/Simulink telemetry.

## Overview

The system regulates DC motor speed using a 20-slot optical encoder and a discrete PI controller.

```text
Throttle → Target RPM → PI Controller → PWM → L298N → Motor
                                      ↑              ↓
                                      └── Encoder ───┘
```

MATLAB/Simulink is used for real-time telemetry and performance visualization.

### Project Demonstration

[![Watch the Project Demonstration](https://img.youtube.com/vi/2M7MMjlisRw/hqdefault.jpg)](https://www.youtube.com/watch?v=2M7MMjlisRw)

*Click the image to watch the full project demonstration.*

## Hardware

- Arduino Uno
- Yellow TT geared DC motor
- L298N H-bridge driver
- 9 V DC motor supply
- 20-slot optical encoder
- 16×2 I²C LCD
- Potentiometer
- Direction pushbutton
- Emergency-stop pushbutton + LED

## Control System

### PI Controller

The controller runs at a nominal **20 Hz** update rate and uses the measured timestep `dt`.

Features include:

- Proportional + integral speed control
- Integral limiting
- Small error deadband
- Dynamic kick-start for starting from rest
- Minimum active PWM

### Encoder Processing

The encoder signal is processed using:

- Interrupt-based pulse timing
- 20 pulses/revolution
- Minimum pulse-period plausibility check
- Four-sample rolling period average
- Exponential moving-average RPM filter

An **11 ms minimum pulse period** is currently used. This rejects pulse intervals corresponding to speeds above approximately 273 RPM, above the motor's experimentally observed operating range.

## Encoder Noise Investigation

During testing, unrealistic RPM spikes were traced to occasional false encoder edges.

The issue was isolated by:

1. Running the motor at fixed PWM with PI control disabled.
2. Examining encoder pulse periods directly.
3. Counting rejected pulse intervals.
4. Testing different pulse-period thresholds.
5. Reducing wire lengths and improving wiring layout.

The results showed that motor operating conditions and wiring affected the encoder signal, highlighting the importance of signal integrity and electrical noise management in closed-loop motor control.

## Safety Features

- **Software E-stop:** Interrupt-triggered latched shutdown that commands PWM to zero.
- **Throttle interlock:** Motor cannot start until the throttle is first returned to zero.
- **Direction interlock:** Direction cannot change until motor speed is sufficiently low.
- **Kick-start control:** Separates starting behavior from normal PI control.

> The current E-stop is software-based and does not provide hardware power isolation. A hardware shutdown path is planned for future development.

## MATLAB / Simulink

Real-time telemetry is transmitted from the Arduino to MATLAB/Simulink using a binary frame:

- 2-byte synchronization header
- 5 × 32-bit floating-point values
- **22 bytes total per frame**

Telemetry includes:

- Encoder period
- Motor PWM
- Target RPM
- Filtered RPM
- Speed error

The Simulink model uses a **0.05 s fixed-step sample time** matching the nominal controller update rate.

![Simulink Telemetry]()

## Step Response Analysis

To objectively verify the controller tuning, the physical potentiometer was temporarily bypassed to command an automated, instantaneous step input from 0 to 150 RPM. The resulting telemetry matrix was exported from Simulink to the MATLAB workspace for quantitative analysis.

![Step Response Analysis](Step_Response.png)

The system demonstrated the following dynamic characteristics during the 150 RPM step test:

- **Rise Time:** 0.850 seconds
- **Settling Time:** 1.600 seconds
- **Percent Overshoot:** 2.74%
- **Steady-State Error:** ~0 RPM

These metrics indicate a stable, slightly underdamped system. Clamping the integral accumulator during the initial hardware kick-start phase prevented integral windup, keeping the overshoot cleanly under 3%. The integral gain successfully drives the steady-state error to zero, with minor RPM variations at steady state attributed to the physical resolution limits of the optical encoder.

## Project Structure

```text
Closed-Loop-Motor-Controller/
├── Motor_Control_Final.ino
├── Motor_Control_Simulink.slx
├── README.md
├── Step_Response_Analysis.png
├── project_1.jpg
├── project_2.jpg
└── simulink_diagram.png
```

## Future Improvements

- Hardware-level E-stop and power isolation
- Improved encoder signal conditioning
- Motor-noise suppression and better decoupling
- Higher-resolution encoder
- Quantitative disturbance recovery testing and load-step analysis
- True hardware-in-the-loop testing with a Simulink motor plant

## Skills Demonstrated

- Closed-loop motor control
- PI control
- Embedded C/C++
- Interrupts and PWM
- Encoder-based speed measurement
- Signal filtering and validation
- Safety interlocks
- Serial telemetry
- MATLAB/Simulink
- Hardware debugging
- Electrical noise / signal-integrity investigation

## How to Run

1. **Hardware Wiring:** Connect the 9V supply to the L298N driver, common ground all components (Arduino, L298N, encoder), and ensure the potentiometer throttle is set to zero before powering on.
2. **Flash Firmware:** Open `Motor_Control_Final.ino` in the Arduino IDE (requires `LiquidCrystal_I2C` library), select Arduino Uno and your COM port, and upload (**115200 Baud**).
3. **Launch Telemetry:** Open `Motor_Control_Simulink.slx` in MATLAB, configure the Serial block to match your Arduino's COM port (`0.05s` sample time, 115200 baud), and press **Run**.
4. **System Operation:** Advance the potentiometer to clear the safety interlock and command target RPM. Track live feedback on the LCD or Simulink scope, and use the hardware buttons for direction changes or software E-stop.
