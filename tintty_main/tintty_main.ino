/**
 * TinTTY main sketch
 * by Nick Matantsev 2017
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 */

#include <SPI.h>
// #include <SoftwareSerial.h>

#include <Adafruit_GFX.h>
#include <TouchScreen.h>
#include <MCUFRIEND_kbv.h>

#include "tintty.h"

// using stock MCUFRIEND 2.4inch shield
MCUFRIEND_kbv tft;

#define ILI9341_WIDTH 240
#define ILI9341_HEIGHT 320

// calibrated settings from TouchScreen_Calibr_native
const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9341
const int TS_LEFT=115,TS_RT=902,TS_TOP=72,TS_BOT=897;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

uint8_t touchCountdown = 0;

uint8_t touchRisingCount = 0; // debounce counter for latch transition
bool touchActive = false; // touch status latch state
#define TOUCH_TRIGGER_COUNT 5

#define MINPRESSURE 200
#define MAXPRESSURE 1000

struct tintty_display ili9341_display = {
  ILI9341_WIDTH,
  ILI9341_HEIGHT,
  ILI9341_WIDTH / TINTTY_CHAR_WIDTH,
  ILI9341_HEIGHT / TINTTY_CHAR_HEIGHT,

  [=](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
    tft.fillRect(x, y, w, h, color);
  },

  [=](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pixels){
    tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
    tft.pushColors(pixels, w * h, 1);
  },

  [=](int16_t offset){
    tft.vertScroll(0, 320, offset);
  }
};

// input serial forwarder RX, TX (TX should not be used anyway)
// @todo separate out to be configuration-specific and replace with touchscreen
// SoftwareSerial inputSerial(9, 10);

// buffer to test various input sequences
char *test_buffer = "-- \e[1mTinTTY\e[m --\r\n";
uint8_t test_buffer_cursor = 0;

void setup() {
  Serial.begin(9600); // normal baud-rate
  // inputSerial.begin(9600); // normal baud-rate

  uint16_t tftID = tft.readID();
  tft.begin(tftID);

  tintty_run(
    [=](){
      // first peek from the test buffer
      while (!test_buffer[test_buffer_cursor]) {
        tintty_idle(&ili9341_display);
        input_idle();
      }

      return test_buffer[test_buffer_cursor];

      // // fall back to normal blocking serial input
      // while (Serial.available() < 1) {
      //   tintty_idle(&ili9341_display);
      //   input_idle();
      // }

      // return (char)Serial.peek();
    },
    [=](){
      // process at least one idle loop to allow input to happen
      tintty_idle(&ili9341_display);
      input_idle();

      // first read from the test buffer
      while (!test_buffer[test_buffer_cursor]) {
        tintty_idle(&ili9341_display);
        input_idle();
      }

      test_buffer_cursor += 1;
      return test_buffer[test_buffer_cursor];

      // // fall back to normal blocking serial input
      // while (Serial.available() < 1) {
      //   tintty_idle(&ili9341_display);
      //   input_idle();
      // }

      // return (char)Serial.read();
    },
    [=](char ch){ Serial.print(ch); },
    &ili9341_display
  );
}

void loop() {
}

void input_idle() {
  // if (inputSerial.available()) {
  //   Serial.write(inputSerial.read());
  // }

  // only poll once every 10 iterations
  if (touchCountdown > 0) {
    touchCountdown--;
    return;
  }

  touchCountdown = 20;

  // get raw touch values and reset the mode of the touchscreen pins due to them being shared
  tp = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // debounce the touch status
  const bool isRising = tp.z > MINPRESSURE && tp.z < MAXPRESSURE;

  if (isRising) {
    touchRisingCount = min(TOUCH_TRIGGER_COUNT, touchRisingCount + 1);
  } else {
    touchRisingCount = max(0, touchRisingCount - 1);
  }

  // perform the status latch transitions
  if (!touchActive) {
    // catch the rising transition and flip the status latch on
    if (isRising && touchRisingCount == TOUCH_TRIGGER_COUNT) {
      // flip to active mode
      touchActive = true;

      const uint16_t xpos = map(tp.x, TS_LEFT, TS_RT, 0, ILI9341_WIDTH);
      const uint16_t ypos = map(tp.y, TS_TOP, TS_BOT, 0, ILI9341_HEIGHT);

      // trigger debug display for now
      // @todo this
      test_buffer_cursor = 0;
    }
  } else if (touchActive) {
    // flip status latch to off once settled back to zero
    if (!isRising && touchRisingCount == 0) {
      touchActive = false;
    }
  }
}
