#include <Usb.h>
#include <hidboot.h>

#include "tintty_usbkeyboard.h"

// extras from https://github.com/arduino/ArduinoCore-samd/commit/0be91985fd090855e3157a8729d88cc608ed90f2
#define UHS_HID_BOOT_KEY_ESCAPE 0x29
#define UHS_HID_BOOT_KEY_DELETE 0x2a
#define UHS_HID_BOOT_KEY_TAB 0x2b

// USB Host Shield connection
USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> Keyboard(&Usb);

// key code events processing
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

// init hook
void tintty_usbkeyboard_init() {
    Usb.Init();
    delay(200); // delay to let things warm up (per examples, does not work otherwise)
    Keyboard.SetReportParser(0, (HIDReportParser*)&tinTTYKeyboardParser);
}

// loop hook
void tintty_usbkeyboard_update() {
    Usb.Task();
}
