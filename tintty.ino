/**
 * TinTTY main sketch
 * by Nick Matantsev 2017
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 */

#include <SPI.h>

#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

#include <Usb.h>
#include <hidboot.h>

// extras from https://github.com/arduino/ArduinoCore-samd/commit/0be91985fd090855e3157a8729d88cc608ed90f2
#define UHS_HID_BOOT_KEY_ESCAPE 0x29
#define UHS_HID_BOOT_KEY_DELETE 0x2a
#define UHS_HID_BOOT_KEY_TAB 0x2b

#include "tintty.h"

// this is following a tutorial-recommended pinout, but changing
// the SPI CS and RS pins for the TFT to be 5 and 4 to avoid
// device conflict with USB Host Shield
TFT_ILI9163C tft = TFT_ILI9163C(5, 8, 4);

// USB Host Shield connection
USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> Keyboard(&Usb);

class TinTTYKeyboardParser : public KeyboardReportParser {
protected:
    virtual void OnKeyDown(uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key);
};

void TinTTYKeyboardParser::OnKeyDown(uint8_t mod, uint8_t key) {
    // process control keys because they are not supported by AVR UsbHost?
    if (mod & 0x11) {
        // Ctrl + [A-Z]
        // per https://github.com/arduino/ArduinoCore-samd/commit/0be91985fd090855e3157a8729d88cc608ed90f2
        if (key >= 0x04 && key <= 0x1d)  {
            OnKeyPressed(key - 3);
        }

        return;
    }

    uint8_t asciiChar = OemToAscii(mod, key);

    if (asciiChar) {
        OnKeyPressed(asciiChar);
    } else {
        // extra unsupported characters
        switch (key) {
            case UHS_HID_BOOT_KEY_ESCAPE:
                OnKeyPressed('\e');
                break;

            case UHS_HID_BOOT_KEY_DELETE:
                OnKeyPressed('\b');
                break;

            case UHS_HID_BOOT_KEY_TAB:
                OnKeyPressed('\t');
                break;
        }
    }
}

void TinTTYKeyboardParser::OnKeyPressed(uint8_t asciiChar) {
    // send character as is, correcting 0x13 (DC3) into 0x0D (CR)
    Serial.print((char)(asciiChar == 0x13
        ? 0x0D
        : asciiChar
    ));
};

TinTTYKeyboardParser tinTTYKeyboardParser;

// buffer to test various input sequences
char *test_buffer = "-- \e[1mTinTTY\e[m --\r\n";
uint8_t test_buffer_cursor = 0;

void setup() {
  Serial.begin(9600); // normal baud-rate

  tft.begin();

  Usb.Init();
  delay(200);
  Keyboard.SetReportParser(0, (HIDReportParser*)&tinTTYKeyboardParser); // @todo needs delay?

  tintty_run(
    [=](){
      // first peek from the test buffer
      char test_char = test_buffer[test_buffer_cursor];

      if (test_char) {
        tintty_idle(&tft);
        return test_char;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
      }

      return (char)Serial.peek();
    },
    [=](){
      // process at least one idle loop to allow input to happen
      tintty_idle(&tft);
      Usb.Task(); // @todo move?

      // first read from the test buffer
      char test_char = test_buffer[test_buffer_cursor];

      if (test_char) {
        tintty_idle(&tft);

        Usb.Task(); // @todo move?

        test_buffer_cursor += 1;
        return test_char;
      }

      // fall back to normal blocking serial input
      while (Serial.available() < 1) {
        tintty_idle(&tft);
        Usb.Task(); // @todo move?
      }

      return (char)Serial.read();
    },
    [=](char ch){ Serial.print(ch); },
    &tft
  );
}

void loop() {
}
