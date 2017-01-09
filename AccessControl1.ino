/*
   -----------------------------------------------------------
   RFID access control project
   Joint project with James Nylen and Daniel Near based on MFRC522 library.
   Linking an Arduino and RPI to our user database to provide access control.


  Workflow:

  Query RFID reader every minute or so and verify it's accessible.  Store this in a boolean flag

  If the RFID reader is not present, attempt to reset/reinitialize it.  If that fails, sleep some and attempt to repeat the process later.

  If an RFID tag is present, read the UID and pass it to the serial interface.

  Pass occasional heartbeat status to the serial port.  Include the processor ticks value.  This could alert the RPI if there's processor resets or errors that need addressing.

  Set status LEDs to default states (solid BLUE LED when both RPI and RFID reader are both connected, solid red if the link is down.
  RPI will send commands to the RPI to set status lights
  Flash red if an invalid badge is presented or flash green if a validated badge is presented.  Rapid blue flash when a badge is presented but not yet validated.

  Output one pin to activate a relay or MOSFET that will release the door latch.


   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/





#include <MFRC522.h>  //library for Mifare FC522 devices
#include <SPI.h>

/*
  For visualizing whats going on hardware we need some leds and to control door lock a relay and a wipe button
  (or some other hardware) Used common anode led,digitalWriting HIGH turns OFF led Mind that if you are going
  to use common cathode led or just seperate leds, simply comment out #define COMMON_ANODE,
*/

//#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define redLed 7    // Set Led Pins
#define greenLed 6
#define blueLed 5
uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

#define relay 4     // Set Relay Pin
unsigned long ul_PreviousMillis = 0UL;
unsigned long ul_PreviousReaderMillis = 0UL;
unsigned long ul_WatchDog = 0UL;
unsigned long ul_Interval = 1000UL;
unsigned long ul_CheckReaderInterval = 30000UL;
byte readCard[4];   // Stores scanned ID read from RFID Module

int setDelay = 5000; //5 seconds lock delay when valid card present
// Create MFRC522 instance.
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

char PiConnected = -1;
char MFCConnected = -1;
char blueState = 0;
char redState = 0;
char greenState = 0;
char AccessState = 1;
char watchDogState = 0;
void setup() {
  // put your setup code here, to run once:
  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);    // Make sure door is locked
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  Serial.println(F("RMM Access Control v0.1"));   // For debugging purposes
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details
  ul_PreviousMillis = millis();
  ul_WatchDog = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long ul_CurrentMillis = millis();
  if ( ul_CurrentMillis - ul_PreviousMillis > ul_Interval)
  {
    ul_PreviousMillis += ul_Interval;
    if (AccessState >= 0)
    {
      if ( watchDogState >= 0)
      {
        toggleBlue();

      }
      else
      {
        blueOFF();
      }
    }

    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    if (successRead == true)
    {
      blueON();
      //send it to serial
      //well, currently the getID() process spits data out serial..  should move that here..
    }
    // (!successRead);   //the program will not go further while you are not getting a successful read
    /* if (PiConnected==1 && MFCConnected==1)
      {
       if( AccessState==-1)
       {
         AccessState==1;
       }
      }
      elseif (PiConnected==-1 || MFCConnected==-1)
      {
       AccessState=-1;
      }*/
    if (MFCConnected > 0)
    {
      AccessState = 1;
    }
    else
    {
      AccessState=-1;
    }
  }
  handleSerial();//look for incoming data
  testWatchDog();//see if watchdog is actively pinged, set RED LED as status
  checkReader();
}

void checkReader();
{
  unsigned long ul_CurrentMillis = millis();
  if (ul_CurrentMillis - ul_PreviousReaderMillis > ul_CheckReaderInterval)
  {
    ul_PreviousReaderMillis += ul_CheckReaderInterval;
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x91)
    {
      MFCConnected = 1;
    }
    else if (v == 0x92)
    {
      MFCConnected = 1;
    }
    else
    {
      MFCConnected = -2;
    }
    // When 0x00 or 0xFF is returned, communication probably failed
    if ((v == 0x00) || (v == 0xFF)) 
    {
      MFCConnected = -1;
      Serial.println(F("WARNING: Communication failure; no response from MFRC522. Is it properly connected?"));
      mfrc522.PCD_Init();    // Initialize MFRC522 Hardware (Will this restore it?)

    }
  }
}
void granted ( uint16_t setDelay)
{
  digitalWrite(redLed, LED_OFF);  // Turn off red LED
  digitalWrite(greenLed, LED_ON);   // Turn on green LED
  digitalWrite(relay, LOW);     // Unlock door!
  delay(setDelay);          // Hold door lock open for given seconds
  digitalWrite(relay, HIGH);    // Relock door
  delay(1000);            // Hold green LED on for a second
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied()
{
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(redLed, LED_ON);   // Turn on red LED
  delay(1000);
}
///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID()
{
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.print("UID=");
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    if (readCard[i] < 0x10)
    {
      Serial.print('0');
    }
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
  {
    Serial.print(F(" = v1.0"));
    MFCConnected = 1;
  }
  else if (v == 0x92)
  {
    Serial.print(F(" = v2.0"));
    MFCConnected = 1;
  }
  else
  {
    Serial.print(F(" (unknown),probably a chinese clone?"));
    MFCConnected = -2;
  }
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    MFCConnected = -1;
    Serial.println(F("WARNING: Communication failure; no response from MFRC522. Is it properly connected?"));
    //Serial.println(F("SYSTEM HALTED: Check connections."));
  }
}
//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn ()
{
  digitalWrite(redLed, LED_OFF);  // Make sure Red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure Green LED is off
  digitalWrite(relay, HIGH);    // Make sure Door is Locked
}
void toggleRed()
{
  if (redState == 0)
  {
    redON();
  }
  else
  {
    redOFF();
  }
}
void redON()
{
  redState = 1;
  digitalWrite(redLed, LED_ON);
}
void redOFF()
{
  redState = 0;
  digitalWrite(redLed, LED_OFF);
}
void toggleBlue()
{
  if (blueState == 0)
  {
    blueON();
  }
  else
  {
    blueOFF();
  }
}
void blueOFF()
{
  blueState = 0;
  digitalWrite(blueLed, LED_OFF);
}
void blueON()
{
  blueState = 1;
  digitalWrite(blueLed, LED_ON);
}

void toggleGreen()
{
  if (greenState == 0)
  {
    greenON();
  }
  else
  {
    greenOFF();
  }
}
void greenOFF()
{
  greenState = 0;
  digitalWrite(greenLed, LED_OFF);
}
void greenON()
{
  greenState = 1;
  digitalWrite(greenLed, LED_ON);
}



void handleSerial()
{
  char inByte = 0;
  if ( Serial.available() > 0)
  {
    //delay(1000);
    inByte = Serial.read();
    if (inByte == 'W') //Set/Reset watchdog timer
    {
      if (watchDogState != 0)
      {
        watchDogState = 0;
        ul_WatchDog = millis();
        Serial.print("S=");
        Serial.println(AccessState, DEC);
        //Serial.println();
      }

    }
    if (inByte == 'w') //Set/Reset watchdog timer
    {
      if (watchDogState != 1 )
      {
        watchDogState = 1;
        ul_WatchDog = millis();
        Serial.print("S=");
        Serial.println(AccessState, DEC);
        //Serial.println();
      }
    }
    if (inByte == 'u') //unlock door for 5 seconds
    {
      Serial.println("Unlocking");
      digitalWrite(relay, HIGH);
      for (char i = 0, i < 6, i++)
      {
        greenON();
        delay(500)
        greenOFF();
        delay(500);
      }
      digitalWrite(relay, LOW);
      Serial.println("Unlock Released!")
    }

  }
  //greenOFF();


}

void testWatchDog()
{
  unsigned long ul_CurrentMillis = millis();
  if ( ul_CurrentMillis - ul_WatchDog > 300000UL)//5 minutes
  {
    watchDogState = -1;
    ul_WatchDog = ul_CurrentMillis;
  }
  if ( watchDogState >= 0 && MFCConnected >= 0 && AccessState >= 0)
  {
    redOFF();
  }
  else
  {
    /*Serial.print("WDFail ");
      Serial.print("WDS=");
      Serial.print(watchDogState,DEC);
      Serial.print(" MFC=");
      Serial.print(MFCConnected,DEC);
      Serial.print(" ACC=");
      Serial.println(AccessState,DEC);
      delay(1000);*/
    redON();
    blueOFF();
  }
}

