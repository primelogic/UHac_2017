
#include <SPI.h>
//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Button.h>
#include <EEPROM.h>
#include "nfc.h"

NFC_Module nfc;

//#define DEBUG

#ifdef DEBUG
  #define p(x) Serial3.print(x)
  #define b(x) Serial3.begin(x)
  #define pl(x) Serial3.println(x)
  #define a() Serial3.available()
  #define r() Serial3.read()
#else
  #define p(x) Serial.print(x)
  #define b(x) Serial.begin(x)
  #define pl(x) Serial.println(x)
  #define a() Serial.available()
  #define r() Serial.read()
#endif

#define ADDRESS 10
#define SECRET 143
#define TRIAL 50
#define MODE 100
#define BALANCE 5
#define TO_TRANSFER 51
/* MODE:
 * 0 - New
 * 1 - Normal
 * 2 - emergency
 */

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

unsigned long amountToTransfer = 0;
uint8_t STATE;

uint8_t ACCOUNTING_STATE;

enum {NORMAL_OPER,SELECT_AMOUNT, EMERGENCY_MODE, ACCOUNT_ONES, ACCOUNT_TENS, TRANSACTION};

Button plusBtn(2, true, true, 20);
Button minusBtn(7, true,true ,20);
Button oneBtn(4, true,true, 20);
Button twoBtn(9, true,true, 20);
Button threeBtn(6, true, true, 20);
Button fourBtn(12, true, true ,20);

volatile boolean isDisconnected = false;

uint8_t buttonSensor = 0;

uint8_t STATE_MEMORY;
u8 TX_BUFFER[20];
u8 TX_LEN;
u8 RX_BUFFER[20];
u8 RX_LEN;

unsigned long amountToReceive; //NEW!

void setup() {
    b(38400);
    switch(EEPROM.read(MODE)) {
    case 0: 
      //GO TO DIEGO's WORK
      nfc.begin();
      break;
    case 2:
      STATE = EMERGENCY_MODE;
      break;
  }
  ACCOUNTING_STATE = ACCOUNT_ONES;
  nfc.SAMConfiguration();
}

void loop() {
  switch(STATE){
    case NORMAL_OPER:
      if(nfc.P2PInitiatorInit()){
        //target was sensed.
        //Display Receiving.
        for ( int i = 0; i < 20; i++ ) {
          TX_BUFFER[i] = 0;
        }
        if (nfc.P2PTargetTxRx(TX_BUFFER, TX_LEN, RX_BUFFER, &RX_LEN)){
          //convert array to long.
          uint32_t currentAmount;
          EEPROM.get(BALANCE, currentAmount);
          amountToReceive = atol(RX_BUFFER);
          EEPROM.put(BALANCE,  currentAmount + amountToReceive );
          //Display successful transfer
        }
      }
      
      break;
    case TRANSACTION:
      //On UI Display transmit,
      if (!nfc.P2PInitiatorInit()) {
        if ( STATE_MEMORY == SELECT_AMOUNT ) {STATE = SELECT_AMOUNT; STATE_MEMORY = TRANSACTION;}
        if ( STATE_MEMORY == NORMAL_OPER ) {STATE = NORMAL_OPER; STATE_MEMORY = TRANSACTION;}
      } else {
        TX_LEN = strlen((const char *)amountToTransfer);
        strcpy(TX_BUFFER, ((String)amountToTransfer).c_str());
    
        if(nfc.P2PInitiatorTxRx(TX_BUFFER, TX_LEN, RX_BUFFER, &RX_LEN)){
          uint32_t currentAmount;
          EEPROM.get(BALANCE, currentAmount);
          EEPROM.put(BALANCE, currentAmount - amountToTransfer);
          //display transfer successful
        }
      }
      
      break;
      
  }
}


