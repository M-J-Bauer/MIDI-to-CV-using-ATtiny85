/**
 * Project:     MIDI-to-CV converter device using ATtiny85 MCU
 * Originated:  Kevin  [https://emalliab.wordpress.com/2019/03/02/attiny85-midi-to-cv/]
 * Extended:    M.J.Bauer 2025  [https://www.mjbauer.biz/]
 *
 * This version supports two CV outputs...  see configuration options below...
 * PB1 = Note Pitch,  PB4 = "Amplitude" (Velocity, Modulation or Breath Pressure).
 *
 * This version supports 'Multi-Trigger' mode on the GATE output. If your MIDI
 * controller (keyboard, sequencer, etc) is Polyphonic, enable 'Multi-Trigger' mode.
 * If your MIDI controller supports Monophonic operation, compliant with the MIDI
 * specification for Legato Mode, then you should disable 'Multi-Trigger' mode.
 *
 * <!> Set MCU fuse bit to use 8 MHz Internal Clock (required for MIDI baudrate).
 *
 * Update May 2025 (MJB): Added gate 'Multi-Trigger' mode
 * Update Aug 2025 (MJB): Added 2nd CV output (PWM on PB4)
 */
#include <SoftwareSerial.h>

//-------------  C O N F I G U R A T I O N   O P T I O N S  -----------------------------
//
#define MIDI_CHAN     12    // Set this to the MIDI channel to respond to (1..16)
// Specify what MIDI message type controls the AMPLD CV output (PWM on PB4)...
#define AMPLD_CV_MSG  'V'   // 'V' = Velocity; 'M' = Modul'n (CC01); 'B' = Breath (CC02)
#define MULTI_TRIG     0    // Multi-trigger Mode:  0 = disable, 1 = enable
//
//---------------------------------------------------------------------------------------
#define MIDILONOTE    36    // Lowest note in 5-octave range (C2)
#define MIDIHINOTE    96    // Highest note in 5-octave range (C7)
#define TRIGPULSE     1000  // Width of the trigger pulse (us)

#define AMPLD_CV   4     // PB4 (MCU pin3) = Ampld CV PWM DAC (0..Vcc)
#define MIDI_TX    4     // PB3 (MCU pin2) = MIDI OUT (TX) - Not used
#define MIDI_RX    3     // PB3 (MCU pin2) = MIDI IN (RX)
#define GATE       2     // PB2 (MCU pin7) = Gate output (5V)
#define PITCH_CV   1     // PB1 (MCU pin6) = Pitch CV PWM DAC (0..Vcc)
#define TRIGGER    0     // PB0 (MCU pin5) = Trigger output (5V)

SoftwareSerial  midiSerial(MIDI_RX, MIDI_TX);  // Instantiate software UART

uint8_t  notePlaying;
uint8_t  notePending;
uint32_t triggerPulseBegin;
uint32_t gateDelayBegin;  // for 'Multi-Trigger' GATE ON pending
uint8_t  midiStatusByte;
uint8_t  midiDataByte1;   // 1st msg byte following Status
uint8_t  midiDataByte2;   // 2nd msg byte following Status

#define SetDutyPWM1A(d)  (OCR1A = d)  // d = PWM duty (0..239)
#define SetDutyPWM1B(d)  (OCR1B = d)  // d = PWM duty (0..239)
#define gateOn()         digitalWrite(GATE, HIGH)
#define gateOff()        digitalWrite(GATE, LOW)
#define triggerOn()    { digitalWrite(TRIGGER, HIGH);  triggerPulseBegin = micros(); }

/**
 * Timer 1 used for PWM output based on Compare Registers A and B to set PWM duty.
 * Set max compare value to 239 in Compare Register C (PWM period = 240 clocks).
 * PWM period is fixed at 30us (= 240 / F_CPU, where F_CPU = 8 MHz);  F_PWM = 33.3 kHz
 * PWM duty varies from 0 to 239 giving 0V to +5.0V output range.
 * MIDI note range:  Lowest note = 36 (C2), Highest note = 96 (C7).
 * So there are 60 notes that can be received, thus making a semitone
 * interval = 240/60 = 4 units of PWM duty value.
 * Thus, for each note received, PWM Compare value = (note - 36) * 4.
 *
 * Timer 1 Control Register bits:
 * Set bit PWM1A = PWM based on OCR1A;  PWM1B = PWM based on OCR1B
 * Set bit COM1A1 = Clear OC1A output on compare match;  ditto for COM1B1
 * Set bit CS10 = Counter run, no prescaler, i.e. clock at F_CPU (8 MHz)
 */
void  setup()
{
  midiSerial.begin(31250); // MIDI Baud rate
  pinMode(TRIGGER, OUTPUT);
  pinMode(GATE, OUTPUT);
  pinMode(PITCH_CV, OUTPUT);
  pinMode(AMPLD_CV, OUTPUT);
  
  TCCR1 = _BV(PWM1A)|_BV(COM1A1)|_BV(CS10);  // Enable PWM1A
  GTCCR = _BV(PWM1B)|_BV(COM1B1);  // Enable PWM1B
  OCR1C = 239;  // Period = 240
  OCR1A = 96;   // initial duty (for test): Note# 60 (C4) => 2.00V
  OCR1B = 180;  // 75% of full-scale
  gateOff();
  digitalWrite(TRIGGER, LOW);
}


void loop()
{
  if (midiSerial.available())  MidiProcess(midiSerial.read());

  // End trigger pulse after a short delay (typ. 1000us)
  if ((micros() - triggerPulseBegin) >= TRIGPULSE)  digitalWrite(TRIGGER, LOW);

  if (!MULTI_TRIG)  return;  // Multi-trigger mode disabled

  // Initiate note pending (set GATE o/p High) in multi-trigger mode...
  if (notePending && (millis() - gateDelayBegin) >= 5)  // 5ms delay expired
  {
    SetDutyPWM1A((notePending - MIDILONOTE) * 4);  // update pitch CV out
    gateOn();
    triggerOn();
    notePlaying = notePending;
    notePending = 0;  // done
  }    
}


void  MidiProcess(uint8_t midibyte)
{
  if ((midibyte >= 0x80) && (midibyte <= 0xEF))
  {
    // MIDI voice-channel message --  Start handling Running Status
    midiStatusByte = midibyte;
    midiDataByte1 = 0xFF;  // invalid
    midiDataByte2 = 0xFF;  // invalid
  }
  else if ((midibyte >= 0xF0) && (midibyte <= 0xF7))
  {
    // MIDI System Common Category message -- Reset Running Status
    midiStatusByte = 0;
  }
  else if ((midibyte >= 0xF8) && (midibyte <= 0xFF))
  {
    // System real-time message -- ignored -- no effect on Running Status
  } 
  else  // MIDI data byte...
  {
    if (midiStatusByte == 0)
    {
      return;  // No command received yet -- bail
    }
    // Looking for the command on our channel
    if (midiStatusByte == (0x80 | (MIDI_CHAN-1)))  // *** Note OFF ***
    {
      if (midiDataByte1 == 0xFF)  midiDataByte1 = midibyte;  // Rx byte is note#
      else  // Rx byte is velocity
      {
        midiDataByte2 = midibyte;
        midiNoteOff(midiDataByte1, midiDataByte2);
        midiDataByte1 = 0xFF;
        midiDataByte2 = 0xFF;
      }
    } 
    else if (midiStatusByte == (0x90 | (MIDI_CHAN-1)))  // *** Note ON ***
    {
      if (midiDataByte1 == 0xFF)  midiDataByte1 = midibyte;  // Rx byte is note#
      else  // Rx byte is velocity
      {
        midiDataByte2 = midibyte;
        if (midiDataByte2 == 0)  midiNoteOff(midiDataByte1, midiDataByte2);
        else  midiNoteOn(midiDataByte1, midiDataByte2);
        midiDataByte1 = 0xFF;
        midiDataByte2 = 0xFF;
      }
    } 
    else if (midiStatusByte == (0xB0 | (MIDI_CHAN-1)))  // *** Control Change ***
    {
      if (midiDataByte1 == 0xFF)  midiDataByte1 = midibyte;  // Rx byte is CC#
      else  // Rx byte is CC data value (level)
      {
        midiDataByte2 = midibyte;  // CC data value
        if (midiDataByte1 == 1 && AMPLD_CV_MSG == 'M')  // Set Modulation level
        {
          SetDutyPWM1B((240 * midiDataByte2) / 128);  // duty = 0..238
        }
        if (midiDataByte1 == 2 && AMPLD_CV_MSG == 'B')  // Set Breath Pressure level
        {
          SetDutyPWM1B((240 * midiDataByte2) / 128);  // duty = 0..238
        }
        midiDataByte1 = 0xFF;  // Ignore unsupported CC message
        midiDataByte2 = 0xFF;
      }
    } 
    // else -- MIDI command we don't process or not on our channel
  }
}


void  midiNoteOn(uint8_t midi_note, uint8_t velocity)
{
  // check note in the correct range of 36 (C2) to 96 (C7)
  if (midi_note < MIDILONOTE) midi_note = MIDILONOTE;
  if (midi_note > MIDIHINOTE) midi_note = MIDIHINOTE;
  
  if (MULTI_TRIG)
  {
    if (notePlaying)  // GATE is already ON...
    {
      gateOff();  // Terminate current note
      notePlaying = 0;
      notePending = midi_note;  // Defer Note-On event
      gateDelayBegin = millis();
    }
    else  // GATE is OFF... Initiate new note
    {
      SetDutyPWM1A((midi_note - MIDILONOTE) * 4);
      gateOn();
      triggerOn();
      notePlaying = midi_note;
    }
  }
  else  // Multi-trigger mode disabled
  {
    // If GATE is already ON, just do a legato pitch change
    SetDutyPWM1A((midi_note - MIDILONOTE) * 4);
    if (!notePlaying)  // If GATE is OFF, do a re-trigger
    {
      gateOn();
      triggerOn();
    }
    notePlaying = midi_note;
  }

  if (AMPLD_CV_MSG == 'V')  // use velocity data to set AMPLD CV output...
    SetDutyPWM1B((240 * velocity) / 128);  // duty = 0..238
}


void  midiNoteOff (uint8_t midi_note, uint8_t midi_level)
{
  // check note in the correct range of 36 (C2) to 90 (C7)
  if (midi_note < MIDILONOTE) midi_note = MIDILONOTE;
  if (midi_note > MIDIHINOTE) midi_note = MIDIHINOTE;

  // Set GATE Low if note# corresponds to the current note sounding
  if (midi_note == notePlaying)
  {
    notePlaying = 0;
    gateOff();
  }
}
