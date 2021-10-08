
// Test Module ADC 
// CELY SEBASTIÁN

#include "F28x_Project.h"
#include "math.h"

#define RESULTS_BUFFER_SIZE 256

struct bits
{
    Uint32 B0:1;                        //00000000000000000000000000001000
    Uint32 B1:1;
    Uint32 B2:1;
    Uint32 B3:1;
    Uint32 pepito:28;
};

union mis_16
{
    Uint32 all;
    struct bits bit;
};

union mis_16 cont, barrido;

void Config_gpio_leds(void);
void ConfigureADC(void);
void ConfigureEPWM(void);
void SetupADCEpwm(Uint16 channel0, Uint16 channel1);
interrupt void adca1_isr(void);

Uint16 AdcaResults[RESULTS_BUFFER_SIZE];
Uint16 k[RESULTS_BUFFER_SIZE];
Uint16 resultsIndex;
unsigned int Dato;
unsigned int ValorADC;
unsigned int Dato1;
unsigned int ValorADC1;
volatile Uint16 bufferFull;

void main(void)
{
    InitSysCtrl();
    InitGpio();

    DINT;
    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();

    EALLOW;
    PieVectTable.ADCA1_INT = &adca1_isr; //function for ADCA interrupt 1
    EDIS;

    ConfigureADC();
    ConfigureEPWM();
    Config_gpio_leds();
    SetupADCEpwm(0,1);

    IER |= M_INT1; //Enable group 1 interrupts
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    for(resultsIndex = 0; resultsIndex < RESULTS_BUFFER_SIZE; resultsIndex++)
    {
        AdcaResults[resultsIndex] = 0;
        k[resultsIndex] = 0;
    }
    resultsIndex = 0;
    bufferFull = 0;

    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

//
// sync ePWM
//
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;

//
//take conversions indefinitely in loop
//
    do
    {
        //
        //start ePWM
        //
        EPwm1Regs.ETSEL.bit.SOCAEN = 1;  //enable SOCA
        EPwm1Regs.TBCTL.bit.CTRMODE = 0; //unfreeze, and enter up count mode

        //
        //wait while ePWM causes ADC conversions, which then cause interrupts,
        //which fill the results buffer, eventually setting the bufferFull
        //flag
        //
        while(!bufferFull);
        bufferFull = 0; //clear the buffer full flag

        //
        //stop ePWM
        //
        EPwm1Regs.ETSEL.bit.SOCAEN = 0;  //disable SOCA
        EPwm1Regs.TBCTL.bit.CTRMODE = 3; //freeze counter

        //at this point, AdcaResults[] contains a sequence of conversions
        //from the selected channel
    }while(1);
}

interrupt void adca1_isr(void)
{
    AdcaResults[resultsIndex++] = AdcaResultRegs.ADCRESULT0;
    k[resultsIndex++] = AdcaResultRegs.ADCRESULT1;

    if(RESULTS_BUFFER_SIZE <= resultsIndex)
    {
        resultsIndex = 0;
        bufferFull = 1;
    }
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; //clear INT1 overflow flag
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
    }

    Dato =  AdcaResultRegs.ADCRESULT0;
    ValorADC = Dato*15/4095;
    Dato1 = AdcaResultRegs.ADCRESULT1;
    ValorADC1 = Dato1*15/4095;

    cont.all = ValorADC;
    GpioDataRegs.GPADAT.bit.GPIO0 = cont.bit.B0;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO1 = cont.bit.B1;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO2 = cont.bit.B2;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO3 = cont.bit.B3;
    DELAY_US(1);

    barrido.all = ValorADC1;
    GpioDataRegs.GPADAT.bit.GPIO6 = barrido.bit.B0;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO7 = barrido.bit.B1;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO8 = barrido.bit.B2;
    DELAY_US(1);
    GpioDataRegs.GPADAT.bit.GPIO9 = barrido.bit.B3;
    DELAY_US(1000000);

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

void ConfigureADC(void)
{
    EALLOW;

    AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);

    //
    //Set pulse positions to late
    //
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //
    //power up the ADC
    //
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    //delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

    EDIS;
}

void ConfigureEPWM(void)
{
    EALLOW;
    // Assumes ePWM clock is already enabled
    EPwm1Regs.ETSEL.bit.SOCAEN    = 0;    // Disable SOC on A group
    EPwm1Regs.ETSEL.bit.SOCASEL    = 4;   // Select SOC on up-count
    EPwm1Regs.ETPS.bit.SOCAPRD = 1;       // Generate pulse on 1st event
    EPwm1Regs.CMPA.bit.CMPA = 0x0800;     // Set compare A value to 2048 counts
    EPwm1Regs.TBPRD = 0x1000;             // Set period to 4096 counts
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;      // freeze counter
    EDIS;
}

// SetupADCEpwm - Setup ADC EPWM acquisition window
void SetupADCEpwm(Uint16 channel0, Uint16 channel1)
{
    Uint16 acqps;

    //
    // Determine minimum acquisition window (in SYSCLKS) based on resolution
    //
    if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION)
    {
        acqps = 14; //75ns
    }
    else //resolution is 16-bit
    {
        acqps = 63; //320ns
    }

    //
    //Select the channels to convert and end of conversion flag
    //
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = channel0;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is 100 SYSCLK cycles
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5; //trigger on ePWM1 SOCA/C


    AdcaRegs.ADCSOC1CTL.bit.CHSEL = channel1;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is 100 SYSCLK cycles
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 5; //trigger on ePWM1 SOCA/C

    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; //end of SOC0 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    EDIS;

}

//
// adca1_isr - Read ADC Buffer in ISR
//


void Config_gpio_leds(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO6 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;

    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO3 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO9 = 0;

    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO6 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO7 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO9 = 0;
    EDIS;
}
