#include "Arduino.h"
#if !defined(SERIAL_PORT_MONITOR)
  #error "Arduino version not supported. Please update your IDE to the latest version."
#endif

#if defined(__SAMD21G18A__)
  // Shield Jumper on HW (for Zero, use Programming Port)
  #define port SERIAL_PORT_HARDWARE
  #define pcSerial SERIAL_PORT_MONITOR
#elif defined(SERIAL_PORT_USBVIRTUAL)
  // Shield Jumper on HW (for Leonardo and Due, use Native Port)
  #define port SERIAL_PORT_HARDWARE
  #define pcSerial SERIAL_PORT_USBVIRTUAL
#else
  // Shield Jumper on SW (using pins 12/13 or 8/9 as RX/TX)
  #include "SoftwareSerial.h"
  SoftwareSerial port(12, 13);
  #define pcSerial SERIAL_PORT_MONITOR
#endif

#include "EasyVR.h"
#include "EasyVR_sound_table.h" //声音对照表
// include the library code:
#include <LiquidCrystal.h>

EasyVR easyvr(port);

//自定义变量
uint32_t mask = 0;
int8_t group = 0;
uint8_t train = 0;
int8_t trained_pw = 0;  //number of existing passwords
int8_t trained_un = 0;  //number of existing user names
bool lock = 0;          // locked/unlocked flag
bool error = 0;         // error flag
char name[32];

String comdata = ""; //String 定义一个空的字符串  
int STATE=1; 
int pwd;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd( 10, 9, 8, 7, 6, 5);

void setup()
{
  //LCD
  lcd.begin(16, 2);
  //GSM communication
  Serial.begin(9600);  
//  pinMode(13,OUTPUT);
  
  // setup PC serial port
  pcSerial.begin(9600);
bridge:
  // bridge mode?
  int mode = easyvr.bridgeRequested(pcSerial);
  switch (mode)
  {
  case EasyVR::BRIDGE_NONE:
    // setup EasyVR serial port
    port.begin(9600);
    // run normally
    pcSerial.println(F("Bridge not requested, run normally"));
    pcSerial.println(F("---"));
    break;
    
  case EasyVR::BRIDGE_NORMAL:
    // setup EasyVR serial port (low speed)
    port.begin(9600);
    // soft-connect the two serial ports (PC and EasyVR)
    easyvr.bridgeLoop(pcSerial);
    // resume normally if aborted
    pcSerial.println(F("Bridge connection aborted"));
    pcSerial.println(F("---"));
    break;
    
  case EasyVR::BRIDGE_BOOT:
    // setup EasyVR serial port (high speed)
    port.begin(115200);
    pcSerial.end();
    pcSerial.begin(115200);
    // soft-connect the two serial ports (PC and EasyVR)
    easyvr.bridgeLoop(pcSerial);
    // resume normally if aborted
    pcSerial.println(F("Bridge connection aborted"));
    pcSerial.println(F("---"));
    break;
  }


// Define the pins that will be used
//  pinMode(11, OUTPUT); //set pin 8 as output for LED
//  digitalWrite(11, LOW); //switch off LED
  
  // initialize EasyVR  
  while (!easyvr.detect())
  {
    pcSerial.println(F("EasyVR not detected!"));
    for (int i = 0; i < 10; ++i)
    {
      if (pcSerial.read() == '?')
        goto bridge;
      delay(100);
    }
  }

  pcSerial.print(F("EasyVR detected, version "));
  lcd.print("EasyVR");
  pcSerial.print(easyvr.getID());

  if (easyvr.getID() == EasyVR::EASYVR3)
    {easyvr.setPinOutput(EasyVR::IO1, LOW); // Shield 2.0 LED off
     digitalWrite(11, HIGH);}
    

  if (easyvr.getID() < EasyVR::EASYVR)
    pcSerial.print(F(" = VRbot module"));
  else if (easyvr.getID() < EasyVR::EASYVR2)
    pcSerial.print(F(" = EasyVR module"));
  else if (easyvr.getID() < EasyVR::EASYVR3)
    pcSerial.print(F(" = EasyVR 2 module"));
  else
    pcSerial.print(F(" = EasyVR 3 module"));
  pcSerial.print(F(", FW Rev."));
  pcSerial.println(easyvr.getID() & 7);

  easyvr.setDelay(0); // speed-up replies

  easyvr.setTimeout(5);
  easyvr.setLanguage(0); //<-- same language set on EasyVR Commander when code was generated


//  group = EasyVR::TRIGGER; //<-- start group (customize)
  int16_t count = 0;

  pinMode(11,OUTPUT);
  pinMode(3,OUTPUT);
  digitalWrite(11,HIGH);
  digitalWrite(3,HIGH);

  easyvr.setMicDistance(EasyVR::HEADSET);   //mic distance
  easyvr.setLevel(EasyVR::NORMAL);          //strictness level for recognition of custom commands
  easyvr.setKnob(EasyVR::TYPICAL);          //Sets the confidence threshold to use for recognition of built-in words or custom grammars
  easyvr.playSound(SND_Hello, EasyVR::VOL_DOUBLE);
}

void loop()
{
  int idx_cmd;
  int idx_pwd;
  int idx;

  GSM_loop();

  if (easyvr.getID() == EasyVR::EASYVR3)
    {easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
     digitalWrite(11, LOW);
     }

  Serial.println("Wait for trigger");
  lcd.begin(16, 2);
  lcd.print("Wait for trigger");
  easyvr.recognizeWord(4); // Wordset 4, recognise grammar number 4 (custom SI trigger word must be here!)
  while (!easyvr.hasFinished()); // wait for trigger

  idx_cmd = easyvr.getWord(); // get recognised command
  if ((idx_cmd == WS4_HELLO_DEVICE) && (lock == 0))  // When the system is unlocked
  {
    if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
    {
      easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
      digitalWrite(11, HIGH);
    }

    easyvr.playSound(SND_Hello_give_command , EasyVR::VOL_DOUBLE);  // ask for a command
    if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
    {
      easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
      digitalWrite(11, LOW);
    }

    Serial.println("Wait for commmand");
    lcd.begin(16, 2);
    lcd.print("say command");
    easyvr.recognizeWord(5); // recognise grammar number 5 (custom SI commands must be here!)
    while (!easyvr.hasFinished()); // wait for command

    if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
    {
      easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
      digitalWrite(11, HIGH);
    }

    idx_cmd = easyvr.getWord(); // get recognised command
  }


  if ((lock == 1) && (idx_cmd == WS4_HELLO_DEVICE)) // system locked, ask for password!
  {
    if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
    {
      easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
      digitalWrite(11, HIGH);
    }
       
      easyvr.playSound(SND_Please_say_your_password , EasyVR::VOL_DOUBLE);  // ask for password

      if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
      {
        easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
        digitalWrite(11, LOW);
      }
       
      lcd.begin(16, 2);
//      lcd.setCursor(1, 1);
      lcd.print("say password");

      int m,n;
      char d[4];
      for( m=0;m<4;m++ )
      {
        Serial.print("Wait for password ");
        Serial.println(m+1);     // Display which position of the password that we are waiting
//        Serial.print(" : ");
        if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
        {
          easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
          digitalWrite(11, LOW);
        }
        easyvr.recognizeWord(3); // Wordset 3, recognise grammar number 3 (custom SI trigger word must be here!)
        while (!easyvr.hasFinished());
        easyvr.playSound(EasyVR::BEEP, EasyVR::VOL_DOUBLE);   // Everytime done recognizing, beep 1 time.
        if (easyvr.getID() == EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
        {
          easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
          digitalWrite(11, HIGH);
        }
        
        Serial.println(easyvr.getWord());
        d[m] = easyvr.getWord();    // Store the identified numbers one by one to the array d
        lcd.setCursor(m+1, 1);
        n=d[m];
        lcd.print(n);
        }

//       Serial.println(d);

//      Serial.println("Say the password");
//      easyvr.recognizeCommand(EasyVR::PASSWORD); // set group 16
//      while (!easyvr.hasFinished()); // wait for password
//
//      if (easyvr.getID() < EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
//        easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
//
//      idx_pwd = easyvr.getCommand(); // get recognised password

      if ( ( d[3]+d[2]*10+d[1]*100+d[0]*1000 )==pwd ) // idx_pwd >= 0   index of username and password are the same, access granted
      {
        delay(500);
        Serial.println("Access granted");
        lcd.begin(16, 2);
//        lcd.setCursor(6, 1);
        lcd.print("Access granted");
        easyvr.playSound(SND_Access_granted , EasyVR::VOL_DOUBLE);
//        digitalWrite(11, LOW); //switch on LED;
//        delay(300);
        digitalWrite(11, HIGH); //switch on LED;
        delay(300);
        digitalWrite(11, LOW); //switch on LED;
        delay(300);
        digitalWrite(11, HIGH); //switch on LED;
        delay(300);
        digitalWrite(11, LOW); //switch on LED;
        delay(300);
        digitalWrite(11, HIGH); //switch on LED;
        delay(300);
        digitalWrite(11, LOW); //switch on LED;

        lock = 0; //system unlocked!
      }
      else // index of username and password differ, access is denied
      {
        delay(500);
        Serial.println("Access denied");
        lcd.begin(16, 2);
//        lcd.setCursor(6, 1);
        lcd.print("Access denied");
        easyvr.playSound(SND_Access_denied , EasyVR::VOL_DOUBLE);
        digitalWrite(3, LOW); //switch on LED;
        delay(300);
        digitalWrite(3, HIGH); //switch on LED;
        delay(300);
        digitalWrite(3, LOW); //switch on LED;
        delay(300);
        digitalWrite(3, HIGH); //switch on LED;
        delay(300);
        digitalWrite(3, LOW); //switch on LED;
        delay(300);
        digitalWrite(3, HIGH);
      }
      

      int16_t err = easyvr.getError();

      if (easyvr.isTimeout() || (err >= 0)) // password timeout, access is denied
      {
        Serial.println("Error, try again...");
        easyvr.playSound(SND_Access_denied , EasyVR::VOL_DOUBLE);
//        digitalWrite(12, HIGH); //switch on LED;
//        delay(300);
//        digitalWrite(12, LOW); //switch on LED;
//        delay(300);
//        digitalWrite(12, HIGH); //switch on LED;
//        delay(300);
//        digitalWrite(12, LOW); //switch on LED;
//        delay(300);
//        digitalWrite(12, HIGH); //switch on LED;
//        delay(300);
      }

  }
  else
  {
    switch (idx_cmd)
    {
//      case WS5_PASSWORD_SETUP:
//
//        Serial.println("password setup");
//        for (idx = 1; idx < 3; ++idx)
//        {
//          if (easyvr.getID() < EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
//            easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off
//
//          if (idx == 1)
//          {
//            easyvr.addCommand (16, trained_pw);
//            strcpy (name, "PWD_");
//            name[4] = '0' + trained_pw;
//            easyvr.setCommandLabel (16, trained_pw, name);
//            easyvr.playSound(SND_Please_say_your_password , EasyVR::VOL_DOUBLE);  // asking for password
//          }
//
//          else easyvr.playSound(SND_Please_repeat , EasyVR::VOL_DOUBLE);  // repeat!
//
//          if (easyvr.getID() < EasyVR::EASYVR3) // checking EasyVR version to manage green LED connected to IO1 on older EasyVR
//            easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
//          easyvr.trainCommand (16, trained_pw);
//
//          while (!easyvr.hasFinished()); // waits for password to be trained
//
//          int16_t err = easyvr.getError();
//          if (err >= 0)
//          {
//            error = 1;
//            Serial.print("Error ");
//            Serial.println(err, HEX);
//          }
//        }
//
//
//        break;

      case WS5_PASSWORD_ACTIVATED:
        Serial.println("password locked!");
        lcd.begin(16, 2);
//        lcd.setCursor(6, 1);
        lcd.print("password locked");
        easyvr.playSound(SND_Pwd_activated , EasyVR::VOL_DOUBLE); // Password activated and system locked!
        lock = 1; //locked!
        break;

      case WS5_LIGHTS_ON:
        Serial.println("Lights ON");
//        digitalWrite(11, HIGH); //switch on LED;
        break;

      case WS5_LIGHTS_OFF:
        Serial.println("Lights OFF");
//        digitalWrite(11, LOW); //switch off LED;
        break;
    }
  }
  
}

  
void GSM_loop()  
{  
    while (Serial.available() > 0)    
    {  
        comdata += char(Serial.read());  
        delay(2);  
    }  
    if (comdata.length() > 0)  
    {  
        Serial.println(comdata);
        pwd=comdata.toInt();
        Serial.println(pwd);
        STATE=!STATE;  
//        digitalWrite(13,STATE);    // The level flip can be used to visualize the operation of the program.
        comdata = "";  
    }
//    char pwd[4];
}
