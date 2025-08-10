I came across this very simple MIDI-to-CV design using an ATtiny85 MCU on "Kevin's Blog", here:

https://emalliab.wordpress.com/2019/03/02/attiny85-midi-to-cv/

The original design has only one CV output. I extended the Arduino sketch to support 2 CV outputs. 
The 2nd CV can be configured (by #defines) to respond to MIDI velocity, modulation (CC1) or breath pressure (CC2) to suit wind controllers.

There is also an option to enable "Multi-Trigger" mode, so that the device will work properly with a polyphonic controller.
See comments in the source code for details.

![ATtiny85-MIDI-CV-schematic](https://github.com/user-attachments/assets/016d90ca-7801-4b64-b908-47c0f566a005)

The CV outputs require a low-pass filter. For most purposes, a simple 1st-order RC filter will suffice. The corner frequency
should be in the range 300Hz to 1kHz. A 2nd-order active filter would be preferable (to eliminate any residual PWM carrier), 
but this seems like overkill compared with the rest of the circuit.

There's no opto-coupler in the MIDI input circuit. For test purposes and for simple MIDI setups, e.g. keyboard + MIDI-CV + synth, 
you don't need optical isolation. You can easily add an opto if you wish... see Kevin's Blog.

Before trying it, I didn't know how well the ATtiny85 was supported by the Arduino IDE. As it turns out... not very well! 
You need to install the "ATtiny CORE" board package. No problem there, but I could not get any of the programming tools to work with Arduino.
If you use a naked ATtiny85 chip (DIP8), i.e. not a breakout board (DigiSpark), the MCU will have no bootloader installed.
However, it is possible to use any Arduino AVR-8 board (e.g. Uno R3 or Nano) as a programming tool, with appropriate code installed,
from within the Arduino IDE app. (For more info, do a Google search.)

On Kevin's Blog, in a reply to a comment, Kevin noted that the MIDI input (software-driven serial port) may be easily overloaded. 
Some MIDI controllers (keyboards, sequencers, etc) send frequent "real-time" messages which can flood the MIDI receiver function. 
The software UART cannot handle all of the incoming messages, so some messages may be lost or corrupted.

Hence, if possible, turn off "real-time" messages on your MIDI keyboard. 
Also, if possible, turn off Pitch-bend and After-Touch, and set the minimum interval between Control-Change messages to 50 or 100 milliseconds.

The ATtiny85 MCU is not a good choice for "serious" MIDI-CV applications. 
The ATmega32U4, which I chose for my [superior MIDI-to-CV translator design](https://www.mjbauer.biz/DIY-MIDI-to-CV-box.html), 
has a full-featured UART (on-chip peripheral) with RX interrupt capability, so it can handle a much greater volume of received messages.

See my other DIY electronic music projects at [www.mjbauer.biz](https://www.mjbauer.biz/).
