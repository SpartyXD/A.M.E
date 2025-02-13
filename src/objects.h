#ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <faces.h>
#define rep(i, n) for(int i=0; i<n; i++)

//SCREEN
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Globals
Servo pwm;
#define MAX_ARDUINO_TIME 3294967295

unsigned long get_time(){
    return (millis()%MAX_ARDUINO_TIME);
}


//Format from seconds to MM:SS
String format_time(long seconds){
    String ans = "";
    //Calculate
    int m = seconds/60;
    int s = seconds - m*60;

    if(m < 10)
        ans += "0";
    ans += String(m);

    ans += ":";
    
    if(s < 10)
        ans += "0";
    ans += String(s);
    return ans;
}

//==========================================================

struct Speaker{
    int pin = 0;
    int channel = 0;

    Speaker(){}

    void init(int pin, int channel){
        this->pin = pin;
        this->channel = channel;
        pinMode(pin, OUTPUT);    
    }

    void beep(unsigned int frec, unsigned int dur){
        pwm.attach(pin, channel);
        pwm.tone(pin, frec, dur);
        delay(dur);
        pwm.detach(pin);
    }


    void startupBeep(){
        beep(700, 100);
        beep(900, 100);
    }

    void actionBeep(){
        beep(700, 100);
    }

    void alarmBeep(){
        beep(1000, 200);
        delay(100);
        beep(800, 300);
    }

    void successBeep(){
        beep(700, 100);
        delay(50);
        beep(1000, 100);
        delay(50);
        beep(1300, 100);
    }

    void gamblingBeep(){
        beep(700, 100);
        delay(50);
        beep(1000, 100);
        delay(200);
    }

    void sadBeep(){
        beep(1300, 100); 
        delay(50);
        beep(1000, 100); 
        delay(50);  
        beep(700, 100);
        delay(50);
        beep(500, 200);
    }

    void celebrationBeep(){
        beep(1000, 200);
        delay(300);
        beep(800, 300);
        delay(300);
        beep(600, 300);    
    }

    void angryBeep(){
        beep(600, 100);
        delay(50);
        beep(800, 100);
        delay(200);  
    }

};


struct Screen{
    int centerX = 42;
    int centerY = 26;
    Speaker* spk;

    const int CHAR_WIDTH = SCREEN_WIDTH/20;
    const int CHAR_HEIGHT = SCREEN_HEIGHT/8;

    Screen(){}

    void init(Speaker &spk){
        this->spk = &spk;
        display.begin(i2c_Address, true); // Address 0x3C default
        display.clearDisplay();
        display.display();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0,0);
        delay(500);
    }

    void header(String title){
        display.setTextSize(1);
        display.setCursor(0, 0);
        printCentered(title, 1, false);
        display.drawLine (0,10,128,10, SH110X_WHITE);  
    }

    void moveCursor(int x=-1, int y=-1){
        int new_x = (x != -1) ? x : display.getCursorX();
        int new_y = (y != -1) ? y : display.getCursorY();
        display.setCursor(new_x, new_y);
    }

    void loading_screen(){
        for(int i=0; i<=100; i+=20){
            display.clearDisplay();
            header("Cargando...");
            display.setCursor(centerX-(String(i).length()*5)+15, centerY);
            print(String(i), 2);
            print("%", 2);
            display.display();

            delay(800);
        }

        display.clearDisplay();
        printCentered("A . M . E");
        display.display();
        spk->startupBeep();
    }

    //FIXME: Messages showing correctly (lenght)
    void print(String message, int sz=1){
        display.setTextSize(sz);
        display.print(message);
    }

    
    void printCentered(String message, int sz=1, bool absolute=true){
        int aux = max(0, int(message.length()-10));
        if(absolute)
            display.setCursor(max(0, int(centerX-(message.length()*2)+18-aux)), centerY);
        else
            display.setCursor(max(0, int(centerX-(message.length()*2)+18-aux)), display.getCursorY());

        print(message, sz);
    }


    void printCenteredNumber(int number, int sz=2){
        display.setTextSize(sz);
        display.setCursor(centerX-(String(number).length()*5)+20, centerY);
        display.print(number);
    }


    void printCenteredTextNumber(String text, int number){
        display.clearDisplay();
        header(text);
        printCenteredNumber(number);
        display.setCursor(0, 50);
    }


    void printClock(int seconds, String message="Focus time!", bool screen_on=true){
        //Status message
        display.clearDisplay();
        header(message);

        if(screen_on){
            display.setCursor(centerX-5, centerY);
            print(format_time(seconds), 2);
        }
        
        display.display();
    }


    void showFace(int idx){
        idx = constrain(idx, 0, N_FACES-1);
        display.clearDisplay();
        display.drawBitmap(0, 0, Faces[idx], 128, 64, SH110X_WHITE);
        display.display();
    }


    void show(){
        display.display();
    }


    void clear(){
        display.clearDisplay();
    }

};


struct Encoder{
    int swpin, swState;
    Speaker* spk;

    volatile bool* encoderTurned;
    volatile bool* encoderDirection; //True = right

    //Debounce
    unsigned long debounce = 50;
    unsigned long last_check = 0;
    unsigned long time_now = 0;
    bool last_state = HIGH;

    Encoder(){}


    void init(int clk, int dt, int sw, volatile bool &turned, volatile bool &dire, Speaker &spk){
        swpin = sw;
        encoderTurned = &turned;
        encoderDirection = &dire;
        this->spk = &spk;

        pinMode(clk, INPUT);
        pinMode(dt, INPUT);
        pinMode(sw, INPUT_PULLUP);
        last_state = digitalRead(sw);
        last_check = millis();
    }

    //Is switch pressed?
    bool isPressed(){
        time_now = get_time();

        if(time_now-last_check <= debounce)
            return false;

        last_check = time_now;
        swState = digitalRead(swpin);

        if(swState == LOW && last_state == HIGH){
            spk->actionBeep();
            last_state = LOW;
            return true;
        }

        last_state = swState;
        return false;
    }

    //-1 = left | 0 = still | 1 = right
    //Detect encoder rotation
    int getRotation(){
        if(!*encoderTurned) 
            return 0;

        *encoderTurned = false;

        if(*encoderDirection)
            return 1; //Derecha
        else
            return -1; //Izquierda
    }

};


struct Arm{
    int pin;
    int channel;
    bool ACTIVE_ARM = true;

    #define RELAXED 103
    #define POINTING 6

    Arm(){}

    void init(int pin, int ch){
        this->pin = pin;
        channel = ch;
        pinMode(pin, OUTPUT);
        pwm.attach(pin, ch);
    }

    void move(int pos, int speed=500){
        if(!ACTIVE_ARM)
            return;
        int real = map(pos, 0, 100, RELAXED, POINTING);
        pwm.write(pin, real, speed, 0.0);
    }
};


struct Potentiometer{
    int pin;

    #define MIN_POT_POS 0
    #define MAX_POT_POS 26

    Potentiometer(){}

    void init(int pin){
        this->pin = pin;
        pinMode(pin, INPUT);
    }

    //0 - 100
    int getReading(){
        int read = analogRead(pin);
        return map(read, 4095, 0, 0, 100);
    }
};



#endif