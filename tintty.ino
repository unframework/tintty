/**
 * TinTTY main sketch
 * by Nick Matantsev 2017
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

#include "tintty.h"

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

  tft.begin();

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

void loop() {
}
