#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
 * Serial message definitions
 */

#define ATR_PROTOCOL_MAX_BUF_SIZE   64

char msgBuf[ATR_PROTOCOL_MAX_BUF_SIZE];
int msgBufPnt = 0;

/*
 * OLED Variables
 **/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
 * Buttons variables
 */
#define B_1 53
#define B_2 65
#define B_3 64
#define B_4 61
#define B_5 60
#define B_6 59
#define B_7 58
#define B_UP 61
#define B_DOWN 60
#define B_LEFT 59
#define B_RIGHT 58

/*
 * Robot location variables
 */
float xLocation, yLocation;

/*
 * Joystick Variables
 **/
int JOY_X = A8;
int JOY_Y = A9;

// Struct to hold the positions
struct JoyPositions {
    int joyX; // Left X=0 ~ Right X=1023
    int joyY; // Back Y=0 ~ Front Y=1023

    String getAbsolutePositions() {
        char toWrite[30];
        snprintf(toWrite, 30, "X%i|Y%i", joyX, joyY);

        return {toWrite};
    }

    String getStringPositions() const {
        char toWrite[30];
        // Pass the string in the right format so the robot can decode it
        snprintf(toWrite, 30, "Joystick;%i;%i;0\n\r", joyX, joyY);

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

/*
 * General Functions
 */

JoyPositions getJoyPositions();

void getButtonPositions();

void handleComm();

void drawMap();

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    // write your initialization code here
    // Joystick switch needs to have pullup
    pinMode(B_1, INPUT_PULLUP);
    pinMode(B_2, INPUT_PULLUP);
    pinMode(B_3, INPUT_PULLUP);
    pinMode(B_4, INPUT_PULLUP);
    pinMode(B_5, INPUT_PULLUP);
    pinMode(B_6, INPUT_PULLUP);
    pinMode(B_7, INPUT_PULLUP);
    pinMode(B_UP, INPUT_PULLUP);
    pinMode(B_DOWN, INPUT_PULLUP);
    pinMode(B_RIGHT, INPUT_PULLUP);
    pinMode(B_LEFT, INPUT_PULLUP);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_X, INPUT);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //Ive changed the address //already chill
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();

    // Draw a single pixel in white
    //display.drawPixel(10, 10, SSD1306_WHITE);

    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    //display.display();

}

unsigned long tenMillis = millis();

JoyPositions joyPositions = getJoyPositions();

void handleAsync() {
    // Doing 10 micros for encoder
    if (millis() > tenMillis + 200) {
        tenMillis = millis();

//        Serial1.print(getJoyPositions().getStringPositions());

        if (joyPositions.needsUpdate()) {
            // If the joy position was updated, send that over bluetooth
            Serial.print(joyPositions.getStringPositions());
            Serial1.print(joyPositions.getStringPositions());
        }
        //getButtonPositions();

        Serial.flush();
        Serial1.flush();

        handleComm();
        drawMap();
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

void getButtonPositions() {
    /*
     * B_4 is up
     * B_5 is down
     * B_6 is left
     * B_7 is right
     */

    String command = "";

    if (digitalRead(B_4) == LOW) {
        command = "Button:Up\n\r";

    } else if (digitalRead(B_5) == LOW) {
        command = "Button:Down\r\n";

    } else if (digitalRead(B_6) == LOW) {
        command = "Button:Left\r\n";

    } else if (digitalRead(B_7) == LOW) {
        command = "Button:Right\r\n";

    }

    if (command != "")
        Serial.print(command);
        Serial1.print(command);
}

bool checkMessage() {
//  Serial.println(msgBuf);
    char *p = msgBuf;
    String str;
    int cnt = 0;
// while ((str = strtok_r(p, ";", &p)) != NULL) // delimiter is the semicolon
    str = strtok_r(p, ";", &p);
    //Serial.println(str);
    if (str == "path") {
        while ((str = strtok_r(p, ";", &p)) != nullptr) {
            if (cnt == 0) {          //path x value
                xLocation = str.toFloat();
            } else if (cnt == 1) {    //path y value
                yLocation = str.toFloat();
            }
            cnt++;
        }
//        Serial.println("Joy ");
//        Serial.print(joystick[0]);
//        Serial.print(" ");
//        Serial.print(joystick[1]);
//        Serial.print(" ");
//        Serial.println(joystick[2]);
    } else if (str == "Command") {

    } else if (str == "Message") {
        while ((str = strtok_r(p, ";", &p)) != nullptr) {
            Serial.print("[");
            Serial.print(str);
            Serial.println("]");
        }

    }
}

void handleComm() {
    while (Serial1.available() > 0){
        char tmpChar = Serial1.read();
        if (msgBufPnt >= ATR_PROTOCOL_MAX_BUF_SIZE){
            Serial.println("Message Overflow");
            if ((tmpChar != '\n') || (tmpChar != '\r')){
                msgBuf[0] = tmpChar;
                msgBufPnt = 1;
            }
        }else{
            if ((tmpChar == '\n') || (tmpChar == '\r')){
                msgBuf[msgBufPnt] = '\0';
                checkMessage();
                msgBufPnt = 0;
            }else{
                msgBuf[msgBufPnt] = tmpChar;
                msgBufPnt++;
            }
        }
    }
}

void drawMap() {
    display.drawPixel(SCREEN_WIDTH/2 + xLocation, SCREEN_HEIGHT/2 + yLocation, SSD1306_WHITE);
    display.display();
}