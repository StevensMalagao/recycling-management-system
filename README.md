# Recycling Management System

## Overview

This project is a weighing system designed for recycling environments. It connects an ESP32 to a Python desktop application to replace manual recording. The system processes weight signals in real-time and logs the data into .xlsx files for administrative management.

## Features

* **Signal Processing:** Uses an **Exponential Moving Average (EMA)** filter to stabilize readings from the load cell and reduce electrical noise.

* **Data Logging:** A Python application handles serial communication and logs timestamped transactions directly to spreadsheets.

* **Calibration:** Calibration factors are saved in the ESP32's non-volatile memory, keeping the scale accurate after power cycles.

* **Timekeeping:** An integrated **DS1302 RTC** provides accurate timestamps even without an internet connection.

## Tech Stack

### Firmware & Hardware

* **ESP32:** Central processing unit. It manages sensor sampling and serial communication.

* **HX711 (24-bit ADC):** Interface for load cells. It converts analog signals into high-resolution digital data.

* **DS1302 (RTC):** Real-time clock source. It ensures accurate timestamping without internet connectivity.

* **C++ (Arduino):** Used for firmware development, focusing on low-latency hardware interaction.

### Software & Data Management

* **Python:** Chosen for its efficiency in handling data streams and library support for office automation.

* **CustomTkinter:** Used to build a modern, intuitive user interface for operators, improving data entry speed and reducing human error.

* **OpenPyXL:** Manages the `.xlsx` files. It automates the generation of monthly logs and dynamic column creation for different material types.

* **PySerial:** Facilitates the bridge between the physical hardware and the computer.

## Signal Filtering

To ensure stability against mechanical vibrations and electrical noise, the system implements a digital low-pass filter through an **Exponential Moving Average (EMA)**. This method calculates the current weight by weighting the latest sensor reading against the previous filtered state using the following formula:

$$W_{filtered} = (\alpha \cdot W_{raw}) + ((1 - \alpha) \cdot W_{previous})$$

This method is highly efficient for the ESP32 as it requires minimal memory (storing only one previous value) while effectively eliminating reading fluctuations on the display. By setting $\alpha$ to 0.6, the scale achieves an optimal balance between filtering stability and response speed when an object is placed on the load cell.

## Hardware Design

The complete circuit schematic and the **Gerber files** for PCB visualization and manufacturing are available in the `docs/` folder. Additionally, the following table details the specific pin assignments and connections established in those designs:

| **Component**         | **Pin (GPIO)**      | **Description**                  |
| --------------------- | ------------------- | -------------------------------- |
| **HX711 (DT)**        | GPIO 5              | Data line for load cell ADC      |
| **HX711 (SCK)**       | GPIO 18             | Clock line for load cell ADC     |
| **RTC DS1302 (RST)**  | GPIO 33             | Reset line for Real Time Clock   |
| **RTC DS1302 (DAT)**  | GPIO 26             | Data I/O for Real Time Clock     |
| **RTC DS1302 (CLK)**  | GPIO 25             | Serial clock for Real Time Clock |
| **I2C LCD (SDA)**     | GPIO 21             | Data line for 20x4 Display       |
| **I2C LCD (SCL)**     | GPIO 22             | Clock line for 20x4 Display      |
| **Buttons (SW1-SW4)** | GPIO 12, 13, 14, 27 | User interface inputs            |

## Installation

### 1. Firmware

1. Open `firmware/scale_controller.ino` in Arduino IDE.

2. Install the required libraries through the Library Manager or the official repositories:

- **HX711** by [bogde](https://github.com/bogde/HX711)
- **RtcDS1302** by [Makuna](https://github.com/Makuna/Rtc)   
- **LiquidCrystal_I2C** by [johnrickman](https://github.com/johnrickman/LiquidCrystal_I2C)

3. Upload the code to the ESP32.

### 2. Software

```bash
cd software
python -m venv venv
source venv/bin/activate  # Or venv\Scripts\activate on Windows
pip install -r requirements.txt
python recycling_app.py
```


