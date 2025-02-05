#ifndef OBJECTS_H
#define OBJECTS_H

#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

//TODO: Add the methods to all objects and make first test

//SCREEN
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Globals
Servo pwm;

//==========================================================

struct Screen{
    int centerX = 36;
    int centerY = 26;

    Screen(){}

    void init(){
        display.begin(i2c_Address, true); // Address 0x3C default
        display.clearDisplay();
        display.display();

        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0,0);
        delay(500);
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


    // void showFace(int idx){
    //     idx = constrain(idx, 0, N_FACES-1);
    //     display.clearDisplay();
    //     display.drawBitmap(0, 0, Caras[idx], 128, 64, SH110X_WHITE);
    // }


    void show(){
        display.display();
    }


    void clear(){
        display.clearDisplay();
    }


};


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


struct Encoder{
    int swpin, swState;

    volatile bool* encoderTurned;
    volatile bool* encoderDirection; //True = right

    Encoder(){}


    void init(int clk, int dt, int sw, volatile bool &turned, volatile bool &dire){
        swpin = sw;
        encoderTurned = &turned;
        encoderDirection = &dire;

        pinMode(clk, INPUT);
        pinMode(dt, INPUT);
        pinMode(sw, INPUT_PULLUP);
    }

    //Is switch pressed?
    bool isPressed(){
        swState = digitalRead(swpin);
        return (swState == LOW);
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