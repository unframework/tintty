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

#define KEYBOARD_HEIGHT 92
#define KEYBOARD_GUTTER 4

#define KEY_WIDTH 16
#define KEY_HEIGHT 16
#define KEY_GUTTER 1

#define KEY_ROW_A_Y (ILI9341_HEIGHT - KEYBOARD_HEIGHT + KEYBOARD_GUTTER + KEY_HEIGHT / 2)
#define KEY_ROW_B_Y (KEY_ROW_A_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_ROW_C_Y (KEY_ROW_B_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_ROW_D_Y (KEY_ROW_C_Y + KEY_GUTTER + KEY_HEIGHT)
#define KEY_ROW_E_Y (KEY_ROW_D_Y + KEY_GUTTER + KEY_HEIGHT)

#define KEY_ROW_A_X(index) (9 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_B_X(index) (17 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_C_X(index) (30 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_D_X(index) (38 + (KEY_WIDTH + KEY_GUTTER) * index)

#define KEY_LSHIFT_WIDTH (KEY_ROW_D_X(0) - KEY_WIDTH / 2 - KEY_GUTTER - 1)
#define KEY_RSHIFT_WIDTH (ILI9341_WIDTH - KEY_ROW_D_X(9) - KEY_WIDTH / 2 - KEY_GUTTER - 1)

#define KEYCODE_SHIFT -20

int16_t touchKeyRowY[] = {
  KEY_ROW_A_Y,
  KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 1,
  KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 2,
  KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 3,
  KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 4
};

const int touchKeyRowCount = 5;

struct touchKey {
  int16_t cx, width;
  char code, shiftCode, label;
};

struct touchKey touchKeyRowA[] = {
  { KEY_ROW_A_X(0), KEY_WIDTH, '`', '~', 0 },
  { KEY_ROW_A_X(1), KEY_WIDTH, '1', '!', 0 },
  { KEY_ROW_A_X(2), KEY_WIDTH, '2', '@', 0 },
  { KEY_ROW_A_X(3), KEY_WIDTH, '3', '#', 0 },
  { KEY_ROW_A_X(4), KEY_WIDTH, '4', '$', 0 },
  { KEY_ROW_A_X(5), KEY_WIDTH, '5', '%', 0 },
  { KEY_ROW_A_X(6), KEY_WIDTH, '6', '^', 0 },
  { KEY_ROW_A_X(7), KEY_WIDTH, '7', '&', 0 },
  { KEY_ROW_A_X(8), KEY_WIDTH, '8', '*', 0 },
  { KEY_ROW_A_X(9), KEY_WIDTH, '9', '(', 0 },
  { KEY_ROW_A_X(10), KEY_WIDTH, '0', ')', 0 },
  { KEY_ROW_A_X(11), KEY_WIDTH, '-', '_', 0 },
  { KEY_ROW_A_X(12), KEY_WIDTH, '=', '+', 0 }
};

struct touchKey touchKeyRowB[] = {
  { KEY_ROW_B_X(0), KEY_WIDTH, 'q', 'Q', 0 },
  { KEY_ROW_B_X(1), KEY_WIDTH, 'w', 'W', 0 },
  { KEY_ROW_B_X(2), KEY_WIDTH, 'e', 'E', 0 },
  { KEY_ROW_B_X(3), KEY_WIDTH, 'r', 'R', 0 },
  { KEY_ROW_B_X(4), KEY_WIDTH, 't', 'T', 0 },
  { KEY_ROW_B_X(5), KEY_WIDTH, 'y', 'Y', 0 },
  { KEY_ROW_B_X(6), KEY_WIDTH, 'u', 'U', 0 },
  { KEY_ROW_B_X(7), KEY_WIDTH, 'i', 'I', 0 },
  { KEY_ROW_B_X(8), KEY_WIDTH, 'o', 'O', 0 },
  { KEY_ROW_B_X(9), KEY_WIDTH, 'p', 'P', 0 },
  { KEY_ROW_B_X(10), KEY_WIDTH, '[', '{', 0 },
  { KEY_ROW_B_X(11), KEY_WIDTH, ']', '}', 0 }
};

struct touchKey touchKeyRowC[] = {
  { KEY_ROW_C_X(0), KEY_WIDTH, 'a', 'A', 0 },
  { KEY_ROW_C_X(1), KEY_WIDTH, 's', 'S', 0 },
  { KEY_ROW_C_X(2), KEY_WIDTH, 'd', 'D', 0 },
  { KEY_ROW_C_X(3), KEY_WIDTH, 'f', 'F', 0 },
  { KEY_ROW_C_X(4), KEY_WIDTH, 'g', 'G', 0 },
  { KEY_ROW_C_X(5), KEY_WIDTH, 'h', 'H', 0 },
  { KEY_ROW_C_X(6), KEY_WIDTH, 'j', 'J', 0 },
  { KEY_ROW_C_X(7), KEY_WIDTH, 'k', 'K', 0 },
  { KEY_ROW_C_X(8), KEY_WIDTH, 'l', 'L', 0 },
  { KEY_ROW_C_X(9), KEY_WIDTH, ';', ':', 0 },
  { KEY_ROW_C_X(10), KEY_WIDTH, '\'', '"', 0 },
  { KEY_ROW_C_X(11), KEY_WIDTH, '\\', '|', 0 }
};

struct touchKey touchKeyRowD[] = {
  { KEY_ROW_D_X(0), KEY_WIDTH, 'z', 'Z', 0 },
  { KEY_ROW_D_X(1), KEY_WIDTH, 'x', 'X', 0 },
  { KEY_ROW_D_X(2), KEY_WIDTH, 'c', 'C', 0 },
  { KEY_ROW_D_X(3), KEY_WIDTH, 'v', 'V', 0 },
  { KEY_ROW_D_X(4), KEY_WIDTH, 'b', 'B', 0 },
  { KEY_ROW_D_X(5), KEY_WIDTH, 'n', 'N', 0 },
  { KEY_ROW_D_X(6), KEY_WIDTH, 'm', 'M', 0 },
  { KEY_ROW_D_X(7), KEY_WIDTH, ',', '<', 0 },
  { KEY_ROW_D_X(8), KEY_WIDTH, '.', '>', 0 },
  { KEY_ROW_D_X(9), KEY_WIDTH, '/', '?', 0 },

  {
    1 + KEY_LSHIFT_WIDTH / 2,
    KEY_LSHIFT_WIDTH,
    KEYCODE_SHIFT,
    KEYCODE_SHIFT,
    '^'
  },

  {
    ILI9341_WIDTH - 2 - KEY_RSHIFT_WIDTH / 2,
    KEY_RSHIFT_WIDTH,
    KEYCODE_SHIFT,
    KEYCODE_SHIFT,
    '^'
  }
};

struct touchKey touchKeyRowE[] = {
  { ILI9341_WIDTH / 2, 100, ' ', ' ', ' ' }
};

struct touchKey *touchKeyRowContents[] = {
  touchKeyRowA,
  touchKeyRowB,
  touchKeyRowC,
  touchKeyRowD,
  touchKeyRowE
};

int touchKeyRowContentsCount[] = {
  13, 12, 12, 12, 1
};

int activeRow = -1;
struct touchKey *activeKey = NULL;
bool shiftIsActive = false;

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

void _input_draw_key(int keyRow, struct touchKey *key) {
  const int16_t rowCY = touchKeyRowY[keyRow];
  const bool isActive = key == activeKey;

  uint16_t keyColor = isActive ? 0xFFFF : 0;
  uint16_t borderColor = isActive ? 0xFFFF : tft.color565(0x80, 0x80, 0x80);
  uint16_t textColor = isActive ? 0 : 0xFFFF;

  tft.setTextColor(textColor);

  const int16_t ox = key->cx - key->width / 2;
  const int16_t oy = rowCY - KEY_HEIGHT / 2;

  tft.drawFastHLine(ox, oy, key->width, borderColor);
  tft.drawFastHLine(ox, oy + KEY_HEIGHT - 1, key->width, borderColor);
  tft.drawFastVLine(ox, oy, KEY_HEIGHT, borderColor);
  tft.drawFastVLine(ox + key->width - 1, oy, KEY_HEIGHT, borderColor);
  tft.fillRect(ox + 1, oy + 1, key->width - 2, KEY_HEIGHT - 2, keyColor);

  tft.setCursor(key->cx - 3, rowCY - 4);
  tft.print(
    key->label == 0
      ? (shiftIsActive ? key->shiftCode : key->code)
      : key->label
  );
}

void _input_draw_all_keys() {
  for (int i = 0; i < touchKeyRowCount; i++) {
    const int keyCount = touchKeyRowContentsCount[i];

    for (int j = 0; j < keyCount; j++) {
      _input_draw_key(i, &touchKeyRowContents[i][j]);
    }
  }
}

void _input_process_touch(int16_t xpos, int16_t ypos) {
  activeRow = -1;

  for (int i = 0; i < touchKeyRowCount; i++) {
    int rowCY = touchKeyRowY[i];

    if (ypos >= rowCY - KEY_HEIGHT / 2 && ypos <= rowCY + KEY_HEIGHT / 2) {
      activeRow = i;
      break;
    }
  }

  if (activeRow == -1) {
    return;
  }

  const int keyCount = touchKeyRowContentsCount[activeRow];

  for (int i = 0; i < keyCount; i++) {
    const struct touchKey *key = &touchKeyRowContents[activeRow][i];

    if (xpos < key->cx - key->width / 2 || xpos > key->cx + key->width / 2) {
      continue;
    }

    // activate the key
    activeKey = key;
    break;
  }

  if (activeKey) {
    if (activeKey->code == KEYCODE_SHIFT) {
      shiftIsActive = !shiftIsActive;

      _input_draw_all_keys();
    } else {
      _input_draw_key(activeRow, activeKey);

      // @todo this
      test_buffer_cursor = 0;
      test_buffer[0] = shiftIsActive ? activeKey->shiftCode : activeKey->code;
      test_buffer[1] = 0;
    }
  }
}

void _input_process_release() {
  const int releasedKeyRow = activeRow;
  const struct touchKey *releasedKey = activeKey;

  activeRow = -1;
  activeKey = NULL;

  if (shiftIsActive && releasedKey->code != KEYCODE_SHIFT) {
    shiftIsActive = false;

    _input_draw_all_keys();
  } else {
    _input_draw_key(releasedKeyRow, releasedKey);
  }
}

void input_init() {
  uint16_t bgColor = tft.color565(0x20, 0x20, 0x20);

  tft.fillRect(0, ILI9341_HEIGHT - KEYBOARD_HEIGHT, ILI9341_WIDTH, KEYBOARD_HEIGHT, bgColor);

  tft.setTextSize(1);

  _input_draw_all_keys();
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

      // @todo support autorepeat
      _input_process_touch(xpos, ypos);
    }
  } else if (touchActive) {
    // flip status latch to off once settled back to zero
    if (!isRising && touchRisingCount == 0) {
      touchActive = false;

      _input_process_release();
    }
  }
}
