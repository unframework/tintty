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
#define TOUCH_POLL_FREQUENCY 40

uint8_t touchRisingCount = 0; // debounce counter for latch transition
bool touchActive = false; // touch status latch state
#define TOUCH_TRIGGER_COUNT 20

#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define KEYBOARD_HEIGHT 90
#define KEYBOARD_GUTTER 4

#define KEY_ROW_A_Y (ILI9341_HEIGHT - KEYBOARD_HEIGHT + KEYBOARD_GUTTER + KEY_HEIGHT / 2)
#define KEY_ROW_B_Y (KEY_ROW_A_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_ROW_C_Y (KEY_ROW_B_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_ROW_D_Y (KEY_ROW_C_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_WIDTH 18
#define KEY_HEIGHT 14
#define KEY_GUTTER 2

#define KEY_ROW_A(index) (14 + (KEY_WIDTH + KEY_GUTTER) * index), KEY_ROW_A_Y
#define KEY_ROW_B(index) (22 + (KEY_WIDTH + KEY_GUTTER) * index), KEY_ROW_B_Y
#define KEY_ROW_C(index) (30 + (KEY_WIDTH + KEY_GUTTER) * index), KEY_ROW_C_Y
#define KEY_ROW_D(index) (38 + (KEY_WIDTH + KEY_GUTTER) * index), KEY_ROW_D_Y

struct touchKey {
  int16_t cx, cy;
  char code;
} keyLayout[] = {
  { KEY_ROW_A(0), '0' },
  { KEY_ROW_A(1), '1' },
  { KEY_ROW_A(2), '2' },
  { KEY_ROW_A(3), '3' },
  { KEY_ROW_A(4), '4' },
  { KEY_ROW_A(5), '5' },
  { KEY_ROW_A(6), '6' },
  { KEY_ROW_A(7), '7' },
  { KEY_ROW_A(8), '8' },
  { KEY_ROW_A(9), '9' },

  { KEY_ROW_B(0), 'Q' },
  { KEY_ROW_B(1), 'W' },
  { KEY_ROW_B(2), 'E' },
  { KEY_ROW_B(3), 'R' },
  { KEY_ROW_B(4), 'T' },
  { KEY_ROW_B(5), 'Y' },
  { KEY_ROW_B(6), 'U' },
  { KEY_ROW_B(7), 'I' },
  { KEY_ROW_B(8), 'O' },
  { KEY_ROW_B(9), 'P' },

  { KEY_ROW_C(0), 'A' },
  { KEY_ROW_C(1), 'S' },
  { KEY_ROW_C(2), 'D' },
  { KEY_ROW_C(3), 'F' },
  { KEY_ROW_C(4), 'G' },
  { KEY_ROW_C(5), 'H' },
  { KEY_ROW_C(6), 'J' },
  { KEY_ROW_C(7), 'K' },
  { KEY_ROW_C(8), 'L' },

  { KEY_ROW_D(0), 'Z' },
  { KEY_ROW_D(1), 'X' },
  { KEY_ROW_D(2), 'C' },
  { KEY_ROW_D(3), 'V' },
  { KEY_ROW_D(4), 'B' },
  { KEY_ROW_D(5), 'N' },
  { KEY_ROW_D(6), 'M' }
};

const int keyCount = sizeof(keyLayout) / sizeof(keyLayout[0]);

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

  input_init();

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

      return test_buffer[test_buffer_cursor++];

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

void input_init() {
  uint16_t bgColor = tft.color565(0x20, 0x20, 0x20);
  uint16_t borderColor = tft.color565(0x80, 0x80, 0x80);
  uint16_t textColor = 0xFFFF;

  tft.fillRect(0, ILI9341_HEIGHT - KEYBOARD_HEIGHT, ILI9341_WIDTH, KEYBOARD_HEIGHT, bgColor);

  tft.setTextSize(1);
  tft.setTextColor(textColor);

  for (int i = 0; i < keyCount; i++) {
    const struct touchKey *key = &keyLayout[i];
    const int16_t ox = key->cx - KEY_WIDTH / 2;
    const int16_t oy = key->cy - KEY_HEIGHT / 2;

    tft.drawFastHLine(ox, oy, KEY_WIDTH, 0xFFFF);
    tft.drawFastHLine(ox, oy + KEY_HEIGHT - 1, KEY_WIDTH, 0xFFFF);
    tft.drawFastVLine(ox, oy, KEY_HEIGHT, 0xFFFF);
    tft.drawFastVLine(ox + KEY_WIDTH - 1, oy, KEY_HEIGHT, 0xFFFF);
    tft.fillRect(ox + 1, oy + 1, KEY_WIDTH - 2, KEY_HEIGHT - 2, 0);

    tft.setCursor(key->cx - 3, key->cy - 4);
    tft.print(key->code);
  }
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

  touchCountdown = TOUCH_POLL_FREQUENCY;

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
