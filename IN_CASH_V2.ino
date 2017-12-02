
/*
 * android instructions:
 * 1.) after connecting, send any text.
 * 
 * Diego Updates:
 * 
 */

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
#define SECRET 143 //CONTAINS THE PASSKEY
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

long amountToTransfer = 0;
uint8_t STATE;

uint8_t ACCOUNTING_STATE;

enum {NORMAL_OPER,SELECT_AMOUNT, EMERGENCY_MODE, ACCOUNT_ONES, ACCOUNT_TENS, TRANSACTION, TRANSACTION_FINISHED, NEW_DEVICE, CHANGE_PASSWORD};

Button plusBtn(2, true, true, 20);
Button minusBtn(7, true,true ,20);
Button oneBtn(4, true,true, 20);
Button twoBtn(9, true,true, 20);
Button threeBtn(6, true, true, 20);
Button fourBtn(12, true, true ,20);

volatile boolean isDisconnected = false;

uint8_t buttonSensor = 0;

uint8_t STATE_MEMORY = 100;
u8 TX_BUFFER[10];
u8 TX_LEN;
u8 RX_BUFFER[10];
u8 RX_LEN;

unsigned long amountToReceive;

void setup() {
  //LOAD ALL EEPROM READ AT UPFRONT.
  b(38400);
  //check mode!
  
  switch(EEPROM.read(MODE)) {
    case 0: 
      //GO TO DIEGO's WORK
      STATE = NEW_DEVICE;
      nfc.begin();
      
      break;
    case 1:
      nfc.begin();
      STATE = NORMAL_OPER;
      break;
    case 2:
      STATE = EMERGENCY_MODE;
      break;
  }
  //Read EEPROM Here.
  ACCOUNTING_STATE = ACCOUNT_ONES;
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  //EEPROM.write(ADDRESS,0);
  nfc.SAMConfiguration();
}


void loop() {
switch(STATE){
  case NORMAL_OPER:
    oneBtn.read();
    twoBtn.read();
    threeBtn.read();
    fourBtn.read();
    plusBtn.read();
    minusBtn.read();
 
    //if (oneBtn.wasPressed()) EEPROM.write(ADDRESS, EEPROM.read(ADDRESS) | 2); //switch to sram
     if (oneBtn.wasPressed()) buttonSensor |= 2;
    if (twoBtn.wasPressed()) buttonSensor |= 4; //switch to sram
    if (threeBtn.wasPressed()) buttonSensor |= 8; //switch to sram
    if (fourBtn.wasPressed()) buttonSensor |= 16; //switch to sram
    if (plusBtn.wasPressed()) buttonSensor |= 32;
    if (minusBtn.wasPressed()) buttonSensor |= 64;

    if (oneBtn.wasReleased() || twoBtn.wasReleased() || threeBtn.wasReleased() || fourBtn.wasReleased() ) {
        //read the value of ADDRESS EEPROM
          if ( buttonSensor == 126 ) {
            STATE = CHANGE_PASSWORD; 
            STATE_MEMORY = NORMAL_OPER;
            goto proceed1;
            }
   
          if ( buttonSensor == EEPROM.read(SECRET)) {
            //if confirmed, go to transmit mode. !CHANGE MODE!
            STATE = SELECT_AMOUNT;
            STATE_MEMORY = NORMAL_OPER;
          } else {
              byte trial = EEPROM.read(TRIAL);
              EEPROM.write(TRIAL, trial + 1);
              //SET INTO EMERGENCY MODE{require phone} 
              if ( trial > 3 ) {
                EEPROM.write(MODE, 2); //EMERGENCY MODE!
                STATE = EMERGENCY_MODE; // MAKETHIS!!!
                STATE_MEMORY = NORMAL_OPER;
              }
              //Do Idle only. disable the device @ emergency mode.
        }
        proceed1:
        buttonSensor = 0;
     }
     if(nfc.P2PTargetInit()){
        
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
          EEPROM.get(BALANCE, currentAmount);
          STATE_MEMORY = NORMAL_OPER;
          STATE = TRANSACTION_FINISHED;
        }
      }
      
     break;

   case NEW_DEVICE:
    
    break;

   case CHANGE_PASSWORD:
   
    break;
    
   case SELECT_AMOUNT:
      plusBtn.read();
      minusBtn.read();
      //make NFC reading passive. (TRANSMITTER) HERE!
      if(nfc.P2PInitiatorInit()) {STATE = TRANSACTION; STATE_MEMORY = SELECT_AMOUNT;}
      switch(ACCOUNTING_STATE) {
         case ACCOUNT_ONES:
           
           if (plusBtn.wasReleased()) amountToTransfer++;//increment amount to transfer.
          
           if (minusBtn.wasReleased()) amountToTransfer--;//increment amount to transfer.
           
           if (plusBtn.pressedFor(600)) ACCOUNTING_STATE = ACCOUNT_TENS;
           if (minusBtn.pressedFor(600)) ACCOUNTING_STATE = ACCOUNT_TENS;
            
            break;
         case ACCOUNT_TENS:
          if (plusBtn.isPressed() && !minusBtn.isPressed()) amountToTransfer += 10;
          if (minusBtn.isPressed() && !plusBtn.isPressed()) amountToTransfer -= 10;
          if (plusBtn.wasReleased() || minusBtn.wasReleased()) ACCOUNTING_STATE = ACCOUNT_ONES;
          break;
      }
      break;
    case TRANSACTION:
      //On UI Display transmit,
      if (!nfc.P2PInitiatorInit()) {
        if ( STATE_MEMORY == SELECT_AMOUNT ) {STATE = SELECT_AMOUNT; STATE_MEMORY = TRANSACTION;}
        if ( STATE_MEMORY == NORMAL_OPER ) {STATE = NORMAL_OPER; STATE_MEMORY = TRANSACTION;}
      } else {
        TX_LEN = strlen(((String)amountToTransfer).c_str());
        strcpy(TX_BUFFER, ((String)amountToTransfer).c_str());
    
        if(nfc.P2PInitiatorTxRx(TX_BUFFER, TX_LEN, RX_BUFFER, &RX_LEN)){
          uint32_t currentAmount;
          EEPROM.get(BALANCE, currentAmount);
          EEPROM.put(BALANCE, currentAmount - amountToTransfer);
          //display transfer successful
          EEPROM.get(BALANCE, currentAmount);
          STATE_MEMORY = TRANSACTION;
          STATE = TRANSACTION_FINISHED;
          //pl("SUCCESS! You have Transfered:" + (String)amountToTransfer);
          //pl("Your Account Balance:" + (String)currentAmount);
         // pl("");
        }
      }
      break;

    case TRANSACTION_FINISHED:
       uint32_t currentAmount;
       EEPROM.get(BALANCE, currentAmount);
       pl("CONGRATULATIONS! " + (String)currentAmount);
       if ( STATE_MEMORY == TRANSACTION ) ;//display_transceiver(currentAmount,tx);
       else if (STATE_MEMORY == NORMAL_OPER) ; //display_tranceiver(currentAmount,rx);
       delay(2000);
       STATE = NORMAL_OPER;
       break;
      
  }
 
 
}



bool display_amount(long& pseudo_amount){
  //TEXT:
  long* amount = &pseudo_amount;
  //*&*amount = 1234; (used to redefine global variable)
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(F("Amount"));
 
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(3,10);
  display.print(F("P"));
  
  if(*amount < 10) {
    display.setCursor(83,10);
    display.print(String(*amount));
  } else if (*amount < 100 && *amount >= 10) {
    display.setCursor(70,10);
    display.print(String(*amount));
  } else if (*amount < 1000 && *amount >= 100) {
    display.setCursor(58,10);
    //display.setCursor(100,10);
    display.print(String(*amount));
  }
  else if (*amount < 10000 && *amount >= 1000) {
    display.setCursor(46,10);
    display.print(String(*amount));
  }
  else if (*amount < 100000 && *amount >= 10000) {
    display.setCursor(34,10);
    display.print(String(*amount));
  }
    else if (*amount < 1000000 && *amount >= 100000) {
    display.setCursor(22,10);
    display.print(String(*amount));
  } else {
    display.setCursor(22,10);
    display.print(F("999999"));
    *&*amount = 999999;
  }

  display.setCursor(display.width() - 36, 10);
  display.print(F(".00"));
  //BORDER:
  //display.drawRect(0, 0, display.width(), display.height(), WHITE);
  display.fillRect(1, 13, 14, 1, 1);
  display.fillRect(1, 15, 14, 1, 1);

  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(0,25);
  display.print(F("Hold buttons to Go  "));
  display.startscrollright(11, 12);
  
  display.display();
  display.clearDisplay();
  return(true);
}

void display_connecting() {

   display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(4,3);
      display.print(F("Connecting"));
  display.display();
  unsigned long timer;
  while(dumpSerial()) {  //1.686s loop

  display.fillRect(33, 25, 10, 3, 1);
      display.display();
      delay(500);
     display.fillRect(33, 25, 10, 3, 0);
     display.display();
     delay(20);
     display.fillRect(58, 25, 10, 3, 1);
     display.display();
      delay(500);
      display.fillRect(58, 25, 10, 3, 0);
      display.display();
      delay(20);
      display.fillRect(83, 25, 10, 3, 1);
     display.display();
     delay(500);
     display.fillRect(83, 25, 10, 3, 0);
     display.display();
     delay(20);
     
  }
  
  
  display.clearDisplay();
  
}


void display_locked() {

   display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(4,3);
      display.print(F("LOCKED"));
  display.display();
  
  display.clearDisplay();
  
}

void display_unlocked() {

   display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(4,3);
      display.print(F("UNLOCKED"));
  display.display();
  
  display.clearDisplay();
  delay(1500);
}

boolean dumpSerial() {
  if ( a()  > 0) {
    while ( a() > 0 ) {
      r();
    }
    return(false);
  }
  return(true);
}

boolean isLocked() {
  String readSerial = "";
  while ( a() > 0 ) {
      readSerial += (char)r();

  }
   readSerial.trim();
      if( readSerial == "UNLOCK") {
      //  p("PASOK!");
        return(false);
      }
      
      delay(10);
  return(true);
}

