#include "Arduino.h"
#include <TouchScreen.h>
#include <Adafruit_GFX.h>

#include "tintty.h"
#include "input.h"

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

#define KEYBOARD_GUTTER 4

#define KEY_WIDTH 16
#define KEY_HEIGHT 16
#define KEY_GUTTER 1

#define KEY_ROW_A_Y (ILI9341_HEIGHT - KEYBOARD_HEIGHT + KEYBOARD_GUTTER)

#define KEY_ROW_A_X(index) (0 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_B_X(index) (20 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_C_X(index) (24 + (KEY_WIDTH + KEY_GUTTER) * index)
#define KEY_ROW_D_X(index) (32 + (KEY_WIDTH + KEY_GUTTER) * index)
#define ARROW_KEY_X(index) (ILI9341_WIDTH - (KEY_WIDTH + KEY_GUTTER) * (4 - index))

#define KEYCODE_SHIFT -20
#define KEYCODE_CAPS -21
#define KEYCODE_CONTROL -22
#define KEYCODE_ARROW_START -30 // from -30 to -27

const int touchKeyRowCount = 5;

struct touchKey {
    int16_t x, width;
    char code, shiftCode, label;
};

struct touchKeyRow {
    int16_t y;

    int keyCount;
    struct touchKey keys[14];
} touchKeyRows[5] = {
    {
        KEY_ROW_A_Y,
        14,
        {
            { 1, KEY_ROW_A_X(1) - 1 - KEY_GUTTER, '`', '~', 0 }, // shrunk width
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
            { KEY_ROW_A_X(12), KEY_WIDTH, '=', '+', 0 },
            { KEY_ROW_A_X(13), ILI9341_WIDTH - 1 - KEY_ROW_A_X(13), 8, 8, 27 }
        }
    },
    {
        KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 1,
        14,
        {
            { 1, (KEY_ROW_B_X(0) - KEY_GUTTER - 1), 9, 9, 26 },

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
            { KEY_ROW_B_X(11), KEY_WIDTH, ']', '}', 0 },
            { KEY_ROW_B_X(12), ILI9341_WIDTH - 1 - KEY_ROW_B_X(12), '\\', '|', 0 }
        }
    },
    {
        KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 2,
        13,
        {
            {
                1,
                (KEY_ROW_C_X(0) - KEY_GUTTER - 1),
                KEYCODE_CAPS,
                KEYCODE_CAPS,
                18
            },

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

            { KEY_ROW_C_X(11), ILI9341_WIDTH - 1 - KEY_ROW_C_X(11), 10, 10, 16 }
        }
    },
    {
        KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 3,
        12,
        {
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
                1,
                (KEY_ROW_D_X(0) - KEY_GUTTER - 1),
                KEYCODE_SHIFT,
                KEYCODE_SHIFT,
                24
            },

            {
                KEY_ROW_D_X(10),
                ILI9341_WIDTH - 1 - KEY_ROW_D_X(10),
                KEYCODE_SHIFT,
                KEYCODE_SHIFT,
                24
            }
        }
    },
    {
        KEY_ROW_A_Y + (KEY_GUTTER + KEY_HEIGHT) * 4,
        6,
        {
            {
                1,
                22,
                KEYCODE_CONTROL,
                KEYCODE_CONTROL,
                'C'
            },

            { (ILI9341_WIDTH - 100) / 2, 100, ' ', ' ', ' ' },

            { ARROW_KEY_X(0), KEY_WIDTH, KEYCODE_ARROW_START + 3, KEYCODE_ARROW_START + 3, 17 },
            { ARROW_KEY_X(1), KEY_WIDTH, KEYCODE_ARROW_START, KEYCODE_ARROW_START, 30 },
            { ARROW_KEY_X(2), KEY_WIDTH, KEYCODE_ARROW_START + 1, KEYCODE_ARROW_START + 1, 31 },
            { ARROW_KEY_X(3), KEY_WIDTH, KEYCODE_ARROW_START + 2, KEYCODE_ARROW_START + 2, 16 }
        }
    }
};

struct touchKeyRow *activeRow = NULL;
struct touchKey *activeKey = NULL;
bool shiftIsActive = false;
bool shiftIsSticky = false;
bool controlIsActive = false;

void _input_set_mode(bool shift, bool shiftSticky, bool control) {
    // reset if current mode already matches
    if (
        shiftIsActive == shift &&
        shiftIsSticky == shiftSticky &&
        controlIsActive == control
    ) {
        shiftIsActive = false;
        shiftIsSticky = false;
        controlIsActive = false;
    } else {
        shiftIsActive = shift;
        shiftIsSticky = shiftSticky;
        controlIsActive = control;
    }
}

void _input_draw_key(const struct touchKeyRow *keyRow, const struct touchKey *key) {
    const int16_t rowCY = keyRow->y;
    const bool isActive = (
        key == activeKey ||
        (shiftIsActive && (
            (key->code == KEYCODE_SHIFT && !shiftIsSticky) ||
            (key->code == KEYCODE_CAPS && shiftIsSticky)
        )) ||
        (controlIsActive && key->code == KEYCODE_CONTROL)
    );

    uint16_t keyColor = isActive ? 0xFFFF : 0;
    uint16_t borderColor = isActive ? 0xFFFF : tft.color565(0x80, 0x80, 0x80);
    uint16_t textColor = isActive ? 0 : 0xFFFF;

    tft.setTextColor(textColor);

    const int16_t ox = key->x;
    const int16_t oy = rowCY;

    tft.drawFastHLine(ox, oy, key->width, borderColor);
    tft.drawFastHLine(ox, oy + KEY_HEIGHT - 1, key->width, borderColor);
    tft.drawFastVLine(ox, oy, KEY_HEIGHT, borderColor);
    tft.drawFastVLine(ox + key->width - 1, oy, KEY_HEIGHT, borderColor);
    tft.fillRect(ox + 1, oy + 1, key->width - 2, KEY_HEIGHT - 2, keyColor);

    tft.setCursor(key->x + (key->width / 2) - 3, rowCY + (KEY_HEIGHT / 2) - 4);
    tft.print(
        key->label == 0
            ? (shiftIsActive ? key->shiftCode : key->code)
            : key->label
    );
}

void _input_draw_all_keys() {
    for (int i = 0; i < touchKeyRowCount; i++) {
        const struct touchKeyRow *keyRow = &touchKeyRows[i];
        const int keyCount = keyRow->keyCount;

        for (int j = 0; j < keyCount; j++) {
            _input_draw_key(keyRow, &keyRow->keys[j]);
        }
    }
}

void _input_process_touch(int16_t xpos, int16_t ypos) {
    activeRow = NULL;

    for (int i = 0; i < touchKeyRowCount; i++) {
        int rowCY = touchKeyRows[i].y;

        if (ypos >= rowCY && ypos <= rowCY + KEY_HEIGHT) {
            activeRow = &touchKeyRows[i];
            break;
        }
    }

    if (activeRow == NULL) {
        return;
    }

    const int keyCount = activeRow->keyCount;

    for (int i = 0; i < keyCount; i++) {
        const struct touchKey *key = &activeRow->keys[i];

        if (xpos < key->x || xpos > key->x + key->width) {
            continue;
        }

        // activate the key
        activeKey = key;
        break;
    }

    if (activeKey) {
        if (activeKey->code == KEYCODE_SHIFT) {
            _input_set_mode(true, false, false);
            _input_draw_all_keys();
        } else if (activeKey->code == KEYCODE_CAPS) {
            _input_set_mode(true, true, false);
            _input_draw_all_keys();
        } else if (activeKey->code == KEYCODE_CONTROL) {
            _input_set_mode(false, false, true);
            _input_draw_all_keys(); // redraw all, to clear previous mode
        } else {
            _input_draw_key(activeRow, activeKey);

            if (shiftIsActive) {
                Serial.print(activeKey->shiftCode);

                // clear back to lowercase unless caps-lock
                if (!shiftIsSticky) {
                    _input_set_mode(false, false, false);
                    _input_draw_all_keys();
                }
            } else if (controlIsActive) {
                if (activeKey->code >= 97 && activeKey->code <= 122) {
                    // alpha control keys
                    Serial.print((char)(activeKey->code - 96));
                } else if (activeKey->code >= 91 && activeKey->code <= 93) {
                    // [, / and ] control keys
                    // @todo other stragglers
                    Serial.print((char)(activeKey->code - 91 + 27));
                }

                // always clear back to normal
                _input_set_mode(false, false, false);
                _input_draw_all_keys();
            } else if (activeKey->code >= KEYCODE_ARROW_START && activeKey->code < KEYCODE_ARROW_START + 4) {
                Serial.print((char)27); // Esc
                Serial.print(tintty_cursor_key_mode_application ? 'O' : '['); // different code depending on terminal state
                Serial.print((char)(activeKey->code - KEYCODE_ARROW_START + 'A'));
            } else {
                Serial.print(activeKey->code);
            }
        }
    }
}

void _input_process_release() {
    const struct touchKeyRow *releasedKeyRow = activeRow;
    const struct touchKey *releasedKey = activeKey;

    activeRow = NULL;
    activeKey = NULL;

    _input_draw_key(releasedKeyRow, releasedKey);
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
