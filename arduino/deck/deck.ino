#include <pitchToFrequency.h>
#include <pitchToNote.h>
#include <frequencyToNote.h>
#include <MIDIUSB.h>

#include <TouchWheel.h>
#include <TouchSlider.h>
#include <AdcTouchChannel.h>

#include <ADCTouch.h>

const byte CMF_DECK_BUTTON_TT = 0;
const byte CMF_DECK_BUTTON_PLAY = 7;
const byte CMF_DECK_BUTTON_CUE = 13;

const byte CMF_DECK_TT1 = 6;
const byte CMF_DECK_TT2 = 5;
const byte CMF_DECK_TT3 = 4;

const byte CMF_DECK_PITCH1 = 12;
const byte CMF_DECK_PITCH2 = 11;
const byte CMF_DECK_PITCH3 = 10;
const byte CMF_DECK_PITCH4 = 9;
const byte CMF_DECK_PITCH5 = 8;

TouchWheel tw(CMF_DECK_TT1, CMF_DECK_TT2, CMF_DECK_TT3);

int ref0, ref1, ref2;     //reference values to remove offset
int refTT, refPlay, refCue;
int max0, max1, max2;
bool led0On, led1On, led2On = false;

byte mode = 0;
byte largest = 0;

void setup() {
  Serial.begin(115200);
  pinMode(CM_DECK_LED_PLAY, OUTPUT);
  pinMode(CM_DECK_LED_CUE, OUTPUT);
  pinMode(CM_DECK_LED_TT, OUTPUT);

  digitalWrite(CM_DECK_LED_PLAY, HIGH);
  digitalWrite(CM_DECK_LED_CUE, HIGH);
  digitalWrite(CM_DECK_LED_TT, HIGH);

//  tw.setFastAdc(); //hack the ADC to run 8x faster
//  tw.calibrate(); //run this when no one is touching sensor

  delay(1000);
  
  ref0 = 773; //ADCTouch.read(CMF_DECK_TT1, 500);    //create reference values to 
  ref1 = 760; //ADCTouch.read(CMF_DECK_TT2, 500);    //account for the capacitance of the pad
  ref2 = 769; //ADCTouch.read(CMF_DECK_TT3, 500);

  refTT = 0; //ADCTouch.read(CMF_DECK_BUTTON_TT, 41);   //no second parameter
  refPlay = 0; //ADCTouch.read(CMF_DECK_BUTTON_PLAY, 36);   //   --> 100 samples
  refCue = 0; //ADCTouch.read(CMF_DECK_BUTTON_CUE, 30);
}

void loop() { 
//  handleButtons();
  handleWheel();
//  handleSlider();
//  handleTouchWheel();
}

boolean isTouching = false;

void handleTouchWheel() {
  int wheel = tw.scan();  //call this frequently and check the results
  if (wheel > 0) { //positive indicates one direction
    Serial.println("Wheel moved up!");
  } else if (wheel < 0) { //negative the other direction
    Serial.println("Wheel moved down!");
  }

  if (tw.isTouching() != isTouching) {
    isTouching = tw.isTouching();
    Serial.print("Wheel is touching: ");
    Serial.println(isTouching);
  }
}

byte maximum(byte a, byte b, byte c)
{
   int max = ( a < b ) ? b : a;
   return ( ( max < c ) ? c : max );
}

int readf(byte adcChannel, byte digitalPin, int samples = 100)
{
  long _value = 0;
  for(int _counter = 0; _counter < samples; _counter ++)
  {
    // set the analog pin as an input pin with a pullup resistor
    // this will start charging the capacitive element attached to that pin
    pinMode(digitalPin, INPUT_PULLUP);
    
    // connect the ADC input and the internal sample and hold capacitor to ground to discharge it
    ADMUX |=   0b11111;

    // start the conversion
    ADCSRA |= (1 << ADSC);

    // ADSC is cleared when the conversion finishes
    while((ADCSRA & (1 << ADSC)));

    pinMode(digitalPin, INPUT);
    _value += analogRead(adcChannel);
  }
  return _value / samples;
}

bool playPressed = false;
bool cuePressed = false;
bool ttPressed = false;

void handleButtons() {
  int value0 = ADCTouch.read(CMF_DECK_BUTTON_TT, 41);   //no second parameter
  int value1 = ADCTouch.read(CMF_DECK_BUTTON_PLAY, 36);   //   --> 100 samples
//  int value2 = ADCTouch.read(CMF_DECK_BUTTON_CUE, 30);
  
//  value0 -= refTT;       //remove offset
//  value1 -= refPlay;
//  value2 -= refCue;
//
//  Serial.print(value0);             //send actual reading
//  Serial.print(" ");
//  Serial.println(value1);
//  Serial.print(" ");
//  Serial.println(value2);
  
  if (value0 > 40 && ttPressed == false) {
    ttPressed = true;
    noteOn(0, 1, 127);
  } else if (value0 < 30 && ttPressed) {
    ttPressed = false;
    noteOff(0, 1, 127);
  }
  
  if (value1 > 40 && cuePressed == false) {
    cuePressed = true;
    noteOn(0, 2, 127);
  } else if (value1 < 30 && cuePressed) {
    cuePressed = false;
    noteOff(0, 2, 127);
  }

  MidiUSB.flush();
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

int previousValue = 0;
bool touching = false;
const int wheelMax = 666;
const int wheelMin = 4;
const int wheelRange = wheelMax - wheelMin;

void handleWheel() {
  int value0 = readf(CMF_DECK_TT1, 37);   //no second parameter
  int value2 = readf(CMF_DECK_TT2, 38);   //   --> 100 samples
  int value1 = readf(CMF_DECK_TT3, 39);
  
  value0 -= ref0;       //remove offset
  value1 -= ref1;
  value2 -= ref2;
  
  if (value0 > 50 || value1 > 50 || value2 > 50) {
    if (value0 > value1) {
      if (value0 > value2) {
        largest = 0;
      } else {
        largest = 2;
      }
    } else {
      if (value1 > value2) {
        largest = 1;
      } else {
        largest = 2;
      }
    }
  } else {
    largest = 4;
  }

  int value = 0;

  switch (largest) {
    case 0: {
      value = 100 + (value1 - value2) * 120 / (value0);
      break;
    }
    case 1: {
      value = 350 - (value0 - value2) * 120 / (value1);
      break;
    }
    case 2: {
      value = 570 - (value1 - value0) * 120 / (value2);
      break;
    }
    case 4: {
      if (touching) {
        touching = false;
        noteOff(0, 0, 127);
        MidiUSB.flush();
      }
      return;
    }
  }
  Serial.print(value0);
  Serial.print('\t');
  Serial.print(value1);
  Serial.print('\t');
  Serial.print(value2);
  Serial.print('\t');
  Serial.print(value);
  Serial.print('\t');
  Serial.print(previousValue);
  Serial.print('\t');
  Serial.println(previousValue - value);

  byte diff = abs(previousValue - value) % 670;

  if (!touching) {
    touching = true;
    previousValue = value;
    noteOn(0, 0, 127);
    MidiUSB.flush();
  } else if (diff > 10 && value != previousValue) {
//    for (byte i = 0; i < diff / 10; ++i) {
//      controlChange(0, 0, value > previousValue ? 1 : 127);
//    }
    MidiUSB.flush();
    previousValue = value;
  }
}

void handleSlider() {
  int value0 = readf(CMF_DECK_PITCH1, 29);   //no second parameter
  int value1 = readf(CMF_DECK_PITCH2, 28);   //   --> 100 samples
  int value2 = readf(CMF_DECK_PITCH3, 27);
  int value3 = readf(CMF_DECK_PITCH4, 26);   //no second parameter
  int value4 = readf(CMF_DECK_PITCH5, 25);   //   --> 100 samples

  Serial.print(value0);             //send actual reading
  Serial.print(" ");
  Serial.print(value1);
  Serial.print(" ");
  Serial.print(value2);
  Serial.print(" ");
  Serial.print(value3);
  Serial.print(" ");
  Serial.println(value4);
}
