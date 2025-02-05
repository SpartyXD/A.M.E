#ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <faces.h>

//SCREEN
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Globals
Servo pwm;
#define MAX_ARDUINO_TIME 3294967295

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

};


struct Screen{
    int centerX = 42;
    int centerY = 26;
    Speaker* spk;

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


    void loading_screen(){
        for(int i=0; i<=100; i+=20){
            display.clearDisplay();
            display.setTextSize(1);
            display.setCursor(centerX, centerY-15);
            display.print("Loading...");

            display.setTextSize(2);
            if(i != 100)
                display.setCursor(centerX+10, centerY);
            else
                display.setCursor(centerX+5, centerY);
            display.print(String(i) + "%");
            display.display();
            delay(800);
        }

        printCentered("Hello :D");
        spk->beep(700, 100);
        spk->beep(900, 100);
        delay(1000);
    }


    void printCentered(String message){
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(centerX, centerY);

        display.println(message);
        display.display();
    }


    void printClock(int seconds, String message="Focus time!"){
        //Calculate
        int m = seconds/60;
        int s = seconds - m*60;

        //Status message
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(32, 6);

        display.println(message);
        display.setCursor(centerX, centerY);
        display.setTextSize(2);

        //Format
        if(m < 10)
            display.print("0");
        display.print(m);

        display.print(":");
        
        if(s < 10)
            display.print("0");
        display.println(s);
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
        time_now = (millis() % MAX_ARDUINO_TIME);

        if(time_now-last_check <= debounce)
            return false;

        last_check = time_now;
        swState = digitalRead(swpin);

        if(swState == LOW && last_state == HIGH){
            spk->beep(700, 100);
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

    Arm(){}

    void init(int pin, int ch){
        this->pin = pin;
        channel = ch;
        pinMode(pin, OUTPUT);
        pwm.attach(pin, ch);
    }

    void move(int pos, int speed=70){
        pwm.write(pin, pos, speed, 0.0);
    }
};


struct Potentiometer{
    int pin;
    Potentiometer(){}

    void init(int pin){
        this->pin = pin;
        pinMode(pin, INPUT);
    }

    //0 - 100
    int getReading(){
        int read = analogRead(pin);
        return map(read, 0, 4095, 0, 100);
    }
};



#endif