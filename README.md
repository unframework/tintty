# TinTTY

## Development

Testing on Windows is easy to do from a Vagrant VM running Linux. Arduino usually exposes the USB connection as serial COM3 port, so the VM is configured to forward the COM3 port to `/dev/ttyS0` inside the Linux environment.

First, ensure the sketch is uploaded (Vagrant will block further COM3 usage when the VM starts).

Start the VM, log in and enter the root shell.

```
vagrant up
vagrant ssh

sudo su -
```

Inside the Linux root shell, the `/dev/ttyS0` serial port will now be accessible to talk to the Arduino board. The default baud rate is 9600, which matches the test sketch serial settings.

Simple echo will not work as expected because the Uno board resets once a new connection starts. Instead, the simplest test is to use GNU Screen.

```
screen /dev/ttyS0
```

Keystrokes will be sent immediately to the sketch. Screen is technically itself a serial terminal, not a host, so certain keys will not behave as expected. But simple letters should produce visual output on the TFT screen. Exit GNU screen by pressing `Ctrl-A` and typing `:quit`.

A proper console test is to start a real shell session using Linux getty. The getty process continuously waits for terminal availability on a given port and spawns a new login shell when the device is ready for display.

Because the Arduino has to spend some time booting and setting up, the sketch emits a CR character once it boots, which lets getty wake up and prompt for login. We use the getty `--wait-cr` option to make it wait for the . The getty process does not interact with the shell that spawns it, so we launch it in the background.

```
/sbin/getty -L --wait-cr ttyS0 &
```

Inside the console, once logged-in, run the sizing command, since the screen is non-standard:

```
stty rows 16 columns 21
```

To terminate getty from the original root shell, just do a normal `kill [pid]` command - it still responds to external SIGTERM.

One fun observation is that resetting the Arduino board will make getty re-prompt for login: it detects lost carrier signal in the serial device, terminates the existing login shell and dutifully waits for terminal availability (and the CR character) again. By the time the sketch boots up after reset, getty is there to listen to it and prompt for login.

## References

Existing AVR-based implementation as initial inspiration:

- https://github.com/mkschreder/avr-vt100 (and updated version https://bitbucket.org/scargill/experimental-terminal/src)

General VT100/ANSI docs:

- http://vt100.net/docs/vt100-ug/chapter3.html
- http://vt100.net/docs/vt220-rm/chapter4.html
- https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html#Control-Sequences
- https://en.wikipedia.org/wiki/ANSI_escape_code#Colors

Other interesting implementations I found:

- http://www.msarnoff.org/terminalscope/ (https://github.com/74hc595/Terminalscope)
- https://madresistor.com/diy-vt100/ (https://gitlab.com/madresistor/diy-vt100-firmware)
- http://geoffg.net/terminal.html
