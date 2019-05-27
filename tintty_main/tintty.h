
#include "Arduino.h"

/**
 * Renderer callbacks.
 */
struct tintty_display {
    int16_t screen_width, screen_height;

    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void (*draw_pixels)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pixels);
    void (*set_vscroll)(int16_t offset); // scroll offset for entire screen
};

/**
 * Main entry point.
 * Peek/read callbacks are expected to block until input is available;
 * while sketch is waiting for input, it should call the tintty_idle() hook
 * to allow animating cursor, etc.
 */
void tintty_run(
    char (*peek_char)(),
    char (*read_char)(),
    void (*send_char)(char str),
    tintty_display *display
);

/**
 * Hook to call while e.g. sketch is waiting for input
 */
void tintty_idle(
    tintty_display *display
);
