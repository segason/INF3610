#include "bsp_init.h"
#include "platform.h"

#include <stdio.h>
#include <xparameters.h>
#include <xil_printf.h>
#include <xil_exception.h>
#include "os_cfg_r.h"

#define TIMER_CLOCK_FREQUENCY 333000000
#define TIMER_TICK_FREQUENCY OS_TICKS_PER_SEC

/**
 * Assign the driver structures for the interrupt controller, the
 * PL Core interrupt source and the GPIO driver
 */
XScuGic gic;
XIntc axi_intc;
XGpio gpSwitch;

int initialize_bsp() {

	//disable_cache_for_section();
	init_platform();
	initialize_timer();
	initialize_gpio();
	return XST_SUCCESS;
}

void initialize_timer() {
    unsigned long load_value = (TIMER_CLOCK_FREQUENCY/TIMER_TICK_FREQUENCY) - 1;

	// Reset the timers, and clear interrupts, and init timer in auto reload mode
    private_timer_init(AUTO_RELOAD_TIMER);

	// Set the number of cycles each timer counts before generating an interrupt and start the timer
    private_timer_request(load_value);
}

void initialize_gpio()
{
	/* À compléter */

	int status ;

	status = XGpio_Initialize(& gpSwitch,GPIO_SW_DEVICE_ID);

	if (status != XST_SUCCESS){
		xil_printf("Error %d while initializing the Gpio\n", status);
		return XST_FAILURE;
	}

	XGpio_InterruptGlobalEnable(&gpSwitch);
	XGpio_InterruptEnable(&gpSwitch, XGPIO_IR_MASK);
}


///////////////////////////////////////////////////////////////////////////
//					      End of HW Setup
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//						 Interrupt Section
///////////////////////////////////////////////////////////////////////////

int prepare_and_enable_irq() {
	int status;

	status = initialize_interrupt_controller();
	if (status != XST_SUCCESS) {
		xil_printf("Error %d while initializing the interrupt controller\n", status);
		return XST_FAILURE;
	}

	status = connect_irqs();
	if (status != XST_SUCCESS) {
		xil_printf("Error %d while connecting the irqs\n", status);
		return XST_FAILURE;
	}

	enable_interrupt_controller();

	return XST_SUCCESS;
}

int initialize_interrupt_controller() {
	int status;

	status = initialize_gic();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	status = initialize_axi_intc();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	return status;
}

int initialize_gic() {
	int status;
	XScuGic_Config* gic_config;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	gic_config = XScuGic_LookupConfig(GIC_DEVICE_ID);
	if (gic_config == NULL)
		return XST_FAILURE;

	status = XScuGic_CfgInitialize(&gic, gic_config, gic_config->CpuBaseAddress);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	return XST_SUCCESS;
}

int initialize_axi_intc() {
	int status;

	status = XIntc_Initialize(&axi_intc, XPAR_AXI_INTC_0_DEVICE_ID);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	return XST_SUCCESS;
}

void enable_interrupt_controller() {
	/*
	 * Initialize the  exception table
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &gic);

	/*
	 * Enable non-critical exceptions
	 */
	Xil_ExceptionEnable();

	/* Start axi interrupt controller */
	XIntc_Start(&axi_intc, XIN_REAL_MODE);
}

int connect_irqs() {
	int status;

	/*IRQ coming INTC*/
	status = connect_intc_irq();
		if (status != XST_SUCCESS)
		return XST_FAILURE;

	/*Timer IRQ */
	status = connect_timer_irq();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	/*Timer 1s ISR*/
	status = connect_fit_timer1s_irq();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	/*Timer 3s ISR*/
	status = connect_fit_timer3s_irq();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	/*GPIO ISR*/
	status =  connect_gpio_isr();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	return XST_SUCCESS;
}

int connect_intc_irq() {
	int status;

	/*
	 * Connect the interrupt handler that will be called when an
	 * interrupt occurs for the device.
	 */
	status = XScuGic_Connect(&gic, PL_INTC_IRQ_ID, (Xil_ExceptionHandler)XIntc_DeviceInterruptHandler, (void*)(uint32_t)axi_intc.CfgPtr->DeviceId);
	if (status != XST_SUCCESS)
		return status;

	/*
	 * Enable the interrupt for the PL device.
	 */
	XScuGic_Enable(&gic, PL_INTC_IRQ_ID);

	return XST_SUCCESS;
}

int connect_timer_irq() {
	int status;

	/*
	 * Connect the interrupt handler that will be called when an
	 * interrupt occurs for the device.
	 */
	status = XScuGic_Connect(&gic, TIMER_INTERRUPT, timer_isr, NULL);
	if (status != XST_SUCCESS)
		return status;


	/*
	 * Enable the interrupt 
	 */
	XScuGic_Enable(&gic, TIMER_INTERRUPT);

	return XST_SUCCESS;
}

int connect_fit_timer1s_irq()
{
	int status;

	status = XIntc_Connect(&axi_intc, FIT_1S_IRQ_ID, fit_timer_1s_isr, NULL);
	if (status != XST_SUCCESS)
		return status;

	XIntc_Enable(&axi_intc, FIT_1S_IRQ_ID);

	return XST_SUCCESS;
}

int connect_fit_timer3s_irq()
{
	int status;

	status = XIntc_Connect(&axi_intc, FIT_3S_IRQ_ID, fit_timer_3s_isr, NULL);
	if (status != XST_SUCCESS)
		return status;

	 XIntc_Enable(&axi_intc, FIT_3S_IRQ_ID);

	 return XST_SUCCESS;
}


int connect_gpio_isr()
{
	int status;

	status = XIntc_Connect(&axi_intc, GPIO_SW_IRQ_ID, gpio_isr, NULL);
	if (status != XST_SUCCESS)
		return status;

	XIntc_Enable(&axi_intc, GPIO_SW_IRQ_ID);

	return XST_SUCCESS;
}


void cleanup() {
	/*
	 * Disconnect and disable the interrupt
	 */
	disconnect_timer_irq();
	disconnect_intc_irq();

	disconnect_fit_timer1s_isr();
	disconnect_fit_timer3s_isr();
	disconnect_gpio_isr();
}

void disconnect_timer_irq() {
	XScuGic_Disable(&gic, TIMER_INTERRUPT);
	XScuGic_Disconnect(&gic, TIMER_INTERRUPT);
}

void disconnect_intc_irq() {
	XScuGic_Disable(&gic, PL_INTC_IRQ_ID);
	XScuGic_Disconnect(&gic, PL_INTC_IRQ_ID);
}

void disconnect_fit_timer1s_isr(){
	XIntc_Disable(&axi_intc, TIMER_INTERRUPT);
	XIntc_Disconnect(&axi_intc, TIMER_INTERRUPT);
}
void disconnect_fit_timer3s_isr(){
	XIntc_Disable(&axi_intc, TIMER_INTERRUPT);
	XIntc_Disconnect(&axi_intc, TIMER_INTERRUPT);
}

void disconnect_gpio_isr(){
	XIntc_Disable(&axi_intc, TIMER_INTERRUPT);
	XIntc_Disconnect(&axi_intc, TIMER_INTERRUPT);
}

///////////////////////////////////////////////////////////////////////////
//						End of Interrupt Section
///////////////////////////////////////////////////////////////////////////
