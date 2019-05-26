/**
 * TinTTY main sketch
 * by Nick Matantsev 2017
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 */

#include <SPI.h>
#include <SoftwareSerial.h>

#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

#include "tintty.h"

// this is following a tutorial-recommended pinout, but changing
// the SPI CS and RS pins for the TFT to be 5 and 4 to avoid
// device conflict with USB Host Shield
// @todo rethink, now that USB Host Shield is not used directly
TFT_ILI9163C tft = TFT_ILI9163C(5, 8, 4);

// input serial forwarder RX, TX (TX should not be used anyway)
// SoftwareSerial inputSerial(9, 10);

// buffer to test various input sequences
char *test_buffer = "-- \e[1mTinTTY\e[m --\r\n";
uint8_t test_buffer_cursor = 0;

void setup() {
  Serial.begin(9600); // normal baud-rate
  // inputSerial.begin(9600); // normal baud-rate

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
      input_idle();

      // first read from the test buffer
      char test_char = test_buffer[test_buffer_cursor];

      if (test_char) {
        tintty_idle(&tft);
        input_idle();

        test_buffer_cursor += 1;
        return test_char;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
        input_idle();
      }

      return (char)Serial.read();
    },
    [=](char ch){ Serial.print(ch); },
    &tft
  );
}

void loop() {
}

void input_idle() {
  // if (inputSerial.available()) {
  //   Serial.write(inputSerial.read());
  // }
}
