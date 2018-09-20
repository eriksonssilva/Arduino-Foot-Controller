#include <ArduinoTapTempo.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <MIDI.h>
#include "reper.h"
#define FN_CC_Switch 0
#define FN_PC_Message 1
#define FN_INCREASE_PRESET 2
#define FN_DECREASE_PRESET 3
#define FN_TAP 4
#define FN_Song_Display 5

LiquidCrystal_I2C lcd(0x3F,16,2);
ArduinoTapTempo tapTempo;

/*struct HairlessMidiSettings : public midi::DefaultSettings
{
   static const bool UseRunningStatus = false;
   static const long BaudRate = 115200;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, HairlessMidiSettings);*/
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

unsigned long prevButPress = 0;
unsigned long prevLcdState = 0;
unsigned long prevSongCount = 0;
unsigned long prevTap = 0;
long lcdTimer = 3000;
const long butTimer = 5;
const long songTimer = 1000;
long tapTimer;
const byte buttons = 12;//Number of digital inputs being used
const byte column = 12;/*this variable is part of the array that controls the foots and LED's states.
Each column on the variable "footDefine", refers to which foots/led's the selected foot should
turn ON or OFF, if the foot "function" equals to 1.
E.G: if foot 1 function equals to 1, it will send a PC message, and according to the footDefine
array, it will set the foots/led's from 2 to 12 ON or OFF accordingly*/
const byte preset = 10;//Number of Presets
const byte foot[buttons] = {2,3,4,5,6,7,8,9,10,11,12,13}; //Array that tells in which PIN the foots are connected
const byte colors = 2;
byte midiCCselect[preset][buttons] = {{55,2,3,4,5,6,18,32,9,10,11,12}, {13,14,15,16,17,18,19,20,21,22,23,24},{25,26,27,28,29,30,31,32,33,34,35,36},
{37,38,39,40,41,42,43,44,45,46,47,48},{49,50,51,52,53,54,55,56,57,58,59,60},{61,62,63,64,65,66,67,68,69,70,71,72},{73,74,75,76,77,78,79,80,81,82,83,84},
{85,86,87,88,89,90,91,92,93,94,95,96},{97,98,99,100,101,102,103,104,105,106,107,108},{109,110,111,112,113,114,115,116,117,118,119,120}}; /*Selects the midi CC or PC message
number for each foot, according to the preset number*/
byte presetSelect; //Variable which changes the preset number. The range is from 0 to 9.
byte songSelect;
byte lastPresetSelect;
byte buttonState[buttons] = {0,0,0,0,0,0,0,0,0,0,0,0};//Stores the button state
byte lastButtonState[buttons] = {0,0,0,0,0,0,0,0,0,0,0,0};//Stores the button previous state
/*this two variables (buttonState and lastButtonState) work as a Switch, preventing sending
  constant midi messages and sending messages only when the foot is stepped*/
byte footState[buttons] = {2,2,2,2,2,2,2,2,2,2,2,2};/*Stores the foot state
 This variable (footState) stores the foot state, as the name suggests. I've made this,
 so I could have total control over its state. I can set it to ON or OFF whenever I want,
 so I don't have to step the same button twice for toggling one effect ON or OFF.
 I had to do this because sometimes (when using PC's for changing from one preset to another)
 the button state was, let's say, OFF. Once I changed from one preset to another, using a PC,
 the button state should be set to ON, but it wasn't, and I had to step it once to fisically
 turn the button state to ON, and then once again, to turn OFF the foot state and the effect. 
 Once this variable  was correctly implemented, all I have to do is step once to turn it ON or OFF, 
 according to what I have programmed it.
 Also, they all have been set to 2 as an initial state. The reason is to avoid boot messages
 Once the loop is ran once, the foot states will be set according to preset 1, or the desired one.*/
byte ledState[colors][buttons] = {{22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},//Array that tells in which PIN the RED LED's are connected
                                  {34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45}}; //Array that tells in which PIN the BLUE LED's are connected
                                  
byte function[preset][buttons] = {{1,5,1,0,0,0,0,0,2,3,2,3}, {1,1,1,1,1,0,0,0,2,3,2,3}, {1,1,1,0,0,1,1,1,2,3,2,3}, 
{0,0,0,0,0,1,1,1,2,3,2,3}, {0,0,0,0,0,1,1,1,2,3,2,3}, {0,0,0,0,0,1,1,1,2,3,2,3}, 
{0,0,0,0,0,1,1,1,2,3,2,3}, {0,0,0,0,0,1,1,1,2,3,2,3}, {0,0,0,0,0,1,1,1,2,3,2,3}, {0,0,0,0,0,1,1,1,2,3,2,3}}; 

/*Defines what each button should do:
0 - Standard ON/OFF switch (Send's CC's)
1 - Sends PC messages
2 - Preset increase
3 - Preset decrease
4 - TAP Tempo (Send's an always ON (127) CC
5 - Displays the current song
6 - Skips to the Next Song
7 - Goes to the Previous Song*/

byte footDefine[preset][column][buttons/*foot number*/] = /*Each of this columns, refers to what ONE pin 
should do (in case its "function" equals to 1.)  And each of the 12 numbers inside the columns, refer to 
the foot/LED state.
0 = OFF
1 = ON
E.G: 
If Column 1 Preset 1 = {0,0,0,0,0,0,0,0,0,0,0,0}, means all foots and LED's will be set to ON once foot 1 is pressed.
If you step any of them (except foot 1), they will turn the equivalent midi message OFF, and also its led off.
If Column 4 Preset 1 = {0,0,1,1,1,0,0,0,0,0,0,0}, means that foots 1, 2, 6, 7, 8, 9, 10, 11 and 12 are ON,
and so are their LED's, whereas foots and LED's 3, 5 are OFF when foot 4 is pressed. 
According to the implemented logic, the button 4 should be OFF. But as it is sending a PC message (function 1),
its state is always ON, and so its LED.*/
/*Preset 1*/ {{{0,0,0,0,0,1,1,1,1,1,1,1}, {1,1,1,1,1,0,1,1,1,1,1,1}, {0,0,1,0,1,0,0,0,0,0,0,0}, {1,0,0,1,1,1,1,1,1,1,1,1}, {0,1,0,1,1,1,1,1,1,1,1,1}, {1,0,1,1,1,1,1,1,1,1,1,1}, {0,0,1,1,0,1,0,1,0,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 2*/ {{1,1,1,1,1,0,0,0,0,1,1,1}, {1,0,1,1,1,1,1,1,1,1,1,1}, {0,0,1,1,1,1,1,1,1,1,1,1}, {1,0,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 3*/ {{0,1,0,1,0,0,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,0,0,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 4*/ {{1,0,1,0,1,0,1,1,0,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,0,0,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 5*/ {{1,1,1,1,0,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 6*/ {{1,1,1,1,1,0,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 7*/ {{1,1,1,1,1,1,0,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 8*/ {{1,1,1,1,1,1,1,0,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset 9*/ {{1,1,1,1,1,1,1,1,0,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}},
/*Preset10*/ {{1,1,1,1,1,1,1,1,1,0,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}}};

byte currentIndex;
byte currentState;
byte footDrawState = 2;
byte footStateDefine;
byte songDisplay;
byte tapCount;
byte tapLed;
byte tapLedState;
byte tapNumber;
boolean needsRedraw = true;


const String effect[buttons] = {"Boost", "Delay", "Reverb", "Chorus", "Flanger", "Phaser", "Tremolo", "Gate", "Compressor"};

void setup() {
  
  for (byte i=0; i<buttons; i++){
  pinMode(foot[i], INPUT_PULLUP);
  pinMode(ledState[0][i], OUTPUT);
  pinMode(ledState[1][i], OUTPUT);
  }
  
  //Starts MIDI and sets baud rate:
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(4, 0);
  if (EEPROM[0] != 1) {
  EEPROM[0] = 1;
  EEPROM[1] = presetSelect;
  EEPROM[2] = songSelect;
  }
  }

void loop() {

unsigned long currentLcdState = millis();
unsigned long currentButPress = millis();
unsigned long currentTap = millis();


    if (currentButPress - prevButPress >= butTimer) {
      //Reads all digital inputs for changes on its state
      for (byte i =0; i < buttons; i++) {
      buttonState[i] = digitalRead(foot[i]);      
      }
    
    for (byte i=0; i<buttons; i++) {
        currentIndex = i;  
        currentState = function[presetSelect][i];    
        if (buttonState[i] != lastButtonState[i]) {
        switch(currentState) {
        
        case FN_CC_Switch://CC Switch
        {  
        if (footState[i] == 1) {//Sends midi CC ON message
        MIDI.sendControlChange(midiCCselect[presetSelect][i], 127, 1);
        digitalWrite(ledState[0][i], 0);
        digitalWrite(ledState[1][i], 1);
        footState[i] = 0;
        footDrawState = 1;
        needsRedraw = true;
        prevLcdState = currentLcdState;
        prevButPress = currentButPress;
        Serial.println("CC ON " + String(midiCCselect[presetSelect][i]));
        }
        else if (footState[i] == 0)  {//Sends midi CC OFF message
        MIDI.sendControlChange(midiCCselect[presetSelect][i], 0, 1);
        digitalWrite(ledState[0][i], 1);
        digitalWrite(ledState[1][i], 1);
        footState[i] = 1;
        footDrawState = 0;
        needsRedraw = true;        
        prevLcdState = currentLcdState;
        prevButPress = currentButPress;
        Serial.println("CC OFF " + String(midiCCselect[presetSelect][i]));
        }
        lastButtonState[i] = buttonState[i];
        if (needsRedraw == true) draw();
        break;
        }
        
        case FN_PC_Message: //PC Message
        if (footState[i] != 2) {  
        MIDI.sendProgramChange(midiCCselect[presetSelect][i], 1);//Sends midi PC OFF message
        for (byte j = 0 ; j < buttons; j ++){
        digitalWrite(ledState[0][j], footDefine[presetSelect][i][j]);
        if (function[presetSelect][j] == FN_PC_Message) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        /*This portion of the code will prevent unwanted LED's to turn on.
        E.G.: If function equals 1, but the LED is NOT the selected one
        it will be turned OFF.
        Also, if function is equal to 1 and ledState[0][i] equals to 0, the RED LED
        would be turned on. This portion of the code will prevent that as well.*/
        }
        else if (function[presetSelect][j] == FN_INCREASE_PRESET) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_DECREASE_PRESET) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_TAP) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_Song_Display) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        }
        digitalWrite(ledState[1][i], 0);/*this will keep the PC led always ON, even if 
        its state was set to 1 on the footDefine array*/
        for (byte j = 0 ; j < buttons; j ++){
        footState[j] = footDefine[presetSelect][i][j];
        }
        prevButPress = currentButPress;
        Serial.println("PC " + String(midiCCselect[presetSelect][i]));
        }
        lastButtonState[i] = buttonState[i];
        break;
        
        {
        case FN_INCREASE_PRESET:  //Here the preset is increased
        if (presetSelect < 9) {
        if (footState[i] != 2) {  
        presetSelect++;
        needsRedraw = true;
        prevButPress = currentButPress;
        EEPROM[1] = presetSelect;        
        Serial.println("PRESET MAIS");
        }
        }
        lastButtonState[i] = buttonState[i];
        if (needsRedraw == true) draw();        
        break;
        }

        {
        case FN_DECREASE_PRESET: //Here the preset is decreased
        if (presetSelect > 0) {
        if (footState[i] != 2) {  
        presetSelect--;
        needsRedraw = true;
        prevButPress = currentButPress;
        EEPROM[1] = presetSelect;
        Serial.println("PRESET MENOS");
        }
        }
        lastButtonState[i] = buttonState[i];
        if (needsRedraw == true) draw();
        break;
        }

        {
        case FN_TAP: //Tap tempo = always ON message  
        if (footState[i] != 2) {
        tapTempo.update(buttonState[i]);  
        prevTap = currentTap;
        prevLcdState = currentLcdState;
        prevButPress = currentButPress;
        needsRedraw = true;
        tapTimer = 60000 / (tapTempo.getBPM() * 4);
        MIDI.sendControlChange(midiCCselect[presetSelect][i], 127, 1);
        tapNumber = midiCCselect[presetSelect][i];        
        Serial.println("TAP " + String(midiCCselect[presetSelect][i]));
        tapCount = 0;
        tapLed = ledState[0][i];
        }
        lastButtonState[i] = buttonState[i];
        if (needsRedraw == true) draw();
        break;
        }

        {
        case FN_Song_Display: //Prints the Current song for 3 seconds
        if (footState[i] != 2) {  
        needsRedraw = true;
        lcdTimer = 5000;
        Serial.println(songDisplay);
        prevLcdState = currentLcdState;
        prevButPress = currentButPress;
        }
        lastButtonState[i] = buttonState[i];
        if (needsRedraw == true) draw();
        break;
        }
        }
        }
        }

        if (currentLcdState - prevLcdState >= lcdTimer) {
        prevLcdState = currentLcdState;
        lcdTimer = 3000;    
        songDisplay = 0;    
        lcd.setCursor(0,1);
        lcd.print("                ");
        }

        if (currentTap - prevTap >= tapTimer) {
    
        prevTap = currentTap;
    
        if (tapCount < 8) {
        if (tapLedState == LOW) {
        tapLedState = HIGH;
        MIDI.sendControlChange(tapNumber,127,1);
        tapCount++;
        } 
        
        else {
        tapLedState = LOW;
        }
        digitalWrite(tapLed, tapLedState);
        }
        else {
        if (tapLedState == LOW) {
        tapLedState = HIGH;
        } 
        else {
        tapLedState = LOW;
        }
        digitalWrite(tapLed, tapLedState);  
        }
        }


        for (int i = 0; i < buttons; i++) {

        if (footState[i] == 2) {  
        presetSelect = EEPROM[1];  
        songSelect = EEPROM[2];
        
        for (byte j = 0 ; j < buttons; j ++){
        digitalWrite(ledState[0][j], footDefine[presetSelect][i][j]);
        
        if (function[presetSelect][j] == FN_PC_Message) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_INCREASE_PRESET) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_DECREASE_PRESET) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_TAP) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        else if (function[presetSelect][j] == FN_Song_Display) {  
        digitalWrite(ledState[0][j], 1);
        digitalWrite(ledState[1][j], 1);
        }
        }
        digitalWrite(ledState[1][i], 0);
        
        for (byte j = 0 ; j < buttons; j ++){
        footState[j] = footDefine[presetSelect][i][j];
        }
        prevButPress = currentButPress;
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print(String("Preset ") + String(presetSelect + 1));        
        }
        }
}
}

void draw() {

  unsigned long currentSongCount = millis();
 
        switch (currentState)
        {
        case FN_CC_Switch:
        {
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print(String("Preset ") + String(presetSelect + 1));
        lcd.setCursor(0, 1);
        
        if (footDrawState == 1){
        lcd.print(String(effect[currentIndex]) + String(" ON"));
        Serial.println(currentIndex + String(" ON"));
        }
        else if (footDrawState == 0) {
        lcd.print(String(effect[currentIndex]) + String(" OFF"));
        Serial.println(currentIndex + String(" OFF"));
        }
        break;
        case FN_INCREASE_PRESET:
        case FN_DECREASE_PRESET:
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print(String("Preset ") + String(presetSelect + 1));
        break;
        case FN_TAP:
        lcd.clear();
        lcd.setCursor(4, 0);        
        lcd.print(String("Preset ") + String(presetSelect + 1));
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(String(tapTempo.getBPM() * 2) + " BPM");
        break;
        case FN_Song_Display:
        if (songDisplay == 0) {
        lcd.setCursor(0, 1);
        lcd.print("                ");        
        lcd.setCursor(0, 1);
        lcd.print(reper[songSelect]);
        songDisplay = 1;
        prevSongCount = currentSongCount;
        }
        else if (songDisplay == 1 && currentSongCount - prevSongCount <= songTimer) {
        songSelect++;
        lcd.setCursor(0, 1);
        lcd.print("                ");        
        lcd.setCursor(0, 1);
        lcd.print(reper[songSelect]);   
        prevSongCount = currentSongCount;
        EEPROM[2] = songSelect;        
        }
        else if (songDisplay == 1 && songSelect > 0 && currentSongCount - prevSongCount >= songTimer) {
        songSelect--;
        lcd.setCursor(0, 1);
        lcd.print("                ");        
        lcd.setCursor(0, 1);
        lcd.print(reper[songSelect]); 
        EEPROM[2] = songSelect;          
        }
        break;
        
}
}
needsRedraw = false;
}
