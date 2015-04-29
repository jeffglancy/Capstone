// Modified from Adafruit Arduino library
// Jeff Glancy
// 9/4/14

/*
Arduino library to control LPD8806-based RGB LED Strips
Copyright (C) Adafruit Industries
MIT license

Clearing up some misconceptions about how the LPD8806 drivers work:

The LPD8806 is not a FIFO shift register.  The first data out controls the
LED *closest* to the processor (unlike a typical shift register, where the
first data out winds up at the *furthest* LED).  Each LED driver 'fills up'
with data and then passes through all subsequent bytes until a latch
condition takes place.  This is actually pretty common among LED drivers.

All color data bytes have the high bit (128) set, with the remaining
seven bits containing a brightness value (0-127).  A byte with the high
bit clear has special meaning (explained later).

The rest gets bizarre...

The LPD8806 does not perform an in-unison latch (which would display the
newly-transmitted data all at once).  Rather, each individual byte (even
the separate G, R, B components of each LED) is latched AS IT ARRIVES...
or more accurately, as the first bit of the subsequent byte arrives and
is passed through.  So the strip actually refreshes at the speed the data
is issued, not instantaneously (this can be observed by greatly reducing
the data rate).  This has implications for POV displays and light painting
applications.  The 'subsequent' rule also means that at least one extra
byte must follow the last pixel, in order for the final blue LED to latch.

To reset the pass-through behavior and begin sending new data to the start
of the strip, a number of zero bytes must be issued (remember, all color
data bytes have the high bit set, thus are in the range 128 to 255, so the
zero is 'special').  This should be done before each full payload of color
values to the strip.  Curiously, zero bytes can only travel one meter (32
LEDs) down the line before needing backup; the next meter requires an
extra zero byte, and so forth.  Longer strips will require progressively
more zeros.  *(see note below)

In the interest of efficiency, it's possible to combine the former EOD
extra latch byte and the latter zero reset...the same data can do double
duty, latching the last blue LED while also resetting the strip for the
next payload.

So: reset byte(s) of suitable length are issued once at startup to 'prime'
the strip to a known ready state.  After each subsequent LED color payload,
these reset byte(s) are then issued at the END of each payload, both to
latch the last LED and to prep the strip for the start of the next payload
(even if that data does not arrive immediately).  This avoids a tiny bit
of latency as the new color payload can begin issuing immediately on some
signal, such as a timer or GPIO trigger.

Technically these zero byte(s) are not a latch, as the color data (save
for the last byte) is already latched.  It's a start-of-data marker, or
an indicator to clear the thing-that's-not-a-shift-register.  But for
conversational consistency with other LED drivers, we'll refer to it as
a 'latch' anyway.

* This has been validated independently with multiple customers'
  hardware.  Please do not report as a bug or issue pull requests for
  this.  Fewer zeros sometimes gives the *illusion* of working, the first
  payload will correctly load and latch, but subsequent frames will drop
  data at the end.  The data shortfall won't always be visually apparent
  depending on the color data loaded on the prior and subsequent frames.
  Tested.  Confirmed.  Fact.
*/

#include "LPD8806.h"
#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*****************************************************************************/

  uint16_t
    numLEDs,    // Number of RGB LEDs in strip
    numBytes;   // Size of 'pixels' buffer below
  uint8_t
    *pixels;    // Holds LED color values (3 bytes each) + latch bytes

// Constructor for use with hardware SPI (specific clock/data pins):
extern void LPD8806init(uint16_t n)
{
	bcm2835_init();
	
	pixels = NULL;
	LPD8806updateLength(n);
  
	//initialize SPI
	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);	//7.4MHz clock speed - should work at 20 MHz

	// Issue initial latch/reset to strip:
	uint16_t i;
	for(i = ((numLEDs+31)/32); i > 0; i--)
	{
		bcm2835_spi_transfer(0);
	}

}

extern void LPD8806quit()
{
	// Issue initial latch/reset to strip:
	uint16_t i;
	for (i = 0; i < numLEDs; i++)
	{
		LPD8806setPixelColor2(i, 0);
	}
	LPD8806show();
	
	bcm2835_spi_end();
	bcm2835_close();
}

// Change strip length (see notes with empty constructor, above):
extern void LPD8806updateLength(uint16_t n)
{
  uint8_t  latchBytes;
  uint16_t dataBytes, totalBytes;

  numLEDs = numBytes = 0;
  if(pixels) free(pixels); // Free existing data (if any)

  dataBytes  = n * 3;
  latchBytes = (n + 31) / 32;
  totalBytes = dataBytes + latchBytes;
  if((pixels = (uint8_t *)malloc(totalBytes))) // Alloc new data
  { 
    numLEDs  = n;
    numBytes = totalBytes;
    memset( pixels           , 0x80, dataBytes);  // Init to RGB 'off' state
    memset(&pixels[dataBytes], 0   , latchBytes); // Clear latch bytes
  }
}

extern uint16_t LPD8806numPixels(void)
{
  return numLEDs;
}

extern void LPD8806show(void)
{
  uint8_t  *ptr = pixels;
  uint16_t i    = numBytes;

  // This doesn't need to distinguish among individual pixel color
  // bytes vs. latch data, etc.  Everything is laid out in one big
  // flat buffer and issued the same regardless of purpose.
	while (i--)
	{
		bcm2835_spi_transfer(*ptr++);
	}
}

// Convert separate R,G,B into combined 32-bit GRB color:
extern uint32_t LPD8806Color(uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t)(g | 0x80) << 16) |
         ((uint32_t)(r | 0x80) <<  8) |
                     b | 0x80 ;
}

// Set pixel color from separate 7-bit R, G, B components:
extern void LPD8806setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = g | 0x80; // Strip color order is GRB,
    *p++ = r | 0x80; // not the more common RGB,
    *p++ = b | 0x80; // so the order here is intentional; don't "fix"
  }
}

// Set pixel color from 'packed' 32-bit GRB (not RGB) value:
extern void LPD8806setPixelColor2(uint16_t n, uint32_t c)
{
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = (c >> 16) | 0x80;
    *p++ = (c >>  8) | 0x80;
    *p++ =  c        | 0x80;
  }
}

// Query color from previously-set pixel (returns packed 32-bit GRB value)
extern uint32_t LPD8806getPixelColor(uint16_t n)
{
  if(n < numLEDs) {
    uint16_t ofs = n * 3;
    return ((uint32_t)(pixels[ofs    ] & 0x7f) << 16) |
           ((uint32_t)(pixels[ofs + 1] & 0x7f) <<  8) |
            (uint32_t)(pixels[ofs + 2] & 0x7f);
  }

  return 0; // Pixel # is out of bounds
}
