#include <ServoExtend.h>

#define     NrButtons    sizeof(ButtonMap)

const int ButtonMap[] = {1, 0, 4, 5, A3, A2, A1, A0};
const int ServoMap[] = {8, 9, 7, 6};
ServoExtend servos[sizeof(ServoMap)];

const byte DebounceDelay = 100; //ms

byte buttonTimes[NrButtons];
byte buttonPrevStates[NrButtons];
byte buttonFlags[NrButtons];
byte buttonDebouncedStates[NrButtons];

const int Led = 11;
const int LedArduino = 13;

byte byteMillis;


void setup(){
    for(byte i = 0; i < NrButtons; i++){
        pinMode(ButtonMap[i], INPUT_PULLUP);
        digitalWrite(i, HIGH);
        
        buttonTimes[i] = 0;
        buttonPrevStates[i] = digitalRead(ButtonMap[i]);
        buttonDebouncedStates[i] = buttonPrevStates[i];
        
    }
    pinMode(Led, OUTPUT);
    pinMode(LedArduino, OUTPUT);
    //Serial.begin(9600);
    //Serial.println("Hoi");
}

void loop(){
    int buttonState;
    
    
    if(!digitalRead(ButtonMap[0])){
        servos[0].attach(ServoMap[0]);
        //servos[0].write(0);
        servos[0].writeTicks(1088);
        analogWrite(Led, 50);
        digitalWrite(LedArduino, HIGH);
        
    }
    else if(!digitalRead(ButtonMap[1])){
        servos[0].attach(ServoMap[0]);
        //servos[0].write(180);
        servos[0].writeTicks(4800);
        analogWrite(Led, 255);
        digitalWrite(LedArduino, HIGH);
    }
    else if(!digitalRead(ButtonMap[2])){
        servos[0].attach(ServoMap[0]);
        //servos[0].write(0);
        //servos[0].writeByte(0);
        servos[0].writeTicks(1088);
        analogWrite(Led, 50);
        digitalWrite(LedArduino, HIGH);
    }
    else if(!digitalRead(ButtonMap[3])){
        servos[0].attach(ServoMap[0]);
        //servos[0].write(180);
        //servos[0].writeByte(255);
        servos[0].writeTicks(4800);
        analogWrite(Led, 255);
        digitalWrite(LedArduino, HIGH);
    }
    else{
        digitalWrite(LedArduino, LOW);
        servos[0].detach();
    }
    /*
    for(int i = 0; i < NrButtons; i++){
        byteMillis = millis();
        buttonState = !digitalRead(ButtonMap[i]);
        
        if(buttonState != buttonPrevStates[i]){
            buttonTimes[i] = byteMillis;
        }
        if(byteMillis - buttonTimes[i] > DebounceDelay && buttonState != buttonDebouncedStates[i]){
            buttonDebouncedStates[i] = buttonState;
            if(buttonState == 1){
                buttonFlags[i] = 1;
            }
        }
    }
    if(buttonFlags[0] == 1){
        buttonFlags[0] = 0;
        servos[0].attach(9);
        servos[0].write(0);
        analogWrite(Led, 50);
        digitalWrite(LedArduino, HIGH);
    }
    else if(buttonFlags[1] == 1){
        buttonFlags[1] = 0;
        servos[0].attach(9);
        servos[0].write(180);
        analogWrite(Led, 255);
        digitalWrite(LedArduino, HIGH);
    }
    else{
        digitalWrite(LedArduino, LOW);
        //servos[0].detach();
    }
    */
    
}
