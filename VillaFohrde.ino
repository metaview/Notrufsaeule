#include "Arduino.h"
#include <avr/sleep.h>
#include <avr/power.h>

// Arduino Nano, ATmega328P

//#define NEW_MP3LIB 1
#ifdef NEW_MP3LIB
  #include <DFPlayerMini_Fast.h>
#else
  #include "DFRobotDFPlayerMini.h"
#endif

// used for MP3 playing
#include <AltSoftSerial.h>
AltSoftSerial altSerial;
#define FPSerial altSerial

// connections
#define ENABLE_RING   13 // Power for the H-Bridge
#define OUTPUT_RING2  12
#define OUTPUT_RING1  11
#define BUTTON_MP3    10
#define TX_MP3        9
#define RX_MP3        8
#define INPUT_PIR     7
#define BUTTON_RING   6
#define PHONE_BTN     5
#define LED_EXTERN    4
#define RING_TIMEOUT  (20*1000)

#ifdef NEW_MP3LIB
  DFPlayerMini_Fast myMP3;
#else
  DFRobotDFPlayerMini myDFPlayer;
#endif
void printDetail(uint8_t type, int value);

unsigned long lastPlaying = 0;
unsigned long lastRing = 0;

int oldPIR = 0;
int oldPhone = 0;
int oldBtnRing = 0;
int oldBtnMP3 = 0;

// the setup function runs once when you press reset or power the board
void setup()
{
  FPSerial.begin(9600);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // turn the LED off
  pinMode(BUTTON_MP3, INPUT_PULLUP);
  pinMode(BUTTON_RING, INPUT_PULLUP);
  pinMode(INPUT_PIR, INPUT);
  pinMode(PHONE_BTN, INPUT_PULLUP);
  pinMode(OUTPUT_RING1, OUTPUT);
  digitalWrite(OUTPUT_RING1, LOW);
  pinMode(OUTPUT_RING2, OUTPUT);
  digitalWrite(OUTPUT_RING2, LOW);
  pinMode(LED_EXTERN, OUTPUT);
  digitalWrite(LED_EXTERN, LOW);
  pinMode(ENABLE_RING, OUTPUT);
  digitalWrite(ENABLE_RING, LOW);

  // Pin-Change-Interrupts aktivieren
  PCICR |= (1 << PCIE0);  // PCINT0-7 (Pins 8-13)
  PCICR |= (1 << PCIE2);  // PCINT16-23 (Pins 0-7)
  
  // Pins aktivieren
  PCMSK0 |= (1 << PCINT2); // Pin 10 (BUTTON_MP3)
  PCMSK2 |= (1 << PCINT23); // Pin 7 (INPUT_PIR)
  PCMSK2 |= (1 << PCINT22); // Pin 6 (BUTTON_RING)
  PCMSK2 |= (1 << PCINT21); // Pin 5 (PHONE_BTN)

  Serial.begin(115200);
  Serial.println("AltSoftSerial Test Begin");
  altSerial.begin(9600);

  Serial.println();
  Serial.println(F("Villa Förde"));
  Serial.println(F("Initializing MP3 Player ... (May take 3~5 seconds)"));
  
#ifdef NEW_MP3LIB
  myMP3.begin(altSerial, true);
#else
  while (!myDFPlayer.begin(FPSerial, true, true))
  {
    //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
  }
#endif
  Serial.println(F("MP3 Player online."));

  stopTheMusic();

  lastRing = lastPlaying = millis();
  //ringTheBell();
 
//  digitalWrite(LED_EXTERN, HIGH);

  oldPIR = digitalRead(INPUT_PIR);
  oldPhone = digitalRead(PHONE_BTN);
  oldBtnRing = digitalRead(BUTTON_RING);
  oldBtnMP3 = digitalRead(BUTTON_MP3);

  ringTheBell();
}

// the loop function runs over and over again forever
void loop()
{
  //digitalWrite(LED_EXTERN, digitalRead(INPUT_PIR));
  //return;

  //Serial.print("Radar: ");
  //Serial.println(digitalRead(INPUT_PIR));
  digitalWrite(LED_EXTERN, digitalRead(INPUT_PIR)); // monitor sensor

  unsigned long now = millis();
#ifdef NEW_MP3LIB
  if (!myMP3.isPlaying())
#else
  if (myDFPlayer.available())
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  int state = myDFPlayer.readState();
//  Serial.println(state, BIN);
  int playing = state & 0x0001;
  //Serial.println(digitalRead(PHONE_BTN));
  if (playing == 0)
#endif
  {
    // not playing
    if ((digitalRead(INPUT_PIR) == 1) && (oldPIR == 0))
    {
      // wenn eine Bewegung erkannt wird: klingeln
      // aber nur maximal alle 30 sekunden, bzw, nach dem das Abspielen vorbei ist, auch wieder 30 sekunden warten
      if ((now > (lastRing + RING_TIMEOUT)) && (now > (lastPlaying + RING_TIMEOUT)))
      {
        for (int i=0; i<5; i++)
        {
          ringTheBell();
          for (int k=0; k<10; k++)
          {
            delay(100);
            if (digitalRead(PHONE_BTN) == 1)
              break;
          }
          if (digitalRead(PHONE_BTN) == 1)
            break;
        }
        now = lastRing = millis();
      }
    }
    if ((digitalRead(PHONE_BTN) == 1) && (oldPhone == 0))
    {
      // wenn der Hörer abgehoben wird: abspielen
      //if ((millis() - lastStart) > 30*1000)
      {
        // only restart after 10 secs
        playTheMusic();
        //digitalWrite(LED_EXTERN, LOW);  // turn the LED off
      }
    }
    if ((digitalRead(BUTTON_MP3) == 0) && (oldBtnMP3 == 1))
    {
      playTheMusic();
      //----Read imformation----
#ifdef NEW_MP3LIB
      Serial.print("Version: ");
      Serial.println(myMP3.currentVersion()); //read version
      Serial.print("Mode: ");
      Serial.println(myMP3.currentMode()); //read mp3 state
      Serial.print("Volume: ");
      Serial.println(myMP3.currentVolume()); //read current volume
      Serial.print("EQ: ");
      Serial.println(myMP3.currentEQ()); //read EQ setting
      Serial.print("FileCount: ");
      Serial.println(myMP3.numSdTracks()); //read all file counts in SD card
      Serial.print("CurrentFileNumber: ");
      Serial.println(myMP3.currentSdTrack()); //read current play file number
#else
      Serial.print("State: ");
      Serial.println(myDFPlayer.readState()); //read mp3 state
      Serial.print("Volume: ");
      Serial.println(myDFPlayer.readVolume()); //read current volume
      Serial.print("EQ: ");
      Serial.println(myDFPlayer.readEQ()); //read EQ setting
      Serial.print("FileCount: ");
      Serial.println(myDFPlayer.readFileCounts()); //read all file counts in SD card
      Serial.print("CurrentFileNumber: ");
      Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
#endif
    }
    if ((digitalRead(BUTTON_RING) == 0) && (oldBtnRing == 1))
    {
      // ring the bell
      ringTheBell();
    }
    //if (now > (lastRing + RING_TIMEOUT))
    //  digitalWrite(LED_EXTERN, LOW);
  }
  else
  {
    // playing
    lastPlaying = millis();
    if ((digitalRead(PHONE_BTN) == 0) && (digitalRead(BUTTON_MP3) == 1))
    {
      // stop playing
      stopTheMusic();
      //digitalWrite(LED_EXTERN, HIGH);
    }
  }
  oldPIR = digitalRead(INPUT_PIR);
  oldPhone = digitalRead(PHONE_BTN);
  oldBtnRing = digitalRead(BUTTON_RING);
  oldBtnMP3 = digitalRead(BUTTON_MP3);

  delay(100);
/*
  playing = myDFPlayer.readState() & 0x0001;
  if (playing == 0)
  {
    // not playing: go to sleep
    if (myDFPlayer.available())
    {
      printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }

    if (((now - lastRing) > RING_TIMEOUT) && ((now - lastPlaying) > RING_TIMEOUT))
      goToSleep();
  }
*/
}


void goToSleep()
{
  Serial.println("Go to sleep now");
  set_sleep_mode(SLEEP_MODE_PWR_SAVE); // Tiefster Sleep-Modus
  sleep_enable();
  sleep_cpu();
  
  // Wird hier fortgesetzt nach Interrupt
  sleep_disable();
  Serial.println("Wake up");
}

// Interrupt-Handler (muss existieren, kann leer sein)
ISR(PCINT0_vect)
{
  // Aufwachen durch Pin 10
}

ISR(PCINT2_vect)
{
  // Aufwachen durch Pins 5, 6, 7
}

void ringTheBell()
{
  int TIME1 = 35;
  int TIME2 = 5;
  // ring the bell
  digitalWrite(ENABLE_RING, HIGH);
  for (int i=0; i<4; i++)
  {
    digitalWrite(OUTPUT_RING2, HIGH);
    digitalWrite(OUTPUT_RING1, LOW);
    delay(TIME1);
    digitalWrite(OUTPUT_RING2, HIGH);
    digitalWrite(OUTPUT_RING1, HIGH);
    delay(TIME2);
    digitalWrite(OUTPUT_RING2, LOW);
    digitalWrite(OUTPUT_RING1, HIGH);
    delay(TIME1);
    digitalWrite(OUTPUT_RING2, HIGH);
    digitalWrite(OUTPUT_RING1, HIGH);
    delay(TIME2);
  }
  digitalWrite(ENABLE_RING, LOW);
}

void stopTheMusic()
{
  Serial.println("stopTheMusic");
#ifdef NEW_MP3LIB
  myMP3.stop();
  myMP3.sleep();
#else
  myDFPlayer.pause();  //pause the mp3
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
  myDFPlayer.disableDAC();  //Disable On-chip DAC
  myDFPlayer.outputSetting(false, 0); //output setting, enable the output and set the gain to 20
//  myDFPlayer.sleep();
#endif
}

void playTheMusic()
{
  Serial.println("playTheMusic");
#ifdef NEW_MP3LIB
  myMP3.wakeUp();
  myMP3.volume(25);
  myMP3.play(1);
#else
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
//  myDFPlayer.reset();     //Reset the module
//  delay(2*1000);
  myDFPlayer.enableDAC();  //Enable On-chip DAC
  myDFPlayer.outputSetting(true, 20); //output setting, enable the output and set the gain to 20
  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
//  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
//  myDFPlayer.EQ(DFPLAYER_EQ_POP);
//  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
//  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
//  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
//  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  myDFPlayer.volume(25);  //Set volume value. From 0 to 30
  myDFPlayer.play(1);  //Play the first mp3
#endif
}

#ifndef NEW_MP3LIB

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }  
}

void allFunctionsMP3()
{
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  
  //----Set volume----
  myDFPlayer.volume(10);  //Set volume value (0~30).
  myDFPlayer.volumeUp(); //Volume Up
  myDFPlayer.volumeDown(); //Volume Down
  
  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
//  myDFPlayer.EQ(DFPLAYER_EQ_POP);
//  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
//  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
//  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
//  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  
  //----Set device we use SD as default----
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_U_DISK);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_AUX);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
//  myDFPlayer.outputDevice(DFPLAYER_DEVICE_FLASH);
  
  //----Mp3 control----
//  myDFPlayer.sleep();     //sleep
//  myDFPlayer.reset();     //Reset the module
//  myDFPlayer.enableDAC();  //Enable On-chip DAC
//  myDFPlayer.disableDAC();  //Disable On-chip DAC
//  myDFPlayer.outputSetting(true, 15); //output setting, enable the output and set the gain to 15
  
  //----Mp3 play----
  myDFPlayer.next();  //Play next mp3
  delay(1000);
  myDFPlayer.previous();  //Play previous mp3
  delay(1000);
  myDFPlayer.play(1);  //Play the first mp3
  delay(1000);
  myDFPlayer.loop(1);  //Loop the first mp3
  delay(1000);
  myDFPlayer.pause();  //pause the mp3
  delay(1000);
  myDFPlayer.start();  //start the mp3 from the pause
  delay(1000);
  myDFPlayer.playFolder(15, 4);  //play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
  delay(1000);
  myDFPlayer.enableLoopAll(); //loop all mp3 files.
  delay(1000);
  myDFPlayer.disableLoopAll(); //stop loop all mp3 files.
  delay(1000);
  myDFPlayer.playMp3Folder(4); //play specific mp3 in SD:/MP3/0004.mp3; File Name(0~65535)
  delay(1000);
  myDFPlayer.advertise(3); //advertise specific mp3 in SD:/ADVERT/0003.mp3; File Name(0~65535)
  delay(1000);
  myDFPlayer.stopAdvertise(); //stop advertise
  delay(1000);
  myDFPlayer.playLargeFolder(2, 999); //play specific mp3 in SD:/02/004.mp3; Folder Name(1~10); File Name(1~1000)
  delay(1000);
  myDFPlayer.loopFolder(5); //loop all mp3 files in folder SD:/05.
  delay(1000);
  myDFPlayer.randomAll(); //Random play all the mp3.
  delay(1000);
  myDFPlayer.enableLoop(); //enable loop.
  delay(1000);
  myDFPlayer.disableLoop(); //disable loop.
  delay(1000);

  //----Read imformation----
  Serial.println(myDFPlayer.readState()); //read mp3 state
  Serial.println(myDFPlayer.readVolume()); //read current volume
  Serial.println(myDFPlayer.readEQ()); //read EQ setting
  Serial.println(myDFPlayer.readFileCounts()); //read all file counts in SD card
  Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
  Serial.println(myDFPlayer.readFileCountsInFolder(3)); //read file counts in folder SD:/03
}

#endif
