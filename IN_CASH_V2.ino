
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

byte passwordCache; //new. temporary storage for initial password
byte changePasswordCache; //new. temporary storage for changing password
byte newPasswordCache;
byte confirmPasswordCache; //new. temporary storage for confirming password
bool passwordNotSet; //new.
bool timerFlag = 0;
unsigned long previousMillis;
unsigned long delTime = 10000;

uint8_t STATE;

uint8_t ACCOUNTING_STATE;

enum {NORMAL_OPER,SELECT_AMOUNT, EMERGENCY_MODE, ACCOUNT_ONES, ACCOUNT_TENS, TRANSACTION, TRANSACTION_FINISHED, NEW_DEVICE, CHANGE_PASSWORD, CONFIRM_PASSWORD, NEW_PASSWORD};

Button plusBtn(2, true, true, 40);
Button minusBtn(3, true,true ,40);
Button oneBtn(4, true,true, 40);
Button twoBtn(5, true,true, 40);
Button threeBtn(6, true, true, 40);
Button fourBtn(7, true, true ,40);

volatile boolean isDisconnected = false;

uint8_t buttonSensor = 0;

uint8_t STATE_MEMORY = 100;
u8 TX_BUFFER[10];
u8 TX_LEN;
u8 RX_BUFFER[10];
u8 RX_LEN;

long amountToReceive;
long dynamicBalance;
long amountToTransfer;

void setup() {
  //LOAD ALL EEPROM READ AT UPFRONT.
  b(38400);
  //check mode!
  //EEPROM.put(BALANCE,2000);
  EEPROM.write(MODE, 0);
  EEPROM.write(BALANCE, 100);

  EEPROM.get(BALANCE, dynamicBalance);
 // EEPROM.write(MODE, 1);
  //EEPROM.write(SECRET, 0 );
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
        pl("ENTERED: " + buttonSensor);
        //read the value of ADDRESS EEPROM
          if ( buttonSensor == 126 ) {
            STATE = CHANGE_PASSWORD; 
            STATE_MEMORY = NORMAL_OPER;
            goto proceed1;
            }
   
          if ( buttonSensor == EEPROM.read(SECRET)) {
            //if confirmed, go to transmit mode. !CHANGE MODE!
            EEPROM.write(TRIAL, 0);
            STATE = SELECT_AMOUNT;
            STATE_MEMORY = NORMAL_OPER;
          } else {
              byte trial = EEPROM.read(TRIAL);
              EEPROM.write(TRIAL, trial + 1);
              //SET INTO EMERGENCY MODE{require phone} 
              if ( trial >= 2 ) {
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
      displayTagLine();
      
     break;

     case EMERGENCY_MODE:
     pl("I AM AT EMERGENCY MODE");
     delay(1000);
     displayStr(F("EMERGENCY   MODE"), 2, 11, 0);
     break;

   case NEW_DEVICE:
   oneBtn.read();
    twoBtn.read();
    threeBtn.read();
    fourBtn.read();
    //pakita mo na set password: 
    displayStr(F("Set Your Personal         Password"), 1, 12, 10); // displayStr( String input,int txtSize, int posx, int posy)
    if (oneBtn.wasPressed()){bitWrite(passwordCache, 1, 1);}
    if (twoBtn.wasPressed()){bitWrite(passwordCache, 2, 1);}
    if (threeBtn.wasPressed()){bitWrite(passwordCache, 3, 1);}
    if (fourBtn.wasPressed()){bitWrite(passwordCache, 4, 1);} 

    if (oneBtn.wasReleased()|| 
        twoBtn.wasReleased()|| 
        threeBtn.wasReleased()|| 
        fourBtn.wasReleased()){
          EEPROM.write(100, 1);
    EEPROM.write(143, passwordCache);
    pl(passwordCache);
    STATE = NORMAL_OPER;
    display.clearDisplay();
    display.display();
        }
    
    break;

   case CHANGE_PASSWORD:
   //pakita mo "Enter current password"
   oneBtn.read();
    twoBtn.read();
    threeBtn.read();
    fourBtn.read();
   displayStr("Enter Current PW:", 0, 0, 0); // displayStr( String input,int txtSize, int posx, int posy)
    if (oneBtn.wasPressed()){bitWrite(changePasswordCache, 1, 1);}
    if (twoBtn.wasPressed()){bitWrite(changePasswordCache, 2, 1);}
    if (threeBtn.wasPressed()){bitWrite(changePasswordCache, 3, 1);}
    if (fourBtn.wasPressed()){bitWrite(changePasswordCache, 4, 1);}
    
    if (oneBtn.wasReleased()|| 
        twoBtn.wasReleased()|| 
        threeBtn.wasReleased()|| 
        fourBtn.wasReleased()){
          if (changePasswordCache == EEPROM.read(SECRET)) STATE = NEW_PASSWORD;
          else {
            //pakita mo "wrong password, try again"
            changePasswordCache = 0;
            displayStr("TRY AGAIN!", 2, 10, 10);   //displayStr( String input,int txtSize, int posx, int posy)
            delay(500); 
          }
        }
    break;

    case NEW_PASSWORD:
    oneBtn.read();
    twoBtn.read();
    threeBtn.read();
    fourBtn.read();
    displayStr("Enter new password:",0,0,0);
    if (oneBtn.wasPressed()){bitWrite(newPasswordCache, 1, 1);}
    if (twoBtn.wasPressed()){bitWrite(newPasswordCache, 2, 1);}
    if (threeBtn.wasPressed()){bitWrite(newPasswordCache, 3, 1);}
    if (fourBtn.wasPressed()){bitWrite(newPasswordCache, 4, 1);}
    
    if (oneBtn.wasReleased()|| 
        twoBtn.wasReleased()|| 
        threeBtn.wasReleased()|| 
        fourBtn.wasReleased()){
          
          STATE = CONFIRM_PASSWORD;
        }   
    break;


   case CONFIRM_PASSWORD: //new
   oneBtn.read();
    twoBtn.read();
    threeBtn.read();
    fourBtn.read();
   //pakita mo "Confirm your password"
   displayStr("Confirm New Password.",0,0,0);
    if (oneBtn.wasPressed()){bitWrite(confirmPasswordCache, 1, 1);}
    if (twoBtn.wasPressed()){bitWrite(confirmPasswordCache, 2, 1);}
    if (threeBtn.wasPressed()){bitWrite(confirmPasswordCache, 3, 1);}
    if (fourBtn.wasPressed()){bitWrite(confirmPasswordCache, 4, 1);}
    
    if (oneBtn.wasReleased()|| 
        twoBtn.wasReleased()|| 
        threeBtn.wasReleased()|| 
        fourBtn.wasReleased()){
          if (confirmPasswordCache == newPasswordCache){
              //pakita mo "Password Changed!"
              
              EEPROM.write(SECRET, confirmPasswordCache);
              STATE = NORMAL_OPER;
              displayStr("Password Changed!",1,0,0);  
              delay(1000);
          }
          else {
            //pakita mo "Passwords do not match"
            STATE = NORMAL_OPER;     
            displayStr("Passwords do not match!", 1, 0,0);    
            delay(1000);
          }
          display.clearDisplay();
          display.display();
          
        }   
    break;
    
   case SELECT_AMOUNT: //err

      if ( timerFlag == 0 ) {
        previousMillis = millis();
        timerFlag = 1;
      }
      
      display_amount(amountToTransfer);
      plusBtn.read();
      minusBtn.read();
      if ( millis() - previousMillis  > delTime ) {
        timerFlag = 0;
        STATE = NORMAL_OPER;
        STATE_MEMORY = SELECT_AMOUNT;
        amountToTransfer = 0;
      }
      //make NFC reading passive. (TRANSMITTER) HERE!
      if(nfc.P2PInitiatorInit()) {STATE = TRANSACTION; STATE_MEMORY = SELECT_AMOUNT;}
      switch(ACCOUNTING_STATE) {
         case ACCOUNT_ONES:
           
           if (plusBtn.wasReleased()){
            previousMillis = millis();
            amountToTransfer++;//increment amount to transfer.
            if ( amountToTransfer > dynamicBalance ) amountToTransfer = dynamicBalance;
           }
          
           if (minusBtn.wasReleased()) {
            previousMillis = millis();
            amountToTransfer--;//increment amount to transfer.
            if ( amountToTransfer < 0 ) amountToTransfer = 0;
           }
           
           if (plusBtn.pressedFor(600)) ACCOUNTING_STATE = ACCOUNT_TENS;
           if (minusBtn.pressedFor(600)) ACCOUNTING_STATE = ACCOUNT_TENS;
            
            break;
         case ACCOUNT_TENS:
          if (plusBtn.isPressed() && !minusBtn.isPressed()) {
            previousMillis = millis();
            amountToTransfer += 10;
            if ( amountToTransfer > dynamicBalance ) amountToTransfer = dynamicBalance;
          }
          if (minusBtn.isPressed() && !plusBtn.isPressed()) {
            previousMillis = millis();
            amountToTransfer -= 10;
            if ( amountToTransfer < 0 ) amountToTransfer = 0;
          }
          if (plusBtn.wasReleased() || minusBtn.wasReleased()) ACCOUNTING_STATE = ACCOUNT_ONES;
          break;
      }
      break;
    case TRANSACTION:
      //On UI Display transmit,
      if (nfc.P2PInitiatorInit()) {
        TX_LEN = strlen(((String)amountToTransfer).c_str());
        strcpy(TX_BUFFER, ((String)amountToTransfer).c_str());
    
        if(nfc.P2PInitiatorTxRx(TX_BUFFER, TX_LEN, RX_BUFFER, &RX_LEN)){
          displayStr("Loading...",2,0,0);
          uint32_t currentAmount;
          EEPROM.get(BALANCE, currentAmount);
          EEPROM.put(BALANCE, currentAmount - amountToTransfer);
          //display transfer successful
          EEPROM.get(BALANCE, currentAmount);
          STATE_MEMORY = TRANSACTION;
          STATE = TRANSACTION_FINISHED;
    
        }
      } 
      break;

    case TRANSACTION_FINISHED:
       EEPROM.get(BALANCE, dynamicBalance);
     if ( dynamicBalance <= amountToTransfer ) amountToTransfer = dynamicBalance;
     pl("CONGRATULATIONS! " + (String)dynamicBalance);
     if ( STATE_MEMORY == TRANSACTION ) {STATE = SELECT_AMOUNT; STATE_MEMORY = TRANSACTION_FINISHED; displayStr("SUCCESS!", 2, 20, 13);}
     else if (STATE_MEMORY == NORMAL_OPER) {STATE = NORMAL_OPER; STATE_MEMORY = TRANSACTION_FINISHED; displayStr("SUCCESS!", 2, 20, 13);}
     //STATIC LOADING DISPLAY (2sec)
     delay(2000);
     previousMillis = millis();
      
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
  display.print(F(" Tap to Transfer "));
  //display.startscrollright(11, 12);
  
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


void displayTagLine() {

   display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(24,0);
      display.print(F("InCash"));
      display.setTextSize(0);
      display.setTextColor(WHITE);
      display.setCursor(4,16);
      display.print(F("The Future of Bankin"));
   
  display.display();
  display.display();
  
  display.clearDisplay();
  
}

void displayStr( String input,int txtSize, int posx, int posy) {

   display.setTextSize(txtSize);
      display.setTextColor(WHITE);
      display.setCursor(posx,posy);
      display.print(input);
  display.display();
  
  display.clearDisplay();
  
}
/*
void display_success(bool transmit) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
  if (transmit) {
      display.printg("You have successfully transferred: PHP " + amountToTransfer);
      
  } else {
    display.print("You have received: PHP " + amountToReceive);
  }
   display.print("Your Remaining Balance: PHP " + dynamicBalance);
   display.display();
   display.clearDisplay();
}*/

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

