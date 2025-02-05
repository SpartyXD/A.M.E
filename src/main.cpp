#include <objects.h>

//PINS
#define SERVOPIN 10
#define BUZZERPIN 6
#define POTPIN 2
#define DTPIN 5
#define CLKPIN 7
#define SWPIN 4

//===================================
//ENCODER
volatile bool encoderDirection;
volatile bool encoderTurned;

void updateEncoder(){
    encoderDirection = (digitalRead(CLKPIN) != digitalRead(DTPIN));
    encoderTurned = true;
}

//===================================

Screen screen;
Arm arm;
Speaker speaker;
Encoder encoder;
Potentiometer pot;

void setup() {
    screen.init();
    arm.init(SERVOPIN, 0);
    speaker.init(BUZZERPIN, 2);
    pot.init(POTPIN);
    encoder.init(CLKPIN, DTPIN, SWPIN, encoderTurned, encoderDirection);
    attachInterrupt(digitalPinToInterrupt(CLKPIN), updateEncoder, CHANGE);
    
    screen.printCentered("Hello :D");
    speaker.beep(700, 100);
    speaker.beep(900, 100);
    delay(1000);
}

int i = 0;
void loop(){
    i += encoder.getRotation();
    int r = pot.getReading();
    r = map(r, 0, 100, 0, 180);
    arm.move(r);

    if(encoder.isPressed())
        speaker.beep(750, 100);
    
    screen.printCentered(String(i));
    delay(20);
}
