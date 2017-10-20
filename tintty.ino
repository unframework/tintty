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
#include "tintty_usbkeyboard.h"

// this is following a tutorial-recommended pinout, but changing
// the SPI CS and RS pins for the TFT to be 5 and 4 to avoid
// device conflict with USB Host Shield
TFT_ILI9163C tft = TFT_ILI9163C(5, 8, 4);

// buffer to test various input sequences
char *test_buffer = "-- \e[1mTinTTY\e[m --\r\n";
uint8_t test_buffer_cursor = 0;

void setup() {
  Serial.begin(9600); // normal baud-rate

  tintty_usbkeyboard_init();

  tft.begin();

  tintty_run(
    [=](){
      // first peek from the test buffer
      char test_char = test_buffer[test_buffer_cursor];

      if (test_char) {
        tintty_idle(&tft);
        return test_char;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
      }

      return (char)Serial.peek();
    },
    [=](){
      // process at least one idle loop to allow input to happen
      tintty_idle(&tft);
      tintty_usbkeyboard_update(); // @todo move?

      // first read from the test buffer
      char test_char = test_buffer[test_buffer_cursor];

      if (test_char) {
        tintty_idle(&tft);
        tintty_usbkeyboard_update(); // @todo move?

        test_buffer_cursor += 1;
        return test_char;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
        tintty_usbkeyboard_update(); // @todo move?
      }

      return (char)Serial.read();
    },
    [=](char ch){ Serial.print(ch); },
    &tft
  );
}

void loop() {
}
