#include <stdint.h>
#include "tm4c1294ncpdt.h"
//#include "PLL.h"
#include "SysTick.h"
#include "onboardLEDs.h"

#define DELAY 1

// Flash LED D1 (PN1)
void FlashLED1(int count) {
		while(count--) {
			GPIO_PORTN_DATA_R ^= 0b00000010; 	// toggle D1 on/off
			SysTick_Wait10ms(DELAY);			// quick pause
			GPIO_PORTN_DATA_R ^= 0b00000010; 	
			SysTick_Wait10ms(DELAY);			// another pause
		}
}

// Flash LED D3 (PF4)
void FlashLED3(int count) {
		while(count--) {
			GPIO_PORTF_DATA_R ^= 0b00010000; 	// toggle D3 on/off
			SysTick_Wait10ms(DELAY);			
			GPIO_PORTF_DATA_R ^= 0b00010000; 	
			SysTick_Wait10ms(DELAY);			
		}
}

// Flash LED D4 (PF0)
void FlashLED4(int count) {
		while(count--) {
			GPIO_PORTF_DATA_R ^= 0b00000001; 	// toggle D4 on/off
			SysTick_Wait10ms(DELAY);			
			GPIO_PORTF_DATA_R ^= 0b00000001; 	
			SysTick_Wait10ms(DELAY);			
		}
}

// Flash all 4 onboard LEDs together
void FlashAllLEDs(){
		GPIO_PORTN_DATA_R ^= 0b00000011; 		// toggle D1 & D2 (PN0, PN1)
		GPIO_PORTF_DATA_R ^= 0b00010001; 		// toggle D3 & D4 (PF4, PF0)
		SysTick_Wait10ms(25);					// wait a bit
		GPIO_PORTN_DATA_R ^= 0b00000011;		
		GPIO_PORTF_DATA_R ^= 0b00010001; 		// turn back off
		SysTick_Wait10ms(25);					
}

// placeholder for I2C transmit LED flash (not used)
void FlashI2CTx(){
//	FlashLED1(1);
}

// placeholder for I2C receive LED flash (not used)
void FlashI2CRx(){
//	FlashLED2(1);
}

// flash all LEDs to show some kind of error (placeholder)
void FlashI2CError(int count) {
//		while(count--) {
//			FlashAllLEDs();
//		}
}

// setup all onboard LEDs for use
void onboardLEDs_Init(void){
	// Setup Port N (D1 & D2)
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;		// turn on clock for Port N
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){};	// wait for it
	GPIO_PORTN_DIR_R |= 0x03;        					// set PN0 and PN1 as outputs
	GPIO_PORTN_AFSEL_R &= ~0x03;     					// no alternate functions
	GPIO_PORTN_DEN_R |= 0x03;        					// enable digital I/O
	GPIO_PORTN_AMSEL_R &= ~0x03;     					// turn off analog

	// Setup Port F (D3 & D4)
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;			// turn on clock for Port F
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0){};	// wait for it
	GPIO_PORTF_DIR_R |= 0x11;        					// set PF4 and PF0 as outputs
	GPIO_PORTF_AFSEL_R &= ~0x11;     					// no alternate functions
	GPIO_PORTF_DEN_R |= 0x11;        					// enable digital I/O
	GPIO_PORTF_AMSEL_R &= ~0x011;     					// turn off analog

	FlashAllLEDs();	// quick flash to show all ready
	return;
}
