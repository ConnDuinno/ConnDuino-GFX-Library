/*
This is a fork of the Adafruit_GFX library for Arduino and 
combatible boards. The purpose of the fork is to add  
capabilities for:
 1. drawing bitmaps stored in an external eeprom (i2c)
 2. drawing bitmaps stored in ram (without the progmem directive)
 3. include user interface widgets

This library will be used for the ConnDuino board projects.
For more info about the board visit:  www.connduino.com


              ADAFRUIT_GFX master, copyright notice
=========================================================================
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!
 
Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "ConnD_GFX.h"
#include "ConnD_EEPROM.h"
#include "Wire.h"
#if USE_I2C_FONT==0
	#include "glcdfont.c"
#endif
#ifdef __AVR__
 #include <avr/pgmspace.h>
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif


#define CONND_GFX_I2C_READBLOCK 16


ConnD_GFX::ConnD_GFX(int16_t w, int16_t h):
  WIDTH(w), HEIGHT(h)
{
  _width    = WIDTH;
  _height   = HEIGHT;
  rotation  = 0;
  cursor_y  = cursor_x    = 0;
  textsize  = 1;
  textcolor = textbgcolor = 0xFFFF;
  wrap      = true;
}



// Draw a circle outline
void ConnD_GFX::drawCircle(int16_t x0, int16_t y0, int16_t r,
    uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void ConnD_GFX::drawCircleHelper( int16_t x0, int16_t y0,
               int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    } 
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void ConnD_GFX::fillCircle(int16_t x0, int16_t y0, int16_t r,
			      uint16_t color) {
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void ConnD_GFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
    uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Bresenham's algorithm - thx wikpedia
void ConnD_GFX::drawLine(int16_t x0, int16_t y0,
			    int16_t x1, int16_t y1,
			    uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// Draw a rectangle
void ConnD_GFX::drawRect(int16_t x, int16_t y,
			    int16_t w, int16_t h,
			    uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

void ConnD_GFX::drawFastVLine(int16_t x, int16_t y,
				 int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x, y+h-1, color);
}

void ConnD_GFX::drawFastHLine(int16_t x, int16_t y,
				 int16_t w, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x+w-1, y, color);
}

void ConnD_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
			    uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}

void ConnD_GFX::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

// Draw a rounded rectangle
void ConnD_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t color) {
  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void ConnD_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w,
				 int16_t h, int16_t r, uint16_t color) {
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void ConnD_GFX::drawTriangle(int16_t x0, int16_t y0,
				int16_t x1, int16_t y1,
				int16_t x2, int16_t y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void ConnD_GFX::fillTriangle ( int16_t x0, int16_t y0,
				  int16_t x1, int16_t y1,
				  int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }
}

void ConnD_GFX::drawBitmap(int16_t x, int16_t y,
			      const uint8_t *bitmap, int16_t w, int16_t h,
			      uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        drawPixel(x+i, y+j, color);
      }
    }
  }
}

// Draw a 1-bit color bitmap at the specified x, y position from the
// provided bitmap buffer (must be PROGMEM memory) using color as the
// foreground color and bg as the background color.
void ConnD_GFX::drawBitmap(int16_t x, int16_t y,
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color, uint16_t bg) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        drawPixel(x+i, y+j, color);
      }
      else {
      	drawPixel(x+i, y+j, bg);
      }
    }
  }
}


// Draws a 1-bit bitmap from a sketch byte array 
//  (declared without the PROGMEM directive).
// The w=width, h=height dimensions should be multiple of 8.
void ConnD_GFX::drawBitmapRAM(	int16_t x, int16_t y, const uint8_t *bitmap, 
									int16_t w, int16_t h,
									uint16_t color) {

	int16_t i, j, k, byteWidth = w/8;
	int16_t  x0 = x;
	
	for (i=0; i<h; i++){
		for (j=0; j<byteWidth; j++){
			uint8_t b = bitmap[0]; 
			for (k=0; k<8; k++){
				if ( b & 128 )    drawPixel(x, y, color);
				x++;	
				b<<=1;	//next bit
			}
			bitmap++;	//next byte
		}
		y++;
		x=x0;
	}
}


// Draws a 1-bit bitmap from a byte array stored in an eeprom i2c device.
// The w=width, h=height dimensions should be multiples of 8.
// The memAddr argument tells where the position in memory space
//    of the 1st byte. This should be multiple of 16 (i.e. 0,16,32 etc.)
void ConnD_GFX::drawBitmap_i2c(int16_t x, int16_t y, int16_t memAddr,
									int16_t w, int16_t h,
									uint16_t color) {


	int16_t byteLen = h*w/8;	//the total number of bytes to be read
	int16_t x0   = x;			//the min x of the drawn pixels
	int16_t xEnd = x+w;			//the max x of the drawn pixels
	uint8_t data;				//will store the currently read byte 
	int16_t bytes=0;			//counter for the read bytes 
	//int16_t nBlocks = (byteLen + EEPROM_READ_BLOCK_SIZE -1 )/EEPROM_READ_BLOCK_SIZE; 
					//the number of blocks to be read
	uint8_t  b, bit;	

	_ee->setMemAddr(memAddr);
	Wire.endTransmission();
	
	while (true){
		//fill the buffer with next block
		Wire.requestFrom(_ee->getI2CAddr(), (uint8_t)CONND_GFX_I2C_READBLOCK);

		for (uint8_t b = 0; b < CONND_GFX_I2C_READBLOCK; b++){
			if ( bytes < byteLen ) {
				if (Wire.available()){
					data = Wire.read();
					for (bit=0; bit<8; bit++){
						if ( data & 128 )    drawPixel(x, y, color);
						x++;	
						data<<=1;	//next bit
					}
					bytes++; 
					if (x>=xEnd){
						y++;
						x=x0;
					}
				}
			}
			else return;
		}//next byte in block	
	}//next block
}


// Draws a 1-bit bitmap from a byte array stored in an eeprom i2c device.
// The w=width, h=height dimensions should be multiples of 8.
// The memAddr argument tells where the position in memory space
//    of the 1st byte. This may be anything. Page borders are respected.
// 
void ConnD_GFX::drawBitmap_i2c(int16_t x, int16_t y, int16_t memAddr,
									int16_t w, int16_t h,
									uint16_t color, uint16_t bg) {


	int16_t byteLen = h*w/8;	//the total number of bytes to be read
	int16_t x0   = x;			//the min x of the drawn pixels
	int16_t xEnd = x+w;			//the max x of the drawn pixels
	uint8_t data;				//will store the currently read byte 
	uint8_t blockBytes = (memAddr / CONND_GFX_I2C_READBLOCK + 1)*CONND_GFX_I2C_READBLOCK - memAddr;
	uint8_t  b, bit;	

	_ee->setMemAddr(memAddr);
	Wire.endTransmission();
	
	Wire.requestFrom(_ee->getI2CAddr(), blockBytes); //initial request

	for (int16_t b=0; b<byteLen; b++){
		if (blockBytes==0){
			//subsequent request fetch a full block size
			Wire.requestFrom(_ee->getI2CAddr(), (uint8_t)CONND_GFX_I2C_READBLOCK);
			blockBytes = CONND_GFX_I2C_READBLOCK;
		}
		if(Wire.available()){
			data = Wire.read();
			blockBytes--;
			drawByte(x, y, data, color, bg, 1);
			x += 8;
			if (x>=xEnd){
				y++;
				x=x0;
			}
		}
	}
}




//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void ConnD_GFX::drawXBitmap(int16_t x, int16_t y,
                              const uint8_t *bitmap, int16_t w, int16_t h,
                              uint16_t color) {
  
  int16_t i, j, byteWidth = (w + 7) / 8;
  
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i % 8))) {
        drawPixel(x+i, y+j, color);
      }
    }
  }
}


#if ARDUINO >= 100
size_t ConnD_GFX::write(uint8_t c) {
#else
void ConnD_GFX::write(uint8_t c) {
#endif
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
#if USE_I2C_FONT==0
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
#else
	  drawChar_i2c(cursor_x, cursor_y, c, textcolor, textbgcolor);
	  uint8_t padding = _font->charW[c - _font->firstChar] + 1;
	  cursor_x += padding;
	  if (wrap && (cursor_x > (_width - padding))) {
		  cursor_y += _font->byteH * 8;
		  cursor_x = 0;
	  }
#endif
  }
#if ARDUINO >= 100
  return 1;
#endif
}


#if USE_I2C_FONT==0
void ConnD_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
			    uint16_t color, uint16_t bg, uint8_t size) {

  if((x >= _width)            || // Clip right
     (y >= _height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for (int8_t i=0; i<6; i++ ) {
    uint8_t line;
    if (i == 5) 
      line = 0x0;
    else 
      line = pgm_read_byte(font+(c*5)+i);
    for (int8_t j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        } 
      } else if (bg != color) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, bg);
        else {  // big size
          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}
#else
void ConnD_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
	uint16_t color, uint16_t bg, uint8_t size) {}
#endif

#if USE_I2C_FONT>0
void  
ConnD_GFX::drawChar_i2c(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg){

	uint8_t w		 = _font->charW[c - _font->firstChar];
	uint16_t memAddr = _font->charOffset[c - _font->firstChar] + _font->dataAddr0;
	int8_t byteLen   = _font->byteH * w;	//the total number of bytes to be read
	
	uint8_t data;
	uint8_t iCol=0;
	int16_t x0 = x;
	int16_t y0 = y;

	
	_ee->setMemAddr(memAddr);
	Wire.endTransmission();

	uint8_t blockBytes = byteLen < CONND_GFX_I2C_READBLOCK ?
							byteLen : (uint8_t)CONND_GFX_I2C_READBLOCK;
	uint8_t block0    = blockBytes;
	uint8_t bytesLeft = byteLen;
	Wire.requestFrom(_ee->getI2CAddr(), blockBytes); //initial request


	for (int16_t b = 0; b<byteLen; b++){
		if (blockBytes <= 0){
			bytesLeft -= block0;
			blockBytes = bytesLeft < CONND_GFX_I2C_READBLOCK ?
								 bytesLeft : (uint8_t)CONND_GFX_I2C_READBLOCK;
			block0 = blockBytes;
			Wire.requestFrom(_ee->getI2CAddr(), blockBytes);
		}
		if (Wire.available()){
			data = Wire.read();
			blockBytes--;
			drawByte(x, y0, data, color, bg, 0);	
			iCol++;
			if (iCol >= w){
				iCol = 0;
				x = x0;
				y0 += 8;
			}
			else{
				x++;
			}
		}
	}
}
#else
void
ConnD_GFX::drawChar_i2c(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg){}
#endif

void ConnD_GFX::setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

#if USE_I2C_FONT==0
void ConnD_GFX::setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}
#endif

void ConnD_GFX::setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg 
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}

void ConnD_GFX::setTextColor(uint16_t c, uint16_t b) {
  textcolor   = c;
  textbgcolor = b; 
}

void ConnD_GFX::setTextWrap(boolean w) {
  wrap = w;
}

uint8_t ConnD_GFX::getRotation(void) const {
  return rotation;
}

void ConnD_GFX::setRotation(uint8_t x) {
  rotation = (x & 3);
  switch(rotation) {
   case 0:
   case 2:
    _width  = WIDTH;
    _height = HEIGHT;
    break;
   case 1:
   case 3:
    _width  = HEIGHT;
    _height = WIDTH;
    break;
  }
}

// Return the size of the display (per current rotation)
int16_t ConnD_GFX::width(void) const {
  return _width;
}
 
int16_t ConnD_GFX::height(void) const {
  return _height;
}

void ConnD_GFX::invertDisplay(boolean i) {
  // Do nothing, must be subclassed if supported
}

