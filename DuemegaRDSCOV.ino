/*
  RogRDS ENCODER ARDUINO DUE 
*/

#include "stdio.h"
#include "RDS.h"
#include <SPI.h>
#include <Ethernet.h>
//#include <Wire.h>
//#include "mioRX8025.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
//#include "pwm_lib.h"
#include "commands.h"


//#define OLED_RESET 16
//Adafruit_SSD1306 display(OLED_RESET);
//mioRX8025 rtc;

//#define XPOS 0
//#define YPOS 1
//#define DELTAY 2

//using namespace arduino_due::pwm_lib;
////
//#define PWM_PERIOD_PIN_35 5263 //=19KHZ // hundredth of usecs (1e-8 secs)
//#define PWM_DUTY_PIN_35 2631 // 10 usecs in hundredth of usecs (1e-8 secs)

// defining pwm object using pin 35, pin PC3 mapped to pin 35 on the DUE
// this object uses PWM channel 0
//pwm<pwm_pin::PWMH0_PC3> pwm_pin35;

#define SINC_19K_IN A1
#define SINC_19K_STATE_OUT 15

#define TA_select A7
#define TP_select A8
#define M_S_select A9

//char tomb[65];
//int  serInIndx  = 0;
char COMMAND[10], VALUE[256];

//unsigned char hour=0;
//unsigned char minute=0;
//unsigned char second=0;
//unsigned char week=0;
//unsigned char year=0;
//unsigned char month=0;
//unsigned char date=0;

/*
  =======================================================================================================================================
*/
void setup() {

  // rtc.RX8025_time[0]=(unsigned char) 0x00; //second
  // rtc.RX8025_time[1]=(unsigned char) 0x32; //minute
  // rtc.RX8025_time[2]=(unsigned char) 0x09; //hour
  // rtc.RX8025_time[3]=(unsigned char) 0x06; //day of week
  // rtc.RX8025_time[4]=(unsigned char) 0x11; //date
  // rtc.RX8025_time[5]=(unsigned char) 0x05; //month
  // rtc.RX8025_time[6]=(unsigned char) 0x12; //year BCD format

  // rtc.RX8025_init();
  // rtc.setRtcTime(); //solo per settare la prima volta
  //
  //  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  //
  //  display.display();
  //  delay(3000);
  //
  //  display.setTextSize(1);  // small font size
  //  display.setTextColor(WHITE);
  //  display.clearDisplay();

  pinMode(SINC_19K_IN, INPUT);      // sets the digital pin 7 as input
  pinMode(SINC_19K_STATE_OUT, OUTPUT);

  pinMode(TA_select, INPUT_PULLUP);
  pinMode(TP_select, INPUT_PULLUP);
  pinMode(M_S_select, INPUT_PULLUP);

  // starting 19 khz PWM signals
  //  pwm_pin35.start(PWM_PERIOD_PIN_35,PWM_DUTY_PIN_35);

  Serial.begin(250000);
  //Serial.begin(19200);

  Serial.println("RDS START:");

  RDS.begin(100);
  RDS.PS_mode(false);

  // rds.ct(2018,10,27,15,30,900);
  //  rtc.getRtcTime();

  // display time in digital format
  //  display.setCursor(11,2);
  //  display.print(rtc.hour, DEC);
  //  printDigits(rtc.minute);
  //  printDigits(rtc.second);
  //  display.display();

}

/*
  =======================================================================================================================================
  ---LOOP-------*/
void loop() {

  //  rtc.getRtcTime();

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
  // close the connection:
  //Serial.end();

  // Serial.print(rtc.hour,DEC);
  // Serial.print(":");
  // Serial.print(rtc.minute,DEC);
  // Serial.print(":");
  // Serial.println(rtc.second,DEC);

  //   // display time in digital format
  //  display.clearDisplay();
  //  display.setCursor(11,2);
  //  display.print(rtc.hour, DEC);
  //  printDigits(rtc.minute);
  //  printDigits(rtc.second);
  //  display.display();

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

//void testdrawchar(void) {
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(0, 0);
//
//  for (uint8_t i = 0; i < 168; i++) {
//    if (i == '\n') continue;
//    display.write(i);
//    if ((i > 0) && (i % 21 == 0))
//      display.println();
//  }
//  display.display();
//  delay(1);
//}
//
//void testdrawcircle(void) {
//  for (int16_t i = 0; i < display.height(); i += 2) {
//    display.drawCircle(display.width() / 2, display.height() / 2, i, WHITE);
//    display.display();
//    delay(1);
//  }
//}
//
//void testfillrect(void) {
//  uint8_t color = 1;
//  for (int16_t i = 0; i < display.height() / 2; i += 3) {
//    // alternate colors
//    display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
//    display.display();
//    delay(1);
//    color++;
//  }
//}
//
//void testdrawtriangle(void) {
//  for (int16_t i = 0; i < min(display.width(), display.height()) / 2; i += 5) {
//    display.drawTriangle(display.width() / 2, display.height() / 2 - i,
//                         display.width() / 2 - i, display.height() / 2 + i,
//                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
//    display.display();
//    delay(1);
//  }
//}
//
//void testfilltriangle(void) {
//  uint8_t color = WHITE;
//  for (int16_t i = min(display.width(), display.height()) / 2; i > 0; i -= 5) {
//    display.fillTriangle(display.width() / 2, display.height() / 2 - i,
//                         display.width() / 2 - i, display.height() / 2 + i,
//                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
//    if (color == WHITE) color = BLACK;
//    else color = WHITE;
//    display.display();
//    delay(1);
//  }
//}
//
//void testdrawroundrect(void) {
//  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
//    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, WHITE);
//    display.display();
//    delay(1);
//  }
//}
//
//void testfillroundrect(void) {
//  uint8_t color = WHITE;
//  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
//    display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
//    if (color == WHITE) color = BLACK;
//    else color = WHITE;
//    display.display();
//    delay(1);
//  }
//}
//
//void testdrawrect(void) {
//  for (int16_t i = 0; i < display.height() / 2; i += 2) {
//    display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, WHITE);
//    display.display();
//    delay(1);
//  }
//}
//
//void testdrawline() {
//  for (int16_t i = 0; i < display.width(); i += 4) {
//    display.drawLine(0, 0, i, display.height() - 1, WHITE);
//    display.display();
//    delay(1);
//  }
//  for (int16_t i = 0; i < display.height(); i += 4) {
//    display.drawLine(0, 0, display.width() - 1, i, WHITE);
//    display.display();
//    delay(1);
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i = 0; i < display.width(); i += 4) {
//    display.drawLine(0, display.height() - 1, i, 0, WHITE);
//    display.display();
//    delay(1);
//  }
//  for (int16_t i = display.height() - 1; i >= 0; i -= 4) {
//    display.drawLine(0, display.height() - 1, display.width() - 1, i, WHITE);
//    display.display();
//    delay(1);
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i = display.width() - 1; i >= 0; i -= 4) {
//    display.drawLine(display.width() - 1, display.height() - 1, i, 0, WHITE);
//    display.display();
//    delay(1);
//  }
//  for (int16_t i = display.height() - 1; i >= 0; i -= 4) {
//    display.drawLine(display.width() - 1, display.height() - 1, 0, i, WHITE);
//    display.display();
//    delay(1);
//  }
//  delay(250);
//
//  display.clearDisplay();
//  for (int16_t i = 0; i < display.height(); i += 4) {
//    display.drawLine(display.width() - 1, 0, 0, i, WHITE);
//    display.display();
//    delay(1);
//  }
//  for (int16_t i = 0; i < display.width(); i += 4) {
//    display.drawLine(display.width() - 1, 0, i, display.height() - 1, WHITE);
//    display.display();
//    delay(1);
//  }
//  delay(250);
//}
//
//void testscrolltext(void) {
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.setCursor(10, 0);
//  display.clearDisplay();
//  display.println("scroll");
//  display.display();
//  delay(1);
//
//  display.startscrollright(0x00, 0x0F);
//  delay(2000);
//  display.stopscroll();
//  delay(1000);
//  display.startscrollleft(0x00, 0x0F);
//  delay(2000);
//  display.stopscroll();
//  delay(1000);
//  display.startscrolldiagright(0x00, 0x07);
//  delay(2000);
//  display.startscrolldiagleft(0x00, 0x07);
//  delay(2000);
//  display.stopscroll();
//}
//
//void printDigits(int digits) {
//  // utility function for digital clock display: prints preceding colon and leading 0
//  display.print(":");
//  if (digits < 10)
//    display.print('0');
//  display.print(digits);
//}
