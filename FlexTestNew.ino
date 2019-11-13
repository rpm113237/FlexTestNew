//This is a restart to attempt to make the code make sense.  Get rid of the 10X repetitious if's
#include <EEPROMex.h>
// define Solenoids
#define Sol_0_Taut 5
#define Sol_1_Taut 7
#define Sol_0_Flex 4
#define Sol_1_Flex 6
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
#define FlexMS 50
#define TAUTMS  50
#define SLACKMS (CYCLEMS/2 - FlexMS - TAUTMS)

#define STORECNT 500   //store the structure modulo this
//#define SLACKMS 500


#define Flex false
#define TAUT true

#define CR Serial.print("\n")
#define TAB Serial.print("\t")
#define SP Serial.print(" ")

struct tracestruct {
  bool trintact;      //true if intact
  bool trFailTaut;    //true if failed taut
  bool trFailFlex;   //true if failed Flex
  long int FirstFail; //Count at which first failed
  long int LastGood;  //Count at which last good
  int  numGoods;       //number of times good since first fail
};

struct tracestruct traces;

struct cablestruct {
  tracestruct traces[numtraces];
  int SolTautPin;     // solonoid to pull tight
  int SolFlexPin;    //solonoid to push Flex
  int DrvPin;         //pin to drive low or make input.
  char traceID[numtraces][15];     //such as "48V 8mm", "48V abc" etc.
  int pinSns[numtraces] = {Sns48V, SnsRED, SnsIR, Sns5V, SnsGND};
  //since can't figure out structur for not and enoughis enough
} cablex[numcables];

//int top;
// Always get the adresses first and in the same order
int addrCycleCnt = EEPROM.getAddress(sizeof(CycleCnt));
int addStruc0 = EEPROM.getAddress(sizeof(cablex[0]));
int addStruc1 = EEPROM.getAddress(sizeof(cablex[1]));
//int addrTop = EEPROM.getAddress(sizeof(top));   //just to see top address

void getStruct() {
  EEPROM.readBlock (addStruc0, cablex[0]);
  EEPROM.readBlock(addStruc1, cablex[1]);
}

void InitStructs() {
  int cs = 0, tr = 0;
  long int tstCnt = 0;

  tstCnt = EEPROM.readLong(addrCycleCnt);
  if (tstCnt > 0) { //Has been running?
    Serial.print ("Stored Count at start up = "); Serial.println(tstCnt);CR;CR;
    CycleCnt = tstCnt;
    getStruct();
    reportResults(true);
    //while(1);
  }
  else {         // stored cnt zero or 0xFFFFFFF
    //init cycle count to zero
    CycleCnt = 0;   //could be == zero or ==0xFF
    EEPROM.writeLong(addrCycleCnt, CycleCnt);
    Serial.print( "CycleCnt initialized to "); Serial.println( CycleCnt);

    //structure stuff that has tp be done individually.
    strcpy( cablex[0].traceID[0], "48V_0");
    strcpy( cablex[0].traceID[1], "RED_0");
    strcpy( cablex[0].traceID[2], "IR_0");
    strcpy( cablex[0].traceID[3], "P5V_0");
    strcpy( cablex[0].traceID[4], "GND_0");

    strcpy( cablex[1].traceID[0], "48V_1");
    strcpy( cablex[1].traceID[1], "RED_1");
    strcpy( cablex[1].traceID[2], "IR_1");
    strcpy( cablex[1].traceID[3], "5V_1");
    strcpy( cablex[1].traceID[4], "GND_1");

    cablex[0].SolFlexPin = Sol_0_Flex;
    cablex[1].SolFlexPin = Sol_1_Flex;
    cablex[0].SolTautPin = Sol_0_Taut;
    cablex[1].SolTautPin = Sol_1_Taut;

    cablex[0].DrvPin = DrvPin_0;
    cablex[1].DrvPin = DrvPin_1;

    for (cs = 0; cs < numcables; cs++) {    //run through  cables
      for (tr = 0; tr < numtraces; tr++) {  //then traces
        cablex[cs].traces[tr].trintact = true;
        cablex[cs].traces[tr].trFailTaut = false;
        cablex[cs].traces[tr].trFailFlex = false;
        //        cablex[cs].traces[tr].FirstFail = 12345678 + tr+ (cs+1)*10;
        cablex[cs].traces[tr].FirstFail = 0;
        cablex[cs].traces[tr].LastGood = 0;
        cablex[cs].traces[tr].numGoods = 0;
      }
    }
    writeStruc();
    //    EEPROM.updateBlock(addStruc0, cablex[1]);
    //    EEPROM.updateBlock(addStruc1, cablex[2]);
    Serial.println(" @ Initialization"); reportResults(true);

  }
}

void writeStruc(void) {
  
  //    Serial.print("\n************Store Struc at Cnt =" ); Serial.println(CycleCnt);
  EEPROM.updateBlock(addStruc0, cablex[0]);
  EEPROM.updateBlock(addStruc1, cablex[1]);
  


}

void reportResults(bool verbose) {
  int cs = 0, tr = 0;
  int i = 0, j = 0;
  char cntstr[15]; //for long counts

  ltoa(CycleCnt, cntstr, 10);
  //Serial.print ("At CycleCnt = "); Serial.println(cntstr);

//  if (verbose) {    //print all the stuff
//    Serial.println("\n**************In reportResults (verbose)***********");
//    for (cs = 0; cs < numcables; cs++) {
//      Serial.print ("FlexPin = "); Serial.print(cablex[cs].SolFlexPin);
//      Serial.print (" TautPin = "); Serial.print(cablex[cs].SolTautPin); CR;
//    }   CR;
//  }


  Serial.print ("**********START Report at CycleCnt = "); Serial.print(cntstr);Serial.println ("**********");
  for (cs = 0; cs < numcables; cs++) {
    ltoa(CycleCnt, cntstr, 10);
    Serial.print("Side = "); Serial.println(cs); 
    for (tr = 0; tr < numtraces; tr++) {
      Serial.print(cablex[cs].traceID[tr]); TAB;
      if (cablex[cs].traces[tr].trintact == true)Serial.print("GOOD\t");
      else {  //not good.
        ltoa(cablex[cs].traces[tr].FirstFail, cntstr, 10); //real
        Serial.print ("OPEN @ "); Serial.print(cntstr); TAB;
        if (cablex[cs].traces[tr].trFailTaut)Serial.print("Fail: Taut\t");
        else if (cablex[cs].traces[tr].trFailFlex)Serial.print("Fail: Flex\t");
      }
      if (cablex[cs].traces[tr].LastGood >= 0) {
        ltoa(cablex[cs].traces[tr].LastGood, cntstr, 10); //real
        Serial.print("Last GOOD @ "); Serial.print(cntstr); TAB;
        Serial.print("Num GOODs "); Serial.print(cablex[cs].traces[tr].numGoods); TAB;
      }
      CR;

    }//end tr for

    CR;
  }//end for cs
  ltoa(CycleCnt, cntstr, 10);
  Serial.print ("**********END Report at CycleCnt = "); Serial.print(cntstr);Serial.println ("**********\n");


}//end report


void setup() {
  // put your setup code here, to run once:
  char CntStr[30];    //for diag out
  int i = 0, j = 0;
  long int Elong = 0;
  bool verbose = true;
  Serial.begin(115200);
  Serial.println("\nStarting Serial.......");

  EEPROM.setMaxAllowedWrites (10000);    //default is 100
  EEPROM.setMemPool(0, EEPROMSizeUno);

  //  while(1);
  //  uncomment the following to reset EEPROM
//    ZeroEEPROM();
  //uncomment to print out EEPROM addresses
  //  PrintEEPROMAddresses();
  InitStructs();

  //set up solenoids as outputs
  for (i = 0; i < numcables; i++) { //cleaner in initstructs???
    pinMode(cablex[i].SolTautPin, OUTPUT);
    //    Serial.print(cablex[i].SolTautPin); SP; Serial.print(i); SP; Serial.println(" Taut Pin At Setup");
    pinMode(cablex[i].SolFlexPin, OUTPUT);
    //    Serial.print(cablex[i].SolFlexPin);  SP; Serial.print(i); SP; Serial.println(" Flex Pin At Setup");
  }
  //  Serial.println("After set solenoids--silly to print here");
  //  reportResults(verbose);    //deubug
}

void solenoidsSet(int side, bool istaut) {
  int i = 0;
  // if istaut, fire taut solenoids for int side
  //if !istaut, fire Flex solenoids for int side
  // turn all solenoids OFF then (side) ON
  //if side <0; means leave all off.
  
  if (side >= 0) {
//        Serial.print("For Side = "); Serial.println(side);
    if (istaut){
      digitalWrite(cablex[side].SolTautPin, SolON);
//      Serial.print("setting TAUT ON, pin = "); Serial.println(cablex[side].SolTautPin);
      }
      
    if (!istaut){
      digitalWrite(cablex[side].SolFlexPin, SolON);   
//      Serial.print("setting Flex ON, pin = "); Serial.println(cablex[side].SolFlexPin);
      }
  }
  else {
    //    Serial.print("Slack");
    for (i = 0; i < numcables; i++) {
      digitalWrite(cablex[i].SolTautPin, SolOFF);
      digitalWrite(cablex[i].SolFlexPin, SolOFF);
    }

  }
  //  CR;
}// end


void setSide(int side) {
  //Init side for test
  int cs = 0;
    
  for (cs = 0; cs < numcables; cs++) pinMode(cablex[cs].DrvPin, INPUT_PULLUP);
  //make everything an input
  pinMode(cablex[side].DrvPin, OUTPUT);
//  Serial.print("set Drive pin Low"); SP; Serial.print(side); SP; Serial.println(cablex[side].DrvPin);
  digitalWrite(cablex[side].DrvPin, LOW);
  delay(1);
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
            Serial.print("cktraces finds high for cable ="); Serial.println(tnm);
        if (cablex[cs].traces[tnm].trintact == true) {
          cablex[cs].traces[tnm].FirstFail = CycleCnt;
          cablex[cs].traces[tnm].trintact = false;
          if (taut == true)cablex[cs].traces[tnm].trFailTaut = true;
          else cablex[cs].traces[tnm].trFailFlex = true;
          //Serial.print ("Do we get end of digital read high? "); Serial.println(cs);
        }// end if == true
      }// end if High
      else {        //good (LOW)
        if (cablex[cs].traces[tnm].trintact == false) { //good, but has failed before
          cablex[cs].traces[tnm].LastGood = CycleCnt;
          cablex[cs].traces[tnm].numGoods++;
        }//end

      }//end else

    }//end for tnm
  
  }//end for cs
  pinMode(cablex[cs].DrvPin, INPUT_PULLUP); //float it

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
  istaut = true;

  for (i = 0; i < numcables; i++) solenoidsSet(i, istaut);  //set all taut
  delay (TAUTMS);
  cktraces(istaut);
  solenoidsSet(-1, istaut);
  delay(SLACKMS);

  istaut = false;
  for (i = 0; i < numcables; i++) solenoidsSet(i, istaut);
  delay (FlexMS);
  cktraces(istaut);
  solenoidsSet(-1, istaut);
  delay(SLACKMS);


  reportResults(false);
  //    if (CycleCnt < 3* STORECNT){    //limit the debug stores
  if (CycleCnt % STORECNT == 0) {
    EEPROM.writeLong(addrCycleCnt, CycleCnt);
    writeStruc();
//    Serial.println("in loop readback = ");
    reportResults(true);
    Serial.println("-----------------------------------");
    Serial.print(" Data Stored at CycleCnt = "); Serial.println(CycleCnt);
    Serial.println("-----------------------------------\n");

  }
  //    }   //end debug
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
  Serial.print(addStruc0); Serial.print(" \t\t\t "); Serial.print(sizeof(cablex[0]));  Serial.println(" (first structure)");
  Serial.print(addStruc1); Serial.print(" \t\t\t "); Serial.print(sizeof(cablex[1]));  Serial.println(" (second structure)");
  //  Serial.print(addrTop); Serial.print(" \t\t\t "); Serial.print(sizeof(top)); Serial.println(" (int)--just here to tag the structure");
}
