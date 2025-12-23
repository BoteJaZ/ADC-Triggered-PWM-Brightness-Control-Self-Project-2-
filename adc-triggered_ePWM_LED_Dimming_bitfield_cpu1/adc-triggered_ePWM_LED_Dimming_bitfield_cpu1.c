//#############################################################################
//
// To brightness of an LED using ADC
// CPU1 controls ePWM1 and ADCA
// ePWM1 - sawtooth waveform of 10 kHz
// Used for loop to update CMPA in which duty ratio is varied and by extension brightness of the LED
//
//#############################################################################

//
// Included Files
//
#include "F28x_Project.h"
#include "driverlib.h"
#include "device.h"

volatile float pot;

__interrupt void adcaint1_isr(void)
{
    uint16_t pot_raw;
    
    // Read the results from the ADC Result register
    pot_raw = AdcaResultRegs.ADCRESULT0;      // SOC0 (A0)    

    // Transform the unsigned 12-bit numbers to a floating point value
    // 12 bit number - 0 to 4095 levels
    pot = pot_raw /4095.0f;

    // Execute our control algorithm
    EPwm1Regs.CMPA.bit.CMPA = pot * 4999;

    // Cleanup
    // Clear ADC interrupt flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

    // Clear PIE Group 1 ACK
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;

}

//
// Main
//

void main(void)
{
    // Initialize the system
    InitSysCtrl();

    // Initialize interrupts at the global level
    // Initialize the PIE module
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;

    // Initialize the PIE vector table
    InitPieVectTable();

    // Set the frequency clock for ePWM module to less than or equal to 100 MHz
    EALLOW;
    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV = 1;

    // Assign ePWM1 to CPU1 
    DevCfgRegs.CPUSEL0.bit.EPWM1 = 0;

    // Enable clock access to ePWM1 via CPU1
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;

    // Assign ADC A module to CPU1
    DevCfgRegs.CPUSEL11.bit.ADC_A = 0;

    // Enable clock access to ADC A via CPU1
    CpuSysRegs.PCLKCR13.bit.ADC_A = 1;
    EDIS;

    // Stop timer clock for ePWM1
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // Configure the ePWM1 module
    // Time base sub-module
    // Clock fed to ePWM module is 100 MHz. TBCLK = 100 MHz/ (HSPCLKDIV * CLKDIV) = 100 MHz/ (2 * 1) = 50 MHz
    // Configure ePWM1
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 2;
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 1;
    EPwm1Regs.TBCTL.bit.PRDLD = 0;
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;        // up count mode

    // Load value in Period Register
    EPwm1Regs.TBPRD = 5000 - 1;             // 10kHz sawtooth waveform

    // Counter-compare sub-module 
    EPwm1Regs.CMPCTL.bit.LOADAMODE = 0;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = 1;
    

    // Action Qualifier sub-module
    // TBCTR = 0, A will be high (bit ZRO - AQCTLA/B)
    EPwm1Regs.AQCTLA.bit.ZRO = 2;
    EPwm1Regs.AQCTLA.bit.CAU = 1;

    // Configure ADC A 
    // Divide the clock so that ADCCLK <= 50 MHz
    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;

    // Power-up the ADC module
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    DELAY_US(1000);
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    // Set the mode and resolution of the ADC module - library function
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);

    // Configure the channel
    // One Channel - SOC0
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 0;    // software trigger
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;      // A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 14;

    // Configure the interrupt
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0;
    AdcaRegs.ADCINTSEL1N2.bit.INT1CONT = 0;

    // Register the ISR for ADC
    PieVectTable.ADCA1_INT = &adcaint1_isr;
    EDIS;

    // Configure the PIE module for ADCAINT1 interrupt INT1.1
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    // Enable global interrupts and configure IER
    // Enable adc a interrupt 1
    IER = 0x1U;

    EINT; 

    // Configure GPIO0 as ePWM functionality
    // GPIO0 Group 00b
    EALLOW;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 0;

    // GPIO0 as ePWM pin
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
    EDIS;

    // Start the timer counter for ePWM 1
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
     
    // Infinite loop
    while (1) 
    {
        AdcaRegs.ADCSOCFRC1.bit.SOC0 = 1;
    }
}

//
// End of File
//
