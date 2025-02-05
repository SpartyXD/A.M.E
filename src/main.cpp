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
String messages[] = {"Hola :D", "Sigue asi!", "No pares!", "\tTKM", "Juguemos?", "FOCUS!"};
int N_MESSAGES = sizeof(messages)/sizeof(messages[0]);

// 0-Idle | 1-Timer | 2-Game | 3-Decision
int CURRENT_MODE = 0;
#define N_MODES 4

struct Menu{
    int N_OPTIONS = 0;
    String* options;

    int current=0;

    Menu(){}

    void init(int n, String options[]){
        N_OPTIONS = n;
        this->options = options;
    }

    void show(){
        display.clearDisplay();
        display.setTextSize(1);
        int x = 0;
        rep(i, N_OPTIONS){
            display.setCursor(0, x);
            if(i == current)
                display.print("-> ");
            display.print(options[i]);
            x += 19;
        }
        display.display();
    }

    //-1 if not selected, otherwise index
    int update(){
        int p = encoder.getRotation();
        current = constrain(current + p, 0, N_OPTIONS-1);

        if(encoder.isPressed())
            return current;
        else
            return -1;
    }
};

struct idleMode{
    unsigned long last_change = 0;
    unsigned long time_now = 0;
    unsigned long random_delay = 5000;

    bool first_boot = true;
    bool show_message = false;
    int idx = 0;

    bool on_menu = false;
    String options[N_MODES] = {"Volver", "Timer", "Pong", "Gambling"};
    Menu menu;

    idleMode(){
        menu.init(4, options);
    }


    void updateRandom(){
        if(first_boot){
            first_boot = false;
            last_change = get_time();
            return;
        }

        if(encoder.isPressed()){
            on_menu = true;
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


    void menuSelector(){
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            CURRENT_MODE = choice;
            on_menu = false;
        }
    }

    void run(){
        if(!on_menu){
            updateRandom();
            if(show_message)
                screen.printCentered(messages[idx]);
            else
                screen.showFace(idx);
        }
        else
            menuSelector();
    }
};


struct timerMode{
    bool running = false;
    bool setting = true;
    
    #define MAX_TIMER_TIME 10800 //3 hours
    #define STEP 300
    #define ALARM_DELAY 800

    //Alarm & time management
    long time_left = 0;
    unsigned long last_change = 0;
    unsigned long time_now = 0;
    bool screen_on = true;


    bool on_menu = false;
    String options[4] = {"Reanudar", "Ajustar", "Reiniciar", "Salir"};
    Menu menu;

    timerMode(){menu.init(4, options);}

    void menuSelector(){
        options[0] = "Reanudar ( " + format_time(time_left) + " )";
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            if(choice == 0)
                running = true; //Come back
            else if(choice == 1)
                setting = true; //Adjust
            else if(choice == 2){
                //Restart
                time_left = 0;
                setting = true;
            }
            else{
                CURRENT_MODE = 0; //back to idle
                setting = true;
            }

            on_menu = false;
        }
    }

    void run(){
        if(on_menu)
            menuSelector();
        else if(setting)
            setting_mode();
        else if(running)
            running_mode();
        else
            time_is_up();
    }

    void time_is_up(){
        if(encoder.isPressed()){
            //Show congrats face
            screen.showFace(HAPPY);
            delay(200);
            speaker.successBeep();
            delay(1300);

            //Back to setting mode
            setting = true;
            return;
        }

        time_now = get_time();

        //Beep
        if(time_now-last_change <= ALARM_DELAY)
            return;

        last_change = time_now;

        //Alternate screen number
        screen.printClock(0, "TIME IS UP!", screen_on);
        screen_on = !screen_on;
        speaker.alarmBeep();
    }


    void setting_mode(){
        //Check button
        if(encoder.isPressed()){
            if(time_left == 0){
                on_menu = true;
                setting = false;
            }
            else{
                setting = false;
                running = true;
            }
            return;
        }

        //Adjust time
        int p = encoder.getRotation();
        time_left = constrain(time_left + STEP*p, 0 ,MAX_TIMER_TIME);

        //Print current clock
        screen.printClock(time_left, "Ajustar tiempo");
    }


    void running_mode(){
        //Pause timer?
        if(encoder.isPressed()){
            on_menu = true;
            running = false;
            return;
        }

        //Update time
        time_now = get_time();
        if(time_now-last_change < 1000)
            return;
        
        last_change = time_now;
        time_left = constrain(time_left-1, 0, MAX_TIMER_TIME);
        
        //Show clock
        screen.printClock(time_left);

        //Time is up?
        if(time_left == 0)
            running = false;

    }
};

struct gameMode{

    bool on_menu = false;
    String options[N_MODES] = {"Volver", "Timer", "Pong", "Gambling"};
    Menu menu;

    gameMode(){menu.init(4, options);}

    void menuSelector(){
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            //DO STUFF
            on_menu = false;
        }
    }

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

    bool on_menu = false;
    String options[N_MODES] = {"Volver", "Timer", "Pong", "Gambling"};
    Menu menu;
    
    decisionMode(){menu.init(4, options);}

    void menuSelector(){
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            //DO STUFF
            on_menu = false;
        }
    }

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
        // gameScreen.run();
        screen.printCentered("GAME MODE!");
        delay(1000);
        CURRENT_MODE = 0;
        break;
    case 3:
        // decisionScreen.run();
        screen.printCentered("GAMBLING MODE!");
        delay(1000);
        CURRENT_MODE = 0;
        break;
    default:
        break;
    }
    delay(20);
}