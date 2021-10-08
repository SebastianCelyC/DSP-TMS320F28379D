/////////////////////////////////////////////////////////
//LABORATORIO: MUESTREO
//CELY CARO JUAN SEBASTIÁN
////////////////////////////////////////////////////////

#include "F28x_Project.h"
#include "math.h"

#define k 2.0*3.141592/1000.0  //Define args of the sine signal
#define Buffer_Size 1000
#define M N/2

//init variables
unsigned int N = 2;
unsigned int ValorADC;
unsigned int nf;
unsigned int fs = 1;
Uint32 divisor=1,Tf,Ti;

//structures of the functions
void Config_Gpio_Pins(void);
void ConfigureADC(void);
void ConfigureEPWM1(void);
void ConfigureEPWM2(void);
void SetupADCEpwm();
void CambiarFreq(void);
//interrupts
interrupt void adca1_isr(void);
interrupt void ePWM_isr(void);

//Data buffer
Uint16 Buff[Buffer_Size];

void main(void)
{
  //init sytem control
  InitSysCtrl();
  
  //allows interrupts
  DINT;
  InitPieCtrl();
  IER = 0x0000; // Disable CPU interrupts
  IFR = 0x0000; // Clear all CPU interrupt flags
  //
  InitPieVectTable();
  
  //allows interrupts in the table
  EALLOW;
  PieVectTable.ADCA1_INT = &adca1_isr; //function for ADCA interrupt 1
  PieVectTable.EPWM1_INT = &ePWM_isr;
  EDIS;
  
  //call functions
  Config_Gpio_Pins();
  ConfigureADC();
  
  //sync clock
  EALLOW;
  CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
  EDIS;
  
  //call pwm module
  ConfigureEPWM1();
  ConfigureEPWM2();
  
  EALLOW;
  CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
  EDIS;
  
  //configure ADC module
  SetupADCEpwm();
  
  IER |= M_INT1; //ADC
  IER |= M_INT3; //PWM
  EINT; // Enable Global interrupt INTM
  ERTM; // Enable Global real time interrupt DBGM
  
  PieCtrlRegs.PIEIER1.bit.INTx1 = 1;
  PieCtrlRegs.PIEIER3.bit.INTx1 = 1; //PWM
  
  do
  {
  }while(1);
}

//Interrupt to receive the signal
interrupt void adca1_isr(void)
{
  static Uint16 resultsIndex = 0;
  Uint16 Valor_ADC;
  
  //save data in buffer
  Valor_ADC = AdcaResultRegs.ADCRESULT0;
  Buff[resultsIndex++] = Valor_ADC;
  
  if(resultsIndex >= Buffer_Size)
  {
    resultsIndex = 0;
    CambiarFreq();// call the function that change freq
  }
  
  AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
  
  if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
  {
    AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; //clear INT1 overflow flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
  }
  
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//modify freq with the TBPRD equation
void CambiarFreq(void)
{
  if(N == 2) EPwm2Regs.TBPRD = 50000;
  if(N == 4) EPwm2Regs.TBPRD = 25000;
  if(N == 8) EPwm2Regs.TBPRD = 12500;
  if(N == 16) EPwm2Regs.TBPRD = 6250;
  if(N == 32) EPwm2Regs.TBPRD = 3125;
  if(N == 64) EPwm2Regs.TBPRD = 1263;
  if(N == 128) EPwm2Regs.TBPRD = 782;
  if(N == 256) EPwm2Regs.TBPRD = 391;
  if(N == 512) EPwm2Regs.TBPRD = 196; // Set period to 4096 counts
}

//interrupt to generate a sine signal with 100Hz
interrupt void ePWM_isr(void)
{
  static Uint16 Counter_Pwm = 0;
  //float
  EPwm1Regs.CMPA.bit.CMPA = (Uint16)(100*sin(k*Counter_Pwm) + 250);
  if(++Counter_Pwm >= 1000)
  Counter_Pwm = 0;
  EPwm1Regs.ETCLR.bit.INT = 1;
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

void ConfigureADC(void)
{
  EALLOW;
  AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
  AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
  AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;
  AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
  DELAY_US(1000);
  EDIS;
}

void ConfigureEPWM1(void)
{
  EPwm1Regs.TBPRD = 500; // Set period to 4096 counts
  EPwm1Regs.TBCTR = 0;
  EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1; //Divide por 1
  EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1; //Divide por 1
  EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
  EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
  EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
  EPwm1Regs.CMPCTL.bit.SHDWAMODE = TB_SHADOW;
  EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
  EPwm1Regs.AQCTLA.bit.CAD = AQ_SET;
  EPwm1Regs.CMPA.bit.CMPA = EPwm1Regs.TBPRD >> 1;
  EPwm1Regs.ETSEL.bit.INTEN = 1;
  EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_PRD;
  EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;
}

void ConfigureEPWM2(void)
{
  EPwm2Regs.TBPRD = 50000; //
  EPwm2Regs.TBCTR = 0;
  EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1; //Divide por 2
  EPwm2Regs.TBCTL.bit.HSPCLKDIV = 5; //Divide por 10
  EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; //unfreeze, and enter up count   mode
  EPwm2Regs.ETSEL.bit.SOCAEN = 1; //enable SOCA
  EPwm2Regs.ETSEL.bit.SOCASEL = 4; // Select SOC on up-count
  EPwm2Regs.ETPS.bit.SOCAPRD = 1; // Generate pulse on 1st event
  EPwm2Regs.CMPA.bit.CMPA = 0; //0 // Set compare A value to 2048 counts
  EPwm2Regs.CMPB.all = 12500;
  EPwm2Regs.TBCTL.bit.PRDLD = TB_SHADOW;
  EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_PRD;
  EPwm2Regs.CMPCTL.bit.SHDWAMODE = TB_SHADOW;
  EPwm2Regs.AQCTLA.bit.ZRO = AQ_SET;
  EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;
  EPwm2Regs.AQCTLB.bit.CBU = AQ_CLEAR;
  EPwm2Regs.AQCTLB.bit.ZRO = AQ_SET;
}

void SetupADCEpwm()
{
  Uint16 acqps;
  
  if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION)
  {
    acqps = 14; //75ns - 14
  }
  else
  {
    acqps = 63; //320ns
  }
  
  EALLOW;
  AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0; //SOC0 will convert pin A0
  AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is 100 SYSCLK cycles
  AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 7; //trigger on ePWM1 SOCA/C
  AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; //end of SOC0 will set INT1 flag
  AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1; //enable INT1 flag
  AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
  EDIS;
}

void Config_Gpio_Pins(void)
{
  EALLOW;
  GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
  GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;
  GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;
  GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;
  GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;
  GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0;
  GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
  EDIS;
}
