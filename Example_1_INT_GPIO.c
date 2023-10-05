/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#include <stdio.h>
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
// We add this library to manage the AXI GPIO Module
#include "xgpio.h"
// We add this library to manage the AXI TIMER Module
#include "xtmrctr.h"
//We add this library to handle the Interrupt Controller Driver (or equivalently the Interrupt Controller IP-Core)
#include "xintc.h"
#include "xstatus.h"
// Microblaze Interrupts Configuration
#include <xil_exception.h>

/************************** Function Prototypes ******************************/
void DeviceDriverHandler(void *CallbackRef);
void DeviceDriverHandler1(void *CallbackRef);
void DeviceDriverHandler2(void *CallbackRef);

int leds_blink= 0b0;
int toggle=0;
u32 Timer_Count;
//Initialization of GPIO Module
XGpio GPIO;
//Initialization of Timer Module
XTmrCtr TMRInst;

int main()
{	int Status;

    /************************** GPIO IP-Core ******************************/
	//Initialization of GPIO Module
    XGpio_Initialize(&GPIO, XPAR_GPIO_0_DEVICE_ID);
    /************************** TIMER IP-Core ******************************/
    //Initialization of Timer Module
    XTmrCtr_Initialize(&TMRInst, XPAR_AXI_TIMER_0_DEVICE_ID);
    // We set a reset value for the timer. It will start in this value when finish tha count. With 0xFA0A1EFF timer counts 1 sec. See [1] to check how to calculate the sime counted by the timer.
    XTmrCtr_SetResetValue(&TMRInst,0,0xFA0A1EFF);
    // We set configuration options fot the timer, predefined in xtmrctr.h. In this case, we enable the auto reload. See [2]
    XTmrCtr_SetOptions(&TMRInst,0,XTC_AUTO_RELOAD_OPTION);
    // With this function we can read the actual counter value of the timer.
    Timer_Count = XTmrCtr_GetValue(&TMRInst,0);
    // We initialize the counter.
    XTmrCtr_Start(&TMRInst,0);
    /************************** Interrupt Controller IP-Core ******************************/
    //Initialization of INTC Module
    XIntc INTInst;
    XIntc_Initialize(&INTInst,XPAR_AXI_INTC_0_DEVICE_ID);
    XIntc_SelfTest(&INTInst);
    // AXI-INTC Module Connection between Interrupt Source and Handler
    XIntc_Connect(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_0_INTR, (XInterruptHandler)DeviceDriverHandler,(void *)0);
    XIntc_Connect(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_1_INTR, (XInterruptHandler)DeviceDriverHandler1,(void *)0);
    XIntc_Connect(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_2_INTR, (XInterruptHandler)DeviceDriverHandler2,(void *)0);
    // Mode determines if software is allowed to simulate interrupts or real interrupts are allowed to occur. Note that these modes are
    // mutually exclusive.
    //XIN_SIMULATION_MODE enables simulation of interrupts only
    //XIN_REAL_MODE enables hardware interrupts only
	 XIntc_Start(&INTInst, XIN_REAL_MODE);
	 // Enable all interrupts
	 // XIntc_MasterEnable(XPAR_AXI_INTC_0_BASEADDR);
	 XIntc_Enable(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_0_INTR);
	 XIntc_Enable(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_1_INTR);
	 XIntc_Enable(&INTInst, XPAR_AXI_INTC_0_SYSTEM_BTN_2_INTR);
	 //Write all LEDS to ON

	 /************************** Configure Interrupts in Microblaze ******************************/
	 // To setup the exception table (in fact this is not used in MicroBlaze)
	 //Xil_ExceptionInit();
	 //to register this intc ISR handler XIntc_DeviceInterruptHandler () at the MicroBlaze
	 Xil_ExceptionRegisterHandler(XPAR_AXI_INTC_0_SYSTEM_BTN_0_INTR,(Xil_ExceptionHandler)DeviceDriverHandler,(void *)0);
	 Xil_ExceptionRegisterHandler(XPAR_AXI_INTC_0_SYSTEM_BTN_1_INTR,(Xil_ExceptionHandler)DeviceDriverHandler1,(void *)0);
	 Xil_ExceptionRegisterHandler(XPAR_AXI_INTC_0_SYSTEM_BTN_2_INTR,(Xil_ExceptionHandler)DeviceDriverHandler2,(void *)0);
	 //to enable interrupts at the MicroBlaze.
	 Xil_ExceptionEnable();

	 while(1){
		      // We get the counter value of the timer
		     Timer_Count = XTmrCtr_GetValue(&TMRInst,0);
		      // We compare if the counter is greater that 4244967295 (0xFD050F7F), which is the half way from 4194967295 (0xFA0A1EFF) to 4294967295 (0xFFFFFFFF).
		     // based on this, we toggle all LEDs, which should blink as 0.5s. ON  -->0.5s
		      if(Timer_Count>= 4244967295) toggle=1;
		      else toggle=0;

		      if(toggle==1){
		    	  leds_blink|=0b1;
		    	  XGpio_DiscreteWrite(&GPIO, 2, leds_blink);
		    	  	      }
		      else{
		    	  leds_blink&=0xFFFFFFFFE;
		    	  XGpio_DiscreteWrite(&GPIO, 2, leds_blink);
		      }
		  }

  return 0;
}

void DeviceDriverHandler(void *CallbackRef)
{
	 leds_blink|=0b0010;
	 XGpio_DiscreteWrite(&GPIO, 2, leds_blink);


}

void DeviceDriverHandler1(void *CallbackRef)
{
	leds_blink|=0b0100;
	XGpio_DiscreteWrite(&GPIO, 2, leds_blink);

}

void DeviceDriverHandler2(void *CallbackRef)
{
	leds_blink|=0b1000;
	XGpio_DiscreteWrite(&GPIO, 2, leds_blink);

	/*
	 * Indicate the interrupt has been processed using a shared variable.
	 */
	//InterruptProcessed = TRUE;

}

// Ver este link para comprender un puntero a vacío (void *)0
// https://docs.hektorprofe.net/cpp/07-punteros-memoria/02-null-nullptr-void0/
