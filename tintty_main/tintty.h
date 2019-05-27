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
    MCUFRIEND_kbv *tft
);

/**
 * Hook to call while e.g. sketch is waiting for input
 */
void tintty_idle(
    MCUFRIEND_kbv *tft
);
