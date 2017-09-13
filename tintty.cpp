// @todo move out direct TFT references
#include <TFT_ILI9163C.h>
#define TFT_BLACK   0x0000
#define TFT_BLUE    0x001F
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW  0xFFE0
#define TFT_WHITE   0xFFFF

#include "tintty.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

const int16_t max_col = SCREEN_WIDTH / CHAR_WIDTH;
const int16_t max_row = SCREEN_HEIGHT / CHAR_HEIGHT;

struct tintty_state {
    int16_t cursor_col, cursor_row;
    uint16_t bg_tft_color, fg_tft_color;
} state;

void _main(
    char (*read_char)(),
    void (*send_char)(char str),
    TFT_ILI9163C *tft
) {
    // start in default idle state
    char initial_character = read_char();

    if (initial_character >= 0x20 && initial_character <= 0x7e) {
        // output displayable character
        tft->drawChar(
            state.cursor_col * CHAR_WIDTH,
            state.cursor_row * CHAR_HEIGHT,
            initial_character,
            state.fg_tft_color,
            state.bg_tft_color,
            1
        );

        // update caret
        state.cursor_col += 1;

        if (state.cursor_col >= max_col) {
            state.cursor_col = 0;
            state.cursor_row += 1;

            // @todo proper scroll-down
            if (state.cursor_row >= max_row) {
                state.cursor_row = 0;
            }
        }
    }
}

void tintty_run(
    char (*read_char)(),
    void (*send_char)(char str),
    TFT_ILI9163C *tft
) {
    // set up initial state
    state.cursor_col = 0;
    state.cursor_row = 0;
    state.fg_tft_color = TFT_WHITE;
    state.bg_tft_color = TFT_BLACK;

    // main read cycle
    while (1) {
        _main(read_char, send_char, tft);
    }
}
