#include <eRCaGuy_Timer2_Counter.h>

const int pulPinPlat    = 5;
const int dirPinPlat    = 4;
const int dirPinPlatgnd = 2;
const int enPinPlat     = 3;


const int pulPinVert  = 9;
const int dirPinVert  = 8;
const int dirPinVertgnd = 6;
const int enPinVert   = 7;

const int topEndPin    = A0;
const int bottomEndPin = A1;

int stepDelay = 100;

double spindlePitch = 4; // [mm]

// platform motor parameters
float platGain              = 1;
signed long dplatPPS        = 1600 * platGain; // desired platform pulse frequence [pulses/second]
unsigned long platPeriod    = 625;  // desired platform pulse period  [microseconds]
unsigned long platPPR       = 1600; // pulses per revolution on platform motor driver
unsigned long platPos       = 0;    // platform motor position [steps]
unsigned long setPlatPos    = 0;    // desired platform motor position [steps]
int platRed                 = 3;    //reduction between platform and motor

// vertical motor parameters
float vertGain              = 1;
signed long dvertPPS      = 1600 * vertGain; // desired vertical pulse frequency [pulses/second]
unsigned long vertPeriod    = 625;  // desired verticval period [microseconds]
unsigned long vertPPR       = 1600; // pulses per revolution on vertical motor driver
unsigned long vertPos       = 0;    // vertical motor position [steps]
double vertPosmm     = 0;
double dvertPosmm     = 0;
double layerHeight   = 10;
unsigned long setVertPos    = 0;    // desired vertical motor position [steps]

// platform motor variables
unsigned long platRampPeriod  = 0;  // platform pulse period during ramping [microseconds]
signed long cplatPPS          = 1;  // curreny platform pulse frequency [pulses/second]
signed long platFreqStep      = 0;  // incemental value for platform pulse frequency while ramping [pulses/second]
unsigned long cTimePlatform   = 0;  // current time
unsigned long pTimePlatform   = 0;  // previous time
int revs                      = 0;

// vertical motor variables
unsigned long vertRampPeriod  = 0;  // platform pulse period during ramping [microseconds]
signed long cvertPPS        = 1;  // curreny platform pulse frequency [pulses/second]
signed long vertFreqStep    = 0;  // incemental value for platform pulse frequency while ramping [pulses/second]
unsigned long cTimeVert       = 0;  // current time
unsigned long pTimeVert       = 0;  // previous time

// global timer variables
unsigned long cMillis   = 0;  // current time
unsigned long pMillis   = 0;  // previous time

// ramp variables
bool rampBool  = true;
unsigned long rampTime  = 500; //time in milliseconds
int rampInterval        = 100; // time between ramp steps [ms]
int rampi               = 1;  // ramp counter
signed int rampSteps    = 5; //amount of steps in ramp

// switch parameters
bool topEndBool    = false;  //top endswitch boolean
int topEndVal      = 0;
bool bottomEndBool = false;
int bottomEndVal  = 0;

// random booleans
bool disTransBool  = true; // used for transition between disabled and enabled

// Command interpreter variables
String inputString    = "";
String valString      = "";
String commandString  = "";
String returnEntry    = "";
double value   = 0;
bool stringEnd        = false;
bool cmdBool          = false;

// Command booleans
bool disableBool  = true;   // motors are disabled [true] motors are enabled [false]
bool slowBool     = false;  // shows if slowAll command is active
bool runspdBool   = false;  // shows if runspd command is active
bool printBool    = false;
bool dropBool     = false;
bool platBool     = true;
bool vertBool     = true;
bool incPlatBool  = false;  // shows if incPlat command is active
bool incVertBool  = false;  // shows if incVert command is active
bool platDirBool  = false;  // shows if platDirBool command is active
bool vertDirBool  = true;  // shows if vertDirBool command is active
bool initBool     = false;  // shows if init command is active

int progCount = 1;
unsigned long progStartTime = 0;
unsigned long progEndTime = 0;
float diff = 0;
float diffMean = 0;
int H = 0;

unsigned long cMicros = 0 ;

enum Commands { PlatPPS, PlatDir, PlatPPR, MovePlat, IncPlat,
                VertPPS, VertDir, VertPPR, MoveVert, IncVert,
                SetRamp, Initialize, RunSpd, SlowAll, StopAll,
                Default
              };

enum Mode {DisableAll, Homing, Speed, Print, StepPlatform, StepVertical };

Mode mode = DisableAll;
Commands inputCommand = Default;


// ------- MAIN FUNCTIONS ------- //
void setup() {
  //put your setup code here, to run once:

  pinMode(topEndPin, INPUT );
  pinMode(bottomEndPin, INPUT);

  pinMode (pulPinPlat, OUTPUT);
  pinMode (enPinPlat, OUTPUT);
  pinMode (dirPinPlat, OUTPUT);
  pinMode (dirPinPlatgnd, OUTPUT);

  pinMode (pulPinVert, OUTPUT);
  pinMode (enPinVert, OUTPUT);
  pinMode (dirPinVert, OUTPUT);
  pinMode (dirPinVertgnd, OUTPUT);

  digitalWrite(pulPinPlat, HIGH);
  digitalWrite(dirPinPlat, HIGH);
  digitalWrite(dirPinPlatgnd, LOW);
  digitalWrite(enPinPlat, HIGH);

  digitalWrite(pulPinVert, HIGH);
  digitalWrite(dirPinVert, HIGH);
  digitalWrite(dirPinVertgnd, HIGH);
  digitalWrite(enPinVert, HIGH);


  timer2.setup();

  platPos       = 0;

  Serial.begin(2000000);
  //Serial.begin(9600);
}

void loop() {

  // put your main code here, to run repeatedly:
  cMicros = micros();
  cMillis = millis();

  switch (mode) {

    case DisableAll:
      runspdBool = false;
      slowBool = false;
      printBool = false;

      digitalWrite(enPinPlat, LOW);
      digitalWrite(enPinVert, LOW);
      disTransBool = true;
      break;
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
    case Homing:
      if (disTransBool) {
        digitalWrite(enPinPlat, HIGH);
        digitalWrite(enPinVert, HIGH);

        Serial.println("delay");
        delay(2500);
        disTransBool = false;
      }
      homing();
      break;
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
    case Speed:
      checkEndSwitches();
      if (disTransBool) {
        digitalWrite(enPinPlat, HIGH);
        digitalWrite(enPinVert, HIGH);

        Serial.println("delay");
        delay(2500);
        disTransBool = false;
      }

      if (runspdBool) {
        runspd();
      }
      else if (slowBool) {
        slowAll();
      }

      break;
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
    case Print:
    vertDirBool= true;
    //vertBool=true;
    //platBool=true;
    digitalWrite(dirPinVert, HIGH);
    digitalWrite(dirPinVertgnd, LOW);
    
      checkEndSwitches();
      if (disTransBool) {
        digitalWrite(enPinPlat, HIGH);
        digitalWrite(enPinVert, HIGH);

        Serial.println("delay");
        delay(2500);
        disTransBool = false;
      }

      if (printBool) {
        runprint();
      }
      else if (slowBool) {
        slowAll();
      }

      break;
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
    case StepPlatform:
      checkEndSwitches();
      stepPlatform();
      incPlatBool = false;
      break;
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
    case StepVertical:
      checkEndSwitches();
      stepVertical();
      incVertBool = false;
      break;
    default:
      break;
      // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ //
  }


}


// ------- COMMUNICATION FUNCTIONS ------- //
void serialEvent() {

  while (Serial.available()) {

    char inChar = (char)Serial.read();
    returnEntry += inChar;

    //build input to command
    if (cmdBool == false && inChar != ';') {
      inputString += inChar;
    }

    //spacer detected
    if (inChar == ';') {
      cmdBool = true;
    }

    //build input value
    if (cmdBool == true && inChar != ';') {
      valString += inChar;
    }

    //end of input
    if (inChar == '~') {
      stringEnd = true;
      cmdBool = false;

      //activate decode message function
      decodeMessage();
    }

  }

}

void decodeMessage() {

  // ~~~~~~~~~~~~~~~~~ get input value ~~~~~~~~~~~~~~~~~ //
  if (inputString != "" && stringEnd == true) {

    value = valString.toDouble();  //change from string to double
    commandString = inputString;

    Serial.print("inputString: ");
    Serial.print(inputString);
    Serial.print("\n");

    Serial.print("value: ");
    Serial.print(value);
    Serial.print("\n");

    //reset parameters
    valString = "";
    inputString = "";
    stringEnd = false;
  }

  //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
  // + - + - + - + - + - + COMMAND INTERPRETER + - + - + - + - + - + //
  //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//

  //  LIST OF COMMANDS
  //  command; value~
  //
  //  platPPS; pulses per second~
  //  platDir; direction (0/1)~
  //  platPPR; pulses per revolution~
  //  vertPPS; pulses per seconde~
  //  vertDir; direction (0/1)~
  //  vertPPR; pulses per revolution~
  //  setRamp; ramp time in milliseconds~
  //  getPlatPos; returns platform position in steps, no value (0)~
  //  setLayer; layer height in millimeters~
  //
  //  init; no value (0)~
  //  slowAll; no value (0)~
  //  stopAll; no value (0)~
  //  runspd; no value (0)~
  //  runprint; no value(0)~
  //  incPlat; no value (0)~
  //  incVert; no value (0)~
  //
  //  enAll; no value (0)~
  //  enPlat; no value (0)~
  //  enVert; no value (0)~
  //  disPlat; no value (0)~
  //  disVert; no value (0)~


  //prototype
  //    else if (commandString == "command") {
  //      doStuff
  //
  //      commandString = "";
  //    }


  //-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-//
  // -~-~-~-~-~-~-~-~-~- PARAMETER CONFIGURATION -~-~-~-~-~-~-~-~-~- //
  //-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-//

  //set platform pulses per second
  if (commandString == "platPPS") {
    dplatPPS = value; //set platform pulses per second
    platPeriod = (1000000 / dplatPPS); //setplatform period in microseconds

    Serial.print("platPPS: ");
    Serial.print( dplatPPS);
    Serial.print("\n\r");
    Serial.print("platPeriod: ");
    Serial.print( platPeriod);
    Serial.print("\n\r");

    commandString = ""; //clear string
  }

  //set platform direction
  else if (commandString == "platDir") {

    //set direction parameter for platform motor
    if (value > 0) {
      platDirBool = true;
    } else {
      platDirBool = false;
    }

    setDir(); //activate direction change function

    commandString = ""; //clear string
  }

  //set platform pulses per revolution
  else if (commandString == "platPPR") {
    platPPR = value; //set platform pulses per period

    commandString = ""; //clear string
  }

  //set vertical ppulses per second
  else if (commandString == "vertPPS") {
    dvertPPS = value; //set vertical pulses per second
    vertPeriod = (1000000 / dvertPPS); //setplatform period in microseconds
    Serial.print("vertPPS: ");
    Serial.print( dvertPPS);
    Serial.print("\n\r");

    commandString = ""; //clear string
  }

  //set vertical direction
  else if (commandString == "vertDir") {

    //set direction parameter for vertical motor
    if (value > 0) {
      vertDirBool = true;
    } else {
      vertDirBool = false;
    }

    setDir(); //activate direction change function

    commandString = ""; //clear string
  }

  //set vertical pulsesper revolution
  else if (commandString == "vertPPR") {
    vertPPR = value; //set vertical pulses per revolution

    commandString = ""; //clear string
  }

  //set ramptime in [ms]
  else if (commandString == "setRamp") {
    rampTime = value; //set ramp time
    Serial.print("rampTime [ms]: ");
    Serial.print(rampTime);
    Serial.print("\n\r");

    commandString = ""; //clear string
  }

  else if (commandString == "setInterval") {
    rampInterval = value;

    commandString = "";
  }

  else if (commandString == "getPlatPos") {
    Serial.println(platPos);

    commandString = "";
  }

  else if (commandString == "setLayer") {
    layerHeight = value;

    commandString = "";
  }

  //-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~//
  // -~-~-~-~-~-~-~-~-~- MODE SELECTION -~-~-~-~-~-~-~-~-~- //
  //-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~//

  if (commandString == "init") {
    initBool = true;
    disableBool = false;
    mode = Homing;

    commandString = "";
  }

  //slow all motors
  else if (commandString == "slowAll") {
    slowBool = true;
    runspdBool = false;
    printBool = false;

    //rampi=rampSteps;

    commandString = "";
  }

  //emergency stop all motors
  else if (commandString == "stopAll") {
    disableBool = true;
    slowBool = false;
    runspdBool = false;
    mode = DisableAll;
    Serial.println("disabled");

    commandString = ""; //clear string
  }

  //run platform based on speed
  else if (commandString == "runspd") {

    if (!runspdBool) {
      //reset parameters
      cplatPPS = 1;
      cvertPPS = 1;
    }

    // if both motors are disabled, enable them
    if (!platBool && !vertBool) {
      platBool = true;
      vertBool = true;
    }


    runspdBool = true;
    slowBool = false;
    disableBool = false;
    mode = Speed;


    //rampi=0;

    commandString = ""; //clear string
  }

  else if (commandString == "runprint") {
    if (!printBool) {
      //reset parameters
      cplatPPS = 1;
    }
    printBool = true;
    slowBool = false;
    disableBool = false;
    mode = Print;

    commandString = "";
  }

  //send a pulse to platform motor
  else if (commandString == "incPlat") {

    incPlatBool = true;
    disableBool = false;
    mode = StepPlatform;

    commandString = ""; //clear string
  }

  //send a pulse to vertical motor
  else if (commandString == "incVert") {
    incVertBool = true;
    disableBool = false;
    mode = StepVertical;

    commandString = ""; //clear string
  }

  // ENABLE/DISABLE MOTORS
  else if (commandString == "enAll") {
    platBool = true;
    vertBool = true;
    digitalWrite (enPinPlat, HIGH);
    digitalWrite (enPinVert, HIGH);

    if (!runspdBool) {
      //reset parameters
      cplatPPS = 1;
      cvertPPS = 1;
    }

    commandString = "";
  }

  else if (commandString == "enPlat") {
    platBool = true;
    digitalWrite (enPinPlat, HIGH);

    if (!runspdBool) {
      //reset parameters
      cplatPPS = 1;
    }

    commandString = "";
  }

  else if (commandString == "enVert") {
    vertBool = true;
    digitalWrite (enPinVert, HIGH);
    if (!runspdBool) {
      //reset parameters
      cvertPPS = 1;
    }

    commandString = "";
  }

  else if (commandString == "disPlat") {
    platBool = false;
    digitalWrite (enPinPlat, LOW);

    commandString = "";
  }

  else if (commandString == "disVert") {
    vertBool = false;
    digitalWrite (enPinVert, LOW);

    commandString = "";
  }

  //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
  // + - + - + - + END OF INTERPRETER + - + - + - + //
  //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//

  calcRampi();

  if (platPeriod > vertPeriod) {
    stepDelay = vertPeriod / 2;
  }
  else {
    stepDelay = platPeriod / 2;
  }

  if (stepDelay < 10) {
    stepDelay = 10;
  }

  rampBool = true;
}


// ------- MODE FUNCTIONS ------- //
void runspd() {

  platRampPeriod = (1000000 / cplatPPS); // pulse period of platform motor
  vertRampPeriod = (1000000 / cvertPPS);  //pulse period of vertical motor
  // ^ period will increase with respect to rampi and will eventually reach value set by the user ^

  //  ~ ~ ~ ~ PLATFORM STEP ~ ~ ~ ~   //
  if ( (cMicros - pTimePlatform >= platRampPeriod) && platBool ) {
    stepPlatform();
    pTimePlatform = cMicros;
  }

  //  ~ ~ ~ ~ VERTICAL STEP ~ ~ ~ ~   //
  if ( (cMicros - pTimeVert >= vertRampPeriod) && vertBool ) {
    stepVertical();
    pTimeVert = cMicros;
  }

  //  ~ ~ ~ ~ RAMP STEP ~ ~ ~ ~   //
  if ( (cMillis - pMillis >= rampInterval) && rampBool) {

    if (rampi < rampSteps) {
      cplatPPS = cplatPPS + platFreqStep;
      cvertPPS = cvertPPS + vertFreqStep;
      rampi++;
    }

    if (rampi == rampSteps) {
      cplatPPS = dplatPPS;
      cvertPPS = dvertPPS;
      Serial.println("max speed");
      rampBool = false;
    }

    pMillis = cMillis; // update previous time

  }

}

void runprint() {

  platRampPeriod = (1000000 / cplatPPS); // pulse period of platform motor
  // ^ period will increase with respect to rampi and will eventually reach value set by the user ^

  //  ~ ~ ~ ~ PLATFORM STEP ~ ~ ~ ~   //
  if ( (cMicros - pTimePlatform >= platRampPeriod) && platBool ) {
    stepPlatform();
    pTimePlatform = cMicros;
  }

  //  ~ ~ ~ ~ VERTICAL STEP ~ ~ ~ ~   //
  if ( (cMicros - pTimeVert >= 300) && dropBool && vertBool ) {

    if (dvertPosmm - vertPosmm > 0) {
      stepVertical();

    } else {
      dropBool = false;
    }
    pTimeVert = cMicros;
  }

  //  ~ ~ ~ ~ RAMP STEP ~ ~ ~ ~   //
  if ( (cMillis - pMillis >= rampInterval) && rampBool) {

    if (rampi < rampSteps) {
      cplatPPS = cplatPPS + platFreqStep;
      rampi++;
    }

    if (rampi == rampSteps) {
      cplatPPS = dplatPPS;
      Serial.println("max speed");
      rampBool = false;
    }

    pMillis = cMillis; // update previous time

  }

}

void slowAll() {

  platRampPeriod = (1000000 / cplatPPS); // pulse period of platform motor
  vertRampPeriod = (1000000 / cvertPPS);  //pulse period of vertical motor
  // ^ period will increase with respect to rampi and will eventually reach value set by the user ^

  if ( (cMicros - pTimePlatform >= platRampPeriod) && platBool ) {
    PORTD = PORTD ^ B00100000;
    delayMicroseconds(stepDelay);
    PORTD = PORTD | B00100000;
    pTimePlatform = cMicros;

  }

  if ( (cMicros - pTimeVert >= vertRampPeriod) && vertBool ) {
    PORTB = PORTB ^ B00000010;
    delayMicroseconds(stepDelay);
    PORTB = PORTB | B00000010;
    pTimeVert = cMicros;

  }

  if (cMillis - pMillis >= rampInterval && rampBool) {

    if (rampi > 1) {
      cplatPPS = cplatPPS - platFreqStep;
      cvertPPS = cvertPPS - vertFreqStep;
      rampi--;
    }

    if (rampi == 1) {
      cplatPPS = 1;
      cvertPPS = 1;
      rampBool = false;
      slowBool = false;
      disableBool = true;
      mode = DisableAll;
      Serial.println("stopped");
    }

    pMillis = cMillis;

  }



}

void homing() {


  topEndVal = analogRead(topEndPin);
  topEndBool= switchValtoBool(topEndVal);


  if (H == 0) {
    //Serial.println("H=0");
    digitalWrite(dirPinVert, LOW);
    digitalWrite(dirPinVertgnd, HIGH);

    if ( (cMicros - pTimeVert >= 300) && vertBool ) {
      stepVertical();
      pTimeVert = cMicros;
    }

    if (topEndBool) {
      H = 1;
    }

  }

  if (H == 1) {
    digitalWrite(enPinVert, LOW);
    digitalWrite(dirPinVert, HIGH);
    digitalWrite(dirPinVertgnd, LOW);
    digitalWrite(enPinVert, HIGH);
    delay(10);

      for (int i = 0; i < 600; i++) {
        Serial.println(topEndBool);
          PORTB = PORTB ^ B00000010;    //SWITCH POLARITY OF PIN 9, THIS IS FASTER THAN DIGITALWRITE()
          delayMicroseconds(10);
          PORTB = PORTB | B00000010;    //SWITCH POLARITY OF PIN 9, THIS IS FASTER THAN DIGITALWRITE()

        delayMicroseconds(300);
      
      }
      H=2;
  }

  if (H == 2) {
    Serial.println("H=2");
    vertPos = 0;
    initBool = false;
    mode = DisableAll;
    H = 0;
  }

}


// ------- PIN MANIPULATION ------- //
void setDir() {

  digitalWrite(enPinPlat, LOW);
  digitalWrite(enPinVert, LOW);

  if (platDirBool) {
    digitalWrite(dirPinPlat, HIGH);
    digitalWrite(dirPinPlatgnd, LOW);

  } else {
    digitalWrite(dirPinPlat, LOW);
    digitalWrite(dirPinPlatgnd, HIGH);
  };
  
  if (vertDirBool) {
    digitalWrite(dirPinVert, HIGH);
    digitalWrite(dirPinVertgnd, LOW);
  }
  else {
    digitalWrite(dirPinVert, LOW);
    digitalWrite(dirPinVertgnd, HIGH);
  };
  

  digitalWrite(enPinPlat, HIGH);
  digitalWrite(enPinVert, HIGH);

}

void stepPlatform() {

  //toggle pulpin for one step
  PORTD = PORTD ^ B00100000;    //SWITCH POLARITY OF PIN 5, THIS IS FASTER THAN DIGITALWRITE()
  delayMicroseconds(stepDelay);
  PORTD = PORTD | B00100000;    //SWITCH POLARITY OF PIN 5, THIS IS FASTER THAN DIGITALWRITE()

  if (platDirBool) {
    if (platPos >= platPPR) {
      platPos = 0;
      revs++;
      if (revs % platRed == 0 && vertBool) {
        dropBool = true;
        dvertPosmm = vertPosmm + layerHeight;
      }
    }
    platPos++;
  }

  else {
    if (platPos == 0) {
      platPos = platPPR;
      revs--;
      if (revs % platRed == 0 && vertBool) {
        dropBool = true;
        dvertPosmm = vertPosmm + layerHeight;
      }
    }
    platPos--;
  }

}

void stepVertical() {

  //toggle pulpin for one step
  PORTB = PORTB ^ B00000010;    //SWITCH POLARITY OF PIN 9, THIS IS FASTER THAN DIGITALWRITE()
  delayMicroseconds(10);
  PORTB = PORTB | B00000010;    //SWITCH POLARITY OF PIN 9, THIS IS FASTER THAN DIGITALWRITE()
  if (vertDirBool) {
    vertPos++;
  }
  else {
    vertPos--;
  };
  vertPosmm = ((double)vertPos / vertPPR) * spindlePitch;

}


// ------- MISC. FUNCTIONS ------- //
int calcRampi() {

  if (runspdBool) {
    rampSteps = rampSteps;
    platFreqStep = (dplatPPS - cplatPPS) / rampSteps;
    vertFreqStep = (dvertPPS - cvertPPS) / rampSteps;
    rampi = 1;
    Serial.println(platFreqStep);

  }
  else if (slowBool) {
    rampSteps = rampSteps;
    platFreqStep = cplatPPS / rampSteps;
    vertFreqStep = cvertPPS / rampSteps;
  }
  else {
    rampSteps =  (rampTime / rampInterval);
    platFreqStep = dplatPPS / rampSteps;
    vertFreqStep = dvertPPS / rampSteps;

  }

}

bool switchValtoBool(int switchVal){
  if (switchVal>650){ return true;}
  else {return false;}
}

void checkEndSwitches() {
  topEndVal = analogRead(topEndPin);
  topEndBool= switchValtoBool(topEndVal);
  
  bottomEndVal = analogRead(bottomEndPin);
  bottomEndBool= switchValtoBool(bottomEndVal);

  if (topEndBool) {
    Serial.println("topSwitch");
    vertDirBool = true;
    setDir();
    
    while(topEndBool) {
      topEndVal = analogRead(topEndPin);
      topEndBool= switchValtoBool(topEndVal);
      stepVertical();
      delayMicroseconds(300);

    }

//    for (int i=0; i<600; i++) {
//      topEndVal = analogRead(topEndPin);
//      topEndBool= switchValtoBool(topEndVal);
//      stepVertical();
//      delayMicroseconds(300);
//
//    }
    mode = DisableAll;
  }

  if (bottomEndBool && !vertDirBool) {
    Serial.println("bottomSwitch");
    vertDirBool = false;
    setDir();
    while (bottomEndBool) {
      bottomEndVal = analogRead(bottomEndPin);
      bottomEndBool= switchValtoBool(bottomEndVal);
      stepVertical();
      delayMicroseconds(300);
    }
    mode = DisableAll;

  }


}

