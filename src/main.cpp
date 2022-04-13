#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

/*
 * OLED Variables
 **/
bool ALLOCATED = false;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
 * Buttons variables
 */
#define B_1 53
#define B_2 65
#define B_3 64
#define B_UP 61
#define B_DOWN 60
#define B_LEFT 59
#define B_RIGHT 58

/*
 * Joystick Variables
 **/
int JOY_X = A8;
int JOY_Y = A9;

// Struct to hold the positions
struct JoyPositions {
    int joyX; // Left X=0 ~ Right X=1023
    int joyY; // Back Y=0 ~ Front Y=1023

    String getStringPositions() const {
        char toWrite[30];
        snprintf(toWrite, 30, "X%i|Y%i", joyX, joyY);

        return {toWrite};
    }

    // If the joystick didn't move, don't indicate anything and return false
    // But if moved, update the joy value and return true
    bool needsUpdate() {
        int tolerance = 15;
        int newX = analogRead(JOY_X);
        int newY = analogRead(JOY_Y);
        int differenceY = newY - joyY;
        int differenceX = newX - joyX;
        if (differenceX > tolerance || differenceX < (tolerance * -1) || differenceY > tolerance || differenceY < (tolerance * -1)) {
            joyX = newX;
            joyY = newY;
            return true;
        }
        return false;
    }
};


JoyPositions getJoyPositions();

void setup() {
    Serial.begin(9600);
    Serial1.begin(38400);
    // write your initialization code here
    // Joystick switch needs to have pullup
    pinMode(B_1, INPUT_PULLUP);
    pinMode(B_2, INPUT_PULLUP);
    pinMode(B_3, INPUT_PULLUP);
    pinMode(B_UP, INPUT_PULLUP);
    pinMode(B_DOWN, INPUT_PULLUP);
    pinMode(B_RIGHT, INPUT_PULLUP);
    pinMode(B_LEFT, INPUT_PULLUP);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_X, INPUT);

}

unsigned long hundredMillis = millis();
unsigned long tenMillis = millis();
JoyPositions joyPositions = getJoyPositions();

void handleAsync() {
    // Doing 10 micros for encoder
    if (millis() > tenMillis + 200) {
        tenMillis = millis();

        if (joyPositions.needsUpdate()) {
            // If the joy position was updated, send that over bluetooth
            Serial.println(joyPositions.getStringPositions());
            Serial1.println(joyPositions.getStringPositions());
        }
    }
}

void loop() {
    handleAsync();
}

JoyPositions getJoyPositions() {
    struct JoyPositions joyPos{};
    joyPos.joyX = analogRead(JOY_X);
    joyPos.joyY = analogRead(JOY_Y);
    return joyPos;

}