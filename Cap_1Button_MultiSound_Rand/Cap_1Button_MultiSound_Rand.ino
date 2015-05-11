#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"
#include <CapacitiveSensor.h>






SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define DEBOUNCE 5  // button debouncer


// here is where we define the ANALOGIN2 that we'll use. button "1" is the first, button "6" is the 6th, etc
byte ANALOGIN2 = 15;
//original ANALOGIN2
//byte ANALOGIN2[] = {14, 15, 16, 17, 18, 19};
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMANALOGIN2 sizeof(ANALOGIN2)
// we will track if a button is just pressed, just released, or 'pressed' (the current state
volatile byte pressed[NUMANALOGIN2], justpressed[NUMANALOGIN2], justreleased[NUMANALOGIN2];

// this handy function will return the number of bytes currently free in RAM, great for debugging!
int freeRam(void)
{
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while (1);
}

////////////////////////////////////////////////// 
// create an instance of the library
// pin 4 sends electrical energy (7)
// pin 2 senses senses a change (6)
CapacitiveSensor capSensor = CapacitiveSensor(7, 6);


// threshold for turning the lamp on
int threshold = 30;
//////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

// keep track of the que
int que = 0;
//Makes a list of all of the sounds to cycle through
const char sound1[] = "Track01.WAV";
const char sound2[]  = "Track02.WAV";
const char sound3[]  = "Track03.WAV";
const char sound4[]  = "Track04.WAV";
const char sound5[]  = "Track05.WAV";
const char sound6[]  = "Track06.WAV";
const char sound7[]  = "Track07.WAV";
const char sound8[]  = "Track08.WAV";
const char sound9[]  = "Track09.WAV";
const char sound10[]  = "Track10.WAV";
const char sound11[]  = "Track11.WAV";
//const char sound12[]  = "Track012.WAV";
//const char sound13[]  = "Track013.WAV";
//const char sound14[]  = "Track014.WAV";
//const char sound15[]  = "Track015.WAV";
//const char sound16[]  = "Track016.WAV";
//const char sound17[]  = "Track17.WAV";
//const char sound18[]  = "Track18.WAV";
//const char sound19[]  = "Track019.WAV";
//THe star is to make it a pointer, not sure if I need that??
const char* soundList[] = {sound1, sound2, sound3, sound4, sound5, sound6, sound7, sound8, sound9, sound10, sound11};


#define randomHatStartNum 0
int randomHatEndNum = sizeof(soundList);
#define NumberInHat abs(randomHatEndNum - randomHatStartNum) + 1


/////////////////////////////////////////////////////////////////////

void setup() {


  byte i;
  // set up serial port
  Serial.begin(9600);
  putstring_nl("WaveHC with ");
  //Serial.print(NUMANALOGIN2, DEC);
  //putstring_nl("ANALOGIN2");

  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!

  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // pin13 LED
  pinMode(13, OUTPUT);

  // Make input & enable pull-up resistors on switch pins
  pinMode(15, INPUT);
   
  //digitalWrite(15, HIGH);


  
  if (!card.init()) {         //play with 8 MHz spi (default faster!)
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while (1);                           // then 'halt' - do nothing!
  }

  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part))
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while (1);                           // then 'halt' - do nothing!
  }

  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);    // FAT16 or FAT32?

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while (1);                            // then 'halt' - do nothing!
  }

  // Whew! We got past the tough parts.
  putstring_nl("Ready!");
/////////////////////////
//  TCCR2A = 0;
//  TCCR2B = 1 << CS22 | 1 << CS21 | 1 << CS20;
//  //Timer2 Overflow Interrupt Enable
//  TIMSK2 |= 1 << TOIE2;
//  Serial.println("end of setup");
}




//SIGNAL(TIMER2_OVF_vect) {
//  check_switches();
//    Serial.println("Timer triggered");
//      
//}



////////////////////////////////////
void check_switches(){
  static byte previousstate[1];
  static byte currentstate[1];
  byte index = 0;
  //static byte previousstate[1];
  //static byte currentstate[1];
  // store the value reported by the sensor in a variable
  long sensorValue = capSensor.capacitiveSensor(30);
  Serial.println("sensor value:");
  Serial.println(sensorValue);
  
  //currentstate[index] = digitalRead(buttons[index]); 

  if (sensorValue > threshold){
    pressed[0] = 1;
  } else {
    pressed[0] = 0;
  }

//       if (currentstate[index] == previousstate[index]) {
//       if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
//         // just pressed
//         justpressed[index] = 1;
//       }
//       else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
//         // just released
//         justreleased[index] = 1;
//       }
//       pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
//     }
//     //Serial.println(pressed[index], DEC);
//     previousstate[index] = currentstate[index];   // keep a running tally of the buttons
}
char* currentSound;
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
void loop() {
  
//    for (int i = 1; i < 19; ++i)
//      {
//        const char soundList[i]  = "Track" + i + ".WAV";
//        return soundList;
//      }




  check_switches();
  
  if ( pressed[0] ) {
    Serial.println("sensor was triggered");
    playfile(currentSound);
    while (wave.isplaying && pressed[0] ) {
      Serial.print("still playng sound.....");
      digitalWrite(13,HIGH);
    }
    //assign the sound according to the que
    wave.stop();
    que = que + 1;
    if ( que == sizeof(soundList)){
      que = 0;
    }
  
    
    
    const char* currentSound = soundList[randomHat()];
  }
  wave.stop();
}


int randomHat(){
  static int  totalNumInHat = abs(randomHatEndNum - randomHatStartNum) + 1;
  static int currentNumInHat = 0; 
  abs(randomHatEndNum - randomHatStartNum) + 1;
  int randArray[abs(randomHatEndNum - randomHatStartNum) + 1];
  int i;		//as long as they can be changed without harming other code
  int thePick;		//this is the return variable with the random number from the pool		
  int theIndex;

  if  (currentNumInHat == 0){                   // hat is emply - all have been choosen - fill up array again
    for (i = 0 ; i<=(totalNumInHat - 1); i++){  // Put 1 TO numberInHat in array - starting at address 0.
      if (randomHatStartNum < randomHatEndNum){
      randArray[i] = randomHatStartNum + i; }	
      else if (randomHatStartNum > randomHatEndNum){  // defensive programming in case startNum is greater than endNum
       randArray[i] = randomHatEndNum + i; }
       else{ 
       return randomHatStartNum; }	        // startNum and endNum must be same number - return one - and bail out 
     }					
    currentNumInHat = abs(randomHatEndNum - randomHatStartNum) + 1;   // reset current Number in Hat
    //if something should happen when the hat is empty do it here
  }

  theIndex = random(currentNumInHat);	        //choose a random index	
  thePick = randArray[theIndex];

  for (i = theIndex; i<= (currentNumInHat - 1); i++){
    randArray[i] =  randArray[i + 1];           // bump all the higher array contents down one,  erasing the last number chosen
  }
  currentNumInHat--;                            // decrement counter
  return thePick;
}




void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file ");
    Serial.print(name);
    return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }

  // ok time to play! start playback
  wave.play();
}
