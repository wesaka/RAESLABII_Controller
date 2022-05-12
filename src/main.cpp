#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
 * Serial message definitions
 */

#define ATR_PROTOCOL_MAX_BUF_SIZE   128

// Receiving messages
char msgBuf[ATR_PROTOCOL_MAX_BUF_SIZE];
int msgBufPnt = 0;

// Sedning messages
char outboundMsgBuf[ATR_PROTOCOL_MAX_BUF_SIZE];
int outboundMsgBufPnt = 0;

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
int xLocation, yLocation;

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
        snprintf(toWrite, 30, "<Joystick;%04d;%04d;0>\n", joyX, joyY);

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

void sendMessage(String message);

void drawMap();

void setup() {
    Serial.begin(38400);
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
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
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

int i = 48;
void handleAsync() {
    // Doing 10 micros for encoder
    if (millis() > tenMillis + 1000) {
        tenMillis = millis();

//        Serial.write(i);
//        if (i >= 90) {
//            i = 48;
//        } else {
//            i++;
//        }

//        if (joyPositions.needsUpdate()) {
//            // If the joy position was updated, send that over bluetooth
//            //Serial.print(joyPositions.getStringPositions());
//            Serial.print(joyPositions.getStringPositions());
//        }
        joyPositions.needsUpdate();
        Serial.print(joyPositions.getStringPositions());

        //getButtonPositions();

        Serial.flush();

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
        command = "<Joystick;512;512;0>\n";

    }
//    else if (digitalRead(B_5) == LOW) {
//        command = "Button:Down\r";
//
//    } else if (digitalRead(B_6) == LOW) {
//        command = "Button:Left\r";
//
//    } else if (digitalRead(B_7) == LOW) {
//        command = "Button:Right\r";
//
//    }

    if (command != "") {
        Serial.print(command);
    }
}

bool checkMessage() {
//  Serial.println(msgBuf);
    char *p = msgBuf;
    char *token;
    int cnt = 0;
// while ((str = strtok_r(p, ";", &p)) != NULL) // delimiter is the semicolon

// Read until a < is found - indicates the start of the command
    int mSize = 0;
    while (mSize < ATR_PROTOCOL_MAX_BUF_SIZE) {
        if (*p == '<') {
            // Found the start of the command
            // Advance one more char to get it ready for parsing the data
            p++;
            break;
        }
        else p++;

        mSize++;
    }

    if (mSize == ATR_PROTOCOL_MAX_BUF_SIZE) return false;

    // Initialize a temporary buffer to hold the information
    char tempBuf[ATR_PROTOCOL_MAX_BUF_SIZE] = {0};
    int tempIdx = 0;

    // Copy all the contents of the message until we hit the closing character to another string
    while (*p != '>') {
        // If '>' is never found, throw everything
        if (*p == '\0') {
            memset(msgBuf, 0, ATR_PROTOCOL_MAX_BUF_SIZE);
            memset(tempBuf, 0, ATR_PROTOCOL_MAX_BUF_SIZE);
            break;
        }

        tempBuf[tempIdx] = *p;
        tempIdx++;
        p++;
    }

    memset(msgBuf, 0, ATR_PROTOCOL_MAX_BUF_SIZE);

    token = strtok(tempBuf, ";");

    // Check the string returned by the robot for all the data
    while (token != nullptr) {
        if (strcmp(token, "path") == 0) {
            // If the string reads path, get the next two coordinates to figure out where we are
            token = strtok(nullptr, ";");
            xLocation = atoi(token);
            token = strtok(nullptr, ";");
            yLocation = atoi(token);
        } else if (strcmp(token, "orient") == 0) {

        } else if (strcmp(token, "linear") == 0) {

        } else if (strcmp(token, "angular") == 0) {

        } else if (strcmp(token, "totdist") == 0) {

        }

        // Get the next token
        token = strtok(nullptr, ";");
    }
}

void handleComm() {
    while (Serial.available() > 0){
        char tmpChar = Serial.read();
        Serial.write(tmpChar);
        if (msgBufPnt >= ATR_PROTOCOL_MAX_BUF_SIZE){
            //Serial.println("Message Overflow");
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

void sendMessage(String message) {
    int index = 0;
    for (char c : message) {
        Serial.write(c);
        delay(1);
    }
    Serial.write('\r');
}

void drawMap() {
    display.drawPixel((SCREEN_WIDTH/2) + (xLocation/2), (SCREEN_HEIGHT/2) + (yLocation/2), SSD1306_WHITE);
    display.display();
}