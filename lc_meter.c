/*

lc_meter
========

A LC+ESR Meter Template 

Measures capacitance (< 1nF) and inductance using LC tank frequency measuring method.
Measures capacitance (> 1nF) with time-to-charge method.
Measures (or rather attempts to measure) capacitor ESR values by pulse injecting currents through capacitors.
Bare minimal component count, experimental hookup.

I called this a "Meter Template" instead of a "Meter" as I consider this hookup as a base to develop a "Full" meter. There are quite a few things "missing" when compare w/ other common designs you can find on the internet, that affects accuracy and usability.

Please read build thread in http://forum.43oh.com/topic/4179-educational-boosterpack-lc-esr-meter-template/

Schematic

             MSP430G2553
           ---------------  
       /|\|            XIN|-
        | |               | 
        --|RST        XOUT|-
          |               |                    /|\
     (black)  P1.3  .-----|-------o-----o---o   |
          |         ^     |       |     |   |   |
          |        / \    |       |     |  | | | |100k x 2
          |       /__+\   |       |     |  | | | |
          |        | |    |       |     |   |   |     10uF    
     (blue)   P1.1 | o----|-------|-----|---o---o--+||--o-----o-----o-------o
          |        |      |       |     |       |       |     |     |       |
     (yellow)      | P1.2-|-o     |     |       |       |     |     |_      |
          |        |      | |    | |47k| |47k  | |     --1nF --1nF    )82uH |
          |        |      || |   | |   | |     | |100k ---   ---      )     |
          |        |      || |100 |     |       |       |     |1%    _)     |
          |        |      | |     |     |       |       |     |      |      |  
     (yellow) P1.6 o------|-|-----|-----o---||--o-------o     o    o o----o o
          |               | |     |        0.1uF|            \      \ .....\
     (white)  P1.4 o------|-o-----o  o          |             o      o      o-----(A)
          |               |          |          |             |      |       
          |               |          |          |             |      o------------(B)
          |               |         _|_        _|_           _|_    _|_    L=C 
          |               |         ///        ///           ///    /// 
		  |               |                  
  

Some Desgin Notes

The LC (Cx up to 1nF) meter part utilizes P1.1, P1.3, P1.6. Comparator A is used along w/ other passive components to form a LC tank circuit. This oscillates at a frequency based on the inductor and capacitor values. Where Freq = 1 / (2 * Pi * Sqrt(L * C))
 
A DPDT switch is used to select capacitance or inductance measure mode. With the subject capacitor / inductor attached, we measure the frequency of the oscillation, and use the above formula to derive the unknown L or C value.
 
 
The high range Capacitor measurement utilizes P1.4 and P1.3. P1.4 is configured as Comparator A inverting input (-) while we use internal 0.55V as the non-inverting (+) input. We setup the comparator interrupt to trip TimerA and measures the time it takes to charge a subject capacitor to 0.55V. Since the time to charge is linear to the capacitance, we used a stored calibrated value as a reference to work out subject capacitance. P1.3 is used to start "charge" the target capacitor via a 47k resistor. I later found out that for larger caps it was taking too long to measure. And eventually I make use of the ESR measuring resistor (via P1.2) to add a high-high range. Now I first charge the unknown cap thru P1.3 (47k) and if it times out 50 times, I would switch to P1.2 (100ohm) to charge the unknon cap quicker. The P1.3 charging will measure up to 90uF while the P1.2 high-high range will do whatever it does. I haven't got mF caps to test, you can try and calculate, guess it would be 90uF * 470 = 40mF

 
The ESR measurement is based on the high range capacitance measurement (P1.4, P1.3) with an additional P1.2 pin. Here we use P1.4 as ADC input. A 100ohm resistor is used between P1.2 and P1.4, with a target capacitor in place between P1.4 and ground, this three nodes (P1.2, P1.4, Gnd) forms a voltage divider. By measuring the voltage at P1.4 we can derived the resistance (or ESR) of the target capacitor.
 
 
The above is a high level overview on the set-up. You can find out more by googling the subject. I studied various designs from the internet and come up w/ this. And at this stage I want to have the most bare-bone set-up to play with.
 
Some Build Notes


I only did it on gcc (both cygwin and mint14), you MUST use the following CFLAGS otherwise the oscillation won't happen (unless to use a 2nF instead of 1nF in the LC tank).

gcc version 4.6.3 20120301 (mspgcc LTS 20120406 patched to 20120502) (GCC)
/cygdrive/c/mspgcc-20120406-p20120502/bin/msp430-gcc -Os -Wall -ffunction-sections -fdata-sections -fno-inline-small-functions -Wl,-Map=lc_meter.map,--cref -Wl,--relax -Wl,--gc-sections -I/cygdrive/c/mspgcc-20120406-p20120502/bin/../msp430/include -mmcu=msp430g2553 -o lc_meter.elf lc_meter.c

As I said this is an experimental template and if you want to have it make bench permanent you should consider the following.

I did not use relays or even switches since this setup does not have enough GIO pins available (as I hook this up w/ a CircuitCo Eductional BoosterPack). I had use a strategy of poking adc values here and there to "detect" the presence of capacitor in different inputs. This is not very reliable and can be replaced w/ switches and/or relays. I strongly advice you to google similar designs in PIC or AVRs and see how they are done.

Also I omit the use of transistor to drive the charging, disarging and switching. If you are to build a reliable unit, you should probably add them back (check and compare w/ other designs). There is also no safety features like diode protections, etc.


Chris Chung 2013 August
. init release

code provided as is, no warranty

you cannot use code for commercial purpose w/o my permission
nice if you give credit, mention my site if you adopt much of my code

*/
//______________________________________________________________________________

#include <msp430.h>

#include <stdint.h>
#include <stdio.h>

#define MHZ 16

#define EBLCD_CLK	BIT5	// P1
#define EBLCD_DATA	BIT7	// P1
#define EBLCD_RS	BIT3	// P2

//______________________________________________________________________
void eblcd_write(uint8_t d, uint8_t cmd) {
	if (cmd) P2OUT &= ~EBLCD_RS;
	else P2OUT |= EBLCD_RS;

	cmd = 8;
	while (cmd--) {
		if (d&0x80) P1OUT |= EBLCD_DATA;
		else P1OUT &= ~EBLCD_DATA;
		d <<= 1;
		P1OUT &= ~EBLCD_CLK;
		P1OUT |= EBLCD_CLK;
		P1OUT &= ~EBLCD_CLK;
	}//while
	__delay_cycles(50);
}

//______________________________________________________________________
void eblcd_setcg(uint8_t which, uint8_t *dp) {
	uint8_t i;
	which <<= 3;
	which |= 0x40;
	for (i=0; i<8; i++) {
		eblcd_write(which+i, 1);
		eblcd_write(*dp++, 0);
	}//for
}

//______________________________________________________________________
void eblcd_clear(uint8_t row) {
	uint8_t i=0;
	row *= 0x40;
    eblcd_write(0x80+row, 1);
	for (i=0;i<16;i++) eblcd_write(' ', 0);
    eblcd_write(0x80+row, 1);
}

//______________________________________________________________________
void eblcd_putc(char c) { 
	eblcd_write(c, 0);
}

void eblcd_puts(char *p, uint8_t row) {
    if (row<2) eblcd_clear(row);
	while (*p) eblcd_write(*p++, 0);
}

//______________________________________________________________________
const char hex_map[] = "0123456789abcdef";
void eblcd_hex8(uint8_t d) {
	eblcd_write(hex_map[d>>4], 0);
	eblcd_write(hex_map[d&0x0f], 0);
}

//______________________________________________________________________
void eblcd_hex32(uint32_t h) {
    eblcd_hex8((h & 0xFF000000) >> 24);
    eblcd_hex8((h & 0x00FF0000) >> 16);
    eblcd_hex8((h & 0x0000FF00) >> 8);
    eblcd_hex8(h & 0xFF);
}

//______________________________________________________________________
void eblcd_hex16(uint16_t h) {
    eblcd_hex8((h & 0xFF00) >> 8);
    eblcd_hex8(h & 0xFF);
}

//______________________________________________________________________
void eblcd_dec16(uint16_t d) {
	uint8_t buf[8], i=0;
	
	do {
		buf[i++] = (d%10) + '0';
		d /= 10;
	} while (d);
	while (i) eblcd_putc(buf[--i]);
}

//______________________________________________________________________
void eblcd_dec32(uint32_t d, uint8_t dec) {
	uint8_t buf[8], i=0;
	
	do {
		buf[i++] = (d%10) + '0';
		d /= 10;
	} while (d);
	if (i==dec) buf[i++] = '0';		// leading zero
	while (i<4) {
		if (i<=dec)
			buf[i++] = '0';
		else
			buf[i++] = ' ';
	}//while
	while (i) {
		if (dec && i==dec) eblcd_putc('.');
		eblcd_putc(buf[--i]);
	}//if
}





//________________________________________________________________________________
void eblcd_setup() {
	//______________ lcd port use
	P1DIR |= EBLCD_CLK|EBLCD_DATA;
	P2DIR |= EBLCD_RS;

	static const uint8_t lcd_init[] = { 0x30, 0x30, 0x39, 0x14, 0x56, 0x6d, 0x70, 0x0c, 0x06, 0x01, 0x00, };
	static const char hello0[] = "     MSP430     ";
	static const char hello1[] = "  POWER PLAYERS ";

    eblcd_write(0x30, 1);
	__delay_cycles(500000);

	const uint8_t *cmd_ptr = lcd_init;
	while (*cmd_ptr) eblcd_write(*cmd_ptr++, 1);		// lcd init sequence
	__delay_cycles(50000);

/*

 ooooo .ooo. ..ooo ooo.. ooo.. ..... ..... ..... 
 ooooo oo.oo ..o.o o.o.. ooo.. ..o.. ..... .ooo. 
 ooooo o...o ..o.o o.o.. ooo.. .o.o. ..... o...o 
 ooooo o...o ooo.o o.ooo ooooo o...o ..... o...o 
 ooooo ..... ..o.o o.o.. ooo.. ..... o...o oo.oo 
 ooooo ..... ..o.o o.o.. ooo.. ..... .o.o. .o.o. 
 ooooo ..... ..ooo ooo.. ooo.. ..... ..o.. oo.oo 
 ooooo ..... ..... ..... ..... ..... ..... ..... 

*/
	const uint8_t cgfonts[8][8] = {		// custom font for component symbols
		{ 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, },
		{ 0x0e, 0x1b, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, },
		{ 0x07, 0x05, 0x05, 0x1d, 0x05, 0x05, 0x07, 0x00, },
		{ 0x1c, 0x14, 0x14, 0x17, 0x14, 0x14, 0x1c, 0x00, },
		{ 0x1c, 0x1c, 0x1c, 0x1f, 0x1c, 0x1c, 0x1c, 0x00, },
		{ 0x00, 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00, },
		{ 0x00, 0x00, 0x00, 0x00, 0x11, 0x0a, 0x04, 0x00, },
		{ 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0a, 0x1b, 0x00, },
	};

	uint8_t i=0;
	eblcd_write(0x38, 1);
	for (i=0;i<8;i++) eblcd_setcg(i, (uint8_t*) cgfonts[i]);	// load custom fonts
	eblcd_write(0x39, 1);
    eblcd_write(0x02, 1);
    eblcd_write(0x80, 1);

	cmd_ptr = (uint8_t*) hello0;
	while (*cmd_ptr) eblcd_write(*cmd_ptr++, 0);				// hello display
	eblcd_write(0x80|0x40, 1);
	cmd_ptr = (uint8_t*) hello1;
	while (*cmd_ptr) eblcd_write(*cmd_ptr++, 0);

	__delay_cycles(5000000);
}

volatile uint16_t ticks=0,clicks=0;
volatile uint16_t capture_cnt=0;
volatile uint16_t ov_cnt=0;


//#define DEBUG	1
#define MEASURE_PIN	BIT4
#define PULL_PIN	BIT3
#define PULSE_PIN	BIT2
//________________________________________________________________________________
uint16_t capture_pulses() {

	capture_cnt = 0;

	ADC10CTL0 = ADC10CTL1 = ADC10AE0 = 0;
	P1OUT &= ~(PULL_PIN|MEASURE_PIN);
	P1DIR &= ~(PULL_PIN|MEASURE_PIN);

	CACTL2 = (P2CA4) | (P2CA3|P2CA2);		// CA1=(+), CA6=(-)
	// CACTL2 |= CAF;						// DO NOT use filter, we won't oscillate w/ them
	CAPD = BIT0|BIT1|BIT2|BIT4|BIT6;		// turn off pins for a cleaner read
	P1DIR |= BIT3;	// p1.3 as CAOUT
	P1SEL |= BIT3;
	P1SEL2 |= BIT3;
	P1OUT &= ~BIT3;
	P1REN &= ~BIT3;
	CACTL1 = CAON;

	_BIS_SR(GIE);

	__delay_cycles(100000);		// adjust this to allow charge of Cb, ie need delay for larger caps
	TA0CTL = TASSEL_2|MC_2|ID_3|TACLR|TAIE;		// smclk/8, cont. need overflow interrupt
	CACTL1 |= CAIE;				// comparator interrupt on, count pulses from our LC tank
	_BIS_SR(LPM0_bits + GIE); 	// we now wait for timerA to overflow, loops items
	_BIC_SR(GIE);
	CACTL1 &= ~CAIE;			// done, no more counting on comparator pulses

	// 'capture_cnt' now has number of pulses within the timerA overflow period
	// frequency of LC tank would be 16Mhz/64k * 'capture_cnt'

	//_____________ done, clean-up turn things off
	TA0CTL = 0;						// no timer
	//CACTL2 = CACTL1 = CAPD = 0;	// DON'T turn off comparator here, continous read will be affected
	_BIS_SR(GIE);

	return capture_cnt;
}


//________________________________________________________________________________
//
// this function is good for measuring higher value capacitors, say greater than 1nF
// we setup reference voltage on the comaparator non-inverting input to 0.55v,
// our target capacitor on the inverting input and time
// how long it takes to charge up the unknown capacitor to 0.55V
// we setup the comparator to trip a timerA interrupt when the 0.55V is reached

float measure_high_cap(uint8_t mode, float *esr) {

	_BIC_SR(GIE);						// stop things, setup time
	P1OUT &= ~(PULL_PIN|MEASURE_PIN);
	P1DIR |= (PULL_PIN|MEASURE_PIN);
	P1SEL &= ~PULL_PIN;
	P1SEL2 &= ~PULL_PIN;

	// setup adc on measure pin, this time for determining whatever a cap is connected
	ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; 
	ADC10CTL1 = INCH_4;
	ADC10AE0 |= MEASURE_PIN;

	uint16_t last_adc=1023;
	uint8_t hit=0;
	while (1) {
#ifdef DEBUG
		eblcd_clear(1); eblcd_dec16(ADC10MEM); eblcd_putc('='); eblcd_dec16(hit);
#endif
		P1DIR &= ~MEASURE_PIN;			// read adc
		ADC10CTL0 |= ENC + ADC10SC;		// Sampling and conversion start
		while (ADC10CTL1 & ADC10BUSY);

		if (ADC10MEM>25 && ADC10MEM<150 && !hit) return 0.0;	// not consider present, return
		if (last_adc<ADC10MEM) return 0.0;		// not discharging, return
		last_adc = ADC10MEM;
		if (ADC10MEM<2) break;			// non-floating, consider a cap is connected
		P1DIR |= MEASURE_PIN;			// try discharge via measure pin
		P1DIR |= PULSE_PIN;
		P1OUT &= ~PULSE_PIN;
		CACTL2 = CACTL1 = CAPD = 0;		
		if (hit>2) eblcd_puts("Discharging", 1);
		__delay_cycles(1000000);		// need some time
		hit++;
	}//while
	P1DIR &= ~PULSE_PIN;

#ifdef DEBUG
	eblcd_clear(0); eblcd_dec16(ADC10MEM); eblcd_putc('<');
#endif

	float cx = 1000.0;
	uint8_t charge_pin = PULL_PIN;

	while (charge_pin) {
	// we sould like to get the cap values 1st;
	// turn off adc, turn on comparator

	ADC10CTL0 = ADC10CTL1 = ADC10AE0 = 0;
	CACTL2 = P2CA3;							// CA4 on inverting (-)
	CACTL1 = CAREF_2|CAREF_1|CAIES|CAON;	// 0.55V on non-inverting (+), Comp. on

	CAPD = MEASURE_PIN;

	// charge, capture time to reach 0.55V
	ov_cnt = 0;
	P1OUT |= charge_pin;
	P1DIR |= charge_pin;

	// setup and start timer, comparator will trip timer when cap charges up to 0.55V
	// also note that for large caps timer will over flow many times 
	// we capture that via timer overflow interrupt flag and will use it for final calculation

	TA0CTL = TASSEL_2|MC_2|ID_2|TACLR|TAIE;	// smclk, cont. div4 need overflow interrupt
	TA0CCTL1 = CM_2|CCIS_1|SCS|CAP;			// falling edge, CCIxB (i.e. comparator)
	_BIS_SR(GIE);

	uint8_t over_range = 0;
	// each overflow is like 2uF, let's break out at 10000uF, can't wait for it forever
	// ideally we could use another pin to alter the supply current to impact sample time
	while (!(TA0CCTL1&CCIFG)) { 
		if (ov_cnt>50) {
			over_range = 1;
			break;
		}//if
	}//if

	CAPD = CACTL2 = CACTL1 = 0;				// comparator off
	TA0CTL = TA0CCTL1 = 0;					// timer off
	_BIC_SR(GIE);							// no more interrupts


#ifdef DEBUG
	eblcd_clear(0); eblcd_dec16(TA0CCR1); eblcd_putc('=');
	eblcd_hex8(ov_cnt); eblcd_putc('=');
	if (ov_cnt) {
		eblcd_clear(0); eblcd_dec16(TA0CCR1); eblcd_putc('=');
		eblcd_hex16(ov_cnt); eblcd_putc('=');
	}//if
	//while (1) { asm("nop"); }
#endif

	if (over_range) {
		if (charge_pin == PULL_PIN)		// we are in low range, try high
			charge_pin = PULSE_PIN;
		else
			charge_pin = 0;
	}//if
	else {
		cx = (float) TA0CCR1;			// time comparator breaches 0.55V
		while (ov_cnt--) cx += 65536.0;	// plus each overflow adds 2^16 units
		cx *= 285.83;					// my calibrated value using a 1nF 1%
		if (charge_pin == PULSE_PIN)	// we are in high range
			cx *= (47000.0/100.0);		// 100ohm instead of 47k
		charge_pin = 0;					// no more trials
	}//else

	// doing ESR
	// try to deplete the charge built-up previous via grounding both measure and pulse pins
	P1OUT &= ~(MEASURE_PIN|PULSE_PIN|PULL_PIN);
	P1DIR |= (MEASURE_PIN|PULSE_PIN|PULL_PIN); 

	ADC10CTL0 = ADC10SHT_2 + ADC10ON;// + ADC10IE; 
	ADC10CTL1 = INCH_4;
	ADC10AE0 |= MEASURE_PIN;
	do {
		__delay_cycles(50000);
		ADC10CTL0 |= ENC + ADC10SC;
		while (ADC10CTL1 & ADC10BUSY);
	} while (ADC10MEM>0);		// wait for test cap to deplete

	}//while


	ADC10CTL0 = ADC10SHT_1 + ADC10ON + REFON;	// now with refV @ 1.5V

	// DEBUG ONLY, pull-up pluse pin and mesaure reference current
	// do it at your own risk, your may blow an IO pin
	//P1OUT |= PULL_PIN; 		// charge 3.5V 0.005mA, can't use this
	//P1OUT |= PULSE_PIN; 		// charge 3.5v 28.4mA (w/ 100ohm) 208mV for 7.5ohm
	//while (1) { asm("nop"); }	// stop here so i could measure w/ a DMM
	// 1500mv = 1024u.. 1u = 1.5mv .. 20mv 0.75ohm 14u .. 2u 0.1ohm

	uint32_t adc=0;
	uint16_t i=0;

	for (i=0;i<256;i++) {
		P1OUT |= PULSE_PIN; 			// current on
		//__delay_cycles(16);			// 1us pulse? i don't see a difference, not used
		P1DIR &= ~MEASURE_PIN;
		ADC10CTL0 |= ENC + ADC10SC;		// start adc conversion
		while (ADC10CTL1 & ADC10BUSY);
		P1OUT &= ~PULSE_PIN; 			// current off
		P1DIR |= MEASURE_PIN;
		//ADC10CTL0 ^= ADC10ON;
		adc += ADC10MEM;
		//__delay_cycles(32000);			// 1khz for me, could have been 1Khz, or 100Khz
		__delay_cycles(16000);			// 1khz
		//ADC10CTL0 ^= ADC10ON;
	}//while
	adc >>= 6;		// average out oversamples
	// convert adc units into mVs, 10bit adc on 1.5V reference, plus still 4x over-samples
	float mv = ((float) adc) * 1500 / 1024.0 / 4.0;	

	// get ESR via ohm's law V=IR, 
	// our pulse current is (3.6V - pulse pin VDrop)/ (R (100ohm used) + pin resistance)
	// so we divide our adc read voltage w/ the pulse current to get ESR


	// my guestimate + calibration w/ 1, 2.2, 7.5 ohm 1% resistors

	mv = 140.0 / ((3000.0/mv) - 1.0);
	mv *= 100.0;					// we want 0.01 ohm units
	if (mv > 9999) mv = 9999;
	*esr = mv;

	ADC10CTL0 = ADC10CTL1 = ADC10AE0 = 0;
	P1DIR &= ~PULL_PIN;

	return (cx/1000.0);

}
//________________________________________________________________________________
int main(void) {
	WDTCTL = WDTPW + WDTHOLD;
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL  = CALDCO_16MHZ;

	eblcd_setup();
	eblcd_clear(0);
	eblcd_clear(1);

	uint8_t c='=';
	uint8_t cnt=0, wait=0, mode=9, last_open=0, open=0;

	uint16_t f1=0, f2=0, f3=0, last_f3=0;

	while (1) {
		if (c) {
			switch (c) {
				case '=':	// measure c
					{
					//__delay_cycles(MHZ*1000);
					uint32_t x32=0;
					float esr=0.0;
					if (!f1) {
						f1 = capture_pulses();

						if (f1 > 20) {
							if (f1 >= 0x8000) {
								f1 = 0;
								eblcd_puts("Select Cap-Low  ", 0);
							}//if
						}//if
						else {
							f1 = 0;
							eblcd_puts("Pick Capacitance", 0);
						}//else
					}//if
					else {
						if (!f2) {
							if ((f2 = capture_pulses()) > (f1 - (f1>>3))) {

								f1 = f2;
								f2 = 0;
								eblcd_puts("Press Calibrate ", 0);
								eblcd_clear(1);
								eblcd_putc(' ');
								eblcd_putc(' ');
								eblcd_dec16(wait++);
							}//if
							else {
								eblcd_puts("Calibrated", 0);
								eblcd_clear(1);
							}//else
						}//if
						else {
							last_open = open;
							open = 0;
							x32 = measure_high_cap(mode, &esr);
							if (x32) {
								if (mode != 2) {
									//eblcd_puts("Capacitance Hi", 0);
									eblcd_puts(" --\2\4--", 0);
									mode = 2;
								}//if
								last_f3 = 0;
								if (x32 > 5000000000000.0)
									open = 2;
							}//if
							else {
								f3 = capture_pulses();
								if (f3 < 20) {
									if (mode != 0) {
										//eblcd_puts("Inductance", 0);
										eblcd_puts(" -\1\1\1\1-", 0);
										mode = 0;
									}//if
									open = 1;
									last_f3 = 0;
								}//if
								if ((f3>(f1-20)) && (f3<(f1+20))) {
									if (mode != 1) {
										//eblcd_puts("Capacitance", 0);
										eblcd_puts(" --\2\3--", 0);
										mode = 1;
									}//if
									open = 1;
									last_f3 = 0;
								}//if
								else {
									//______________ something connected
									if (mode <= 1) {
										//__________ big flutuation means trouble
										if (last_f3) {
											if ((f3 < (last_f3-20)) || (f3 > (last_f3+20))) {
												open = 2;	// mark out-of-range flag
											}//if
										}//if
										last_f3 = f3;
									}//if
								}//else
							}//else
						}//else
					}//if
					//eblcd_write(0x80+8, 1); eblcd_dec16(f3/8);
#ifdef DEBUG
					eblcd_clear(0);
					//eblcd_putc('=');
					//eblcd_putc(mode + '0');
					//eblcd_putc('=');
					//eblcd_putc(mmode + '0');
					eblcd_dec16(f1/8);
					eblcd_putc('=');
					eblcd_dec16(f2/8); 
					eblcd_putc('=');
					eblcd_dec16(f3/8);
					//eblcd_dec32((MHZ*1000000/0x10000) * (uint32_t) f3); eblcd_putc('=');
#endif
					if (mode <= 2) {
						if (f3 && mode <= 1) {
							float F1=f1, F2=f2, F3=f3;
							F1 *= (MHZ*1000000/0x10000/8);
							F2 *= (MHZ*1000000/0x10000/8);
							F3 *= (MHZ*1000000/0x10000/8);
							float B1 = F1/F3;
							float B2 = F1/F2;
							B1 *= B1; B1 -= 1.0; 
							B2 *= B2; B2 -= 1.0;
							float cx = (B1 / B2) * 1000 * 10;	// 0.1pF units

							float L = 1/(2.0 * 3.14159 * F1);
							L *= L;
							float lx = (B1 * B2) * L * 1000000000000;
							lx *= 10000000;		// 0.1 nH / unit
							x32 = mode == 1 ? (uint32_t) cx : (uint32_t) lx;
						}//if



						uint8_t dec=1, scale=0;
						static const char x_unit[3] = { 'H', 'F', 'F', };
						static const char x_scale[3][3] = {
								{ 'n', 'u', 'm', },
								{ 'p', 'n', 'u', },
								{ 'n', 'u', 'm', }, 
							};

						while (x32 > 9999) {
							x32 /= 10;
							dec--;
							if (!dec) {
								dec = 3;
								scale++;
							}//if
						}//while

						if (last_open == 1 && open != 1) {
							//__________ we don't want to show first reads as things are not settled yet
							eblcd_puts("Wait", 1);
						}//if
						else {
							if (open) {
								if (open == 2) {
									P1DIR |= PULSE_PIN|MEASURE_PIN;		// use chance to discharge large caps
									P1OUT &= ~(PULSE_PIN|MEASURE_PIN);		
									eblcd_puts("Out of Range", 1);
								}//if
								else {
									//___ we can turn off comparator now, no components attached
									//    turning it off will affect continous reads on test subject (low-range)
									//CACTL2 = CACTL1 = CAPD = 0;		
									eblcd_puts("       ", 1);
								}//if
							}//if
							else {
								eblcd_clear(1); 
								eblcd_dec32(x32, dec);
								eblcd_putc(x_scale[mode][scale]); eblcd_putc(x_unit[mode]);
								eblcd_putc(' ');
								if (mode==2) {		// show also esr on high range capacitance
									eblcd_write(0x80+8, 1); eblcd_puts(" -\5\6\5\6-", 9);
									eblcd_write(0x80+0x48, 1); eblcd_dec32(esr, 2); eblcd_putc('\7');
								}//if
							}//else
						}//else
						//eblcd_putc('='); eblcd_dec16(f3);

					}//if

					}
					break;
				case 'x': // filter on-off
					CACTL2 ^= CAF;
					break;

			}//switch
			c = 0;
		}//if

		if (!++cnt) {
			P1DIR &= ~PULSE_PIN;	// disconnect
			c = '=';				// change state to take reading
		}//if
		else {
			//P1DIR |= PULSE_PIN|MEASURE_PIN;		// use chance to discharge large caps
			//P1OUT &= ~(PULSE_PIN|MEASURE_PIN);		
		}//else
		__delay_cycles(50000);

	}//while


}

//________________________________________________________________________________
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void) {
	switch (TA0IV) {
		case 10:
			ov_cnt++;
			//__bic_SR_register_on_exit(LPM0_bits|GIE);
			__bic_SR_register_on_exit(LPM0_bits);
			break;
	}//swtich
}

//________________________________________________________________________________
#pragma vector=COMPARATORA_VECTOR
__interrupt void COMPARATORA_ISR(void) {
	capture_cnt++;
	if (capture_cnt > 0x8000) __bic_SR_register_on_exit(LPM0_bits|GIE);
}
/*
//________________________________________________________________________________
interrupt(WDT_VECTOR) WDT_ISR(void) {
	//paradiso_loop();
}
*/
