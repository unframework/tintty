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

const int16_t IDLE_CYCLE_MAX = 6000;
const int16_t IDLE_CYCLE_ON = 3000;

const int16_t TAB_SIZE = 4;

struct tintty_state {
    int16_t cursor_col, cursor_row;
    uint16_t bg_tft_color, fg_tft_color;

    // @todo deal with integer overflow
    int16_t top_row; // first displayed row in a logical scrollback buffer

    char out_char;
    int16_t out_char_col, out_char_row;

    int16_t idle_cycle_count; // @todo track during blocking reads mid-command
} state;

struct tintty_rendered {
    int16_t cursor_col, cursor_row;
    int16_t top_row;
} rendered;

void _render(TFT_ILI9163C *tft) {
    // if scrolling, prepare the "recycled" screen area
    if (state.top_row != rendered.top_row) {
        // clear the new piece of screen to be recycled as blank space
        int16_t row_delta = state.top_row - rendered.top_row;

        // @todo handle scroll-up
        if (row_delta > 0) {
            // pre-clear the lines at the bottom
            int16_t clear_height = min(SCREEN_HEIGHT, row_delta * CHAR_HEIGHT);
            int16_t clear_sbuf_bottom = (state.top_row * CHAR_HEIGHT + SCREEN_HEIGHT) % SCREEN_HEIGHT;
            int16_t clear_sbuf_top = clear_sbuf_bottom - clear_height;

            // if rectangle straddles the screen buffer top edge, render that slice at bottom edge
            if (clear_sbuf_top < 0) {
                tft->fillRect(
                    0,
                    clear_sbuf_top + SCREEN_HEIGHT,
                    SCREEN_WIDTH,
                    -clear_sbuf_top,
                    state.bg_tft_color
                );
            }

            // if rectangle is not entirely above top edge, render the normal slice
            if (clear_sbuf_bottom > 0) {
                tft->fillRect(
                    0,
                    max(0, clear_sbuf_top),
                    SCREEN_WIDTH,
                    clear_sbuf_bottom - max(0, clear_sbuf_top),
                    state.bg_tft_color
                );
            }
        }

        // update displayed scroll
        tft->scroll((state.top_row * CHAR_HEIGHT) % SCREEN_HEIGHT);

        // save rendered state
        rendered.top_row = state.top_row;
    }

    // render character if needed
    if (state.out_char != 0) {
        tft->drawChar(
            state.out_char_col * CHAR_WIDTH,
            (state.out_char_row * CHAR_HEIGHT) % SCREEN_HEIGHT,
            state.out_char,
            state.fg_tft_color,
            state.bg_tft_color,
            1
        );

        // clear for next render
        state.out_char = 0;

        // the char draw may overpaint the cursor, in which case
        // mark it for repaint
        if (
            rendered.cursor_col == state.cursor_col &&
            rendered.cursor_row == state.cursor_row
        ) {
            rendered.cursor_col = -1;
        }
    }

    // clear old cursor unless it was not shown
    if (
        rendered.cursor_col >= 0 &&
        (
            rendered.cursor_col != state.cursor_col ||
            rendered.cursor_row != state.cursor_row
        )
    ) {
        tft->fillRect(
            rendered.cursor_col * CHAR_WIDTH,
            (rendered.cursor_row * CHAR_HEIGHT + CHAR_HEIGHT - 1) % SCREEN_HEIGHT,
            CHAR_WIDTH,
            1,
            state.bg_tft_color // @todo save the original background colour or even pixel values
        );
    }

    // reflect new cursor position on screen
    // (always redraw cursor to animate)
    tft->fillRect(
        state.cursor_col * CHAR_WIDTH,
        (state.cursor_row * CHAR_HEIGHT + CHAR_HEIGHT - 1) % SCREEN_HEIGHT,
        CHAR_WIDTH,
        1,
        state.idle_cycle_count < IDLE_CYCLE_ON
            ? state.fg_tft_color
            : state.bg_tft_color // @todo save the original background colour or even pixel values
    );

    // save new rendered state
    rendered.cursor_col = state.cursor_col;
    rendered.cursor_row = state.cursor_row;
}

void _ensure_cursor_vscroll() {
    // move displayed window down to cover cursor
    // @todo support scrolling up as well
    if (state.cursor_row - state.top_row >= max_row) {
        state.top_row = state.cursor_row - max_row + 1;
    }
}

void _send_sequence(
    void (*send_char)(char ch),
    char* str
) {
    // send zero-terminated sequence character by character
    while (*str) {
        send_char(*str);
        str += 1;
    }
}

void _exec_escape_code(
    char (*read_char)(),
    void (*send_char)(char ch),
    TFT_ILI9163C *tft
) {
    // read next character after Escape-code
    // @todo time out?
    char esc_character = read_char();

    // @todo support for (, ), #, c
    switch (esc_character) {
        case '[':
            // @todo
            break;

        case 'D':
            // index (move down and possibly scroll)
            state.cursor_row += 1;
            _ensure_cursor_vscroll();
            break;

        case 'M':
            // reverse index (move up and possibly scroll)
            state.cursor_row -= 1;
            _ensure_cursor_vscroll();
            break;

        case 'E':
            // next line
            state.cursor_row += 1;
            state.cursor_col = 0;
            _ensure_cursor_vscroll();
            break;

        case 'Z':
            // Identify Terminal (DEC Private)
            _send_sequence(send_char, "\e[?1;0c"); // DA response: no options
            break;

        default:
            // unrecognized character
            // @todo signal bell?
            break;
    }
}

void _main(
    char (*read_char)(),
    void (*send_char)(char str),
    TFT_ILI9163C *tft
) {
    // start in default idle state
    char initial_character = read_char();

    if (initial_character >= 0x20 && initial_character <= 0x7e) {
        // output displayable character
        state.out_char = initial_character;
        state.out_char_col = state.cursor_col;
        state.out_char_row = state.cursor_row;

        // update caret
        state.cursor_col += 1;

        if (state.cursor_col >= max_col) {
            state.cursor_col = 0;
            state.cursor_row += 1;
            _ensure_cursor_vscroll();
        }

        // reset idle state
        state.idle_cycle_count = 0;
    } else {
        // @todo bell, answer-back (0x05), delete
        switch (initial_character) {
            case '\n':
                // line-feed
                state.cursor_row += 1;
                _ensure_cursor_vscroll();
                break;

            case '\r':
                // carriage-return
                state.cursor_col = 0;
                break;

            case '\b':
                // backspace
                state.cursor_col = max(0, state.cursor_col - 1);
                break;

            case '\t':
                // tab
                {
                    // @todo blank out the existing characters? not sure if that is expected
                    const int16_t tab_num = state.cursor_col / TAB_SIZE;
                    state.cursor_col = min(max_col - 1, (tab_num + 1) * TAB_SIZE);
                }
                break;

            case '\e':
                // Escape-command
                _exec_escape_code(read_char, send_char, tft);
                break;

            default:
                // nothing, just animate cursor
                state.idle_cycle_count = (state.idle_cycle_count + 1) % IDLE_CYCLE_MAX;
        }
    }

    _render(tft);
}

void tintty_run(
    char (*read_char)(),
    void (*send_char)(char str),
    TFT_ILI9163C *tft
) {
    // set up initial state
    state.cursor_col = 0;
    state.cursor_row = 0;
    state.top_row = 0;
    state.fg_tft_color = TFT_WHITE;
    state.bg_tft_color = TFT_BLACK;

    state.out_char = 0;

    rendered.cursor_col = -1;
    rendered.cursor_row = -1;

    // set up fullscreen TFT scroll
    // magic value for second parameter (bottom-fixed-area)
    // compensate for extra length of graphical RAM (GRAM)
    tft->defineScrollArea(0, 32);

    // initial render
    _render(tft);

    // main read cycle
    while (1) {
        _main(read_char, send_char, tft);
    }
}
