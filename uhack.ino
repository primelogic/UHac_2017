

/* button 1 - 2
 * button 2 - 3
 * button 3 - 4
 * button 4 - 5
 * button 5 - 6
 * button 6 - 7
 * EEPROM address 0 - New state or not new
 * EEPROM address 1 - Password
 */


#include <nfc.h>
#include <Wire.h>
#include <Button.h>
#include <EEPROM.h>
byte passwordCache;
byte passwordResetCache;
bool isNew; //indicates whether password is set or not
bool setPasswordMode = false;
bool idleMode = false;
bool resetMode = false;
bool resetReleaseMode = false;
bool enterOldPasswordMode = false;
bool oldPasswordIsMatch = false;


Button bt1(2, true, true, 25);
Button bt2(3, true, true, 25);
Button bt3(4, true, true, 25);
Button bt4(5, true, true, 25);
Button bt5(6, true, true, 25);
Button bt6(7, true, true, 25);

void eepromclear(){
  for (int i = 0 ; i < EEPROM.length() ; i++) {EEPROM.write(i, 0);}
  }

void setup() {
Serial.begin(9600);
eepromclear();
if ((1 and EEPROM.read(0))==0){
  isNew = true; 
}
}

void loop() {
bt1.read();
bt2.read();
bt3.read();
bt4.read();
bt5.read();
bt6.read();
if (isNew == true){
  
  if (setPasswordMode == false){
     Serial.println("Set your password: ");
     setPasswordMode = true;
  }
  
  if (bt1.wasPressed()){bitWrite(passwordCache, 0, 1);}
  if (bt2.wasPressed()){bitWrite(passwordCache, 1, 1);}
  if (bt3.wasPressed()){bitWrite(passwordCache, 2, 1);}
  if (bt4.wasPressed()){bitWrite(passwordCache, 3, 1);}
  if (bt5.wasPressed()){bitWrite(passwordCache, 4, 1);}
  if (bt6.wasPressed()){bitWrite(passwordCache, 5, 1);}
  
  if (bt1.wasReleased()||
      bt2.wasReleased()||
      bt3.wasReleased()||
      bt4.wasReleased()||
      bt5.wasReleased()||
      bt6.wasReleased()){
    EEPROM.write(1, passwordCache);
    EEPROM.write(0, 1);
    Serial.println("Password saved!");
    Serial.println(EEPROM.read(1));
    isNew = false;
  }
}

else {
  //do stuff here when password is already set, or not new anymore
  if (idleMode == false){
    Serial.println("Idle Mode");
    idleMode = true;
  }
  
  if (bt1.pressedFor(3000)&&
      bt2.pressedFor(3000)&&
      bt3.pressedFor(3000)&&
      bt4.pressedFor(3000)&&
      bt5.pressedFor(3000)&&
      bt6.pressedFor(3000)&&
      resetMode == false){
        resetMode = true; 
      }   
   if (bt1.isReleased()&&
       bt2.isReleased()&&
       bt3.isReleased()&&
       bt4.isReleased()&&
       bt5.isReleased()&&
       bt6.isReleased()&&
       resetMode == true&&
       resetReleaseMode == false){
           resetReleaseMode = true;
       }

   if (resetReleaseMode == true&&enterOldPasswordMode == false){
       Serial.println("Enter old password:");
       resetReleaseMode = false;
       enterOldPasswordMode = true;
       }       
        
   }
}




