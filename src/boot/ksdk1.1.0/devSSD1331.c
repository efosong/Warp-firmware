#include <stdint.h>

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];

// Define some layout constants
	// Rows
	const uint8_t eco2_row = 5;
	const uint8_t tvoc_row = 26;
	const uint8_t dust_row = 47;
	// Digit columns
	const uint8_t d1 = 44;
	const uint8_t d2 = 51;
	const uint8_t d3 = 58;
	const uint8_t d4 = 65;

// Define colours
typedef struct _colorgb
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} colorgb;
colorgb white = {0xFF, 0xFF, 0xFF};
colorgb qual_verygood = {0x00, 0xFF, 0x00};
colorgb qual_good     = {0x0F, 0xFF, 0x00};
colorgb qual_ok       = {0xFF, 0xFF, 0x00};
colorgb qual_bad      = {0xFF, 0x0F, 0x00};
colorgb qual_verybad  = {0xFF, 0x00, 0x00};


/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		  = GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}


int drawRectangle(uint8_t p1x, uint8_t p1y, uint8_t p2x, uint8_t p2y, colorgb *color)
{
	writeCommand(kSSD1331CommandDRAWRECT);
	// Send Point 1
	writeCommand(p1x); writeCommand(p1y);
	// Send Point 2
	writeCommand(p2x); writeCommand(p2y);
	// Send Line Color
	writeCommand(color->r);
	writeCommand(color->g);
	writeCommand(color->b);
	// Send Fill Color
	writeCommand(color->r);
	writeCommand(color->g);
	writeCommand(color->b);
	return 0;
}

int clearRectangle(uint8_t p1x, uint8_t p1y, uint8_t p2x, uint8_t p2y)
{
	writeCommand(kSSD1331CommandCLEAR);
	// Send Point 1
	writeCommand(p1x); writeCommand(p1y);
	// Send Point 2
	writeCommand(p2x); writeCommand(p2y);
	return 0;
}

int drawBackground()
{
	colorgb white = {0xFF,0xFF,0xFF};
	uint8_t x, y; // co-ordinates at which to draw characters 
	// eCO2 (ppm)
		// e
		x = 8; y = eco2_row;
		 drawRectangle(x+0,y+3,	x+5,y+9,	&white);
		clearRectangle(x+2,y+4,	x+3,y+5);
		clearRectangle(x+2,y+7,	x+5,y+7);
		// C
		x = 15; y = eco2_row;
		 drawRectangle(x+0,y+1,	x+5,y+8,	&white);
		 drawRectangle(x+1,y+0,	x+4,y+9,	&white); 
		clearRectangle(x+2,y+2,	x+3,y+7);
		clearRectangle(x+4,y+4,	x+5,y+5);
		// O
		x = 22; y = eco2_row;
		 drawRectangle(x+0,y+1,	x+5,y+8,	&white);
		 drawRectangle(x+1,y+0,	x+4,y+9,	&white); 
		clearRectangle(x+2,y+2,	x+3,y+7);
		// _2
		x = 29; y = eco2_row;
		 drawRectangle(x+0,y+5,	x+2,y+9,	&white);
		clearRectangle(x+0,y+6,	x+1,y+6);
		clearRectangle(x+1,y+8,	x+2,y+8);
		// p
		x = 74; y = eco2_row+5;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+3,	x+2,y+4);
		clearRectangle(x+1,y+1,	x+1,y+1);
		// p
		x = 78; y = eco2_row+5;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+3,	x+2,y+4);
		clearRectangle(x+1,y+1,	x+1,y+1);
		// m
		x = 82; y = eco2_row+5;
		 drawRectangle(x+0,y+0,	x+4,y+4,	&white);
		clearRectangle(x+1,y+1,	x+1,y+4);
		clearRectangle(x+3,y+1,	x+3,y+4);
	// tVOC (ppb)
		// t
		x =  1; y = tvoc_row;
		 drawRectangle(x+2,y+0,	x+3,y+9,	&white);
		 drawRectangle(x+2,y+2,	x+5,y+3,	&white);
		 drawRectangle(x+2,y+8,	x+5,y+9,	&white);
		// V
		x =  8; y = tvoc_row;
		 drawRectangle(x+0,y+0,	x+1,y+7,	&white);
		 drawRectangle(x+2,y+6,	x+3,y+9,	&white);
		 drawRectangle(x+4,y+0,	x+5,y+7,	&white);
		// O
		x = 15; y = tvoc_row;
		 drawRectangle(x+0,y+1,	x+5,y+8,	&white);
		 drawRectangle(x+1,y+0,	x+4,y+9,	&white); 
		clearRectangle(x+2,y+2,	x+3,y+7);
		// C
		x = 22; y = tvoc_row;
		 drawRectangle(x+0,y+1,	x+5,y+8,	&white);
		 drawRectangle(x+1,y+0,	x+4,y+9,	&white); 
		clearRectangle(x+2,y+2,	x+3,y+7);
		clearRectangle(x+4,y+4,	x+5,y+5);
		// p
		x = 74; y = 31;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+3,	x+2,y+4);
		clearRectangle(x+1,y+1,	x+1,y+1);
		// p
		x = 78; y = 31;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+3,	x+2,y+4);
		clearRectangle(x+1,y+1,	x+1,y+1);
		// b
		x = 82; y = 31;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+0,	x+2,y+1);
		clearRectangle(x+1,y+3,	x+1,y+3);
	// Dust (ng/m3)
		// D
		x = 1; y = dust_row;
		 drawRectangle(x+0,y+0,	x+3,y+9,	&white);
		 drawRectangle(x+4,y+1,	x+4,y+8,	&white);
		 drawRectangle(x+5,y+2,	x+5,y+7,	&white);
		clearRectangle(x+2,y+2,	x+3,y+7);
		// U
		x =  8; y = dust_row;
		 drawRectangle(x+0,y+0,	x+1,y+8,	&white);
		 drawRectangle(x+1,y+8,	x+4,y+9,	&white);
		 drawRectangle(x+4,y+0,	x+5,y+8,	&white);
		// S
		x = 15; y = dust_row;
		 drawRectangle(x+1,y+0,	x+5,y+1,	&white);
		 drawRectangle(x+0,y+1,	x+1,y+4,	&white);
		 drawRectangle(x+1,y+4,	x+4,y+5,	&white);
		 drawRectangle(x+4,y+5,	x+5,y+8,	&white);
		 drawRectangle(x+0,y+8,	x+4,y+9,	&white);
		// T
		x = 22; y = dust_row;
		 drawRectangle(x+0,y+0,	x+5,y+1,	&white);
		 drawRectangle(x+2,y+2,	x+3,y+9,	&white);
		// n
		x = 74; y = dust_row+6;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+1,y+1,	x+1,y+4);
		// g
		x = 78; y = dust_row+6;
		 drawRectangle(x+0,y+0,	x+2,y+5,	&white);
		clearRectangle(x+1,y+1,	x+1,y+2);
		clearRectangle(x+0,y+4,	x+1,y+4);
		// /
		x = 82; y = dust_row+6;
		 drawRectangle(x+0,y+3,	x+0,y+4,	&white);
		 drawRectangle(x+1,y+2,	x+1,y+2,	&white);
		 drawRectangle(x+2,y+0,	x+2,y+1,	&white);
		// m
		x = 86; y = dust_row+6;
		 drawRectangle(x+0,y+0,	x+4,y+4,	&white);
		clearRectangle(x+1,y+1,	x+1,y+4);
		clearRectangle(x+3,y+1,	x+3,y+4);
		// 3
		x = 92; y = dust_row+4;
		 drawRectangle(x+0,y+0,	x+2,y+4,	&white);
		clearRectangle(x+0,y+1,	x+1,y+1);
		clearRectangle(x+0,y+3,	x+1,y+3);
	return 0;
}



int drawDigit(uint8_t digit, uint8_t x, uint8_t y, colorgb *color)
{
	// First, clear the area
	clearRectangle(x+0,y+0,	x+5,y+9);
	// Draw the digit
	switch (digit){
		case 0:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+2,	x+3,y+7);
			break;
		case 1:
			drawRectangle(x+2,y+0,	x+3,y+9, color);
			break;
		case 2:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+0,y+2,	x+3,y+3);
			clearRectangle(x+2,y+6,	x+5,y+7);
			break;
		case 3:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+0,y+2,	x+3,y+3);
			clearRectangle(x+0,y+6,	x+3,y+7);
			break;
		case 4:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+0,	x+3,y+3);
			clearRectangle(x+0,y+6,	x+3,y+9);
			break;
		case 5:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+2,	x+5,y+3);
			clearRectangle(x+0,y+6,	x+3,y+7);
			break;
		case 6:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+2,	x+5,y+3);
			clearRectangle(x+2,y+6,	x+3,y+7);
			break;
		case 7:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+0,y+2,	x+3,y+9);
			break;
		case 8:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+2,	x+3,y+3);
			clearRectangle(x+2,y+6,	x+3,y+7);
			break;
		case 9:
			 drawRectangle(x+0,y+0,	x+5,y+9, color);
			clearRectangle(x+2,y+2,	x+3,y+3);
			clearRectangle(x+0,y+6,	x+3,y+7);
			break;
		case 10: // draw a dash (-)
			 drawRectangle(x+0,y+4,	x+5,y+5, color);
			break;
		default: // just clear the space
			break;
	}
	return 0;
}

int drawCO2(int eco2)
{
	// choose display colour
	colorgb *color;
	if (eco2 < 700)
		color = &qual_verygood;
	else if (eco2 < 1200)
		color = &qual_good;
	else if (eco2 < 2000)
		color = &qual_ok;
	else if (eco2 < 4500)
		color = &qual_bad;
	else
		color = &qual_verybad;

	// Write readings
	if (eco2 < 400 || eco2 > 8192)
	{// Invalid reading so print ---- (or we wanted to print ----)
		drawDigit(10,             d1, eco2_row, &white);
		drawDigit(10,             d2, eco2_row, &white);
		drawDigit(10,             d3, eco2_row, &white);
		drawDigit(10,             d4, eco2_row, &white);
	}
	else if (eco2 >= 1000)
	{
		drawDigit((eco2/1000)%10, d1, eco2_row, color);
		drawDigit((eco2/100 )%10, d2, eco2_row, color);
		drawDigit((eco2/10  )%10, d3, eco2_row, color);
		drawDigit((eco2     )%10, d4, eco2_row, color);
	}
	else
	{
		drawDigit(11,             d1, eco2_row, color); // clear first digit 
		drawDigit((eco2/100 )%10, d2, eco2_row, color);
		drawDigit((eco2/10  )%10, d3, eco2_row, color);
		drawDigit((eco2     )%10, d4, eco2_row, color);
	}
	return 0;
}

int drawVOC(int tvoc)
{
	// choose display colour
	colorgb *color;
	if (tvoc < 8)
		color = &qual_verygood;
	else if (tvoc < 16)
		color = &qual_good;
	else if (tvoc < 30)
		color = &qual_ok;
	else if (tvoc < 50)
		color = &qual_bad;
	else
		color = &qual_verybad;

	// Write readings
	if (tvoc > 1187)
	{ // Invalid reading so print ---- (or we wanted to print ----)
		drawDigit(10,             d1, tvoc_row, &white);
		drawDigit(10,             d2, tvoc_row, &white);
		drawDigit(10,             d3, tvoc_row, &white);
		drawDigit(10,             d4, tvoc_row, &white);
	}
	else if (tvoc >= 1000)
	{
		drawDigit((tvoc/1000)%10, d1, tvoc_row, color);
		drawDigit((tvoc/100 )%10, d2, tvoc_row, color);
		drawDigit((tvoc/10  )%10, d3, tvoc_row, color);
		drawDigit((tvoc     )%10, d4, tvoc_row, color);
	}
	else if (tvoc >= 100)
	{
		drawDigit(11,             d1, tvoc_row, color); // clear first digit 
		drawDigit((tvoc/100 )%10, d2, tvoc_row, color);
		drawDigit((tvoc/10  )%10, d3, tvoc_row, color);
		drawDigit((tvoc     )%10, d4, tvoc_row, color);
	}
	else if (tvoc >= 10)
	{
		drawDigit(11,             d1, tvoc_row, color); // clear first digit 
		drawDigit(11,             d2, tvoc_row, color); // clear second digit 
		drawDigit((tvoc/10  )%10, d3, tvoc_row, color);
		drawDigit((tvoc     )%10, d4, tvoc_row, color);
	}
	else
	{
		drawDigit(11,             d1, tvoc_row, color); // clear first digit 
		drawDigit(11,             d2, tvoc_row, color); // clear second digit 
		drawDigit(11,             d3, tvoc_row, color); // clear third digit 
		drawDigit((tvoc     )%10, d4, tvoc_row, color);
	}
	return 0;
}

int drawDust(int dust)
{
	// choose display colour
	// TODO choose suitable values
	colorgb *color;
	if (dust< 100)
		color = &qual_verygood;
	else if (dust < 200)
		color = &qual_good;
	else if (dust < 300)
		color = &qual_ok;
	else if (dust < 400)
		color = &qual_bad;
	else
		color = &qual_verybad;

	// Write readings
	if (dust > 9000)
	{ // Invalid reading so print ---- (or we wanted to print ----)
		drawDigit(10,             d1, dust_row, &white);
		drawDigit(10,             d2, dust_row, &white);
		drawDigit(10,             d3, dust_row, &white);
		drawDigit(10,             d4, dust_row, &white);
	}
	else if (dust >= 1000)
	{
		drawDigit((dust/1000)%10, d1, dust_row, color);
		drawDigit((dust/100 )%10, d2, dust_row, color);
		drawDigit((dust/10  )%10, d3, dust_row, color);
		drawDigit((dust     )%10, d4, dust_row, color);
	}
	else if (dust >= 100)
	{
		drawDigit(11,             d1, dust_row, color); // clear first digit
		drawDigit((dust/100 )%10, d2, dust_row, color);
		drawDigit((dust/10  )%10, d3, dust_row, color);
		drawDigit((dust     )%10, d4, dust_row, color);
	}
	else if (dust >= 10)
	{
		drawDigit(11,             d1, dust_row, color); // clear digit 
		drawDigit(11,             d2, dust_row, color); // clear second digit 
		drawDigit((dust/10  )%10, d3, dust_row, color);
		drawDigit((dust     )%10, d4, dust_row, color);
	}
	else
	{
		drawDigit(11,             d1, dust_row, color); // clear first digit
		drawDigit(11,             d2, dust_row, color); // clear second digit 
		drawDigit(11,             d3, dust_row, color); // clear third digit
		drawDigit((dust     )%10, d4, dust_row, color);
	}
	return 0;
}

int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	enableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	//...
	drawBackground();
	// Initialise readings with all '-----'s
	drawCO2(100);
	drawVOC(10000);
	drawDust(10000);

	return 0;
}
