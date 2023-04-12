//Passap knitting mashine e6000 motor drive e4600
//AT89S51 (to replace the original 8051 chip)
//less than 2 Kbytes of object code, Keil C51, Keil evaluation licence
//conditions: no timers (some old e4600 circuits have problems with quartz),
//  only 5 button and 4 LEDs of original operator control panel,

#include <reg51.h>

//flags
bit DelayFlag = 0; // 0 for debug / 1 for knit
bit BU8_3_flag = 0; //default -0- NO PC(excel)control //(!BU8_3_00)bit lpt high - stop
bit colorpermanent = 0;
bit PASSAP_ConsoleControlBit = 0; // default 0 - no Console control

//peripherals
sbit BU8_3_00 =         P0 ^ 0; //pedal switch //PC (EXCEL) control bit
sbit BU8_2_01 =         P0 ^ 1;
sbit BU5_2_02 =         P0 ^ 2; //yarn break control
sbit LStopRelay_03 =    P0 ^ 3; //left edge stop
sbit RStopRelay_04 =    P0 ^ 4; //right edge stop
sbit CoLStopRelay_05 =  P0 ^ 5; //color edge stop
sbit Delayflaginfo_06 = P0 ^ 6; //delay_flag info
sbit ULN_14_07 =        P0 ^ 7; //pedal - motorstatus

sbit n1_KnGoStop_10 =   P1 ^ 0; //key go/stop
sbit LED4_OK_11 =       P1 ^ 1;
sbit LED3_Col_12 =      P1 ^ 2;
sbit LED2_L_13 =        P1 ^ 3;
sbit LED1_R_14 =        P1 ^ 4;
sbit BU7_2_15 =         P1 ^ 5; //autocolor detection
sbit R_ULN_16 =         P1 ^ 6; //output to triac Right
sbit L_ULN_17 =         P1 ^ 7; //output to triac Left

sbit n21_KnOK_20 =   P2 ^ 0; //key OK
sbit Revers_21 =     P2 ^ 1; //key change sence of traveling
sbit M_Col_22 =      P2 ^ 2; //key COL
sbit M_Stop_23 =     P2 ^ 3; //key stop at the end of row
sbit Nothing_24 =    P2 ^ 4;
sbit Nothing_L_25 =  P2 ^ 5; // L
sbit Nothing_R_26 =  P2 ^ 6; // R
sbit Nothing_C_27 =  P2 ^ 7; // C

sbit BU1_3_0_30 =    P3 ^ 0; //serial
sbit BU1_3_1_31 =    P3 ^ 1; //serial
sbit Interrupt0_32 = P3 ^ 2; //interrupt 0 for falling edge on /INT0 (P3.2)
sbit NullPoint_33 =  P3 ^ 3; //interrupt INT1
sbit BU1_3_34 =      P3 ^ 4; // P3.4 (T0 input)
sbit BU1_3_35 =      P3 ^ 5; // P3.5 (T1 input)
sbit BU2_3_36 =      P3 ^ 6; //E6000/1 immediate error - console_BU2_1
sbit BU2_4_37_Start_not_possible = P3 ^ 7; //E6000/2 start not possible - console BU2_2

bit tmp;
bit avostStopKeyWasPressed = 0;

bit a;
bit avost = 0;
bit nextRowStop = 1;
bit BU8_3_inverting = 0;
char bdata counter;
sbit  counter_1_COL = counter ^ 0;
sbit  counter_2_R   = counter ^ 1;
sbit  counter_3_L   = counter ^ 2;
sbit  counter_4_OK  = counter ^ 3;

unsigned int diokta = 11; //  diokta=11;
unsigned int pulseDelaySTART = 151 ; //diokta*14
unsigned int numberOfPulsesWhileTheMotorStart = 10;
unsigned int pulseDelaySTOP = 151; //=diokta*14
unsigned int numberOfPulsesBeforeTheMotorStops = 6;
unsigned int pulseDelayFastSTOP = 110;
unsigned int numberOfPulsesNeededExtremelyToFastStopMotor = 5;
unsigned int triacOpeningPulseWidth = 9;
unsigned char dioktaMultiplier = 2;

char counter2;
char i = 10;
unsigned long d;
unsigned long ee;
unsigned char ii;
unsigned int delay;

unsigned int local_count;
unsigned int count50hz = 0;
unsigned int MicrosSec50 = 1;
unsigned int max_time_one_row = 300;
unsigned int count = 0;
unsigned int totalPulsesToStartOrStopInThisCase;
unsigned int pulseDelay;

void BU8_3_control(void);
void SetControlBits(void);
void Prog_Revers_21(void);
void Prog_M_Col_22(void);
void prog_M_Stop_23(void);
void prog_KnOK(void);
void stoprelay (void);
void Knit_R(void);
void Knit_L(void);
void Knit_C(void);
void Begin(void);
void ToStartMotor(void)
void LastRowStop(void) small;//last row stop
void MakeImpactsStartAndStop(void) small;
void WhileKnittingGenProgPaht(void)
void Stop (void)small;
void SetCounterBinaryByControlPanelLeds(void);
void VeryFastMotorStopToMake(void)small;

void DelayMicroSec(unsigned int MicrosSec50) { //1=~ 50 microsec
  local_count = 0;
  ee = MicrosSec50 * DelayFlag;
  while (local_count <= ee) {
    local_count++;
  };
}

void ErrorIndication( int eErrorCode) { //flashing led according to err number
  do {
    local_count = 0;
    do {
      LED4_OK_11 = 1;
      DelayMicroSec(2000);
      LED4_OK_11 = 0;
      DelayMicroSec(4000);
      local_count++;
      if (!n21_KnOK_20)goto metka_EI;
    } while (local_count < eErrorCode);
    DelayMicroSec(8000);
  } while (1);
metka_EI:;
}


void MakeImpact2(void) { //open triac according to delay and length
  Nothing_24 = NullPoint_33;
  d = 0;
  while (d < pulseDelay) {
    d++;
  };
  L_ULN_17 = (LED2_L_13);
  R_ULN_16 = (LED1_R_14);
  d = 0;
  while (d < triacOpeningPulseWidth) {
    d++;
  };
  L_ULN_17 = 0;
  R_ULN_16 = 0;
}

void MakeImpactsStartAndStop(void) {
  count = 0;
  if (!DelayFlag) {
    pulseDelay = 1;
    triacOpeningPulseWidth = 1;
    totalPulsesToStartOrStopInThisCase = 1;
  };
  tmp = NullPoint_33;
  while ( NullPoint_33 == tmp) {  };
  do {
    MakeImpact2();
    if (!BU5_2_02)Stop(); //yarn break
    if (!n1_KnGoStop_10)Stop();
    while (NullPoint_33) {
      Nothing_24 = NullPoint_33;
    };
    MakeImpact2();
    pulseDelay = pulseDelay - ii;
    count++;
    while (!NullPoint_33) {
      Nothing_24 = NullPoint_33;
    };
    if (pulseDelay < 0) return;
  } while (count < totalPulsesToStartOrStopInThisCase);
}

void ToStartMotor(void) { //
  pulseDelay = pulseDelaySTART;
  totalPulsesToStartOrStopInThisCase = numberOfPulsesWhileTheMotorStart;
  MakeImpactsStartAndStop();
}
void LastRowStop(void) { //
  pulseDelay = pulseDelaySTOP;
  totalPulsesToStartOrStopInThisCase = numberOfPulsesBeforeTheMotorStops;
  MakeImpactsStartAndStop();
}
void VeryFastMotorStopToMake(void) {
  R_ULN_16 = 0;
  L_ULN_17 = 0;
  avost = 1;
  pulseDelay = pulseDelayFastSTOP;
  totalPulsesToStartOrStopInThisCase = numberOfPulsesNeededExtremelyToFastStopMotor;
  MakeImpactsStartAndStop();
}

void SetCounterBinaryByControlPanelLeds() {
  while (!n1_KnGoStop_10) {};
  DelayMicroSec(1000);
  counter = 0;
  LED2_L_13 = 1;   LED1_R_14 = 1; LED3_Col_12 = 1; LED4_OK_11 = 0;
  while (n1_KnGoStop_10)  {
    if (!M_Stop_23) {     //1
      LED3_Col_12 = !LED3_Col_12;
      while (!M_Stop_23) {};
    };
    if (!Revers_21) {     //2
      LED1_R_14 = !LED1_R_14;
      while (!Revers_21 ) {};
    };
    if (!M_Col_22) {      //4
      LED2_L_13 = !LED2_L_13;
      while (!M_Col_22) {};
    };

    if (!n21_KnOK_20) {   //8
      LED4_OK_11 = !LED4_OK_11;
      while (!n21_KnOK_20) {};
    };
    DelayMicroSec(2000);
  };
  if (!LED3_Col_12) {
    counter_1_COL = 1;
  };  //1
  if (!LED1_R_14) {
    counter_2_R = 1;
  };     //2
  if (!LED2_L_13) {
    counter_3_L = 1;
  };      //3
  if (!LED4_OK_11) {
    counter_4_OK = 1;
  };    //4
  // debug!!! galochka mean led ne gorit
  counter2 = counter_1_COL;
  counter2 = counter2 + counter_2_R * 2 ;
  counter2 = counter2 + counter_3_L * 4 ;
  counter2 = counter2 + counter_4_OK * 8;
  while (!n1_KnGoStop_10) {
    LED4_OK_11 = 0;
    LED2_L_13 = 1;
    LED1_R_14 = 1;
    LED3_Col_12 = (counter2 == counter);
  };
  DelayMicroSec(1000);
}

void SettingSettings(void) { //to find optimal delays to start / stop motor
  LED2_L_13 =  !LED2_L_13  ;
  DelayMicroSec(1000);
  LED3_Col_12 = !LED3_Col_12;
  DelayMicroSec(2000);
  if (!M_Col_22 & !M_Stop_23) { //pulseDelaySTOP=
    SetCounterBinaryByControlPanelLeds();
    pulseDelaySTOP = diokta * counter; //find 1 - (250 * 6)=1500
  };

  if (!Revers_21 & !M_Stop_23) { // triacOpeningPulseWidth=
    SetCounterBinaryByControlPanelLeds();
    triacOpeningPulseWidth = diokta * counter;
  };
  if (!Revers_21 & !M_Col_22) {
    SetCounterBinaryByControlPanelLeds();
    numberOfPulsesBeforeTheMotorStops = counter; //find 1 - (5 * 1)=5
  };
  //-START------n1_KnGoStop_10
  if (!n1_KnGoStop_10 & !M_Stop_23) {
    SetCounterBinaryByControlPanelLeds();
    pulseDelaySTART = diokta * counter;
  };//4
  if (!n1_KnGoStop_10 & !Revers_21) {
    SetCounterBinaryByControlPanelLeds();
    dioktaMultiplier = counter;
  };
  if (!n1_KnGoStop_10 & !M_Col_22) {
    SetCounterBinaryByControlPanelLeds();
    numberOfPulsesWhileTheMotorStart = counter; //find 1 - (5 * 2)=10
  };
}

void SetControlBits(void) {
  if (!M_Col_22) {
    colorpermanent = !colorpermanent; //go to color changer next second row
  };
  if (!Revers_21) { //BU8_3
    BU8_3_flag = !BU8_3_flag;
  };
  if (!n1_KnGoStop_10) {//0 for debug
    DelayFlag = !DelayFlag;
  };
  if (!n21_KnOK_20) { //ConsoleControl
    PASSAP_ConsoleControlBit = !PASSAP_ConsoleControlBit;
  };
  LED2_L_13 = DelayFlag;
  LED1_R_14 = PASSAP_ConsoleControlBit; // E6000
  LED4_OK_11 = BU8_3_flag;
  LED3_Col_12 = colorpermanent;
  DelayMicroSec(6000); //20170802
}
//sbit n21_KnOK_20=   P2^0; //key OK
//sbit Revers_21=     P2^1; //key change sence of traveling
//sbit M_Col_22=      P2^2; //key COL
//sbit M_Stop_23=     P2^3; //key stop at the end of row

void main (void) { //20170802
  R_ULN_16 = 0;
  L_ULN_17 = 0;
  BU8_3_flag = 0;         //  LED4_OK_11= PC rows control
  colorpermanent = 0;     //  LED3_Col_12
  DelayFlag = 1;          //  LED2_L_13= debug
  PASSAP_ConsoleControlBit = 0; //0- bo control,   LED1_R_14= console error control

  while (M_Stop_23) {
    SetControlBits();
  };
  while (n21_KnOK_20) {
    SettingSettings();
    //Nothing_R_26 =  0;
  };
  while (M_Col_22) {
    if (!n21_KnOK_20) prog_KnOK() ;// control bits and return

    if (!n1_KnGoStop_10) {
      d = 0;
      while (NullPoint_33) {};
      while (!NullPoint_33) {};
      while (NullPoint_33) {
        d++;
      };
      diokta = d / 16;
      ii = diokta * dioktaMultiplier;
      ErrorIndication(diokta);//display diokta (binary) control panel leds
    };
  };
  DelayMicroSec(5000);

  LED4_OK_11 = 0;
  LED3_Col_12 = 0;
  LED2_L_13 =  1;
  LED1_R_14 =  0;
  nextRowStop = 1;
  Begin();
}
void Begin() {

  R_ULN_16 = 0;
  L_ULN_17 = 0;
  DelayMicroSec(3000);
  do {
    if (nextRowStop | avost) {
      nextRowStop = 0;
      avost = 0;
      avostStopKeyWasPressed = 0;
      do {
        if (!Revers_21)   Prog_Revers_21();
        if (!M_Col_22)    Prog_M_Col_22() ;
        if (!M_Stop_23)   prog_M_Stop_23();
        if (!n21_KnOK_20) prog_KnOK()     ;
        if (colorpermanent) LED3_Col_12 = ~LED3_Col_12;
        DelayMicroSec(2000);
      } while (n1_KnGoStop_10);
    };
    do {} while (!n1_KnGoStop_10); //anti-bounce
    if (colorpermanent)LED3_Col_12 = 1;
    count50hz = 0;
    count = 0;

    LED4_OK_11 = 0;
    R_ULN_16 = 0;
    L_ULN_17 = 0;
    if (LED2_L_13)   {
      Knit_L ();
      return;
    };//L
    if (LED1_R_14 & (!LED3_Col_12)) {
      Knit_R ();
      return;
    };//R
    if (LED1_R_14 & ( LED3_Col_12)) {
      Knit_C ();
      return;
    };
  } while (1);
}

void Prog_Revers_21() {//change direction
  LED2_L_13 = ~LED2_L_13;
  LED1_R_14 = ~LED2_L_13;
  while (!Revers_21) {};
}

void Prog_M_Col_22()  {//go to color changer and set color changer permamemt
  LED3_Col_12 = ~LED3_Col_12;
  while (!M_Col_22) {
    if (!Revers_21 ) {
      colorpermanent = ~colorpermanent;
      LED4_OK_11 = ~LED4_OK_11;
      LED3_Col_12 = colorpermanent;
      DelayMicroSec(1000);
    };
    while (!Revers_21) {};
  }
}

void prog_M_Stop_23() {// stop and the end of this row
  nextRowStop = 1;
  LED4_OK_11 = ~LED4_OK_11;
  while (!M_Stop_23) {};
  DelayMicroSec(5000);
}

void reset (void) {
  ((void (code *) (void)) 0x0000) ();
}

void prog_KnOK() { //set and reset
  do {
    if (!Revers_21 & !M_Stop_23) { //SetControlBits();
      LED4_OK_11 = ~LED4_OK_11;
      DelayMicroSec(10000);
      LED4_OK_11 = ~LED4_OK_11;
      do {
        SetControlBits();
      } while (M_Stop_23);
    };
    if (!Revers_21 & !n21_KnOK_20) { //SettingSettings();
      LED4_OK_11 = ~LED4_OK_11;
      DelayMicroSec(10000);
      LED4_OK_11 = ~LED4_OK_11;
      while (n21_KnOK_20) SettingSettings();
    };
    if (!Revers_21 & !n1_KnGoStop_10)reset();//reset programm
    if (!Revers_21 & !M_Col_22) {//just return to kniting
      LED4_OK_11 = ~LED4_OK_11;
      return;
    };
  } while (!Revers_21 & !M_Col_22);
}

void BU8_3_control() {  //(!BU8_3_00)bit lpt high - stop
  //BU8_3 use for external control of e4600
  while (n1_KnGoStop_10) {
    LED4_OK_11 = ~LED4_OK_11; //indicate that stop because of BU8_3
    if (!n1_KnGoStop_10) return;
    if (!n21_KnOK_20) return  ;
    if (!M_Col_22)    Prog_M_Col_22() ;
    DelayMicroSec(1500);
  };  //flash that stop because of BU8_3
}
void Knit_R () {  //RStopRelay_04  R_ULN_16=1;  BU8_3_00
  Nothing_R_26 = 1;
  ToStartMotor();
  R_ULN_16 = 1;
  do {
    WhileKnittingGenProgPaht(); //in fact, knitting
    if (avostStopKeyWasPressed) return;
  }//emergency stop
  while (RStopRelay_04);
  R_ULN_16 = 0;
  while (!RStopRelay_04) {};
  LED1_R_14 = 0;
  LED2_L_13 = 1;
  if (nextRowStop) LastRowStop();
  if (avostStopKeyWasPressed) return;
  if (!BU8_3_flag) return;
  if ( BU8_3_00) BU8_3_control();
}
void Knit_L () {   //LStopRelay_03   L_ULN_17  (!BU8_3_00)
  Nothing_L_25 = 1;
  ToStartMotor();
  L_ULN_17 = 1;
  do {
    WhileKnittingGenProgPaht();
    if (avostStopKeyWasPressed) return;
  }
  while (LStopRelay_03);
  L_ULN_17 = 0;
  while (!LStopRelay_03) {};
  LED2_L_13 = 0;
  LED1_R_14 = 1;
  if (nextRowStop) LastRowStop(); //20170802
  if (avostStopKeyWasPressed) return;
  if (!BU8_3_flag) return;
  if ( !BU8_3_00) BU8_3_control();
}
void Knit_C( ) {  //CoLStopRelay_05  R_ULN_16  BU8_3_00
  Nothing_C_27 = 1;
  ToStartMotor();
  R_ULN_16 = 1;
  do {
    WhileKnittingGenProgPaht();
    if (avostStopKeyWasPressed) return;
  }
  while (CoLStopRelay_05);
  R_ULN_16 = 0;
  while (!CoLStopRelay_05) {};
  LED3_Col_12 = 0;
  LED1_R_14 = 0;
  LED2_L_13 = 1;
  if (nextRowStop) LastRowStop(); //20170802
  if (avostStopKeyWasPressed) return;
  if (!BU8_3_flag) return;
  if ( BU8_3_00) BU8_3_control();
}

unsigned int WhileKnitting() { //errors

  if (a != NullPoint_33 ) {
    count50hz++;
    a = NullPoint_33;
  };
  if ( count50hz == max_time_one_row) { // control max time to knit one row
    Stop ();
    avostStopKeyWasPressed = 1;
    return 6;
  };
  if (BU2_3_36 & PASSAP_ConsoleControlBit) { //immediate error - console_BU2_1
    Stop();
    avostStopKeyWasPressed = 1;
    return 5;
  };

  if (!BU7_2_15) { //autocolor detection - force STOP
    VeryFastMotorStopToMake();
    return 4;
  };
  if (!M_Stop_23) { // need stop at the end of row
    nextRowStop = 1;
  };
  if (!BU5_2_02) { //yarn break
    VeryFastMotorStopToMake();
    avostStopKeyWasPressed = 1;
    return 3;
  };
  if (!n1_KnGoStop_10) {
    VeryFastMotorStopToMake();
    nextRowStop = 1;
    avostStopKeyWasPressed = 1;
    return 0;
  };
  return 0;
}

void WhileKnittingGenProgPaht() { //parh between start and stop motor
  LED4_OK_11 = 0;
  if ((WhileKnitting()) > 0) ErrorIndication(WhileKnitting());
}
void Stop () {
  R_ULN_16 = 0;
  L_ULN_17 = 0;
  avost = 1;
  DelayMicroSec(300); //20170802
}