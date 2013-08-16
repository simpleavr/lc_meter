
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

