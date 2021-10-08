/**
 * main.c
 */

// This code allows to led a blink in pin 40 -> GPIO 40

#include "F28x_Project.h"

void Conf_GPIO_led(void);

int main(void)
{
    InitSysCtrl();
    // Disable peripherics to reduce energy consumption
    DisablePeripheralClocks();
  
    // Call configuration leds
    Conf_GPIO_led();

    while (1)
    {
        GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
        GpioDataRegs.GPATOGGLE.bit.GPIO0 = 1;
        DELAY_US(400000);
    };
}

void Conf_GPIO_led(void) {
    // Enable registers to write
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 1;

    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;
    EDIS;
  // Disable registers to write
}
