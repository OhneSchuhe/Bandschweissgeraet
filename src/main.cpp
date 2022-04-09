#include <Arduino.h>

String MegaStatus = "Hello World";

enum OpModes
{
  MODE_INIT, // 0
  MODE_STANDBY,
  MODE_WELDSTART,
  MODE_WELDEND,
  MODE_FASTENSTART,
  MODE_FASTENEND // 5
};
int OPMODE = 0;  // Switch variable

enum MosfetModes // enum for MOSFET control
{
  MOSFET_OFF, // 0
  MOSFET_ON
};

// ############## Config for periodic functions
uint64_t lastStatus = 0;          // counter for status message
uint16_t statusInterval = 3000; // interval for the status messages in ms
uint64_t lastFastenAct = 0;         // counter for fasten actuation
uint32_t fastenInterval = 500;     // duration of one fasten actuation in ms
uint64_t lastWeldAct = 0;         // counter for fasten actuation
//WELDING
uint16_t weldIntervalMin = 500;     // duration of one fasten actuation in ms
uint16_t weldIntervalMax = 5000;     // duration of one fasten actuation in ms
uint16_t weldIntervalInc = 0;  // interval increment for setting via poti
uint16_t weldInterval = 0;  // calculcated weld interval
//WELDING
uint64_t lastBtnRead = 0; // counter for the button readings in ms
uint64_t lastReedRead = 0; // counter for the reed contact readings in ms
uint64_t lastPotiRead = 0; // counter for the potentiometer readings in ms
//TODO: Maybe use different debounce time for poti?
uint32_t debounceInterval = 50; // interval for debouncing inputs in ms  

uint16_t potiValue = 0;

bool reedValue = false;  // internal value of the reed switch
bool btnValue = false;  // internal value of the external button


// ############## I/O Pins for LEDs and Relays
int Pin_POTI = A0;
int Pin_REED = 9;
int Pin_BTN = 8;
int Pin_WELD = 7;  // MOSFET welding
int Pin_FASTEN = 6;  // MOSFET fasten


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
  // init the MOSFETS to off
  digitalWrite(Pin_WELD, MOSFET_OFF); // set all mosfets to off
  digitalWrite(Pin_FASTEN, MOSFET_OFF);
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
      /* check if all inputs are off */
      if (!btnValue and !reedValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    case MODE_STANDBY:
      // wait for trigger
      if (btnValue)
      {
        lastFastenAct = millis();
        OPMODE = MODE_FASTENSTART;
      }
      else if (reedValue)
      {
        lastWeldAct = millis();
        OPMODE = MODE_WELDSTART;
      }
      break;
    case MODE_WELDSTART:
      digitalWrite(Pin_WELD, MOSFET_ON);
      if ((millis() - lastWeldAct) > weldInterval)
      {
        digitalWrite(Pin_WELD, MOSFET_OFF);
        OPMODE = MODE_WELDEND;
      }
      break;
    case MODE_WELDEND:
      if (!reedValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    case MODE_FASTENSTART:
      // enable MOSFET for motor
      digitalWrite(Pin_FASTEN, MOSFET_ON);
      // if we are over the interval, disable MOSFET for motor
      if ((millis() - lastFastenAct) > fastenInterval)
      {
        digitalWrite(Pin_FASTEN, MOSFET_OFF);
        OPMODE = MODE_FASTENEND;
      }
      break;
    case MODE_FASTENEND:
      // wait untill button is released, return to standby
      if (!btnValue)
      {
        OPMODE = MODE_STANDBY;
      }
      break;
    default:
      break;
    }
  }
void SensorRead()
{
  // debounce everything
  // check if the read value is different from last btn state
  if (digitalRead(Pin_BTN) != btnValue)  
  {
       lastBtnRead = millis();
  }
  // check if the read value is different from last reed state
  if (digitalRead(Pin_REED) != reedValue)  
  {
       lastReedRead = millis();
  }
  // check if the read value is different from last poti state
  if (analogRead(Pin_POTI) != potiValue)  
  {
       lastPotiRead = millis();
  }
  //check if we crossed the debounce interval for btn
  if ((millis() - lastBtnRead) > debounceInterval)
  {
    // if btn still has a different value than last measurement
    if (digitalRead(Pin_BTN) != btnValue)  
    {
        btnValue = digitalRead(Pin_BTN);  // assign read value to internal value
    }
  }
  //check if we crossed the debounce interval for reed
  if ((millis() - lastReedRead) > debounceInterval)
  {
    // if btn still has a different value than last measurement
    if (digitalRead(Pin_REED) != reedValue)  
    {
        reedValue = digitalRead(Pin_REED);  // assign read value to internal value
    }
  }
  //check if we crossed the debounce interval for poti
  if ((millis() - lastPotiRead) > debounceInterval)
  {
    // if btn still has a different value than last measurement
    if (digitalRead(Pin_POTI) != potiValue)  
    {
        potiValue = analogRead(Pin_POTI);  // assign read value to internal value
    }
  }
  
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
}