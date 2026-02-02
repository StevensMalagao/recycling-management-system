#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "HX711.h"
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- LCD Display Configuration ---
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- RTC DS1302 Configuration ---
const int DS1302_IO_PIN   = 26;
const int DS1302_SCLK_PIN = 25;
const int DS1302_CE_PIN   = 33;

ThreeWire myWire(DS1302_IO_PIN, DS1302_SCLK_PIN, DS1302_CE_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);

// --- HX711 Load Cell Configuration ---
#define LOADCELL_DOUT_PIN 5
#define LOADCELL_SCK_PIN  18
#define KNOWN_WEIGHT 10000 

float sensorResult; 
float calibrationFactor;
bool isCellInitialized = false;
float weightGrams;
float loadedCalibrationFactor = 0;

float filteredWeight = 0.0;
const float FILTER_FACTOR = 0.6; 

HX711 hx711_cell;
Preferences preferences;

// --- Switch Pin Configuration ---
const int sw1 = 12; // Calibrate / Confirm
const int sw2 = 13; // Tare
const int sw3 = 14; // Weigh and Record
const int sw4 = 27; // Show Date/Time / Cancel

// --- LCD Helper Functions ---
void displayLCDMessage(String line1, String line2 = "", String line3 = "", String line4 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (line2 != "") {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
    if (line3 != "") {
        lcd.setCursor(0, 2);
        lcd.print(line3);
    }
    if (line4 != "") {
        lcd.setCursor(0, 3);
        lcd.print(line4);
    }
}

/**
 * @brief Replaces delay(), but checks sw4 for cancellation.
 * @param ms Milliseconds to wait.
 * @return 'true' if sw4 was pressed (canceled), 'false' if time finished normally.
 */
bool delayWithCancellation(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (digitalRead(sw4) == LOW) {
            displayLCDMessage("Action Canceled");
            Serial.println("Action canceled by user.");
            
            while(digitalRead(sw4) == LOW); 
            delay(1000); 
            lcd.clear();
            return true; 
        }
        delay(20); 
    }
    return false;
}

void setup() {
    Serial.begin(115200);

    lcd.init();
    lcd.backlight();
    displayLCDMessage("Starting System...", "Please wait.");
    delay(1000); 

    initRTC();
    initHX711();

    pinMode(sw1, INPUT_PULLUP);
    pinMode(sw2, INPUT_PULLUP);
    pinMode(sw3, INPUT_PULLUP);
    pinMode(sw4, INPUT_PULLUP);
    
    lcd.clear();
    displayLCDMessage("System Ready.");
    delay(1500); 
    lcd.clear();
}

void loop() {
    // Button 1: Start Calibration
    if (digitalRead(sw1) == LOW) {
        calibrateHX711(); 
        lcd.clear(); 
        while(digitalRead(sw1) == LOW); 
        delay(50); 
    }
    // Button 2: Tare Scale
    else if (digitalRead(sw2) == LOW) { 
        tareFunction(); 
        lcd.clear();
        while(digitalRead(sw2) == LOW); 
        delay(50);
    }
    // Button 3: Weigh and Send Data
    else if (digitalRead(sw3) == LOW) { 
        displayAndLogWeight(); 
        lcd.clear();
        while(digitalRead(sw3) == LOW); 
        delay(50);
    }
    // Button 4: Display Date/Time
    else if (digitalRead(sw4) == LOW) { 
        displayDateTimeLCD(); 
        lcd.clear();
        while(digitalRead(sw4) == LOW); 
        delay(50);
    }
    // DEFAULT STATE: Continuous weight display
    else {
        float weight = getHX711Weight(); 

        lcd.setCursor(0, 0);
        lcd.print("Current Weight:     "); 
        lcd.setCursor(0, 1);
        
        char buffer[20];
        dtostrf(weight / 1000.0, 8, 2, buffer); 

        lcd.print(String(buffer) + " kg       "); 
        
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");

        delay(100); 
    }
}

void displayDateTimeLCD() {
    RtcDateTime now = Rtc.GetDateTime();
    char datestring[20];
    sprintf(datestring, "%02d/%02d/%04d", now.Day(), now.Month(), now.Year());
    char timestring[10];
    sprintf(timestring, "%02d:%02d:%02d", now.Hour(), now.Minute(), now.Second());

    displayLCDMessage("Date: " + String(datestring), "Time: " + String(timestring));
    delayWithCancellation(2500); 
}

String getDateString() {
    RtcDateTime now = Rtc.GetDateTime();
    char datestring[20];
    sprintf(datestring, "%02d/%02d/%04d", now.Day(), now.Month(), now.Year());
    return String(datestring);
}

void initRTC() {
    Serial.print("Compile date: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Rtc.Begin();
    if (!Rtc.IsDateTimeValid()) {
        Serial.println("RTC lost confidence in date/time!");
        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        Rtc.SetDateTime(compiled);
    }
    if (Rtc.GetIsWriteProtected()) {
        Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning()) {
        Rtc.SetIsRunning(true);
    }
}

void initHX711() {
    preferencias.begin("CF", false);
    delay(100); 
    hx711_cell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    loadedCalibrationFactor = preferencias.getFloat("CFVal", 0);

    if (loadedCalibrationFactor != 0) {
        displayLCDMessage("Calibration OK", "Scale ready.");
        delay(1500); 

        hx711_cell.set_scale(loadedCalibrationFactor);
        hx711_cell.tare();
        filteredWeight = 0.0; 
        isCellInitialized = true;
    } else {
        displayLCDMessage("Calib. not found", "Press Calibrate", "to proceed.");
        delay(2500); 
    }
}

float getHX711Weight() {
    if (isCellInitialized) {
        if (hx711_cell.is_ready()) {
            weightGrams = hx711_cell.get_units(5); 

            if (filteredWeight == 0.0) {
                filteredWeight = weightGrams;
            } else {
                filteredWeight = (weightGrams * FILTER_FACTOR) + (filteredWeight * (1.0 - FILTER_FACTOR));
            }

            if (filteredWeight < 10 && filteredWeight > -10) return 0.0;
            return filteredWeight;
        }
    }
    return 0;
}

void calibrateHX711() {
    displayLCDMessage("Start calibration?", "", "Confirm: (Btn 1)", "Cancel:  (Btn 4)");

    while (true) {
        if (digitalRead(sw1) == LOW) {
            while (digitalRead(sw1) == LOW);
            delay(50);
            break; 
        }
        if (digitalRead(sw4) == LOW) {
            displayLCDMessage("Calibration", "Canceled.");
            while (digitalRead(sw4) == LOW);
            delay(1500);
            return; 
        }
        delay(20);
    }

    bool previousState = isCellInitialized;
    isCellInitialized = false; 

    displayLCDMessage("Starting", "Calibration...");
    if (delayWithCancellation(1000)) { isCellInitialized = previousState; return; } 

    if (hx711_cell.is_ready()) {
        displayLCDMessage("Remove all weight", "from scale.");
        if (delayWithCancellation(3000)) { isCellInitialized = previousState; return; } 

        hx711_cell.set_scale();
        hx711_cell.tare();
        filteredWeight = 0.0; 
        
        displayLCDMessage("Place weight", "known (" + String(KNOWN_WEIGHT/1000) + "kg)", "Confirm: (Btn 1)");

        while (true) {
            if (digitalRead(sw1) == LOW) {
                while (digitalRead(sw1) == LOW); 
                delay(50);
                break; 
            }
            if (digitalRead(sw4) == LOW) {
                displayLCDMessage("Calibration", "Canceled.");
                while (digitalRead(sw4) == LOW); 
                delay(1500);
                isCellInitialized = previousState;
                return; 
            }
            delay(20); 
        }

        displayLCDMessage("Calibrating...", "Please wait.");

        float sensorSum = 0.0;
        for (byte i = 0; i < 5; i++) {
            if (digitalRead(sw4) == LOW) {
                 displayLCDMessage("Calibration", "Canceled.");
                 while (digitalRead(sw4) == LOW);
                 delay(1500);
                 isCellInitialized = previousState;
                 return; 
            }
            sensorSum += hx711_cell.get_units(10); 
            if (delayWithCancellation(200)) { isCellInitialized = previousState; return; }
        }
        
        sensorResult = sensorSum / 5.0; 
        calibrationFactor = sensorResult / KNOWN_WEIGHT;
        preferencias.putFloat("CFVal", calibrationFactor);
        
        hx711_cell.set_scale(calibrationFactor);
        isCellInitialized = true;

        displayLCDMessage("Success!", "Scale ready.");
        delayWithCancellation(2000); 

    } else {
        displayLCDMessage("Error: HX711", "not found.");
        delayWithCancellation(2000);
        isCellInitialized = previousState;
    }
}

void tareFunction() {
    if (isCellInitialized) {
        hx711_cell.tare();
        filteredWeight = 0.0; 
        displayLCDMessage("Scale Tared");
        delayWithCancellation(1500);
    } else {
        displayLCDMessage("Error: Cell", "not initialized.");
        delayWithCancellation(1500); 
    }
}

void displayAndLogWeight() {
    float weight = getHX711Weight(); 
    String dateStr = getDateString();
    RtcDateTime now = Rtc.GetDateTime();
    char timestring[10];
    sprintf(timestring, "%02d:%02d:%02d", now.Hour(), now.Minute(), now.Second());

    Serial.print("Date: ");
    Serial.print(dateStr);
    Serial.print(", Weight: ");
    Serial.print(weight/1000, 2); 
    Serial.println(); 

    displayLCDMessage("Weight Logged:", String(weight/1000, 2) + " kg", "Date: " + dateStr, "Time: " + String(timestring));
    delayWithCancellation(2500); 
}