#include <Arduino.h>

String MegaStatus = "Hello World";

enum OpModes
{
  MODE_INIT, // 0
  MODE_STANDBY,
  MODE_WELDSTART,
  MODE_WELDEND,
  MODE_FASTENSTART,
  MODE_FASTENEND,
  MODE_ERR // 6
};
int OPMODE = 0;  // Switch variable

enum MosfetModes // enum for MOSFET control
{
  MOSFET_OFF, // 0
  MOSFET_ON
};

// ############## Config for periodic functions
uint64_t lastStatus = 0;          // counter for status message
uint32_t statusInterval = 300000; // interval for the status messages in ms
uint64_t lastFastenAct = 0;         // counter for fasten actuation
uint32_t FastenInterval = 3000;     // duration of one fasten actuation in ms

// ############## I/O Pins for LEDs and Relays
int Pin_POTI = 0;
int Pin_REED = 0;
int Pin_BTN = 0;
int Pin_WELD = 0;  // MOSFET welding
int Pin_FASTEN = 0;  // MOSFET fasten


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
  pinMode(Pin_REED, INPUT);
  pinMode(Pin_BTN, INPUT);
  pinMode(Pin_WELD, OUTPUT); // set the outputs
  pinMode(Pin_FASTEN, OUTPUT);
  // init the MOSFETS to off
  digitalWrite(Pin_WELD, MOSFET_OFF); // set all mosfets to off
  digitalWrite(Pin_FASTEN, MOSFET_OFF);
}

void setup() {
   Serial.begin(115200);  // set serial to 115200
   setupIO();             // setup the IO pins
   statusPrinter(0);
   MegaStatus = "INIT";
   statusPrinter(0);
}

void StateMachine()
  {
    switch (OPMODE)
    {
    case MODE_INIT:
      /* check if all inputs are off and check the Poti for its value */
      break;
    
    default:
      break;
    }
  }

void loop() {
  statusPrinter(0);
  StateMachine();
}