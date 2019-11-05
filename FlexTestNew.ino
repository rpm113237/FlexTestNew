//This is a restart to attempt to make the code make sense.  Get rid of the 10X repetitious if's
#include <EEPROMex.h>
// define Solenoids
#define Sol_Left 2
#define Sol_Right 3
#define SolON HIGH
#define SolOFF LOW
// Define Sensors
#define Sns48V 23
#define SnsRED 25
#define SnsIR 27
#define Sns5V 29
#define SnsGND 31
//define Drivers
#define LtDrv 47
#define RtDrv 49
//arrays to store how many times rt & left has been cycled
//These will be stored in EEPROM?
//Need way to init???
long int CycleCnt = 0;
//  long int LtCnt[] = {0,0,0,0,0};
//  long int RtCnt[] = {0,0,0,0,0};
char OutString[150];   //for sprintf

enum lines {Plus48, RED, IR, Plus5, GND};



#define LOOSEMS 500
#define TAUTMS  25
#define LOOSE false
#define TAUT true

//  struct linestruct{
//    // contains everything--this an entry in an array of structs
//    char ID[12];    //"48V":, "REDSig", "IRSig", etc.
//    int SnsPin;     //pin to read status Input w pullup
//    int DrvPin;     //Pin to drive low for sense
//    long int FirstFail;  //First Detected failure
//    long int LatestGood; //Last low read
//    int NumIntrmt;    //number of good reads since first fail
//    bool Intact;    //== true if Intact
//    bool fail_on_taut;  // true if failed taut
//    bool fail_on_loose; // true if failed on loose
//  };
//
//  struct linestruct linfo[2];

struct linestatus {
  long int FirstFail;  //number of cycles to first failure
  bool Intact;        // = OPEN or GOOD
  bool FailOnTaut;    //Was it stretched on failure, or slack?
};

struct FlexSide {
  linestatus Pwr48V;
  linestatus SigRED;
  linestatus SigIR;
  linestatus Pwr5V;
  linestatus PwrGnd;
} LtSide, RtSide, TstStruc;

// Always get the adresses first and in the same order
int addrCycleCnt = EEPROM.getAddress(sizeof(CycleCnt));
int addrLeftSide = EEPROM.getAddress(sizeof(LtSide));
int addrRightSide = EEPROM.getAddress(sizeof(RtSide));

void InitFlexBd(void) {  //initializesn all the pins; set up Structs
  //define sense.  Input Pullup, left/right are driven low
  long int tstCnt = 0;

  pinMode(Sns48V, INPUT_PULLUP);
  pinMode(SnsRED, INPUT_PULLUP);
  pinMode(SnsIR, INPUT_PULLUP);
  pinMode(Sns5V, INPUT_PULLUP);
  pinMode(SnsGND, INPUT_PULLUP);


  tstCnt = EEPROM.readLong(addrCycleCnt);

  if (tstCnt > 0) {
    EEPROM.readBlock (addrLeftSide, LtSide);
    EEPROM.readBlock (addrRightSide, RtSide);

  }

  else {  // the defaults

    LtSide.Pwr48V = {0, true, true};
    RtSide.Pwr48V = {0, true, true};

    LtSide.SigRED = {0, true, true};
    RtSide.SigRED = {0, true, true};

    LtSide.SigIR = {0, true, true};
    RtSide.SigIR = {0, true, true};

    LtSide.Pwr5V = {0, true, true};
    RtSide.Pwr5V = {0, true, true};

    LtSide.PwrGnd = {0, true, true};
    RtSide.PwrGnd = {0, true, true};
  }
  // set up drivers as inputs--they change to outputs

  pinMode(LtDrv, INPUT);
  pinMode(RtDrv, INPUT);

  //set up solenoid drivers as outputs (& High= Denergized)

  pinMode(Sol_Left, OUTPUT);
  pinMode(Sol_Right, OUTPUT);
  digitalWrite(Sol_Left, SolOFF); //actually HIGH turns off
  digitalWrite(Sol_Right, SolOFF); //actually HIGH turns off
}

void setup() {
  // put your setup code here, to run once:
  char CntStr[30];    //for diag out
  int i = 0;
  long int Elong = 0;
  Serial.begin(115200);
  Serial.println("Starting.......");

  // uncomment the following to reset EEPROM
  //  for (int i = 0 ; i < EEPROMSizeMega ; i++) {
  //    EEPROM.write(i, 0xFF);
  //  }

  //   // Always get the adresses first and in the same order
  //    int addrCycleCnt      = EEPROM.getAddress(sizeof(CycleCnt));
  //    int addrLeftSide    = EEPROM.getAddress(sizeof(LtSide));
  //    int addrRightSide      = EEPROM.getAddress(sizeof(RtSide));


  Serial.println("-----------------------------------");
  Serial.println("Following adresses have been issued");
  Serial.println("-----------------------------------");

  Serial.println("begin address \t\t size");
  Serial.print(addrCycleCnt);      Serial.print(" \t\t\t "); Serial.print(sizeof(CycleCnt)); Serial.println(" (long)");
  Serial.print(addrLeftSide);       Serial.print(" \t\t\t "); Serial.print(sizeof(LtSide));  Serial.println(" (struc RtSide)");
  Serial.print(addrRightSide);      Serial.print(" \t\t\t "); Serial.print(sizeof(RtSide)); Serial.println(" (struc LtSide)");

  Elong = EEPROM.readLong(addrCycleCnt);
  Serial.print( "CycleCnt at init = "); Serial.println( Elong);
  if (Elong > 0) {          //Elong is
    sprintf(CntStr, "Stored Count at start up = %ld ", Elong);
    Serial.println (CntStr);
    Serial.print ("length of CntStr = "); Serial.println (sizeof(CntStr));
    CycleCnt = Elong;
  }
  else {
    CycleCnt = 0;   //initialize to memory?
    EEPROM.writeLong(0, CycleCnt);
  }

  //  Serial.print ("length of lengthstruct = "); Serial.println ( sizeof(linestruct));
  InitFlexBd();
  //  int i =0;
  //  while (i<10) {
  //    i++;
  //    Serial.print( "iteration = ");
  //    Serial.println(i);
  //    solenoidsON();
  //    delay(TAUTMS);
  //    solenoidsOFF();
  //    delay (LOOSEMS);
  //  }




}


void solenoidsOFF() {
  digitalWrite(Sol_Left, SolOFF); //HIGH turns ON
  digitalWrite(Sol_Right, SolOFF); //LOW turns off

}


void solenoidsON() {
  digitalWrite(Sol_Left, SolON); //HIGH turns ON
  digitalWrite(Sol_Right, SolON); //LOW turns off


}

void setLeft(void) {
  // Set left Drive LOW, Right Drive as Input
  pinMode(LtDrv, OUTPUT);
  pinMode(RtDrv, INPUT);
  digitalWrite(LtDrv, LOW);
}

void setRight(void) {
  // Set left Drive LOW, Right Drive as Input
  pinMode(RtDrv, OUTPUT);
  pinMode(LtDrv, INPUT);
  digitalWrite(RtDrv, LOW);
}


void ckLtSide(bool tight) {   //if tight--see loose def's
  char SCnt[30], Icnt[30];

  setLeft();    //set up for left side low, right floating
  //itoa (CycleCnt, SCnt,10);
  if (tight) sprintf  (OutString, "(Taut)  LEFT   Side; Count = %lu ", CycleCnt);
  if (!tight) sprintf  (OutString, "(LOOSE) LEFT   Side; Count = %lu ", CycleCnt);

  //strcat(OutString, SCnt); strcat(OutString, " ");

  if (digitalRead(Sns48V) == HIGH) {    //floated high
    if (LtSide.Pwr48V.Intact == true) {
      LtSide.Pwr48V.FirstFail = CycleCnt;
      LtSide.Pwr48V.Intact = false;
      strcat(OutString, "48V:OPEN; ");
      LtSide.Pwr48V.FailOnTaut = tight;
    }
    else {   //make this a standalone if on first fail to print out firt time.
      strcat(OutString, "48V:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", LtSide.Pwr48V.FirstFail);
      strcat(OutString, Icnt);

    }

  }
  else strcat(OutString, "48V:GOOD; ");


  if (digitalRead(SnsRED) == HIGH) {    //floated high
    if (LtSide.SigRED.Intact == true) {
      LtSide.SigRED.FirstFail = CycleCnt;
      LtSide.SigRED.Intact = false;
      strcat(OutString, "RED: Good; ");
      LtSide.SigRED.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "RED:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", LtSide.SigRED.FirstFail);
      strcat(OutString, Icnt);

    }
  }
  else strcat(OutString, "REDSig: GOOD; ");

  if (digitalRead(SnsIR) == HIGH) {    //floated high
    if (LtSide.SigIR.Intact == true) {
      LtSide.SigIR.FirstFail = CycleCnt;
      LtSide.SigIR.Intact = false;
      strcat(OutString, "IRsig:OPEN ");
      LtSide.SigIR.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "IR:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", LtSide.SigIR.FirstFail);
      strcat(OutString, Icnt);

    }

  }

  else if ( LtSide.SigIR.Intact == true) strcat(OutString, "IRSig:GOOD " );

  if (digitalRead(Sns5V) == HIGH) {    //floated high
    if (LtSide.Pwr5V.Intact == true) {
      LtSide.Pwr5V.FirstFail = CycleCnt;
      LtSide.Pwr5V.Intact = false;
      strcat(OutString, "5V:OPEN ");
      LtSide.Pwr5V.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "5V:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", LtSide.Pwr5V.FirstFail);
      strcat(OutString, Icnt);

    }
  }
  else strcat(OutString, "5V:GOOD ");

  if (digitalRead(SnsGND) == HIGH) {    //floated high
    if (LtSide.PwrGnd.Intact == true) {
      LtSide.PwrGnd.FirstFail = CycleCnt;
      LtSide.PwrGnd.Intact = false;
      strcat(OutString, "GND:OPEN "); //strcat(OutString, itoa(CycleCnt));
      LtSide.PwrGnd.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "GND:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", LtSide.PwrGnd.FirstFail);
      strcat(OutString, Icnt);

    }

  }
  else strcat(OutString, "GND:GOOD"); //strcat(OutString, "\n"); //hang on a cr

  Serial.println (OutString);

}



void ckRtSide(bool tight) {   //if tight--see loose def's
  char SCnt[30], Icnt[10];

  setRight();    //set up for left side low, right floating
  //itoa (CycleCnt, SCnt,10);

  if (tight) sprintf  (OutString, "(Taut)  RIGHT  Side; Count = %lu ", CycleCnt);
  if (!tight) sprintf  (OutString, "(LOOSE) RIGHT  Side; Count = %lu ", CycleCnt);

  //  strcpy (OutString, "RIGHT Side; Count = ");
  //  if (tight) strcpy  (OutString, "(Taut)  RIGHT Side; Count = ");
  //  if (!tight) strcpy (OutString, "(LOOSE) RIGHT Side; Count = ");
  // strcat(OutString, SCnt); strcat(OutString, " ");

  if (digitalRead(Sns48V) == HIGH) {    //floated high
    if (RtSide.Pwr48V.Intact == true) {
      RtSide.Pwr48V.FirstFail = CycleCnt;
      RtSide.Pwr48V.Intact = false;
      strcat(OutString, "48V:OPEN; ");
      RtSide.Pwr48V.FailOnTaut = tight;
    }
    else {   //make this a standalone if on first fail to print out firt time.
      strcat(OutString, "48V:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", RtSide.Pwr48V.FirstFail);
      strcat(OutString, Icnt);
    }

  }
  else strcat(OutString, "48V:GOOD; ");


  if (digitalRead(SnsRED) == HIGH) {    //floated high
    if (RtSide.SigRED.Intact == true) {
      RtSide.SigRED.FirstFail = CycleCnt;
      RtSide.SigRED.Intact = false;
      strcat(OutString, "Redsig: Good; ");
      RtSide.SigRED.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "RED:OPEN @ Cnt = ");
      itoa(RtSide.SigRED.FirstFail, SCnt, 10);
      sprintf(Icnt, "%ld ", RtSide.SigRED.FirstFail);
      strcat(OutString, Icnt);

    }
  }
  else strcat(OutString, "RedSig: GOOD; ");

  if (digitalRead(SnsIR) == HIGH) {    //floated high
    if (RtSide.SigIR.Intact == true) {
      RtSide.SigIR.FirstFail = CycleCnt;
      RtSide.SigIR.Intact = false;
      strcat(OutString, "IRsig:OPEN ");
      RtSide.SigIR.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "IR:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", RtSide.SigIR.FirstFail);
      strcat(OutString, Icnt);

    }

  }

  else if ( RtSide.SigIR.Intact == true) strcat(OutString, "IRSig:GOOD " );

  if (digitalRead(Sns5V) == HIGH) {    //floated high
    if (RtSide.Pwr5V.Intact == true) {
      RtSide.Pwr5V.FirstFail = CycleCnt;
      RtSide.Pwr5V.Intact = false;
      strcat(OutString, "5V:OPEN ");
      RtSide.Pwr5V.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "5V:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", RtSide.Pwr5V.FirstFail);
      strcat(OutString, Icnt);

    }
  }
  else if (RtSide.Pwr5V.Intact == true) strcat(OutString, "5V:GOOD  ");

  if (digitalRead(SnsGND) == HIGH) {    //floated high
    if (RtSide.PwrGnd.Intact == true) {
      RtSide.PwrGnd.FirstFail = CycleCnt;
      RtSide.PwrGnd.Intact = false;
      strcat(OutString, "GND:OPEN at Cnt =  "); //strcat(OutString, itoa(CycleCnt));
      RtSide.PwrGnd.FailOnTaut = tight;
    }
    else {
      strcat(OutString, "GND:OPEN @ Cnt = ");
      sprintf(Icnt, "%ld ", RtSide.PwrGnd.FirstFail);
      strcat(OutString, Icnt);

    }

  }
  else strcat(OutString, "GND:GOOD"); //strcat(OutString, "\n"); //hang on a cr

  Serial.println (OutString);


}




void tstLines(void)
{

  /*
    Deenergize both solenoids; wait LOOSEMS ms

    Set left Drive LOW, Right Drive as Input
    Test each line for LOW; record results
    Switch to Right Drive LOW, Left Drive as Input
    record failures as flex
    Test inputs,record results

    Energize both solenoids (which pulls taut), wait TAUTMS
    Repeat testa, both sides
    record failures as taut
    Denergize both solenoids

    output results
  */
  CycleCnt++;
  solenoidsOFF();
  delay (LOOSEMS);    //wait for loose

  ckLtSide(LOOSE);
  ckRtSide(LOOSE);
  Serial.print("\n");

  solenoidsON();  //goes taut
  delay (TAUTMS); //time to go taut

  ckLtSide(TAUT);
  ckRtSide(TAUT);
  Serial.print("\n");
  solenoidsOFF();

}



void loop() {
  // put your main code here, to run repeatedly:

  /*fire both solenoids (which pulls taut), wait
     Then set left Drive LOW, Right Drive as Input
     Test each line for LOW; record results
     Switch to Right Drive LOW, Left Drive as Input
     Test inputs,record results
     unfire both solenoids, wait longer
     Repeat testa, both sides
     record whether failure occurs on taut or flex
     output results
  */


  tstLines();

  if (CycleCnt % 500 == 0) {
    EEPROM.writeLong(addrCycleCnt, CycleCnt);
    EEPROM.writeBlock(addrLeftSide, LtSide);
    EEPROM.writeBlock(addrRightSide, RtSide);
    Serial.println("-----------------------------------");
    Serial.print(" CycleCnt written out at CycleCnt = "); Serial.println(CycleCnt);
    Serial.println("-----------------------------------");

  }



  //

}