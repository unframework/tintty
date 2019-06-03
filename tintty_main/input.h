#include "Arduino.h"

#include <MCUFRIEND_kbv.h>

extern MCUFRIEND_kbv tft;
extern void test_send_char(char charCode);

// @todo move?
#define ILI9341_WIDTH 240
#define ILI9341_HEIGHT 320

#define KEYBOARD_HEIGHT 92

void input_init();
void input_idle();
