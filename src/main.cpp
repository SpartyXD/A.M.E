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
//OBJECTS
Screen screen;
Arm arm;
Speaker speaker;
Encoder encoder;
Potentiometer pot;

//===================================

void setup() {
    speaker.init(BUZZERPIN, 2);
    arm.init(SERVOPIN, 0);
    screen.init(speaker);
    pot.init(POTPIN);
    encoder.init(CLKPIN, DTPIN, SWPIN, encoderTurned, encoderDirection, speaker);
    attachInterrupt(digitalPinToInterrupt(CLKPIN), updateEncoder, CHANGE);
    
    screen.loading_screen();
}

//===================================
//MODE HANDLE
String messages[] = {"Hola :D", "Sigue asi!", "No pares!", "TKM", "Juguemos?", "FOCUS!"};
int N_MESSAGES = sizeof(messages)/sizeof(messages[0]);

// 0-Idle | 1-Timer | 2-Game | 3-Decision
int CURRENT_MODE = 0;

struct idleMode{
    unsigned long last_change = 0;
    unsigned long time_now = 0;
    unsigned long random_delay = 5000;

    bool first_boot = true;
    bool show_message = false;
    int idx = 0;

    idleMode(){}

    void updateRandom(){
        if(first_boot){
            first_boot = false;
            last_change = get_time();
            return;
        }

        time_now = get_time();
        if(time_now-last_change <= random_delay)
            return;
        
        last_change = time_now;
        show_message = random(1, 1001) <= 400; //MESSAGE PROBABILITY = 0.4
        int new_idx = idx;

        if(show_message){
            while(new_idx == idx)
                new_idx = random(0, N_MESSAGES);
            idx = new_idx;
        }
        else{
            while(new_idx == idx || new_idx == SAD || new_idx == ANGRY)
                new_idx = random(0, N_FACES);
            idx = new_idx;
        }
    }

    void run(){
        updateRandom();
        if(show_message)
            screen.printCentered(messages[idx]);
        else
            screen.showFace(idx);
    }
};

struct timerMode{
    timerMode(){}

    void run(){
        //TODO: IMPLEMENT
    }
};

struct gameMode{
    gameMode(){}

    void run(){
        //TODO: IMPLEMENT
    }
};

struct decisionMode{
    int choices = 2;
    bool setting_up = true;
    int threshold = 50;

    /*
    Main idea: Lift up the potentiometer arm and once ready
    (adjust number of choices with encoder) turn arm down and
    get a random number (gambling). Press encoder button to go back to idle
    the selected number will appear on screen until arm is lifted up again for more
    gambling
    */
    
    decisionMode(){}

    void run(){
        //Back to idle?
        if(encoder.isPressed())
            CURRENT_MODE = 0; 
        
        setting_up = pot.getReading() > threshold;


    }

};

idleMode idleScreen;
timerMode timerScreen;
gameMode gameScreen;
decisionMode decisionScreen;

//===================================

void loop(){
    switch (CURRENT_MODE){
    case 0:
        idleScreen.run();
        break;
    case 1:
        timerScreen.run();
        break;
    case 2:
        gameScreen.run();
    case 3:
        decisionScreen.run();
    default:
        break;
    }
    delay(20);
}