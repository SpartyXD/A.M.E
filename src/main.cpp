#include <objects.h>
#include <esp_adc_cal.h>
#include <birthday.h>

//=====================================

//PINS
#define BAT_ADC 2 //Battery
#define SERVOPIN 10
#define BUZZERPIN 6
#define POTPIN 4
#define DTPIN 5
#define CLKPIN 7
#define SWPIN 9

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
HappyBday bday;

//===================================

#define DEBG_MODE false

void setup() {
    if(DEBG_MODE)
        Serial.begin(115200);
    speaker.init(BUZZERPIN, 2);
    bday.init(speaker);
    
    arm.init(SERVOPIN, 0);
    screen.init(speaker);
    pot.init(POTPIN);
    encoder.init(CLKPIN, DTPIN, SWPIN, encoderTurned, encoderDirection, speaker);
    attachInterrupt(digitalPinToInterrupt(CLKPIN), updateEncoder, CHANGE);
    
    screen.loading_screen();
    delay(2000);
}

//==================================
//Menu handling
struct Menu{
    int N_OPTIONS = 0;
    int MAX_OPTIONS = 4;
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
        int i=0;
        for(i=current; i < min(int(current+MAX_OPTIONS), int(N_OPTIONS)); i++){
            display.setCursor(0, x);
            if(i == current)
                display.print("-> ");
            display.print(options[i]);
            x += 16;
        }

        //Check if the menu can be scrolled..
        if(i < N_OPTIONS){
            display.fillRect(115, 40, 8, 16, SH110X_WHITE);
            display.fillTriangle(110, 56, 128, 56, 119, 62, SH110X_WHITE);
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

//===================================
//POWER OFF MODE
bool POWER_ON = true;

struct powerOffMode{
    bool first_time = true;

    powerOffMode(){}

    void turn_off_all(){
        display.clearDisplay();
        display.display();
        arm.move(0);
    }

    void wake_up(){
        screen.loading_screen();
        POWER_ON = true;
    }

    void run(){
        if(first_time){
            turn_off_all();
            first_time = false;
        }

        //Check if reactivated
        if(encoder.isPressed()){
            wake_up();
            first_time = true;
        }
    }

};

powerOffMode powerOffScreen;
//===================================
//Idle mode
String messages[] = {
    "Hola :D", "Sigue asi!", "No pares!",
    "TKM :)", "Juguemos?", "FOCUS!",
    "Persiste", "SLAY", "Cuenta conmigo",
    "No pivote?", "Miedo al exito?", "Apunta alto"};

int N_MESSAGES = sizeof(messages)/sizeof(messages[0]);

// Idle | Battery check | Timer | Pong | Gambling | APAGAR
String CURRENT_MODE = "Idle";
bool LOW_BATTERY = false;

struct idleMode{
    unsigned long last_change = 0;
    unsigned long time_now = 0;
    unsigned long random_delay = 120000;

    bool first_boot = true;
    bool show_message = false;
    int idx = 0;

    bool on_menu = false;
    String options[6] = {"Volver", "Feliz cumple", "Timer", "Pong", "Gambling", "APAGAR"};
    Menu menu;

    idleMode(){
        menu.init(6, options);
    }


    void updateRandom(){
        if(first_boot){
            first_boot = false;
            idx = 0;
            show_message = false;
            last_change = get_time();
            arm.move(0);
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
            CURRENT_MODE = options[choice];
            on_menu = false;

            if(CURRENT_MODE == "Volver")
                CURRENT_MODE = "Idle";
            else if(CURRENT_MODE == "Pong"){
                display.clearDisplay();
                screen.printCentered("PONG");
                display.display();
                speaker.successBeep();
                delay(800);
            }
            else if(CURRENT_MODE == "Gambling"){
                display.clearDisplay();
                screen.moveCursor(15, screen.centerY);
                screen.print("LET'S GO GAMBLING");
                display.display();

                speaker.gamblingBeep();
                delay(50);    
                speaker.gamblingBeep();
                delay(50);
                speaker.successBeep();
            }

            //Turn off screen
            if(CURRENT_MODE == "APAGAR"){
                POWER_ON = false;
                display.clearDisplay();
                screen.printCentered("Good Bye :p");
                display.display();
                speaker.sadBeep();
                CURRENT_MODE = "Idle";
            }
        }
    }

    void run(){
        if(!on_menu){
            updateRandom();
            if(show_message){
                display.clearDisplay();
                screen.printCentered(messages[idx]);
                display.display();
            }
            else
                screen.showFace(idx);
        }
        else
            menuSelector();
    }
};

idleMode idleScreen;


//============================================
//Battery related
#define CRITICAL_VOLTAGE 3.6
#define MAX_VOLTAGE 4.2
#define BATTERY_MODE_VOLTAGE 4.1

float CURRENT_VOLTAGE = 4.2;
bool BATTERY_MODE = false;

esp_adc_cal_characteristics_t adc_chars;
bool battery_initialized = false;

float getVoltage(){
    if(!battery_initialized){
        battery_initialized = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    }
    
    float voltage = (esp_adc_cal_raw_to_voltage(analogRead(BAT_ADC), &adc_chars))*2;
    voltage /= 1000.0;

    if(abs(CURRENT_VOLTAGE-voltage) > 0.5)
        return CURRENT_VOLTAGE; //Avoid spikes of noise
    else
        return voltage;
}



struct batteryCheckMenu{
    float charge_percentage = 0.0;

    batteryCheckMenu(){}

    void run(){
        //Back to idle?
        if(encoder.isPressed()){
            CURRENT_MODE = "Idle"; 
            idleScreen.first_boot = true;
            return;
        }
        
        charge_percentage = map(CURRENT_VOLTAGE, CRITICAL_VOLTAGE, MAX_VOLTAGE, 0.0, 100.0);
        display.clearDisplay();
        screen.header("Nivel de bateria");
        display.setCursor(screen.centerX-(String(charge_percentage).length()*5)+15, screen.centerY);
        screen.print(String(charge_percentage), 2);
        screen.print("%", 2);
        display.display();
    }

};

struct lowBatteryMenu{
    unsigned long time_now=0;
    unsigned long last_warning=0;
    unsigned long warning_delay = 180000;
    bool first_time = true;

    lowBatteryMenu(){}

    void run(){
        if(first_time){
            display.clearDisplay();
            screen.printCentered("Poca bateria :(");
            screen.moveCursor(-1, display.getCursorY()+20);
            screen.printCentered("( cargame )", 1, false);
            display.display();
            speaker.sadBeep();
            first_time = false;
        }

        time_now = get_time();
        if(time_now-last_warning<=warning_delay)
            return;

        last_warning = time_now;
        speaker.sadBeep();

    }

};

batteryCheckMenu batteryCheckScreen;
lowBatteryMenu lowBatteryScreen;

//===================================


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

    //Arms movement
    unsigned long last_arm_move = 0;
    #define ALARM_MOVE_DELAY 3000
    bool arm_up = true;


    bool on_menu = false;
    String options[4] = {"Reanudar", "Ajustar", "Reiniciar", "Salir"};
    Menu menu;

    timerMode(){menu.init(4, options);}

    void menuSelector(){
        options[0] = "Reanudar ( " + format_time(time_left) + " )";
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            if(choice == 0){
                if(time_left > 0){
                    running = true; //Come back
                    setting = false;
                }
                else{
                    setting = true;
                    running = false;
                }
            }
            else if(choice == 1){
                setting = true; //Adjust
                running = false;
            }
            else if(choice == 2){
                //Restart
                time_left = 0;
                setting = true;
                running = false;
            }
            else{
                CURRENT_MODE = "Idle"; //back to idle
                idleScreen.first_boot = true;
                setting = true;
                running = false;
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

        if(time_now-last_arm_move >= ALARM_MOVE_DELAY){
            last_arm_move = time_now;

            if(arm_up)
                arm.move(100);
            else
                arm.move(0);

            arm_up = !arm_up;
        }

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
        if(time_left > 300)
            screen.printClock(time_left);
        else
            screen.printClock(time_left, "Queda poco!");

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
    int cpu_speed = 3;
    int x_vel = 3;
    int y_vel = 3;

    int x_pos = 40;
    int y_pos = 26;

    //Scores
    int WINNING_SCORE = 3;
    #define MAX_SCORE 20
    #define MAX_DIF 5
    int l_score = 0;
    int r_score = 0;

    bool playing = false;
    bool ending = false;
    bool choosing_dif = false;
    bool choosing_points = false;

    //Timers
    unsigned long time_now = 0;
    unsigned long last_change = 0;
    unsigned long change_delay = 5000;

    bool on_menu = false;
    String options[3] = {"Reanudar", "Reiniciar", "Salir"};
    String start_options[4] = {"Jugar!", "Dificultad", "Cant. Rondas", "Salir"};
    Menu menu;
    Menu startMenu;

    gameMode(){
        menu.init(3, options);
        startMenu.init(4, start_options);
    }

    void menuSelector(){
        int choice = menu.update();
        menu.show();

        if(choice != -1){
            if(choice == 0){
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
                CURRENT_MODE = "Idle";
                idleScreen.first_boot = true;
            }
            on_menu = false;
        }
    }


    void startMenuSelector(){
        int choice = startMenu.update();
        start_options[1] = "Dificultad ( " + String(cpu_speed) + " )";
        start_options[2] = "Cant. rondas ( " + String(WINNING_SCORE) + " )";
        startMenu.show();

        if(choice != -1){
            if(choice == 0){
                start_game();
            }
            else if(choice == 1){
                choosing_dif = true;
                playing = false;
                ending = false;
                choosing_points = false;
            }
            else if(choice == 2){
                choosing_points = true;
                choosing_dif = false;
                playing = false;
                ending = false;
            }
            else{
                playing = false;
                ending = false;
                choosing_dif = false;
                choosing_points = false;

                CURRENT_MODE = "Idle";
                idleScreen.first_boot = true;
            }
            on_menu = false;
        }   
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

        //Set out by diff
        paddle_high = max(20 - cpu_speed, 2);
        x_vel = 1 + cpu_speed;
        y_vel = x_vel;

        //Start screen
        display.clearDisplay();
        screen.printCentered("A jugar :D");
        display.display();
        speaker.successBeep();
        delay(800);
    }

    void run(){
        if(on_menu)
            menuSelector();
        else if(choosing_dif)
            difficulty_menu();
        else if(ending)
            ending_menu();
        else if(playing)
            playing_loop();
        else if(choosing_points)
            point_menu();
        else
            startMenuSelector();
    }


    void point_menu(){
        if(encoder.isPressed()){
            choosing_points = false;
            return;
        }

        WINNING_SCORE += encoder.getRotation();
        WINNING_SCORE = constrain(WINNING_SCORE, 1, MAX_SCORE);

        screen.printCenteredTextNumber("Cant. rondas:", WINNING_SCORE);

        //Easter egg
        if(WINNING_SCORE == MAX_SCORE){
            screen.moveCursor(screen.centerX-10, 50);
            screen.print("( INSANO )");
        }
        
        display.display();     
    }


    void difficulty_menu(){
        if(encoder.isPressed()){
            choosing_dif = false;
            return;
        }

        cpu_speed += encoder.getRotation();
        cpu_speed = constrain(cpu_speed, 0, MAX_DIF);

        screen.printCenteredTextNumber("Dificultad:", cpu_speed);

        //Easter egg
        if(cpu_speed == MAX_DIF){
            screen.moveCursor(screen.centerX-10, 50);
            screen.print("( INSANO )");
        }
        
        display.display();
    }


    void ending_menu(){
        if(r_score == WINNING_SCORE){
            //Bot wins
            screen.showFace(HAPPY);
            speaker.celebrationBeep();
            arm.move(100);
            speaker.celebrationBeep();
            arm.move(0);
            screen.showFace(LOOK_LEFT);
            speaker.successBeep();
            arm.move(100);
            screen.showFace(LOOK_RIGHT);
            speaker.successBeep();
            arm.move(0);   

            //Show messages
            display.clearDisplay();
            screen.printCentered("TE GANEEE!");
            display.display();
            delay(2500);
        }
        else{
            //Bot losses
            screen.showFace(ANGRY);
            speaker.angryBeep();
            arm.move(100);
            speaker.angryBeep();
            arm.move(0);
            speaker.angryBeep();
            arm.move(100);
            screen.showFace(SAD);
            arm.move(0);
            speaker.sadBeep();

            //Show message
            display.clearDisplay();
            screen.printCentered("HAS GANADO!");
            display.display();
            delay(2500);
        }

        ending = false;
        playing = false;
        choosing_dif = false;
    }


    void playing_loop(){
        //Menu?
        if(encoder.isPressed()){
            on_menu = true;
            playing = false;
            return;
        }

        //Update positions
        l_pos = map(pot.getReading(), MAX_POT_POS, MIN_POT_POS, 0, SCREEN_HEIGHT-paddle_high);

        //cpu

        //Move generator
        int needs_move = random(1, 1001);
        if(needs_move <= max(190*cpu_speed, 500)){
            if(r_pos + paddle_high < y_pos)
                r_pos = constrain(r_pos+cpu_speed*2+1, 0, SCREEN_HEIGHT-paddle_high);
            else if(r_pos > y_pos)
                r_pos = constrain(r_pos-cpu_speed*2+1, 0, SCREEN_HEIGHT-paddle_high);
        }

        //Write to servo
        arm.move(map(r_pos, 0, SCREEN_HEIGHT-paddle_high, 100, 0));

        // Check for ball bouncing into paddles:
        if (ball_on_right_paddle() || ball_on_left_paddle()){
            x_vel = -x_vel;
            speaker.beep(300, 100);
        }

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

    }


    void ball_reset(bool left){
        //If left is true, then ball should launch from the left, otherwise launch from the right
        //Ball should launch at a random Y location and at a random Y velocity

        y_pos = random(display.height());
        if (random(2)>0)
            y_vel = abs(y_vel);
        else
            y_vel = -abs(y_vel);

        if (left){
            x_vel = abs(x_vel);
            x_pos = paddle_width-1;
        }
        else{
            x_vel = -abs(x_vel);
            x_pos = display.width()-paddle_width;  
        }
    }

    bool ball_on_right_paddle(){
        return((x_pos+x_vel) >= (SCREEN_WIDTH-paddle_width-1) && y_pos >= r_pos && y_pos <= (r_pos + paddle_high));
    }

    bool ball_on_left_paddle(){
        return((x_pos+x_vel) <= paddle_width+1 && y_pos >= l_pos && y_pos <= (l_pos + paddle_high));
    }

};


struct decisionMode{
    bool setting_up = false;
    bool gambling = false;
    int threshold = (MAX_POT_POS-MIN_POT_POS)/2;
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
            CURRENT_MODE = "Idle"; 
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
        else{
            display.clearDisplay();
            screen.header("Decision maker");
            screen.printCentered("Levanta mi");
            screen.moveCursor(-1, display.getCursorY()+10);
            screen.printCentered("Brazo derecho", 1, false);
            display.display();
        }
    }


    void setting_up_menu(){
        int p = encoder.getRotation();
        choices = constrain(choices + p, 2, 99);

        screen.printCenteredTextNumber("Opciones:", choices);
        screen.print("Bajar brazo = elegir");
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
        screen.moveCursor(0, 50);
        screen.printCentered("Click = continuar", 1, false);
        display.display();
        
        while(!encoder.isPressed()){}

        gambling = false;
    }

};


timerMode timerScreen;
gameMode gameScreen;
decisionMode decisionScreen;


//===================================

void loop(){
    //Check battery
    CURRENT_VOLTAGE = getVoltage();
    // if(CURRENT_VOLTAGE <= CRITICAL_VOLTAGE && !LOW_BATTERY){
    //     LOW_BATTERY = true;
    //     lowBatteryScreen.first_time = true;
    // }
    // else if(CURRENT_VOLTAGE >= CRITICAL_VOLTAGE && LOW_BATTERY)
    //     LOW_BATTERY = false;

    //Activate battery mode
    BATTERY_MODE = (CURRENT_VOLTAGE < BATTERY_MODE_VOLTAGE);
    arm.ACTIVE_ARM = !BATTERY_MODE;
    
    if(LOW_BATTERY){
        lowBatteryScreen.run();
        delay(100);
        return;
    }


    //Is powered off?
    if(!POWER_ON){
        powerOffScreen.run();
        return;
    }


    //Normal behaviour
    if(CURRENT_MODE == "Idle")
        idleScreen.run();
    else if(CURRENT_MODE == "Feliz cumple"){
        screen.clear();
        screen.printCentered("Feliz cumple :D");
        screen.show();
        bday.run();
        CURRENT_MODE = "Idle";
        screen.showFace(HAPPY);
        delay(1500);
        screen.clear();
        screen.printCentered("Con cariño");
        screen.show();
        speaker.successBeep();
        delay(900);
        screen.clear();
        screen.printCentered("By Mati :)");
        screen.show();
        delay(1000);
    }
    else if(CURRENT_MODE == "Timer")
        timerScreen.run();
    else if(CURRENT_MODE == "Pong")
        gameScreen.run();
    else if(CURRENT_MODE == "Gambling")
        decisionScreen.run();
    
    delay(20);
}
