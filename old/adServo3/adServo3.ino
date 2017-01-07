/******************************************************************************
    adServo v3 - Use to control servo's with an adServo by Timo Engelgeer
    Created by Timo Engelgeer (Septillion), January 13, 2015
    
    Can control up to 4 servo's connected to S1 - S4 of the adServo.
    In this version in this version full analog control.
    
    All buttons are debounced with the Bounce2 library.
    
    /////////Change settings, analog//////////////////////
    press and hold the mode button until prog-LED turns on to enter program mode
    
    Short press mode button to go to next setting.
    
    --one flash--
    First/green end position: Green => Clockwise, Red => Counterclockwise
    Starts in mid position
    --Two flashes--
    First/green end position: Green => Clockwise, Red => Counterclockwise
    Starts in mid position
    --Three flashes--
    Servo's start moving in current speed setting. Green => faster, Red => slower
    
    To save => Press and hold mode button until prog-LED turns on soft
    To cancel => Press reset switch or power down
    
    NOTE: If a servo is not moved when in end position change (one or two flashes) the current setting 
    isn't changed. So you can change every servo independent.
    
    NOTE: Both end positions can lie anywhere on the movement of the servo
    
    CHANGELOG
    
   ******** v3 ************
   -Updates ServoControl
   -Added program mode for analog changing of settings
    
   
******************************************************************************/

#include <Bounce2.h>
#include <ServoExtend.h>
#include <ServoControl.h>
#include <EEPROM.h>


/******************************************************************************
 * Define shortcuts
 ******************************************************************************/
#define     NrButtons    sizeof(ButtonMap)
#define     NrServos     sizeof(ServoMap)



/******************************************************************************
 * Vars and pinmaps
 ******************************************************************************/
const byte ButtonMap[] = {1, 0, 4, 5, A3, A2, A1, A0};  //Pins of the buttons
const byte ServoMap[] = {8, 9, 7, 6};    //Pins of the servo's

ServoControl servoControl;  //To control the servo's

const byte Led = 11;        //Led on the adServo
const byte LedArduino = 13; //Led on the Arduino board
const byte ProgramPin = 10; //Pin og the program switch

const int ServoMid = 504;

Bounce buttons[NrButtons] = Bounce();    //A Bounce object for each turnout switch
Bounce programButton = Bounce();        //A bounce object for the program button

unsigned long progButtonMillis;


/******************************************************************************
 * Prototypes
 ******************************************************************************/
 void(* resetFunc) (void) = 0;//declare reset function at address 0
 void progMode();             //The mode to change settings
 void resetSettings();        //reset settings to default

/******************************************************************************
 * EEPROM adresses
 ******************************************************************************/
 const byte EEGlobalReset           = 0;
 const byte EEServoDirs             = 1;
 const byte EEServoSpeedStep        = 17;
 const byte EEServoSpeedInterval    = 25;
 
 
/******************************************************************************
 * Setup
 ******************************************************************************/
void setup(){
    byte buttonPresses = 0;

    //setup the servo's
    for(byte i = 0; i < NrServos; i++){
        
        //read the values from EEPROM and setup
        servoControl.setEndPos(i, 0, EEPROM.read((int)(EEServoDirs + (i * 2))) * 4);    //set Position A (0-1012)
        servoControl.setEndPos(i, 1, EEPROM.read((int)(EEServoDirs + (i * 2) + 1)) * 4); //set Position B (0-1012)
        servoControl.setSpeed(i, EEPROM.read(EEServoSpeedStep + i), EEPROM.read(EEServoSpeedInterval + i));  //set speed, interval every (0 + 1) * 20ms, a step of 5
        
        servoControl.init(i, ServoMap[i]);//init the servo, attach pin and goto default position
    }    
    
    //Set up all turnout buttons, make them input with pullup and attach to bounce
    for(byte i = 0; i < NrButtons; i++){
        pinMode(ButtonMap[i], INPUT_PULLUP);
        //digitalWrite(ButtonMap[i], HIGH);    //fixed December 12, 2014, removed 13 January, 2015
         
        buttons[i].attach(ButtonMap[i]);
        if(buttons[i].read() == LOW){
            buttonPresses++;
        }
    }
    
    
    //set up the program button 
    pinMode(ProgramPin, INPUT_PULLUP);
    digitalWrite(ProgramPin, HIGH);
    programButton.attach(ProgramPin);
    
    //setup the LEDs
    pinMode(Led, OUTPUT);
    pinMode(LedArduino, OUTPUT);
    
    //Check if the decoder reset flag is set, will also reset a Arduino with clean EEPROM
    if(EEPROM.read(EEGlobalReset) == 255 || (programButton.read() == LOW && buttonPresses >= 3)){
        pinMode(Led, OUTPUT);
        digitalWrite(Led, HIGH);
        delay(1000);
        resetSettings();
        delay(1000);
    }
    
    #if defined(DEBUG) || defined(DEBUG_SERVOCONTROL)
        Serial.begin(9600);
    #endif
}

/******************************************************************************
 * THE loop :)
 ******************************************************************************/
void loop(){
    byte numButtons = 0;    //Counter for the number of buttons pressed
    
    servoControl.update();  //update all servo's
    
    //check all buttons for a press
    for(byte i = 0; i < NrButtons; i++){
        buttons[i].update();    //update the button state
        
        //button is pressed an is even (have priority over odd)
        if(buttons[i].read() == LOW && i % 2 == 0){    
            servoControl.gotoEndPos(i / 2, 0);    //go to position A
            numButtons += 1;
        }
        //button is pressed and is odd, check if the even button isn't pressed
        else if(buttons[i].read() == LOW && i % 2 == 1 && buttons[i - 1].read() == HIGH){            
            servoControl.gotoEndPos(i / 2, 1);    //go to position B
            numButtons += 1;
        }
    }
    
    //Turn ArduinoLED on if button is pressed
    if(numButtons != 0){
        digitalWrite(LedArduino, HIGH);
    }
    else{
        digitalWrite(LedArduino, LOW);
    }
    
    //Turn LED on soft if a servo is attached
    if(servoControl.isActive() == true){
        analogWrite(Led, 10);
    }
    //Turn ArduinoLED off otherwise
    else{
        analogWrite(Led, 0);
    }  
    
    //Lets check the program Button
    programButton.update();
    if(programButton.read() == LOW){
        if(programButton.fell()){        //Save the mills
            progButtonMillis = millis();
        }
        else if(progButtonMillis + 3000 < millis()){    //go into progMode if button is pressed for 3 sec
            progMode();
        }
    }
}

void progMode(){
    byte progMode = 1;
    byte done = 0;
    unsigned long ledMillis = 0;
    unsigned int ledTimeout = 0;
    bool ledState = false;
    byte ledCounter = 0;
    
    bool endPosChanged[NrServos][MAX_CONTROL_END_POS];
    bool speedChanged[NrServos];
    
    analogWrite(Led, 100);
    while(programButton.read() == LOW){
        programButton.update();
    }
    
    while(progMode > 0){
        switch(progMode){
            case 1:
            case 2:
                if(done == 0){
                    for(byte i = 0; i < NrServos; i++){
                        servoControl.gotoPos(i, ServoMid);
                    }
                    done = 1;
                }
                for(byte i = 0; i < NrButtons; i++){
                    buttons[i].update();
                                       
                    //decrease
                    if(buttons[i].fell() && i % 2 == 0){
                        if(servoControl.getPos(i / 2) > 0){
                            servoControl.gotoPos(i / 2, servoControl.getPos(i / 2) - 4);
                        }
                    }
                    //increase
                    else if(buttons[i].fell() && i % 2 == 1 && buttons[i - 1].read() == HIGH){
                        if(servoControl.getPos(i / 2) < 1012){
                            servoControl.gotoPos(i / 2, servoControl.getPos(i / 2) + 4);
                        }
                    }
                    if(buttons[i].fell()){
                        endPosChanged[i / 2][progMode - 1] = true;
                        servoControl.setEndPos(i / 2, progMode - 1, servoControl.getPos(i / 2));
                    }
                }
                break;
            case 3:
                if(done == 0){    //go to start position
                    for(byte i = 0; i < NrServos; i++){
                        servoControl.gotoEndPos(i, 0);
                    }
                    done = 1;
                }
                
                //Do the wiggeling
                for(byte i = 0; i < NrServos; i++){
                    if(!servoControl.isActive(i)){    //only change endPos after detached to wait @ endPos
                        if(servoControl.isAtEndPos(i, 0)){
                            servoControl.gotoEndPos(i, 1);
                        }
                        else if(servoControl.isAtEndPos(i, 1)){
                            servoControl.gotoEndPos(i, 0);
                        }
                    }
                }
                
                for(byte i = 0; i < NrButtons; i++){
                    buttons[i].update();
                }
                
                for(byte i = 0; i < (NrButtons / 2); i ++){
                    byte buttonNr = i * 2;
                    //increase speed
                    if(buttons[buttonNr].fell()){
                        speedChanged[i] = true;
                        servoControl.speedUp(i);
                        
                    }
                    //decrease speed
                    else if(buttons[buttonNr + 1].fell()){
                        speedChanged[i] = true;
                        servoControl.speedDown(i);
                    }
                }
                
                servoControl.update();
                break;
            case 4:
                //save settings from last
                
                progMode = 1;
                break;
        }
        
        //Let's handle the programButton
        programButton.update();
        //save millis so we can chack for long press
        if(programButton.fell()){
            progButtonMillis = millis();
        }
        //if it rose, just a short press, goto next progMode
        else if(programButton.rose()){
            progMode++;
            done = 0;
            if(progMode > 3){
                progMode = 1;
            }
        }
        //long press?
        else if(programButton.read() == LOW && progButtonMillis + 3000 < millis()){
            //end progmode and save
            progMode = 0;
        }
        
        //LED control, bink number of progMode, followed by long off and repeat
        if(ledMillis + ledTimeout < millis()){
            ledMillis = millis();
            if(ledCounter < progMode){    //as long as we haven't turned on the led progMode times
                if(ledState){    //turn off LED and save state
                    analogWrite(Led, 0);
                    ledState = false;
                }
                else{        //turn on lED, save state and increase counter because we blinked
                    analogWrite(Led, 255);
                    ledState = true;
                    ledCounter++;
                }
                ledTimeout = 150;    //set timer for a blink
            }
            //Done blinking progMode times, turn off led+state, reset counter and wait long to start over
            else{
                ledState = 0;
                analogWrite(Led, 0);
                ledCounter = 0;
                ledTimeout = 1000;
            }
        }
        
        servoControl.update();
    }
    
    //lets start saving
    for(byte servoNr = 0; servoNr < NrServos; servoNr++){
        //save all changed endPos
        for(byte i = 0; i < MAX_CONTROL_END_POS; i++){
            if(endPosChanged[servoNr][i]){
                EEPROM.write((EEServoDirs + (servoNr * 2) + i), (servoControl.getEndPos(servoNr, i)/ 4));
            }
        }
        //save speed change
        if(speedChanged[servoNr]){
            servoSpeed_t servoSpeed = servoControl.getSpeed(servoNr);
            
            EEPROM.write(EEServoSpeedStep + servoNr, servoSpeed.step);
            EEPROM.write(EEServoSpeedInterval + servoNr, servoSpeed.interval);
        }
    }
    
    //now reset the Arduino
    resetFunc();
}

void resetSettings(){
    //reset positions and speed
    for(byte i = 0; i < 8; i++){
        //position
        EEPROM.write((EEServoDirs + (i * 2)), 0);
        EEPROM.write((EEServoDirs + (i * 2) + 1), 253);
        
        //speed
        EEPROM.write((EEServoSpeedStep + i), 5);
        EEPROM.write((EEServoSpeedInterval + i), 0);
    }
    
    //clear rest flag
    EEPROM.write(EEGlobalReset, 0);
    
    while(programButton.read() == LOW){
        //keep waiting
    }
    resetFunc(); //call reset  
}
