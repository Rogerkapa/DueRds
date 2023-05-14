/*
  RDS ENCODER ARDUINO DUE modded from Jonas work
*/

#include "stdio.h"
#include "RDS.h"

#include "commands.h"


#define SINC_19K_IN A1
#define SINC_19K_STATE_OUT 15

#define TA_select A7
#define TP_select A8
#define M_S_select A9

char COMMAND[10], VALUE[256];


/*
  =======================================================================================================================================
*/
void setup() {

  pinMode(SINC_19K_IN, INPUT);      // sets the digital pin 7 as input
  pinMode(SINC_19K_STATE_OUT, OUTPUT);

  pinMode(TA_select, INPUT_PULLUP);
  pinMode(TP_select, INPUT_PULLUP);
  pinMode(M_S_select, INPUT_PULLUP);

  Serial.begin(250000);
  Serial.println("RDS START:");

  RDS.begin(100);
  RDS.PS_mode(false);

}

/*
  =======================================================================================================================================
  ---LOOP-------*/
void loop() {

  digitalWrite(SINC_19K_STATE_OUT, RDS.PILOT_sync_state());

  RDS.TA = !digitalRead(TA_select);
  RDS.TP = !digitalRead(TP_select);
  RDS.M_S = !digitalRead(M_S_select);

  while (Serial.available()) {
    bool Read_Write, Store, Error = false;
    int i = 0, ii = 0;

    Read_Write = Store = false;
    COMMAND[0] = '\0';

    COMMAND[0] = Serial.read();

    if (!Serial.available() && COMMAND[0] == '\r') {
      Error = false;

      Serial.print("\r\n!\r\n");
      Serial.println("error");
    }
    else {
      // STORE ? ---------------------------------------------

      if (!Serial.available()) Error = true;
      if (!Error && (COMMAND[0] == '*')) {
        Store = true;

        COMMAND[0] = Serial.read();
        Serial.println("command0:");
      }

      // COMMAND READ !---------------------------------------
      i = 0;
      if (!Serial.available()) Error = true;
      while (!Error && (COMMAND[i] != '=') && (COMMAND[i] != '\r')) {
        COMMAND[++i] = Serial.read();
        if (i > 8) {
          Error = true;
          break;
        }
      }

      // READ OR WRITE ?--------------------------------------
      if (!Error && (COMMAND[i] == '=')) {
        Read_Write = true;
        Serial.println("command=");

        // VALUE READ !-----------------------------------------
        ii = 0;
        if (!Serial.available()) Error = true;
        do {
          if (ii > 299) {
            Error = true;
            break;
          }
          VALUE[ii] = Serial.read();
        } while (!Error && (VALUE[ii++] != '\r'));
        ii--;
      }
      COMMAND[i] = '\0';
      VALUE[ii] = '\0';

      if (Error) Serial.flush();
      else  Serial.print(CommandProcess(Read_Write, Store));

    }
  }
  delay(1);
  
}

//--------------------------------------------


char* CommandProcess(bool R_W, bool St) {
  int com_search = 0, STATE = '+';
  static char RESPONSE[255];


  Serial.print("command1: ");
  Serial.println(COMMAND);

  com_search = 0;
  while (strcmp(COMMAND, Commands[com_search])) {
    if (com_search > 96) break;
    com_search++;
  }

  Serial.print("value: ");
  Serial.println(VALUE);

  Serial.print("R_W: ");
  Serial.println(R_W);

  Serial.print("Store: ");
  Serial.println(St);
  Serial.println("");

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
