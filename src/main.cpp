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

idleMode idleScreen;


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
                idleScreen.first_boot = true;
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
    //Paddle
    int paddle_high = 18;
    int paddle_width = 3;

    int l_pos = SCREEN_HEIGHT/2;
    int r_pos = SCREEN_HEIGHT/2;

    //Ball
    int cpu_speed = 4;
    int x_vel = 3;
    int y_vel = 3;

    int x_pos = 40;
    int y_pos = 26;

    //Scores
    #define WINNING_SCORE 3
    int l_score = 0;
    int r_score = 0;

    bool playing = false;
    bool ending = false;

    //Timers
    unsigned long time_now = 0;
    unsigned long last_change = 0;
    unsigned long change_delay = 5000;

    bool on_menu = false;
    String options[3] = {"Reanudar", "Reiniciar", "Salir"};
    Menu menu;

    gameMode(){menu.init(3, options);}

    void menuSelector(){
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            if(choice == 0){
                on_menu = false;
                playing = true;
            }
            else if(choice == 1){
                playing = false;
                ending = false;
            }
            else{
                playing = false;
                ending = false;

                //Show sad face before leaving
                screen.showFace(SAD);
                speaker.sadBeep();
                delay(1000);
                CURRENT_MODE = 0;
                idleScreen.first_boot = true;
            }
            on_menu = false;
        }
    }

    void run(){
        if(on_menu)
            menuSelector();
        else if(ending)
            ending_menu();
        else if(playing)
            playing_loop();
        else
            start_menu();
    }


    void start_game(){
        //Restart all and play
        l_score = 0;
        r_score = 0;

        x_pos = 40;
        y_pos = 26;

        l_pos = SCREEN_HEIGHT/2;
        r_pos = SCREEN_HEIGHT/2;

        playing = true;

        screen.printCentered("A jugar :D");
        speaker.successBeep();
        delay(800);
    }


    void ending_menu(){
        if(r_score == WINNING_SCORE){
            //Bot wins
            screen.showFace(HAPPY);
            speaker.celebrationBeep();
            arm.move(180);
            speaker.celebrationBeep();
            arm.move(0);
            screen.showFace(LOOK_LEFT);
            speaker.successBeep();
            arm.move(90);
            screen.showFace(LOOK_RIGHT);
            speaker.successBeep();
            arm.move(0);   

            //Show message
            screen.printCentered("TE GANEEE!");
            delay(2500);
        }
        else{
            //Bot losses
            screen.showFace(ANGRY);
            speaker.angryBeep();
            arm.move(90);
            speaker.angryBeep();
            arm.move(0);
            speaker.angryBeep();
            arm.move(180);
            screen.showFace(SAD);
            arm.move(0);
            speaker.sadBeep();

            //Show message
            screen.printCentered("HAS GANADO!");
            delay(2500);
        }

        ending = false;
        playing = false;
    }


    void playing_loop(){
        //Menu?
        if(encoder.isPressed()){
            on_menu = true;
            playing = false;
            return;
        }

        //Update positions
        l_pos = map(pot.getReading(), 0, 100, 0, SCREEN_HEIGHT-paddle_high);

        //cpu
        if(r_pos + paddle_high < y_pos)
            r_pos = constrain(r_pos+cpu_speed, 0, SCREEN_HEIGHT-paddle_high);
        else if(r_pos > y_pos)
            r_pos = constrain(r_pos-cpu_speed, 0, SCREEN_HEIGHT-paddle_high);

        //Write to servo
        arm.move(map(r_pos, 0, SCREEN_HEIGHT-paddle_high, 0, 180));


        //Check hits on sides
        if(x_pos > SCREEN_WIDTH-1){
            ball_reset(false);
            l_score = constrain(l_score+1, 0, WINNING_SCORE);
            speaker.actionBeep();
        }
        else if(x_pos < 0){
            ball_reset(true);
            r_score = constrain(r_score+1, 0, WINNING_SCORE);
            speaker.actionBeep();          
        }


        //Check hits on ceiling
        if(y_pos > SCREEN_HEIGHT-1 || y_pos < 0)
            y_vel = -y_vel;

        //Update ball
        x_pos += x_vel;
        y_pos += y_vel;


        // Draw pong elements:
        display.clearDisplay();
        display.fillCircle(x_pos, y_pos, 3, SH110X_WHITE);
        display.fillRect(0, l_pos, paddle_width, paddle_high, SH110X_WHITE);
        display.fillRect(SCREEN_WIDTH-paddle_width , r_pos, paddle_width, paddle_high, SH110X_WHITE);

        // Display scores
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(display.width()/4,0);
        display.println(l_score);
        display.setCursor(display.width()*3/4,0);
        display.println(r_score);

        // Display all elements
        display.display();


        //Check if winner
        if(l_score >= WINNING_SCORE || r_score >= WINNING_SCORE){
            speaker.celebrationBeep();
            ending = true;
            playing = false;
            return;
        }


        // Check for ball bouncing into paddles:
        if (ball_on_right_paddle() || ball_on_left_paddle()){
            x_vel = -x_vel;
            speaker.beep(300, 100);
        }

    }


    int positions[4] = {0, 45, 90, 135};
    void start_menu(){
        if(encoder.isPressed()){
            start_game();
            return;
        }

        screen.printCentered("PONG");

        display.setCursor(0, 50);
        display.print("Click = comenzar");
        display.display();

        //Random movements
        time_now = get_time();
        if(time_now-last_change > change_delay){
            last_change = time_now;
            int idx = random(4);
            arm.move(positions[idx], 30);
        }

    }


    void ball_reset(bool left){
        //If left is true, then ball should launch from the left, otherwise launch from the right
        //Ball should launch at a random Y location and at a random Y velocity

        y_pos = random(display.height());
        if (random(2)>0)
            y_vel = 1;
        else
            y_vel = -1;

        if (left){
            x_vel = 1;
            x_pos = paddle_width-1;
        }
        else{
            x_vel = -1;
            x_pos = display.width()-paddle_width;  
        }
    }

    bool ball_on_right_paddle(){
        return(x_pos == SCREEN_WIDTH-paddle_width-1 && y_pos >= r_pos && y_pos <= (r_pos + paddle_high) && x_vel == 1);
    }

    bool ball_on_left_paddle(){
        return(x_pos == paddle_width-1 && y_pos >= l_pos && y_pos <= (l_pos + paddle_high) && x_vel == -1);
    }

};


struct decisionMode{
    bool setting_up = false;
    bool gambling = false;
    int threshold = 50;
    int choices = 2;

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
        if(encoder.isPressed()){
            CURRENT_MODE = 0; 
            setting_up = true;
            gambling = false;
            idleScreen.first_boot = true;
            return;
        }
        
        if(!setting_up)
            setting_up = pot.getReading() > threshold;

        if(gambling)
            gambling_menu();
        else if(setting_up)
            setting_up_menu();
        else
            screen.print("Levanta mi brazo\n derecho");
    }


    void setting_up_menu(){
        int p = encoder.getRotation();
        choices = constrain(choices + p, 2, 99);

        screen.printCenteredTextNumber("Opciones:", choices);
        display.print("Bajar brazo = elegir");
        display.display();

        gambling = pot.getReading() <= threshold;
        setting_up = !gambling;
    }


    void gambling_menu(){
        //Animation
        screen.showFace(IDLE);
        speaker.gamblingBeep();
        screen.showFace(LOOK_RIGHT);
        speaker.gamblingBeep();  
        screen.showFace(LOOK_LEFT);
        speaker.gamblingBeep();
        screen.showFace(HAPPY);
        speaker.successBeep();  

        //Calculate & show
        int number = random(1, choices+1);
        screen.printCenteredTextNumber("ELEGIDO:", number); 
        display.display();
        delay(6000);

        gambling = false;
    }

};


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
        break;
    case 3:
        decisionScreen.run();
        break;
    default:
        break;
    }
    delay(20);
}