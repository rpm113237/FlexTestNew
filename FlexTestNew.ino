//This is a restart to attempt to make the code make sense.  Get rid of the 10X repetitious if's
#include <EEPROMex.h>
// define Solenoids
#define Sol_0_Taut 4
#define Sol_1_Taut 5
#define Sol_0_Loose 6
#define Sol_1_Loose 7
#define SolON HIGH
#define SolOFF LOW
// Define Sensors
#define Sns48V 2
#define SnsRED 3
#define SnsIR 8
#define Sns5V 9
#define SnsGND 10
//define Drivers
#define DrvPin_0 11
#define DrvPin_1 12

long int CycleCnt = 0;
#define numtraces 5
#define numcables 2
char OutString[150];   //for sprintf
#define CYCLEMS 1000
#define LOOSEMS 25
#define TAUTMS  25
#define SLACKMS (CYCLEMS - LOOSEMS - TAUTMS)/2
//#define SLACKMS 500


#define LOOSE false
#define TAUT true

#define CR Serial.print("\n")
#define TAB Serial.print("\t")
#define SP Serial.print(" ")

struct tracestruct {
  bool trintact;      //true if intact
  bool trFailTaut;    //true if failed taut
  bool trFailLoose;   //true if failed loose
  long int FirstFail; //Count at which first failed
  long int LastGood;  //Count at which last good
  int  numGoods;       //number of times good since first fail
};

struct tracestruct traces;

struct cablestruct {
  tracestruct traces[numtraces];
  int SolTautPin;     // solonoid to pull tight
  int SolLoosePin;    //solonoid to push loose
  int DrvPin;         //pin to drive low or make input.
  char traceID[numtraces][15];     //such as "48V 8mm", "48V abc" etc.
  int pinSns[numtraces] = {Sns48V, SnsRED, SnsIR, Sns5V, SnsGND};
  //since can't figure out structur for not and enoughis enough
} cablex[numcables];

int top;
// Always get the adresses first and in the same order
int addrCycleCnt = EEPROM.getAddress(sizeof(CycleCnt));
int addrStruc = EEPROM.getAddress(sizeof(cablex[2]));
int addrTop = EEPROM.getAddress(sizeof(top));

void InitStructs() {
  int cs = 0, tr = 0;
  long int tstCnt = 0;

  tstCnt = EEPROM.readLong(addrCycleCnt);
  if (tstCnt > 0) { //Has been running?
    Serial.print ("Stored Count at start up = %ld "); Serial.println(tstCnt);
    CycleCnt = tstCnt;
    EEPROM.readBlock (addrStruc, cablex[2]);
  }
  else {         // stored cnt zero or 0xFFFFFFF
    //init cycle count to zero
    CycleCnt = 0;   //could be == zero or ==0xFF
    EEPROM.writeLong(0, CycleCnt);
    Serial.print( "CycleCnt initialized to "); Serial.println( CycleCnt);

    //structure stuff that has tp be done individually.
    strcpy( cablex[0].traceID[0], "P48V_0");
    strcpy( cablex[0].traceID[1], "RED_0");
    strcpy( cablex[0].traceID[2], "IR_0");
    strcpy( cablex[0].traceID[3], "P5V_0");
    strcpy( cablex[0].traceID[4], "PGND_0");

    strcpy( cablex[1].traceID[0], "P48V_1");
    strcpy( cablex[1].traceID[1], "RED_1");
    strcpy( cablex[1].traceID[2], "IR_1");
    strcpy( cablex[1].traceID[3], "P5V_1");
    strcpy( cablex[1].traceID[4], "PGND_1");

    cablex[0].SolLoosePin = Sol_0_Loose;
    cablex[1].SolLoosePin = Sol_1_Loose;
    cablex[0].SolTautPin = Sol_0_Taut;
    cablex[1].SolTautPin = Sol_1_Taut;

    cablex[0].DrvPin = DrvPin_0;
    cablex[1].DrvPin = DrvPin_1;

    for (cs = 0; cs < numcables; cs++) {    //run through  cables
      for (tr = 0; tr < numtraces; tr++) {  //then traces
        cablex[cs].traces[tr].trintact = true;
        cablex[cs].traces[tr].trFailTaut = false;
        cablex[cs].traces[tr].trFailLoose = false;
        cablex[cs].traces[tr].FirstFail = 0;
        cablex[cs].traces[tr].LastGood = 0;
        cablex[cs].traces[tr].numGoods = 0;
      }
    }
    EEPROM.updateBlock(addrStruc, cablex[2]);
    Serial.println("Initialization"); reportResults();
  }
}

void reportResults(void) {
  int cs = 0, tr = 0;
  int i = 0, j = 0;
  char cntstr[15]; //for long counts

  ltoa(CycleCnt, cntstr, 10);
  Serial.print ("At CycleCnt = "); Serial.println(cntstr);

  for (cs = 0; cs < numcables; cs++) {
    ltoa(CycleCnt, cntstr, 10);
    Serial.print(".......Side = "); Serial.print(cs); Serial.print (" CycleCnt = "); Serial.print(cntstr);
    Serial.println(" ................");
    //  for (i = 0; i<numtraces; i++){Serial.print(cablex[cs].traceID[i]); TAB;} CR;
    for (tr = 0; tr < numtraces; tr++) {
      Serial.print(cablex[cs].traceID[tr]); TAB;
      if (cablex[cs].traces[tr].trintact == true)Serial.print("GOOD\t");
      else {  //not good.
        ltoa(cablex[cs].traces[tr].FirstFail, cntstr, 10); //real
        Serial.print ("OPEN @ "); Serial.print(cntstr); TAB;
        if (cablex[cs].traces[tr].trFailTaut)Serial.print("Fail: Taut\t");
        else if (cablex[cs].traces[tr].trFailLoose)Serial.print("Fail: Loose\t");
      }
      if (cablex[cs].traces[tr].LastGood >= 0) {
        ltoa(cablex[cs].traces[tr].LastGood, cntstr, 10); //real
        //        ltoa(987654321, cntstr,10);   //for test
        Serial.print("Last GOOD @ "); Serial.print(cntstr); TAB;
        Serial.print("Num GOODs "); Serial.print(cablex[cs].traces[tr].numGoods); TAB;
      }
      CR;

    }//end tr for

    CR;
  }//end for cs

  CR;
}//end report


void setup() {
  // put your setup code here, to run once:
  char CntStr[30];    //for diag out
  int i = 0, j = 0;
  long int Elong = 0;
  Serial.begin(115200);
  Serial.println("Starting.......");

  EEPROM.setMaxAllowedWrites (10000);    //default is 100
  EEPROM.setMemPool(0, EEPROMSizeUno);

  //  while(1);
  //  uncomment the following to reset EEPROM
  //ZeroEEPROM();
  //uncomment to print out EEPROM addresses
  //PrintEEPROMAddresses();
  InitStructs();
  //set up solenoids as outputs
  for (i = 0; i < numcables; i++) { //cleaner in initstructs???
    pinMode(cablex[i].SolTautPin, OUTPUT);
    pinMode(cablex[i].SolLoosePin, OUTPUT);
  }

}

void solenoidsSet(int side, bool istaut) {
  int i = 0;
  // if istaut, fire taut solenoids for int side
  //if !istaut, fire loose solenoids for int side
  // turn all solenoids OFF then (side) ON
  //if side <0; means leave all off.

  

  if (side >= 0) {
    //    Serial.print("For Side = "); Serial.println(side);
    if (istaut)digitalWrite(cablex[side].SolTautPin, SolON);
    //    Serial.print("setting TAUT ON, pin = "); Serial.println(cablex[side].SolTautPin);}
    if (!istaut)digitalWrite(cablex[side].SolLoosePin, SolON);
    //    Serial.print("setting LOOSE ON, pin = "); Serial.println(cablex[side].SolLoosePin);}
  }
   else{
//    Serial.print("Slack");
    for (i = 0; i < numcables; i++) {
      digitalWrite(cablex[i].SolTautPin, SolOFF);
      digitalWrite(cablex[i].SolLoosePin, SolOFF);
  }

}
  //  CR;
}// end


void setSide(int side) {
  //Init side for test
  //
  int cs = 0;
  for (cs = 0; cs < numcables; cs++) pinMode(cablex[cs].DrvPin, INPUT_PULLUP);
  //make everthing an input
  pinMode(cablex[side].DrvPin, OUTPUT);
  digitalWrite(cablex[side].DrvPin, LOW);

}


void cktraces(bool taut) {
  //if taut =
  int cs, tnm;
  //bool taut = true;   //fake for development

  //prereq set up solonoids for taut; delay tauttime

  for (int cs = 0; cs < numcables; cs++) {
    setSide(cs);

    for (tnm = 0; tnm < numtraces; tnm++) {

      pinMode(cablex[cs].pinSns[tnm], INPUT_PULLUP);    //should be anyway
      delay(5); //give it 5 ms to settle.               //move to setup???

      if (digitalRead(cablex[cs].pinSns[tnm]) == HIGH) { //floated HIGH==bad
        //          Serial.print("cktraces finds high for cable ="); Serial.println(tnm);
        if (cablex[cs].traces[tnm].trintact == true) {
          cablex[cs].traces[tnm].FirstFail = CycleCnt;
          cablex[cs].traces[tnm].trintact = false;
          if (taut == true)cablex[cs].traces[tnm].trFailTaut = true;
          else cablex[cs].traces[tnm].trFailLoose = true;
          //Serial.print ("Do we get end of digital read high? "); Serial.println(cs);
        }// end if == true
      }// end if High
      else {        //good (LOW)
        if (cablex[cs].traces[tnm].trintact == false) { //good, but has failed before
          cablex[cs].traces[tnm].LastGood = CycleCnt;
          cablex[cs].traces[tnm].numGoods++;
        }//end

      }//end else

    }//end for

  }

}//end cktraces


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
  int i = 0;    //Old Fortran progammers never die
  bool istaut = true;

  //first, do taut
  CycleCnt++;
  //  Serial.print("CycleCnt (MAIN LOOP) = "); Serial.println (CycleCnt);
  istaut = true;
  for (i = 0; i < numcables; i++) solenoidsSet(i, istaut);  //set all taut
    delay (TAUTMS);
    cktraces(istaut);
    solenoidsSet(-1, istaut);
    delay(SLACKMS);

    istaut = false;
    for (i=0; i<numcables; i++) solenoidsSet(i, istaut);
    delay (LOOSEMS);
    cktraces(istaut);
    solenoidsSet(-1, istaut);
    delay(SLACKMS);
  

  reportResults();

  //  if (CycleCnt % 500 == 0) {
  //    EEPROM.writeLong(addrCycleCnt, CycleCnt);
  //    EEPROM.writeBlock(addrLeftSide, LtSide);
  //    EEPROM.writeBlock(addrRightSide, RtSide);
  //    Serial.println("-----------------------------------");
  //    Serial.print(" CycleCnt written out at CycleCnt = "); Serial.println(CycleCnt);
  //    Serial.println("-----------------------------------");
  //
  //  }

}

void ZeroEEPROM(void) {
  Serial.print("EEPROMSizeUno ="); Serial.println(EEPROMSizeUno);
  for (int i = 0 ; i < EEPROMSizeUno ; i++) {
    //Serial.print(" writing at i = "); Serial.println(i);
    EEPROM.updateByte(i, 0xFF);
  }
}

void PrintEEPROMAddresses(void) {
  Serial.println("-----------------------------------");
  Serial.println("Following adresses have been issued");
  Serial.println("-----------------------------------");

  Serial.println("begin address \t\t size");
  Serial.print(addrCycleCnt); Serial.print(" \t\t\t "); Serial.print(sizeof(CycleCnt)); Serial.println(" (long)");
  Serial.print(addrStruc); Serial.print(" \t\t\t "); Serial.print(sizeof(addrStruc));  Serial.println(" (structure)");
  Serial.print(addrTop); Serial.print(" \t\t\t "); Serial.print(sizeof(top)); Serial.println(" (int)--just here to tag the structure");
}
