/******************************************************************************
    adServo v2 - Use to control servo's with an adServo by Timo Engelgeer
    Created by Timo Engelgeer (Septillion), December 10, 2014
    
    Can control up to 4 servo's connected to S1 - S4 of the adServo.
    In this version only push buttons and no setup mode yet!
    
    All buttons are debounced with the Bounce2 library.
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
const byte ButtonMap[] = {1, 0, 4, 5, A3, A2, A1, A0};
const byte ServoMap[] = {8, 9, 7, 6};

ServoControl servoControl;  //To control the servo's

const byte Led = 11;        //Led on the adServo
const byte LedArduino = 13; //Led on the Arduino board
const byte ProgramPin = 10; //Pin og the program switch

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
    //Servo outputs LOW
    for(byte i = 0; i < NrServos; i++){
        pinMode(ServoMap[i], OUTPUT);
        digitalWrite(ServoMap[i], LOW);
    }
    
    //Check if the decoder reset flag is set, will also reset a Arduino with clean EEPROM
    if(EEPROM.read(EEGlobalReset) == 255){
        pinMode(Led, OUTPUT);
        digitalWrite(Led, HIGH);
        delay(1000);
        resetSettings();
        delay(1000);
    }
    
    //Set up all turnout buttons, make them input with pullup and attach to bounce
    for(byte i = 0; i < NrButtons; i++){
        pinMode(ButtonMap[i], INPUT_PULLUP);
        digitalWrite(ButtonMap[i], HIGH);    //fixed December 12, 2014
        
        buttons[i].attach(ButtonMap[i]);        
    }
    
    //set up the program button 
    pinMode(ProgramPin, INPUT_PULLUP);
    digitalWrite(ProgramPin, HIGH);
    programButton.attach(ProgramPin);
    
    //setup the LEDs
    pinMode(Led, OUTPUT);
    pinMode(LedArduino, OUTPUT);
    
    //setup the servo's
    for(byte i = 0; i < NrServos; i++){
        
        //read the values from EEPROM and setup
        servoControl.setPos(i, 0, EEPROM.read((int)(EEServoDirs + (i * 2))) * 4);    //set Position A (0-1012)
        servoControl.setPos(i, 1, EEPROM.read((int)(EEServoDirs + (i * 2) + 1)) * 4); //set Position B (0-1012)
        servoControl.setSpeed(i, EEPROM.read(EEServoSpeedStep + i), EEPROM.read(EEServoSpeedInterval + i));  //set speed, interval every (0 + 1) * 20ms, a step of 5
        
        servoControl.init(i, ServoMap[i]);//init the servo, attach pin and goto default position
    }
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
            servoControl.goSwitch(i / 2, 0);    //go to position A
            numButtons += 1;
        }
        //button is pressed and is odd, check if the even button isn't pressed
        else if(buttons[i].read() == LOW && i % 2 == 1 && buttons[i - 1].read() == HIGH){            
            servoControl.goSwitch(i / 2, 1);    //go to position B
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
    while(true){
        digitalWrite(Led, HIGH);
        delay(100);
        digitalWrite(Led, !digitalRead(Led));
        delay(200);
    }
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
    resetFunc(); //call reset  
}
