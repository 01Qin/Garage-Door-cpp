# **Embedded Systems Project: Automated Garage Door Controller (C++)**
This repository contains the firmware for an automated garage door opener test system based on the RP2040. The system uses a stepper motor to drive a simulated door and features both local button operation and remote MQTT control.

---
* **Architecture:** Developed a real-time control system using C++ for the RP2040, implementing object-oriented logic for garage door automation.
* **Hardware Integration:** Managed hardware peripherals including a stepper motor for motion, a rotary encoder for position tracking, and limit switches for safety boundaries.
* **Event-Driven Design:** Developed an interrupt-driven encoder interface utilizing queues to bridge the gap between high-frequency Interrupt Service Routines (ISRs) and main application logic.
* **IoT Connectivity:** Integrated MQTT protocol for remote telemetry and command execution, featuring real-time status reporting and error handling.

---

## System Overview
The test system consists of the following components:
* **Stepper Motor:** Moves the door block up and down.
* **Rotary Encoder:** Detects movement and rotation direction.
* **Limit Switches:** Two switches used to detect when the door reaches the end of its travel range.
* **Garage "Door":** A plastic block driven back and forth by the motor via a belt.

---

## Wiring and Pin Assignments
The system connects to an RP2040 controller using a 6-pin JST connector for the motor and 4-pin Grove connectors for sensors.

| Component | Pins | Details |
| :--- | :--- | :--- |
| **Stepper Motor** | GP2, GP3, GP6, GP13 | Connected to driver pins IN1 - IN4. |
| **Rotary Encoder** | GP20 - GP22 | Requires Grove connector with two GPIOs. |
| **Limit Switches** | GPIO Bank 0 | Requires Grove connector and pull-up resistors. |

---

## Functional Specifications

### Calibration
Calibration is required to determine the travel distance before normal operation:
* **Trigger:** Press **SW0** and **SW2** simultaneously.
* **Process:** The door runs between limit switches to determine the total steps using the encoder.
* **Safety:** The motor stops when a switch is closed to avoid hitting the physical switch body.
* **State:** Calibration and door state are preserved across power cycles.

### Local Operation
The **SW1** button provides the following control logic:
* **Closed Door:** Starts opening.
* **Open Door:** Starts closing.
* **Moving Door:** Stops the movement.
* **Stopped Door:** Starts movement in the opposite direction.

### MQTT Connectivity
The system communicates its state and receives remote commands via MQTT:
* **Status Topic:** `garage/door/status`
    * Reports door state: `Open`, `Closed`, or `In between`.
    * Reports error state: `Normal` or `Door stuck`.
    * Reports calibration state: `Calibrated` or `Not calibrated`.
* **Command Topic:** `garage/door/command`
* **Response Topic:** `garage/door/response`

---

## Technical Implementation
* **Object-Oriented Design:** The firmware is implemented with objects representing hardware entities.
* **Interrupts:** Uses RP2040 GPIO Bank 0 interrupts to handle rotary encoder signals.
* **Queues:** Employs queues to pass rotation events from the Interrupt Service Routine (ISR) to the main program.



---
## Author
**Quinn**  
**Embedded Software Engineer / Developer**
