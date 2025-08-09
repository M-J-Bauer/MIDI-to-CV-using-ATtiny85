I came across this very simple MIDI-to-CV design using an ATtiny85 MCU on a blog written by "Kevin", here:

https://emalliab.wordpress.com/2019/03/02/attiny85-midi-to-cv/

The original design has only one CV output. I extended the Arduino sketch to support 2 CV outputs. 
The 2nd CV can be configured (by #defines) to respond to MIDI velocity, modulation (CC1) or breath pressure (CC2) to suit wind controllers.

There is also an option to enable "Multi-Trigger" mode, so that the device will work properly with a polyphonic controller.

![ATtiny85-MIDI-CV-schematic](https://github.com/user-attachments/assets/016d90ca-7801-4b64-b908-47c0f566a005)

The CV outputs require a low-pass filter. For most purposes, a simple 1st-order RC filter will suffice. The corner frequency
should be in the range 300Hz to 1kHz. For example, use R = 2k2, C = 100n, or R = 1k0, C = 220n. The resistor value should be low,
or use an op-amp unity-gain buffer (voltage follower) to obtain a low output impedance. A 2nd-order active filter would be
preferable (to eliminate any residual PWM carrier), but this seems like overkill compared with the rest of the circuit.

See comments in the source code for details.

See my other DIY electronic music projects at [mjbauer.biz](https://www.mjbauer.biz/).
