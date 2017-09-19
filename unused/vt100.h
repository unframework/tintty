/**
    This file is part of FORTMAX kernel.

    FORTMAX kernel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FORTMAX kernel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FORTMAX kernel.  If not, see <http://www.gnu.org/licenses/>.

    Copyright: Martin K. SchrÃ¶der (info@fortmax.se) 2014
*/

extern class TFT_ILI9163C tft;

#define VT100_SCREEN_WIDTH 128
#define VT100_SCREEN_HEIGHT 128
#define VT100_CHAR_WIDTH 6
#define VT100_CHAR_HEIGHT 8
#define VT100_HEIGHT (VT100_SCREEN_HEIGHT / VT100_CHAR_HEIGHT)
#define VT100_WIDTH (VT100_SCREEN_WIDTH / VT100_CHAR_WIDTH)

#define BAUD_STORE 4

void vt100_init(void (*send_response)(char *str));
void vt100_putc(uint8_t ch);
void vt100_puts(const char *str);
