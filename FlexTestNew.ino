//This is a restart to attempt to make the code make sense.  Get rid of the 10X repetitious if's
#include <EEPROMex.h>
// define Solenoids
#define Sol_0_Taut 2
#define Sol_1_Taut 3
#define Sol_0_Loose 4
#define Sol_1_Loose 5
#define SolON HIGH
#define SolOFF LOW
// Define Sensors
#define Sns48V 6
#define SnsRED 7
#define SnsIR 8
#define Sns5V 9
#define SnsGND 10
//define Drivers
#define DrvPin 11   //no need to have in structure
//#define RtDrv 49

long int CycleCnt = 0;
#define numtraces 5
#define numcables 2
char OutString[150];   //for sprintf

#define LOOSEMS 500
#define TAUTMS  25
#define LOOSE false
#define TAUT true

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
  int SolTautPin;       // solonoid to pull tight
  int SolLoosePin;       //solonoid to push loose
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

  if (tstCnt > 0){  //Has been running?
    EEPROM.readBlock (addrStruc, cablex[2]);
  }
  else {         // zero or 0xFFFFFFF 
    strcpy( cablex[0].traceID[0], "48V_0");
    strcpy( cablex[0].traceID[1], "RED_0");
    strcpy( cablex[0].traceID[2], "IR_0");
    strcpy( cablex[0].traceID[3], "Pwr5V_0");
    strcpy( cablex[0].traceID[4], "PwrGND_0");
  
    strcpy( cablex[1].traceID[0], "48V_1");
    strcpy( cablex[1].traceID[1], "RED_1");
    strcpy( cablex[1].traceID[2], "IR_1");
    strcpy( cablex[1].traceID[3], "Pwr5V_1");
    strcpy( cablex[1].traceID[4], "PwrGND_1");
  
    cablex[0].SolLoosePin = Sol_0_Loose;
    cablex[1].SolLoosePin = Sol_1_Loose;
    cablex[0].SolTautPin = Sol_0_Taut;
    cablex[1].SolTautPin = Sol_1_Taut;

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
  }
}
             
    


void InitFlexBd(void) {  //initializesn all the pins; set up Structs
    

  //set up solenoid drivers as outputs (& High= Denergized)

  //  pinMode(Sol_Left, OUTPUT);
  //  pinMode(Sol_Right, OUTPUT);
  //  digitalWrite(Sol_Left, SolOFF); //actually HIGH turns off
  //  digitalWrite(Sol_Right, SolOFF); //actually HIGH turns off
}

void setup() {
  // put your setup code here, to run once:
  char CntStr[30];    //for diag out
  int i = 0;
  long int Elong = 0;
  
  EEPROM.setMaxAllowedWrites (10000);    //default is 100
  EEPROM.setMemPool(0, EEPROMSizeMega);
  Serial.begin(115200);
  Serial.println("Starting.......");
  Serial.print("EEPROMSizeMega ="); Serial.println(EEPROMSizeMega);
 //  uncomment the following to reset EEPROM
    for (int i = 0 ; i < EEPROMSizeMega ; i++) {
      Serial.print(" writing at i = "); Serial.println(i);
      EEPROM.updateByte(i, 0xFF);
    }

  Serial.println("-----------------------------------");
  Serial.println("Following adresses have been issued");
  Serial.println("-----------------------------------");

  Serial.println("begin address \t\t size");
  Serial.print(addrCycleCnt); Serial.print(" \t\t\t "); Serial.print(sizeof(CycleCnt)); Serial.println(" (long)");
  Serial.print(addrStruc); Serial.print(" \t\t\t "); Serial.print(sizeof(addrStruc));  Serial.println(" (structure)");
  Serial.print(addrTop); Serial.print(" \t\t\t "); Serial.print(sizeof(top)); Serial.println(" (int)--just here to tag the structure");

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

}

//void solenoidsOFF() {
//  digitalWrite(Sol_Left, SolOFF); //HIGH turns ON
//  digitalWrite(Sol_Right, SolOFF); //LOW turns off
//
//}


//void solenoidsON() {
//  digitalWrite(Sol_Left, SolON); //HIGH turns ON
//  digitalWrite(Sol_Right, SolON); //LOW turns off

//}

//void setLeft(void) {
//  // Set left Drive LOW, Right Drive as Input
//  pinMode(LtDrv, OUTPUT);
//  pinMode(RtDrv, INPUT);
//  digitalWrite(LtDrv, LOW);
//}

//void setRight(void) {
//  // Set left Drive LOW, Right Drive as Input
//  pinMode(RtDrv, OUTPUT);
//  pinMode(LtDrv, INPUT);
//  digitalWrite(RtDrv, LOW);
//}

void ckttraces(bool taut) {
  int cs, tnm;
  //bool taut = true;   //fake for development

  //set up solonoids for taut; delay tauttime

  for (int cs = 0; cs <= numcables; cs++) {
    for (tnm = 0; tnm <= numtraces; tnm = tnm++); {
      pinMode(DrvPin, OUTPUT);
      digitalWrite(DrvPin, LOW);   //why not in setup.  Do it once?  We are not driving two independent systems
      pinMode(cablex[cs].pinSns[tnm], INPUT_PULLUP);    //should be anyway
      delay(5); //give it 5 ms to settle.

      if (digitalRead(cablex[cs].pinSns[tnm]) == HIGH) { //floated HIGH==bad
        if (cablex[cs].traces[tnm].trintact == true) {
          cablex[cs].traces[tnm].FirstFail = CycleCnt;
          cablex[cs].traces[tnm].trintact = false;
          if (taut == true)cablex[cs].traces[tnm].trFailTaut = true;
          else cablex[cs].traces[tnm].trFailLoose = true;
        }
      }
      else {        //good (LOW)
        if (cablex[cs].traces[tnm].trintact == false) { //good, but has failed before
          cablex[cs].traces[tnm].LastGood = CycleCnt;
          cablex[cs].traces[tnm].numGoods++;
        }

      }

    }

  }
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
  // solenoidsOFF();
  delay (LOOSEMS);    //wait for loose

  //  ckLtSide(LOOSE);
  //  ckRtSide(LOOSE);
  //Serial.print("\n");

  // solenoidsON();  //goes taut
  delay (TAUTMS); //time to go taut

  //  ckLtSide(TAUT);
  //  ckRtSide(TAUT);
  //  Serial.print("\n");
  //  solenoidsOFF();

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

  //  if (CycleCnt % 500 == 0) {
  //    EEPROM.writeLong(addrCycleCnt, CycleCnt);
  //    EEPROM.writeBlock(addrLeftSide, LtSide);
  //    EEPROM.writeBlock(addrRightSide, RtSide);
  //    Serial.println("-----------------------------------");
  //    Serial.print(" CycleCnt written out at CycleCnt = "); Serial.println(CycleCnt);
  //    Serial.println("-----------------------------------");
  //
  //  }



  //

}
