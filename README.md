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

![Simulink Telemetry](simulink_diagram.png)

## Results

The controller was tuned using experimental step-response tests and demonstrated stable speed tracking under the tested conditions.

![Step Response](Step_Response.png)

## Project Structure

```text
Closed-Loop-Motor-Controller/
├── Motor_Control_Final.ino
├── Motor_Control_Simulink.slx
├── README.md
├── Step_Response.png
├── project_1.jpg
├── project_2.jpg
└── simulink_diagram.png
```

## Future Improvements

- Hardware-level E-stop and power isolation
- Improved encoder signal conditioning
- Motor-noise suppression and better decoupling
- Higher-resolution encoder
- Quantitative control metrics such as rise time, settling time, overshoot, and disturbance recovery
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

## FSAE Relevance

This project provided practical experience with concepts directly relevant to automotive motor-control systems:

- Feedback control
- Sensor processing
- Embedded real-time software
- Motor actuation
- Signal integrity
- Fault handling
- Safety architecture
- MATLAB/Simulink validation

## How to Run

### 1. Hardware Setup

Connect the system according to the pin mapping above.

- Power the motor through the L298N using the external motor supply.
- Power the Arduino and encoder from the appropriate Arduino supply.
- Connect the Arduino, L298N, and encoder grounds together.
- Make sure the throttle is at zero before powering the system.

### 2. Arduino Firmware

Open:

```text
Motor_Control_Final.ino
```

Install the required Arduino libraries:

- `Wire`
- `LiquidCrystal_I2C`

Select:

```text
Board: Arduino Uno
```

Select the correct COM port and upload the firmware.

### 3. Serial Communication

The firmware uses:

```text
Baud rate: 115200
```

Make sure any serial monitoring or MATLAB/Simulink configuration uses the same baud rate.

### 4. MATLAB / Simulink

Open:

```text
Motor_Control_Simulink.slx
```

Configure the serial interface for the Arduino's COM port.

The model expects telemetry at the controller's nominal:

```text
Sample time: 0.05 s
```

Start the Simulink model and verify that telemetry such as target RPM, filtered RPM, PWM, and error is being received.

### 5. Operating the Controller

1. Power the system with the throttle at zero.
2. Confirm that the startup throttle interlock is cleared.
3. Set the desired direction.
4. Gradually increase the throttle.
5. Monitor RPM and PWM through the LCD and Simulink telemetry.
6. Use the E-stop whenever an immediate software shutdown is required.

> **Safety:** This is a low-voltage prototype and is not a safety-rated automotive system. The current E-stop is software-based and should not be relied upon as a hardware power-disconnection mechanism.
