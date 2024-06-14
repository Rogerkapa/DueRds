/*
  RDS ENCODER ARDUINO DUE modded from Jonas work
  CT Group_4A added
  Oled lcd added 2024
  
*/

#include "stdio.h"
#include "genR.h"
#include "commandsR.h"
#include "pwm_lib.h"            // Pwm Lib for 19khz internal https://github.com/antodom/pwm_lib//
#include "RTClib.h"             // Date and time lib functions using a PCF8563 RTC connected via I2C and Wire lib
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#define OLED_RESET -1 
Adafruit_SH1106 display(OLED_RESET);
#define XPOS 0
#define YPOS 1

#define SINC_19K_IN A1          //Capture external 19Khz pilot to sync the 57khz RDS signal
#define SINC_19K_STATE_OUT 15   //State of sync Led

#define TA_select A7
#define TP_select A8
#define M_S_select A9

 using namespace arduino_due::pwm_lib;
//
 #define PWM_PERIOD_PIN_35  5264 //=19KHZ // hundredth of usecs (1e-8 secs)
 #define PWM_DUTY_PIN_35    2632 // 10 usecs in hundredth of usecs (1e-8 secs)

// Defining pwm object using pin 35, pin PC3 mapped to pin 35 on the DUE
// this object uses PWM channel 0
 pwm<pwm_pin::PWMH0_PC3> pwm_pin35;

RTC_PCF8563 rtc;

char COMMAND[10], VALUE[256];

void setup() {

// Starting 19khz PWM signals output
  pwm_pin35.start(PWM_PERIOD_PIN_35,PWM_DUTY_PIN_35);
    
  pinMode(SINC_19K_IN, INPUT);      // sets the digital pin A1 as input
  pinMode(SINC_19K_STATE_OUT, OUTPUT);

  pinMode(TA_select, INPUT_PULLUP);
  pinMode(TP_select, INPUT_PULLUP);
  pinMode(M_S_select, INPUT_PULLUP);

  //Serial.begin(115200);
  //Serial.begin(250000);
  Serial.begin(19200); //For Magic RDS prog
  
  Serial.println("RDS START:");
  delay(2000);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C); 
  display.display();
  delay(1000);
  
  rtc.begin();
  RDS.begin(100);
  RDS.PS_mode(false);

}

void loop() {
  
  digitalWrite(SINC_19K_STATE_OUT, RDS.PILOT_sync_state());

  RDS.TA = !digitalRead(TA_select);
  RDS.TP = !digitalRead(TP_select);
  RDS.M_S = !digitalRead(M_S_select);

 // listen for incoming commands
if (Serial.available()) {
  bool Read_Write, Store, Error = false;
  int i = 0, ii = 0;
  char COMMAND[10];
  char VALUE[300];

  Read_Write = Store = false;
  COMMAND[0] = '\0';

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') {
      break;
    }
    COMMAND[i++] = c;
    if (i >= 9) {
      Error = true;
      break;
    }
  }
  COMMAND[i] = '\0';

  if (!Error && COMMAND[0] == '\r') {
    Error = false;
    Serial.print("\r\n!\r\n\r\n");
  } else {
    // STORE ? ---------------------------------------------
    if (!Error && COMMAND[0] == '*') {
      Store = true;
    }

    // READ OR WRITE ?--------------------------------------
    if (!Error && strchr(COMMAND, '=')) {
      Read_Write = true;

      // VALUE READ !-----------------------------------------
      while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') {
          break;
        }
        if (ii >= 299) {
          Error = true;
          break;
        }
        VALUE[ii++] = c;
      }
      VALUE[ii] = '\0';
    }

    if (Error) {
      Serial.flush();
    } else {
      Serial.print(CommandProcess(Read_Write, Store));
     }
   }
  delay(1);
 }
}

//--------------------------------------------

char* CommandProcess(bool R_W, bool St) {
  int com_search = 0, STATE = '+';
  static char RESPONSE[255];


//  Serial.print("command: ");
//  Serial.println(COMMAND);

  com_search = 0;
  while (strcmp(COMMAND, Commands[com_search])) {
    if (com_search > 96) break;
    com_search++;
  }

  switch (com_search) {

    case PI_comm:
      STATE = '+';

      break;

    case PS_comm:
      STATE = '+';
      RDS.PS_Set(VALUE);
      break;

    case DI_comm:
      STATE = '+';

      break;

    case LEVEL_comm:
      STATE = '+';

      break;

    case PHASE_comm:
      RDS.SineTable_UP(atoi(VALUE), 255);

      break;

    case UECP_comm:
      STATE = '+';

      break;

    default:
      STATE = '!';
      break;
  }

  if (R_W) {
    int res_i = 0, comm_i = 0, val_i = 0;
    if (St) RESPONSE[res_i++] = '*';
    while (COMMAND[comm_i] != '\0') RESPONSE[res_i++] = COMMAND[comm_i++];
    RESPONSE[res_i++] = '=';
    while (VALUE[val_i] != '\0') RESPONSE[res_i++] = VALUE[val_i++];
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i++] = STATE;
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i] = '\0';
  }
  else {
    int res_i = 0, comm_i = 0, val_i = 0;
    while (COMMAND[comm_i] != '\0') RESPONSE[res_i++] = COMMAND[comm_i++];
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    while (VALUE[val_i] != '\0') RESPONSE[res_i++] = VALUE[val_i++];
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i++] = STATE;
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i++] = '\r';
    RESPONSE[res_i++] = '\n';
    RESPONSE[res_i] = '\0';
  }

  return RESPONSE;
}
