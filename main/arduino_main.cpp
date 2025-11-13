// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include <uni.h>
#include <QTRSensors.h>
#include "controller_callbacks.h"
#include <Wire.h>
#include <Arduino_APDS9960.h>
#include <bits/stdc++.h>
#include <ESP32SharpIR.h>

#define APDS9960_INT 2
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 100000

#define Motor1Pin1  16  // Control pin 1
#define Motor1Pin2  17  // Control pin 2
#define Motor2Pin1  18  // Control pin 3
#define Motor2Pin2  19  // Control pin 4


extern ControllerPtr myControllers[BP32_MAX_GAMEPADS]; // BP32 library allows for up to 4 concurrent controller connections, but we only need 1

TwoWire I2C_0 = TwoWire(0);
APDS9960 apds = APDS9960(I2C_0, APDS9960_INT);

QTRSensors qtr;
uint16_t sensors[2];

ESP32SharpIR FrontSensor(ESP32SharpIR::GP2Y0A21YK0F, 12);
ESP32SharpIR LeftSensor(ESP32SharpIR::GP2Y0A21YK0F, 14);
ESP32SharpIR RightSensor(ESP32SharpIR::GP2Y0A21YK0F, 27);

void dumpGamepad(ControllerPtr ctl) {
    Console.printf(
        "DPAD: %d A: %d B: %d X: %d Y: %d LX: %d LY: %d RX: %d RY: %d L1: %d R1: %d L2: %d R2: %d\n",
        ctl->dpad(),        // D-pad
        ctl->a(),           // Letter buttons
        ctl->b(),
        ctl->x(),
        ctl->y(),
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->l1(),           // Bumpers
        ctl->r1(),
        ctl->l2(),
        ctl->r2()
    );
}

void setup() {
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys(); 
    esp_log_level_set("gpio", ESP_LOG_ERROR); // Suppress info log spam from gpio_isr_service
    uni_bt_allowlist_set_enabled(true);
    pinMode(Motor1Pin1, OUTPUT);
    pinMode(Motor1Pin2, OUTPUT);
    pinMode(Motor2Pin1, OUTPUT);
    pinMode(Motor2Pin2, OUTPUT);
    I2C_0.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
    apds.setInterruptPin(APDS9960_INT);
    apds.begin();
    Serial.begin(115200);
    qtr.setTypeAnalog(); // or setTypeAnalog()
    qtr.setSensorPins((const uint8_t[]) {32, 33}, 2); // pin numbers go in the curly brackets {}, and number of sensors in use goes after

    // calibration sequence
    for (uint8_t i = 0; i < 250; i++) { 
        Console.printf("calibrating %d/250\n", i); // 250 is the number of calibrations recommended by manufacturer
        qtr.calibrate(); 
        delay(20);
    }

    FrontSensor.setFilterRate(1.0f);
    LeftSensor.setFilterRate(1.0f);
    RightSensor.setFilterRate(1.0f);
}

void moveNoob(Controller* controller) {
    if(controller && controller -> isConnected()) {
        if(controller -> axisRX() > 0) {
            digitalWrite(Motor1Pin1, LOW);
            digitalWrite(Motor1Pin2, HIGH);
            digitalWrite(Motor2Pin1, HIGH);
            digitalWrite(Motor2Pin2, LOW);
        }
        if(controller -> axisRX() < 0) {
            digitalWrite(Motor1Pin1, HIGH);
            digitalWrite(Motor1Pin2, LOW);
            digitalWrite(Motor2Pin1, LOW);
            digitalWrite(Motor2Pin2, HIGH);
        }
        if(controller -> axisY() < 0) {
            digitalWrite(Motor1Pin1, LOW);
            digitalWrite(Motor1Pin2, HIGH);
            digitalWrite(Motor2Pin1, LOW);
            digitalWrite(Motor2Pin2, HIGH);
        }
        if(controller -> axisY() > 0) {
            digitalWrite(Motor1Pin1, HIGH);
            digitalWrite(Motor1Pin2, LOW);
            digitalWrite(Motor2Pin1, HIGH);
            digitalWrite(Motor2Pin2, LOW);
        }

        if(controller -> axisRX() == 0 && controller -> axisY() == 0) {
            digitalWrite(Motor1Pin1, LOW);
            digitalWrite(Motor2Pin1, LOW);
            digitalWrite(Motor1Pin2, LOW);
            digitalWrite(Motor2Pin2, LOW);
        }
    }
}

void color(){
    int r, g, b, a;
    while (!apds.colorAvailable()) { delay(5); } // Wait until color is read from the sensor 
    apds.readColor(r, g, b, a);
    Console.printf("RED: %d GREEN: %d BLUE: %d AMBIENT: %d\n", r, g, b, a);
    delay(100);
}

void line(){
    qtr.readLineBlack(sensors); // Get calibrated sensor values returned into sensors[]
    Console.printf("S1: %d S2: %d\n", sensors[0], sensors[1]);
    delay(250);
}

void IRSensor(){
Console.printf("front: %.2f left: %.2f right: %.2f\n",
    FrontSensor.getDistanceFloat(),
    LeftSensor.getDistanceFloat(),
    RightSensor.getDistanceFloat());
    delay(100);
}

void loop() {
    vTaskDelay(1); // Ensures WDT does not get triggered when no controller is connected
    BP32.update(); 
    for (auto myController : myControllers) { // Only execute code when controller is connected
        if (myController && myController->isConnected() && myController->hasData()) {        
            moveNoob(myController);

            dumpGamepad(myController); // Prints the gamepad state, delete or comment if don't need
        }
    }
    //color();
    //line();
    IRSensor();
}

