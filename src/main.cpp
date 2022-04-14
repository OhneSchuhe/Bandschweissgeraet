#include <Arduino.h>

String MegaStatus = "Hello World";

enum OpModes
{
  MODE_INIT, // 0
  MODE_STANDBY,
  MODE_WELDINIT,
  MODE_WELDSTART,
  MODE_WELDEND,
  MODE_FASTENSTART,
  MODE_FASTENEND // 5
};
int OPMODE = 0;  // Switch variable

enum LEDRModes
{
  LEDR_OFF, // 0
  LEDR_ON,
  LEDR_FLASH0,
  LEDR_FLASH5  // 3
};
int LEDRMODE = 0;  // Switch variable
enum LEDGModes
{
  LEDG_OFF, // 0
  LEDG_ON,
  LEDG_FLASH0,
  LEDG_FLASH5  // 3
};
int LEDGMODE = 0;  // Switch variable


enum MosfetModes // enum for MOSFET control
{
  MOSFET_OFF, // 0
  MOSFET_ON
};
enum LEDModes // enum for LED control
{
  LED_OFF, // 0
  LED_ON
};

// ############## Config for periodic functions
uint64_t lastStatus = 0;          // counter for status message
uint16_t statusInterval = 1000; // interval for the status messages in ms
uint64_t lastFastenAct = 0;         // counter for fasten actuation
uint32_t fastenInterval = 500;     // duration of one fasten actuation in ms
uint64_t lastWeldInit = 0;         // counter for weld init
uint64_t lastWeldAct = 0;         // counter for weld actuation
//LED
uint16_t ledToggleInterval = 250;  // interval for LED toggle
uint64_t lastLEDAct = 0;         // counter for LED  actuation

//LED
//WELDING
uint16_t weldDelay = 1000;  // delay to start welding for letting the user get the band clamped
uint16_t weldIntervalMin = 500;     // duration of one fasten actuation in ms
uint16_t weldIntervalMax = 5000;     // duration of one fasten actuation in ms
uint16_t weldIntervalInc = 0;  // interval increment for setting via poti
uint16_t weldInterval = 0;  // calculcated weld interval
//WELDING
//INPUT timings
uint64_t lastBtnRead = 0; // counter for the button readings in ms
uint64_t lastReedRead = 0; // counter for the reed contact readings in ms
uint64_t lastPotiRead = 0; // counter for the potentiometer readings in ms

uint32_t debounceInterval = 50; // interval for debouncing inputs in ms  
uint32_t potiInterval = 150; // interval for refreshing the potentiometer in ms  
//INPUT timings
//SENSORS
//POTI
uint16_t potiValue = 0;
uint16_t lastPotiValue = 0;
//REED SWITCH
bool reedValue = false;  // internal value of the reed switch
bool lastReedValue = false; // last internal value of the reed switch
//BUTTON
bool btnValue = false;  // internal value of the external button
bool lastBtnValue = false;  // last internal value of the external button
//SENSORS
// ############## I/O Pins for LEDs and Relays
int Pin_POTI = A3;
int Pin_LEDR = 15;
int Pin_LEDG = 14;
int Pin_REED = 2;
int Pin_BTN = 3;
int Pin_WELD = 4;  // MOSFET welding
int Pin_FASTEN = 5;  // MOSFET fasten


void statusPrinter(int force)
{
  long now = millis();
  if (now - lastStatus >= statusInterval or force == 1)
  {

    lastStatus = now;
    Serial.print("<{\"Status_Mega\":\"");
    Serial.print(MegaStatus);
    Serial.print("\",\"Uptime\":\"");
    Serial.print(now);
    //Serial.print("\",\"Weld interval\":\"");
    //Serial.print(weldInterval);
    Serial.println("\"}>");
  }
  else if (now < 5 or force == 2)
  { // debug for the boot process

    Serial.print("<{\"Status_Mega\":\"");
    Serial.print(MegaStatus);
    Serial.print("\",#\"Uptime\":\"");
    Serial.print(now);
    Serial.println("\"}>");
  }
}

void setupIO()
{
  pinMode(Pin_POTI, INPUT); // set the inputs 

  pinMode(Pin_REED, INPUT_PULLUP);
  pinMode(Pin_BTN, INPUT_PULLUP);
  pinMode(Pin_WELD, OUTPUT); // set the outputs
  pinMode(Pin_FASTEN, OUTPUT);  
  pinMode(Pin_LEDG, OUTPUT);
  pinMode(Pin_LEDR, OUTPUT);
  // init the MOSFETS to off
  digitalWrite(Pin_WELD, MOSFET_OFF); // set all mosfets to off
  digitalWrite(Pin_FASTEN, MOSFET_OFF);
  digitalWrite(Pin_LEDG, LED_OFF); // set all LEDs to off
  digitalWrite(Pin_LEDR, LED_OFF);
}

void setup() {
   Serial.begin(115200);  // set serial to 115200
   setupIO();             // setup the IO pins
   weldIntervalInc =   (weldIntervalMax - weldIntervalMin) /1023;
   statusPrinter(0);
   MegaStatus = "INIT";
   statusPrinter(0);
   
}

void StateMachine()
  {
    switch (OPMODE)
    {
    case MODE_INIT:
      MegaStatus = "Initialising Module";
      LEDRMODE = LEDR_ON;
      LEDGMODE = LEDG_ON;
      /* check if all inputs are off */
      if (btnValue and reedValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    case MODE_STANDBY:
      MegaStatus = "Standby, waiting for input.";
      LEDRMODE = LEDR_OFF;
      LEDGMODE = LEDG_ON;
      // wait for trigger
      if (!btnValue)
      {
        lastFastenAct = millis();
        OPMODE = MODE_FASTENSTART;
      }
      else if (!reedValue)
      {
        lastWeldInit = millis();
        OPMODE = MODE_WELDINIT;
      }
      break;
    case MODE_WELDINIT:
      MegaStatus = "Waiting for weld delay...";
      LEDRMODE = LEDR_OFF;
      LEDGMODE = LEDG_FLASH0;
      if (((millis() - lastWeldInit) > weldDelay))
      {
        OPMODE = MODE_WELDSTART;
        lastWeldAct = millis();
      }
      break;
    case MODE_WELDSTART:
      MegaStatus = "Welding band...";
      LEDRMODE = LEDR_FLASH0;
      LEDGMODE = LEDG_FLASH0;
      digitalWrite(Pin_WELD, MOSFET_ON);
      if (((millis() - lastWeldAct) > weldInterval) or reedValue)
      {
        digitalWrite(Pin_WELD, MOSFET_OFF);
        OPMODE = MODE_WELDEND;
      }
      break;
    case MODE_WELDEND:
      MegaStatus = "Ended welding, waiting for reed release...";
      LEDRMODE = LEDR_OFF;
      LEDGMODE = LEDG_FLASH0;
      if (reedValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    case MODE_FASTENSTART:
      MegaStatus = "Fastening band...";
      LEDRMODE = LEDR_FLASH0;
      LEDGMODE = LEDG_FLASH0;
      // enable MOSFET for motor
      digitalWrite(Pin_FASTEN, MOSFET_ON);
      // if we are over the interval, disable MOSFET for motor
      if (((millis() - lastFastenAct) > fastenInterval) or btnValue)
      {
        digitalWrite(Pin_FASTEN, MOSFET_OFF);
        OPMODE = MODE_FASTENEND;
      }
      break;
    case MODE_FASTENEND:
      MegaStatus = "Ended fastening, waiting for button release...";
      LEDRMODE = LEDR_OFF;
      LEDGMODE = LEDG_FLASH0;
      // wait untill button is released, return to standby
      if (btnValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    default:
      break;
    }
  }
void LEDRStateMachine()
{
  switch (LEDRMODE)
  {
  case LEDR_OFF:
    digitalWrite(Pin_LEDR, LED_OFF);
    break;
  case LEDR_ON:
    digitalWrite(Pin_LEDR, LED_ON);
    break;
  case LEDR_FLASH0:  // flash with phase 0
    if ((millis() - lastLEDAct) < ledToggleInterval)
    {
      digitalWrite(Pin_LEDR, LED_ON);  

    }
    else if ((millis() - lastLEDAct) < (2 * ledToggleInterval))
    {
      digitalWrite(Pin_LEDR, LED_OFF);  
    }
    else
    {
      lastLEDAct = millis();
    }
    break;
    case LEDR_FLASH5:  // flash with phase 180
    if ((millis() - lastLEDAct) < ledToggleInterval)
    {
      digitalWrite(Pin_LEDR, LED_OFF);  

    }
    else if ((millis() - lastLEDAct) < (2 * ledToggleInterval))
    {
      digitalWrite(Pin_LEDR, LED_ON);  
    }
    else
    {
      lastLEDAct = millis();
    }
    break;
  default:
    break;
  }
}
void LEDGStateMachine()
{
  switch (LEDGMODE)
  {
  case LEDG_OFF:
    digitalWrite(Pin_LEDG, LED_OFF);
    break;
  case LEDG_ON:
    digitalWrite(Pin_LEDG, LED_ON);
    break;
  case LEDG_FLASH0:  // flash with phase 0
    if ((millis() - lastLEDAct) < ledToggleInterval)
    {
      digitalWrite(Pin_LEDG, LED_ON);  

    }
    else if ((millis() - lastLEDAct) < (2 * ledToggleInterval))
    {
      digitalWrite(Pin_LEDG, LED_OFF);  
    }
    else
    {
      lastLEDAct = millis();
    }
    break;
    case LEDG_FLASH5:  // flash with phase 180
    if ((millis() - lastLEDAct) < ledToggleInterval)
    {
      digitalWrite(Pin_LEDG, LED_OFF);  

    }
    else if ((millis() - lastLEDAct) < (2 * ledToggleInterval))
    {
      digitalWrite(Pin_LEDG, LED_ON);  
    }
    else
    {
      lastLEDAct = millis();
    }
    break;
  default:
    break;
  }
}
void SensorRead()
{
  
  bool tmpBtnVal = digitalRead(Pin_BTN);
  bool tmpReedVal = digitalRead(Pin_REED);
  uint16_t tmpPotiVal = analogRead(Pin_POTI);
  MegaStatus = String(tmpReedVal) + "|" + String(reedValue)+ "|" + String(lastReedValue);  // debug
  // debounce everything
  // check if the read value is different from last btn state
  if (tmpBtnVal != lastBtnValue)  
  {
       lastBtnRead = millis();
  }
  // check if the read value is different from last reed state
  if (tmpReedVal != lastReedValue)  
  {
       lastReedRead = millis();
  }
  //check if we crossed the debounce interval for btn
  if ((millis() - lastBtnRead) > debounceInterval)
  {
    // if btn still has a different value than last measurement
    if (tmpBtnVal != btnValue)  
    {
        btnValue = tmpBtnVal;  // assign read value to internal value
        
    }
  }
  //check if we crossed the debounce interval for reed
  if ((millis() - lastReedRead) > debounceInterval)
  {
    // if btn still has a different value than last measurement
    if (tmpReedVal != reedValue)  
    {
        reedValue = tmpReedVal;  // assign read value to internal value
        
    }
  }
  //check if we crossed the debounce interval for poti
  if ((millis() - lastPotiRead) > potiInterval)
  {
 
    // if btn still has a different value than last measurement
    if (tmpPotiVal != potiValue)  
    {
        potiValue = tmpPotiVal;  // assign read value to internal value
        lastPotiRead = millis();
    }
  }
  lastBtnValue = tmpBtnVal;
  lastReedValue = tmpReedVal;
}
void setWeldInterval()
{
  // calculate current weld interval time
  weldInterval = weldIntervalMin + weldIntervalInc * potiValue;
  if (weldInterval > weldIntervalMax)
  {
    weldInterval = weldIntervalMax;
  }
  else if (weldInterval < weldIntervalMin)
  {
    weldInterval = weldIntervalMin;
  }
  
  
}
void loop() {
  statusPrinter(0);
  SensorRead();
  setWeldInterval();
  StateMachine();
  LEDRStateMachine();
  LEDGStateMachine();
}