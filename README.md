# Recycling Management System

Recycling Management System is an integrated hardware and software solution designed to automate weight recording, signal filtering, and material transaction logging for recycling operations. The platform connects a digital scale controller to a desktop application that processes real-time telemetry and generates structured Excel spreadsheets.

---

## Core Capabilities

### Signal Processing and Digital Filtering
To stabilize measurements against mechanical vibrations and electrical noise, the firmware executes a digital low-pass filter using an Exponential Moving Average on continuous raw load cell samples:

$$W_{\text{filtered}} = (\alpha \cdot W_{\text{raw}}) + ((1 - \alpha) \cdot W_{\text{previous}})$$

Setting $\alpha$ to 0.6 eliminates display fluctuations while maintaining instant response times when material is placed on the scale. Calibration parameters determined during setup are stored in non-volatile preferences memory, preserving calibration across power cycles.

### Timestamping and Clock Management
An integrated real-time clock maintains accurate date and time records independently of network availability. Timestamps are transmitted within serial payloads and logged directly into transaction records.

### Automated Data Logging
- Dynamic Worksheet Allocation: Transaction records are automatically grouped into monthly sheets based on entry dates.
- Dynamic Material Columns: As new material types are processed, dedicated weight columns are generated automatically to track cumulative totals per collector.
- Packaging Deductions: Supports automated tare subtraction for common packaging items.

---

## Hardware Specifications and Pin Mapping

The circuit schematic and Gerber manufacturing files are located in the docs directory. The microcontroller pin assignments are detailed below:

| Component | Pin (GPIO) | Description |
| :--- | :--- | :--- |
| HX711 (DT) | GPIO 5 | Data line for load cell ADC |
| HX711 (SCK) | GPIO 18 | Serial clock line for load cell ADC |
| RTC DS1302 (RST) | GPIO 33 | Reset line for Real Time Clock |
| RTC DS1302 (DAT) | GPIO 26 | Data line for Real Time Clock |
| RTC DS1302 (CLK) | GPIO 25 | Serial clock for Real Time Clock |
| I2C LCD (SDA) | GPIO 21 | Data line for 20x4 LCD Display |
| I2C LCD (SCL) | GPIO 22 | Serial clock for 20x4 LCD Display |
| Buttons (SW1-SW4) | GPIO 12, 13, 14, 27 | UI navigation inputs (Calibrate, Tare, Record, Cancel) |

---

## Requirements

- Python 3.8 or higher.
- Arduino IDE or PlatformIO with ESP32 board support.
- Hardware: ESP32 development board, HX711 module with load cell, DS1302 RTC module, 20x4 I2C LCD display, and four tactile push buttons.

---

## Installation

### Firmware Setup

1. Open firmware/scale_controller.ino in Arduino IDE.
2. Install required libraries:
   - HX711 by [bogde](https://github.com/bogde/HX711)
   - RtcDS1302 by [Makuna](https://github.com/Makuna/Rtc)
   - LiquidCrystal_I2C by [johnrickman](https://github.com/johnrickman/LiquidCrystal_I2C)
3. Select the target ESP32 board and port, then upload the code.

### Desktop Application Setup

Navigate to the project root directory and create a virtual environment:

```bash
python -m venv venv
source venv/bin/activate
```

Install the required Python packages:

```bash
pip install -r requirements.txt
```

---

## Usage

### Operating the Scale

- SW1 (Calibrate/Confirm): Initiates calibration wizard.
- SW2 (Tare): Resets current weight display to zero.
- SW3 (Weigh and Log): Captures current stabilized weight and transmits timestamped data over serial port.
- SW4 (Show Date/Time/Cancel): Displays system time or cancels active wizard steps.

### Running the Application

Start the desktop application:

```bash
python software/recycling_app.py
```

1. Select the serial port assigned to the device and click Connect.
2. Select collector name, material category, material type, and packaging.
3. Click Register Transaction to log data into the Excel spreadsheet.
4. Access Manage Database to inspect collector records.
