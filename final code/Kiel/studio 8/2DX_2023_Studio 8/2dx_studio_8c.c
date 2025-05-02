// COMP 2DX3
// Hasan Asim 400512182
// Bus speed = 20 MHz
// Using PF0, PF4 LEDs + extra one on PN1

#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"



// I2C control bits
#define I2C_MCS_ACK             0x00000008  
#define I2C_MCS_DATACK          0x00000008  
#define I2C_MCS_ADRACK          0x00000004  
#define I2C_MCS_STOP            0x00000004  
#define I2C_MCS_START           0x00000002  
#define I2C_MCS_ERROR           0x00000002  
#define I2C_MCS_RUN             0x00000001  
#define I2C_MCS_BUSY            0x00000001  
#define I2C_MCR_MFE             0x00000010  

#define MAXRETRIES              5  // max tries for receiving before giving up

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0; // turn on I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // turn on Port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){}; // wait until ready

  GPIO_PORTB_AFSEL_R |= 0x0C; // PB2, PB3 alt function
  GPIO_PORTB_ODR_R |= 0x08; // PB3 open-drain
  GPIO_PORTB_DEN_R |= 0x0C; // digital enable PB2, PB3
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200; // set I2C function
  I2C0_MCR_R = I2C_MCR_MFE; // master mode
  I2C0_MTPR_R = 0b0000000000000101000000000111011; // set clock (100kbps)
}

void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // turn on Port F
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){}; // wait for it
  GPIO_PORTF_DIR_R=0b00010001; // PF0, PF4 output
  GPIO_PORTF_DEN_R=0b00010001; // digital enable PF0, PF4
  return;
}

// Reset pin (XSHUT) for the ToF sensor on PG0
void PortG_Init(void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6; 
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};
  GPIO_PORTG_DIR_R &= 0x00; // PG0 input
  GPIO_PORTG_AFSEL_R &= ~0x01;
  GPIO_PORTG_DEN_R |= 0x01; // digital enable
  GPIO_PORTG_AMSEL_R &= ~0x01; // turn off analog
  return;
}

// Stepper motor control pins
void PortH_Init (void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};
  GPIO_PORTH_DIR_R = 0b00001111; // PH0-3 output
  GPIO_PORTH_DEN_R = 0b00001111; // digital enable
  return;
}

void PortM_Init (void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};
  GPIO_PORTM_DIR_R = 0b00000001; // PM0 output
  GPIO_PORTM_DEN_R = 0b00000001; // digital enable
  return;
}

// just for tracking button presses
volatile unsigned long Falling_Edges = 0;

// Button input on PJ1
void PortJ_Init(void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};
  GPIO_PORTJ_DIR_R &= ~0x03; // PJ0, PJ1 input
  GPIO_PORTJ_DEN_R |= 0x03; // digital enable
  GPIO_PORTJ_PCTL_R &= ~0x000000F0;
  GPIO_PORTJ_AMSEL_R &= ~0x03; // turn off analog
  GPIO_PORTJ_PUR_R |= 0x03; // weak pull-ups
}

// Puts ToF sensor into shutdown (XSHUT = 0)
void VL53L1X_XSHUT(void){
  GPIO_PORTG_DIR_R |= 0x01; // PG0 = output
  GPIO_PORTG_DATA_R &= 0b11111110; // pull PG0 low
  FlashAllLEDs(); // indicate shutdown
  SysTick_Wait10ms(10); // wait 10ms
  GPIO_PORTG_DIR_R &= ~0x01; // back to input
}

// Make the stepper motor spin clockwise
void CW_spin(void) {
  GPIO_PORTH_DATA_R = 0b00001001;
  SysTick_Wait10ms(1);
  GPIO_PORTH_DATA_R = 0b00000011;
  SysTick_Wait10ms(1);
  GPIO_PORTH_DATA_R = 0b00000110;
  SysTick_Wait10ms(1);
  GPIO_PORTH_DATA_R = 0b00001100;
  SysTick_Wait10ms(1);
  // One step per signal pattern, rotates one way
}

// Full rotation in the opposite direction (counter-clockwise)
void CCW_spin (void) {
  for (int i = 0; i < 512; i++){ // 512 steps = full 360°
    GPIO_PORTH_DATA_R = 0b00001100;
    SysTick_Wait10ms(1);
    GPIO_PORTH_DATA_R = 0b00000110;
    SysTick_Wait10ms(1);
    GPIO_PORTH_DATA_R = 0b00000011;
    SysTick_Wait10ms(1);
    GPIO_PORTH_DATA_R = 0b00001001;
    SysTick_Wait10ms(1);
  }
}

// -------- Main starts here -------- //

int position  = 0;
uint16_t	dev = 0x29;	// ToF sensor I2C address
int status = 0;
int flag = 0; // used for deciding motor direction

int main(void) {
	uint8_t byteDate = 0x00;
	uint8_t id, name = 0x00;
  uint8_t byteData, sensorState = 0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, i = 0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;

	// Setup everything
	PLL_Init();	
	SysTick_Init();
	onboardLEDs_Init();
	PortH_Init ();
	I2C_Init();
	UART_Init();
	PortJ_Init();	
	PortF_Init();
	PortM_Init();
		
	GPIO_PORTF_DATA_R = 0b00000000;

	// Wait until the ToF sensor boots
	while(sensorState == 0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }

	UART_printf("ToF sensor booted.\r\n");

	status = VL53L1X_ClearInterrupt(dev); // must clear to allow next interrupt

  status = VL53L1X_SensorInit(dev); // init with defaults
	Status_Check("SensorInit", status);

  status = VL53L1X_StartRanging(dev); // start ranging (distance sensing)

	int delay = 1; // 1ms delay
	int position  = 0;
	int step = 32; // we’re scanning every 11.25°, so 360/11.25 = 32 scans per full rotation
	double angle = 0; // keeping track of current angle

	// Start collecting distance data
	while(1) {  // keep looping forever to get distance from ToF sensor
	
		// check if the button is pressed
		if((GPIO_PORTJ_DATA_R & 0x1) == 0){ 
			SysTick_Wait10ms(30); // debounce delay
			flag ^= 1; // toggle flag (switch between scanning and not scanning)
		}
	
		// wait until the ToF sensor says new data is ready
		while (dataReady == 0){
			status = VL53L1X_CheckForDataReady(dev, &dataReady);
			VL53L1_WaitMs(dev, 5); // tiny delay so we don’t hammer the sensor
		}
		dataReady = 0; // reset for next loop

		if (flag == 1){ // if flag is on, start rotating and reading
		
			CW_spin(); // move motor 1 step clockwise

			// only read distance when we’re at one of the 32 main positions
			if (position % step == 0 && position != 0){ 
				status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
				status = VL53L1X_GetDistance(dev, &Distance); // grab distance in mm
				status = VL53L1X_GetSignalRate(dev, &SignalRate);
				status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
				status = VL53L1X_GetSpadNb(dev, &SpadNum);

				// do some visual feedback to show it’s working
				FlashLED4(delay); // flash D1
				FlashLED3(delay); // flash D2

				status = VL53L1X_ClearInterrupt(dev); // must clear interrupt to get next reading
			
				angle += 11.25; // update angle

				// print out distance to UART
				sprintf(printf_buffer, "%u\r\n", Distance);
				UART_printf(printf_buffer);
			
				SysTick_Wait10ms(50); // small pause before next scan
			}

			// once we hit a full rotation (512 motor steps = 360°)
			if (position == 512){ 
				flag ^= 1; // stop scanning
				position = 0; // reset motor step counter
				angle = 0; // reset angle

				CCW_spin(); // return motor back to starting spot
				FlashLED1(delay); // show we’re done with a sweep
			}

			position++; // keep track of steps we’ve taken
		}

		// loop keeps going forever, checking if we should keep scanning
	}

}
