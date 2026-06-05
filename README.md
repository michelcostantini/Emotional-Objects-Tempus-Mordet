# Emotional Objects: Tempus Mordet

The goal of the "Emotional Objects" project was to transform an everyday object into an interactive and reactive entity guided by an emotion-driven design. By blending pragmatic product design, robotic art, and affective computing, the project finalized a working prototype subjected to empirical testing. Thus, "Tempus Mordet" (Time Bites) was born: a tabletop grandfather clock with a distinctive personality, designed to offer the peculiar experience of dealing with a haunted entity. With various emotional states and randomized or condition-triggered reactions, 'Tempus Mordet' makes extensive use of its Arduino Mega 2560 and the many peripherals involved.  

---

## Bill of Materials (BOM)

### Electronics
* **1x Microcontroller:** Arduino Mega 2560
* **1x Ultrasonic Distance Sensor:** HC-SR04
* **1x RTC Module:** DS3231 I2C Real-Time Clock (with an independent battery)
* **2x Continuous Rotation Micro Servos:** FS90R
* **5x Standard Positional Micro Servos:** SG90
* **2x LEDs:** Common-Cathode RGB LEDs
* **4x Pushbuttons:** with colored caps (yellow, red, green, blue)
* **2x Microswitches:** preferably 1x pin plunger and 1x standard levers
* **1x Hall Effect Sensor:** A3144

### Power Supply
* **1x Logic Supply:** USB B cable or 9V DC barrel jack (for the Arduino Mega)
* **1x System Supply:** External AC-to-DC 5V, 3A power supply

### Structural Materials
* **Coaxial Pulley System:** 2x 1:1 ratio pulleys linked with an elastic rubber band
* **Flexible Pendulum:** 1x metal spring with a nylon string
* **Casing Materials:** cardboard (colored cardstock and wood-grain vinyl wrap for aesthetic details)

---

## Hardware Configuration & Circuitry

The system utilizes two isolated power networks sharing a common ground: one for the microcontroller and one for the peripheral system. This ensures that potential spikes from concurrent servo movements or sensor triggers do not cause logic brownouts or resets on the microcontroller. 

### Circuit Diagram
This simplified circuit diagram illustrates the wiring required by each component type and core circuital choices.

<img width="709" height="789" alt="Simplified Circuit Diagram" src="https://github.com/user-attachments/assets/adae05d4-8121-445a-a1f8-59f55912b4ac" />

### Pin Assignment Table
While the following table indicates the pins used in the source code, the pin numbers can be easily reconfigured simply by altering the constant declarations in the 'PIN CONFIGURATION' section of the code.

### Component Pin Assignment Table

| Component Name | Type | Pin/Interface |
| :--- | :--- | :--- | 
| **RTC Module** | I2C | `SDA (20)` / `SCL (21)` |
| **Ultrasonic Trigger** | Output | Pin `22` | 
| **Ultrasonic Echo** | Input | Pin `23` | 
| **RGB LED (Red Ch.)** | PWM | Pin `10` | 
| **RGB LED (Green Ch.)**| PWM | Pin `11` | 
| **RGB LED (Blue Ch.)** | PWM | Pin `12` |
| **Minute Hand Servo** | PWM | Pin `5` | 
| **Hour Hand Servo** | PWM | Pin `4` | 
| **Minute Homing Switch**| Input | Pin `44` | 
| **Hour Homing Sensor** | Input | Pin `47` | 
| **Lunar Dial Servo** | PWM | Pin `2` |
| **Brand Text Disk Servo**| PWM | Pin `3` |
| **Pendulum Swing Servo**| PWM | Pin `6` | 
| **Pendulum Rope Servo** | PWM | Pin `13` | 
| **Compartment Lock Servo**| PWM | Pin `9` | 
| **Compartment Latch Switch**| Input | Pin `48` | 
| **Blue Button** | Input | Pin `28` | 
| **Red Button** | Input | Pin `32` |
| **Yellow Button** | Input | Pin `36` | 
| **Green Button** | Input | Pin `40` | 

---

## Software

The firmware implements a layered finite state machine. By employing an asynchronous scheduler that relies only on software timers (updated by the `millis()` function) instead of blocking `delay()` calls, the architecture is further divided into specialized state machines and concurrent handlers of independent units.
You can read an in-depth overview of the architecture and code design in the [Documentation](TempusMordet.ino).

## Installation & Setup Guide

### 1. Software Environment
1. Download and install the latest stable release of the [Arduino IDE](https://www.arduino.cc/en/software).
2. Open the application workspace and navigate to **Tools** -> **Manage Libraries...**.
3. Search for and install the following external dependencies:
   * **`Servo`** by Arduino - Manages the generation of pulse-width modulation (PWM) signals for positional servos, allowing precise, degree-based control.
   * **`NewPing`** by Tim Eckel - Replaces the standard blocking ultrasonic sensor communication methods with optimized, non-blocking echo timing routines.
   * **`RTClib`** by Adafruit - Facilitates data retrieval from the external RTC module with methods such as `now()` that return a nicely formatted date and timestamp.

### 2. Compiling and Uploading
1. Download the [Source Code](TempusMordet.ino).
2. Launch the Arduino IDE, go to **File** -> **Open...**, navigate to the downloaded file, and select it.
3. Connect the Arduino Mega 2560 to an open USB port on your workstation using a standard USB Type-B cable.
4. Configure the parameters of the IDE toolbar dropdowns:
   * **Tools** -> **Board** -> Select **Arduino Mega or Mega 2560**
   * **Tools** -> **Processor** -> Select **ATmega2560 (Mega 2560)**
   * **Tools** -> **Port** -> Select the active port to which you connected your board.
5. Verify that the code does not generate compilation errors by pressing `Ctrl + R` or clicking the checkmark icon.
6. Upload the compiled firmware binary directly into the connected microcontroller by pressing `Ctrl + U` or clicking the arrow icon.

## Troubleshooting Guide

If any unexpected behavior should arise, follow these steps:
1. Disconnect the system from the power supply.
2. Check the pin connections for detachments or erroneous configurations.
3. Check the common ground reference for all components and make sure that it is shared between all of them.

Should the problem persist, refer to this guide for solutions to potential issues:

* **Problem: The Arduino board keeps rebooting during execution.**
  * **Solution:** The Arduino board requires a stable power supply. Ensure that it is drawing current from an independent source that can offer sufficient power.

* **Problem: The entire program freezes instantly upon boot or reset, and the eyes glow red.**
  * **Solution:** This is actually the intended behavior! The system is alerting you that the I2C communication with the RTC module could not be initialized. Ensure that the external chip is properly connected and functioning.

* **Problem: The clock hands spin indefinitely without stopping during the homing phase.**
  * **Solution:** Despite the minimum pressure required by microswitches, they can easily shift from their intended position after continuous mechanical contact. Check that they are still reachable by the servo horns and are firmly attached to their stand.
