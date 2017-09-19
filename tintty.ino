
// A FAST subset-VT100 serial terminal with 40 lines by 40 chars

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <EEPROM.h>

/**
  VT-100 code Copyright: Martin K. SchrÃ¶der (info@fortmax.se) 2014/10/27
  Conversion to Arduino, additional codes and this page by Peter Scargill 2016
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
// #include "ili9340.h"

#include "tintty.h"

/*
#include "vt100.h"

extern char new_br[8]; // baud-rate string - if non-zero will update screen
uint32_t charCounter=0;
uint32_t charShadow=0;
uint8_t  charStart=1;
*/

// this is following a tutorial-recommended pinout, but changing
// the SPI CS and RS pins for the TFT to be 5 and 4 to avoid
// device conflict with USB Host Shield
TFT_ILI9163C tft = TFT_ILI9163C(5, 8, 4);

// buffer to test various input sequences
char test_buffer[256];
uint8_t test_buffer_length = 0;
uint8_t test_buffer_cursor = 0;

void test_buffer_puts(char* str) {
  while (*str && test_buffer_length < sizeof(test_buffer)) {
    test_buffer[test_buffer_length] = *str;
    test_buffer_length += 1;

    str += 1;
  }
}

void setup() {
  Serial.begin(9600); // normal baud-rate
  // Serial1.begin(115200);

  tft.begin();
  // ili9340_init();
  // ili9340_setRotation(0);

//  test_buffer_puts("Hi!\nSecond\nThird\rStart\r\nt\te\ts\tt\bT\eM\eM1\b\eD2\eET\b\b\e[200B");
//  test_buffer_puts("\e[3;2f[3;2]");
//  test_buffer_puts("\e[14;9H[14;9]");
//  test_buffer_puts("\e[6;1H\e[32;40mSome \e[1mgreen\e[0;32m text\e[m (and back to \e[47;30mnormal\e[m)");
//  test_buffer_puts("\e7"); // save
//  test_buffer_puts("\e[14;20H\e[?7l0123456789\e[?7hnext line");
//  test_buffer_puts("\e[34l\e[?25h"); // cursor off and on
//  test_buffer_puts("\e8"); // restore

  tintty_run(
    [=](){
      // first peek from the test buffer
      if (test_buffer_cursor < test_buffer_length) {
        return test_buffer[test_buffer_cursor];
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
      }

      return (char)Serial.peek();
    },
    [=](){
      // first read from the test buffer
      if (test_buffer_cursor < test_buffer_length) {
        char ch = test_buffer[test_buffer_cursor];
        test_buffer_cursor += 1;

        return ch;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
      }

      return (char)Serial.read();
    },
    [=](char ch){ Serial.print(ch); },
    &tft
  );
}

/*
#define PURPLE_ON_BLACK "\e[35;40m"
#define GREEN_ON_BLACK "\e[32;40m"
*/

void loop() {

  /*
  auto respond = [=](char *str){ Serial.print(str); };
  vt100_init(respond);
  sei();

  // reset terminal and clear screen..
  vt100_puts("\e[c");   // terminal ok
  vt100_puts("\e[2J");  // erase entire screen
  vt100_puts("\e[?6l"); // absolute origin
  // print some fixed purple text top and bottom
  vt100_puts(PURPLE_ON_BLACK);
  vt100_puts("\e[2;1HSerial HC2016 Terminal 1.0");
  vt100_puts("\e[3;1HBaud: 115200");
  vt100_puts("\e[3;15HChars:");
  // 4 LEDS in the top corner initially set to OFF
  // ili9340_drawRect(186,6,10,10,ILI9340_RED,ILI9340_BLACK);
  // ili9340_drawRect(200,6,10,10,ILI9340_RED,ILI9340_BLACK);
  // ili9340_drawRect(214,6,10,10,ILI9340_RED,ILI9340_BLACK);
  // ili9340_drawRect(228,6,10,10,ILI9340_RED,ILI9340_BLACK);
  // delimit fixed areas
  // ili9340_drawFastHLine(0,20, 240, ILI9340_BLUE);
  // ili9340_drawFastHLine(0,300, 240, ILI9340_RED);
  vt100_puts("\e[4;38r"); // set the scrolling region
  vt100_puts(GREEN_ON_BLACK);
  vt100_puts("\e[5;1H"); // Set up at clear spot on screen
  // vt100_puts("\e[0q"); // All top corner LEDs off

  if ((EEPROM.read(BAUD_STORE)^EEPROM.read(BAUD_STORE+1))==0xff)
  {
    char bStr[12];
    sprintf(bStr,"\e[%dX",EEPROM.read(BAUD_STORE));
    vt100_puts(bStr);
  }
  else vt100_puts("\e[6X"); // baud rate 6 - i.e. 115200

  while(1){
    char data;
    data=Serial.read();
    if(data == -1)
      {
      // data=Serial1.read();
      if(data == -1)
          {
          //if nothing coming in serial - check for baud rate message
          if (new_br[0])
              {
                vt100_puts("\e7\e[39;7H"); // save cursor and attribs
                vt100_puts(PURPLE_ON_BLACK);
                vt100_puts(new_br);
                vt100_puts("\e8"); // restore cursor and attribs
                new_br[0]=0;
               }
          //or character count update
          if ((charCounter!=charShadow) || (charStart))
            {
              charShadow=charCounter; charStart=0;
              vt100_puts("\e7");  //save cursor and color
              vt100_puts(PURPLE_ON_BLACK);
              char numbers[12]; sprintf(numbers,"%ld",charCounter);
              vt100_puts("\e[39;22H"); vt100_puts(numbers);
              vt100_puts("\e8");  //restore cursor and attribs
            }
            continue;
          }
      }
    charCounter++;
    vt100_putc(data);
  }
  */
}
