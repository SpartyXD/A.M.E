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
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(centerX ,0);  display.print(title);
        display.drawLine (0,9,128,9, SH110X_WHITE);  
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
        spk->startupBeep();
        delay(1000);
    }

    void print(String message){
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0); 

        display.println(message);
        display.display();
    }

    //FIXME: Messages showing correctly (lenght)
    void printCentered(String message){
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(centerX, centerY);

        display.println(message);
        display.display();
    }

    //USE .DISPLAY AFTER XD
    void printCenteredTextNumber(String text, int number){
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(centerX, centerY-15);
        display.print(text);

        display.setTextSize(2);
        display.setCursor(centerX+13, centerY);
        display.print(String(number));
        display.setTextSize(1);

        display.setCursor(0, 50);
    }


    void printClock(int seconds, String message="Focus time!", bool screen_on=true){
        //Status message
        header(message);
        display.setCursor(centerX, centerY);
        display.setTextSize(2);

        if(screen_on)
            display.print(format_time(seconds));
        
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