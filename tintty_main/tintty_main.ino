/**
 * TinTTY main sketch
 * by Nick Matantsev 2017
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 */

#include <SPI.h>

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

#include "input.h"
#include "tintty.h"

// using stock MCUFRIEND 2.4inch shield
MCUFRIEND_kbv tft;

struct tintty_display ili9341_display = {
  ILI9341_WIDTH,
  (ILI9341_HEIGHT - KEYBOARD_HEIGHT),
  ILI9341_WIDTH / TINTTY_CHAR_WIDTH,
  (ILI9341_HEIGHT - KEYBOARD_HEIGHT) / TINTTY_CHAR_HEIGHT,

  [=](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
    tft.fillRect(x, y, w, h, color);
  },

  [=](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pixels){
    tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    tft.pushColors(pixels, w * h, 1);
  },

  [=](int16_t offset){
    tft.vertScroll(0, (ILI9341_HEIGHT - KEYBOARD_HEIGHT), offset);
  }
};

// buffer to test various input sequences
char *test_buffer = "-- \e[1mTinTTY\e[m --\r\n";
uint8_t test_buffer_cursor = 0;

void setup() {
  Serial.begin(9600); // common default baud-rate (also avoids buffer overruns)

  uint16_t tftID = tft.readID();
  tft.begin(tftID);

  input_init();

  tintty_run(
    [=](){
      // peek idle loop
      while (true) {
        // first peek from the test buffer
        if (test_buffer[test_buffer_cursor]) {
          return test_buffer[test_buffer_cursor];
        }

        // fall back to normal blocking serial input
        if (Serial.available() > 0) {
          return (char)Serial.peek();
        }

        // idle logic only after peeks failed
        tintty_idle(&ili9341_display);
        input_idle();
      }
    },
    [=](){
      while(true) {
        // process at least one idle loop first to allow input to happen
        tintty_idle(&ili9341_display);
        input_idle();

        // first read from the test buffer
        if (test_buffer[test_buffer_cursor]) {
          return test_buffer[test_buffer_cursor++];
        }

        // fall back to normal blocking serial input
        if (Serial.available() > 0) {
          return (char)Serial.read();
        }
      }
    },
    [=](char ch){ Serial.print(ch); },
    &ili9341_display
  );
}

void loop() {
}
