/*
 * RGB (and optionally W) LED strip sequencer/controller.
 * @author Patrick H. Rigney
 * @version 1.0 2017-12-17
 * 
 * This program controls an RGB LED strip, allowing control of the brightness of
 * each channel, and the playback of eight different sequences of up to 16 steps
 * each. It uses an Adafruit Trellis with LEDs mounted as the user interface. The
 * LED strips are driven by MOSFETs controlled through the use of PWM on digital
 * pins.
 * 
 * I wanted to use LEDs to light my kids' desks. I was going to install plain
 * white strips with a standard dimmer, but they wanted color. But all of the
 * RGB controllers had canned patterns and I knew they would get tired of them
 * (we use such a beast for lighting on our golf cart and few of the patterns
 * are very interesting). It then occurred to me that I could one using a 'duino,
 * and in the process, given them an application where they could eventually take
 * the code and modify it to do more interesting things.
 * 
 * Target: Arduino Uno (Genuino)
 * 
 * The circuit:
 * Pin configurations are generally settable by modifying the #defines below, but
 * the system was designed as follows:
 * 
 * A4 and A5 - Connect to Trellis SDA and SCL respectively. 5V and GND
 * are connected as usual. This is the "reference" (example) connection for the
 * Trellis. The INT of the Trellis is not used.
 * 
 * Digital 9, 10, and 11 connect to the MOSFET driver gates.
 * The LED strip leads for each color (cathodes) are connected to the MOSFET drains.
 * The MOSFET sources are connected to ground (N-channel enhancement). The driver
 * MUST be connected in common with the Arduino/5V system ground (same reference).
 * 
 * In my design, I color-coded the Trelis LEDs (loosely) to correspond to button
 * functions. Thus the top two rows of the Trellis (buttons 0-7) are red, green,
 * blue, and white (that sequence on each of the first two rows), as these buttons 
 * are used to raise and lower the LED strip brightness of the color channel cor-
 * responding to the LED color (that is, the red Trellis buttons raise and lower the
 * red LED strip channel brightness). In the current design, my strip does not use
 * white, so I have repurposed these buttons and associated variables to adjust the
 * timing of the LED sequences (faster and slower). Trellis buttons 8, 9, 12, and 13
 * (the lower left quadrant) were given white LEDs, and 10, 11, 14, and 15 (the lower
 * right quadrant) were given yellow LEDs. That was pretty much an arbitrary choice.
 * Their color doesn't really matter, but that choice looks tidy in practice. All one
 * color would also work out well (white recommended).
 * 
 * Use:
 * When the 'duino powers up, it initializes the Trellis library and various globals for
 * its button and sequence processing, and waits in the "OFF" state (no LEDs on). The 
 * Trellis is unlit at this point. Press and hold the lower-left Trellis button to turn
 * the unit ON. The system will power up the Trellis display, lighting the 8 color con-
 * trol buttons (0-7) and the lower-left (12) button (as a power indicator, redundant).
 * The system also checks the EEPROM by computing a checksum for its data. If the
 * checksum and data don't agree, the default pattern array is written to EEPROM.
 * 
 * The system powers to IDLE state. In this state, the color output of the LED drivers
 * can be modified using buttons 0-7 (for example, buttons 0 and 4 raise and lower,
 * respectively, the brightness of the red LED strip channel if everything is connected
 * correctly). To run a pattern, briefly press one of the pattern buttons 8-15 (the
 * lower two rows of buttons). There are eight-preprogrammed sequences or patterns
 * defined. Stop a running pattern by either pressing the same pattern button again
 * (the button of the running pattern is the blinking one, so press the blinking one).
 * Pressing the white up/down buttons during run speed up or slow down the pattern.
 * 
 * Patterns can be edited, and the edited pattern is stored in EEPROM. To edit a pattern,
 * "long-press" (more than one second) any of the pattern buttons except 13 (more below).
 * The pattern button will light solid, and the brightness buttons will blink. Adjust the
 * brightness of each channel to achieve the desired color and brightness for the first
 * step of your pattern, then briefly press the (solid) pattern button again. Each press
 * of the pattern button saves the current brightness as a step and moves to the next.
 * A long press of the pattern button ends editing. Editing also ends when the last 
 * possible step (16 currently) has been saved. The pattern then runs. Ending editing
 * with a long press sets the pattern length to the current step, so be careful when
 * editing an existing pattern--if you make a change to step 3 of an 8-step pattern
 * and then bail out with a long press, the pattern is shortened to 3 steps.
 * 
 * To turn the system off, press and hold button 12 (lower left). Because of this, pre-
 * defined pattern 5 cannot be edited, as the long-press is used for power off in pref-
 * erence to entering edit mode. Maybe a two-button sequence could be implemented instead,
 * but for now, this is good enough.
 * 
 * Long-pressing buttons 13 and 16 together while the system is in the OFF state will
 * cause it to "factory reset" by rewriting the origin pattern set back to EEPROM. We do
 * this by simply mangling the EEPROM checksum, and allowing the next reset to see that
 * it's wrong.
 * 
 * Finally, if a button is not pressed for DIMDELAY ms, the system dims the Trellis to
 * its lowest brightness. A button press restores full brightness.
 * 
 */
#include <Adafruit_Trellis.h>
#include <EEPROM.h>

const char PROGMEM copy[] = "(c) 2017 Patrick H. Rigney, All Rights Reserved.";

/* -------- T R E L L I S   C O N F I G U R A T I O N -------- */

Adafruit_Trellis matrix = Adafruit_Trellis();
Adafruit_TrellisSet trellis = Adafruit_TrellisSet(&matrix);
#define numKeys 16

/* -------- L E D   S T R I P   D R I V E R   C O N F I G U R A T I O N -------- */

/* Uno PWM pins are 3, 5, 6, 9, 10, 11 */
#define RGBPIN_RED 10
#define RGBPIN_GREEN 9
#define RGBPIN_BLUE 11

/* 
 * Pre-defined (default) patterns. These are loaded into EEPROM during setup
 * if they have not been previously initialized there. Take care that the total
 * size of the array does not exceed the EEPROM size of the Arduino.
 */
void colorWheel(void);
#define MAXSTEP 16  /* Max steps allowed in pattern. */
typedef struct {
  short len;
  unsigned short dly;
  unsigned long steps[MAXSTEP];
} Pattern;
const Pattern PROGMEM Seqs[] = {
  /* 0 */  { 12, 250, 0x00ff0000, 0x00ff8000, 0x00ffff00, 0x0080ff00, 0x0000ff00, 0x0000ff80, 0x0000ffff, 0x000080ff, 0x000000ff, 0x008000ff, 0x00ff00ff, 0x00ff0080 },
  /* 1 */  { 12, 200, 0x00200000, 0x00400000, 0x00800000, 0x00c00000, 0x00e00000, 0x00ff0000, 0x00e00000, 0x00c00000, 0x00800000, 0x00400000, 0x00200000, 0x00100000 },
  /* 2 */  { 12, 200, 0x00002000, 0x00004000, 0x00008000, 0x0000c000, 0x0000e000, 0x0000ff00, 0x0000e000, 0x0000c000, 0x00008000, 0x00004000, 0x00002000, 0x00001000 },
  /* 3 */  { 12, 200, 0x00000020, 0x00000040, 0x00000080, 0x000000c0, 0x000000e0, 0x000000ff, 0x000000e0, 0x000000c0, 0x00000080, 0x00000040, 0x00000020, 0x00000010 },
  /* 4 */  { 1, 0xffff, 0xFFFFFFFF },
  /* 5 */  { 1, 0xffff, 0x00FF0000 },
  /* 6 */  { 1, 0xffff, 0x0000FF00 },
  /* 7 */  { 1, 0xffff, 0x000000FF },
};
#define NUMSEQ (sizeof Seqs / sizeof(Pattern))

/* Working variables and constants for pattern run */
#define PATTERN_START 0
#define PATTERN_NEXT 1
#define PATTERN_STOP 2
unsigned long nextStepTime;         /* The time (ms) for the next step to run */
short nextStepDelay;                /* Delay (ms) between steps */
const short DelayIncrement = 10;    /* Increment/decrement of delay (white buttons during run) */
const short MaxDelay = 5000;        /* Maximum delay (ms); min is always 0 */
short currentSequence;              /* Current pattern sequence in RAM */

/* Working variables for EEPROM pattern run and edit */
typedef struct {
  Pattern pattern;  /* RAM copy of pattern */
  short nextStep;   /* Next step to run */
} SeqData;
SeqData seqData;
uint8_t editButton;   /* In edit mode, the button that is being edited */

/*  The PatternRunner class defines an abstract for  objects that run each
 *  of the available patterns. By default, seven of the patterns are EEPROM
 *  canned patterns that are run by the EEPROMRunner subclass (defined below).
 *  One pattern is run on the ColorWheelRunner subclass, which is a demo
 *  class to show how to write and connect a pattern runner.
 */
class PatternRunner {
  public:
    virtual void start( int num ){};
    virtual void next( ){};
    virtual void stop( ){};
};
PatternRunner *patternRunners[8];

/* Working variables for RGB drivers */
short red, green, blue, white;        /* Current/new RGB and W values; yes, signed */
const short ColorIncrement = 8;       /* Inc/dec amount for intensity buttons */

/* Some helpful macros for dealing with color */
#define LS(b,n) (((unsigned long)(b & 0xff))<<n)
#define LONGRGB( w, r, g, b ) (LS(w,24)|LS(r,16)|LS(g,8)|(b&0xff))

/* Working variables for system state */
#define STATE_OFF 0
#define STATE_IDLE 1
#define STATE_GETEDITBTN 2
#define STATE_EDITSEQ 3
#define STATE_RUN 4
uint8_t state = STATE_OFF;          /* Current system state */

/* Working variables for button handling */
typedef unsigned short button_t;
#define DIMDELAY 30000
#define NOBUTTON 0x00ff
#define BUTTONMASK 0x00ff
#define LONGPRESS 0x8000
#define SHORTPRESS 0x4000
#define KEYREPEAT 0x2000
#define BUTTON_RED_UP 0
#define BUTTON_GREEN_UP 1
#define BUTTON_BLUE_UP 2
#define BUTTON_WHITE_UP 3
#define BUTTON_RED_DN  4
#define BUTTON_GREEN_DN 5
#define BUTTON_BLUE_DN 6
#define BUTTON_WHITE_DN 7
#define BUTTON_POWER 12
#define BUTTON_RESET 15
unsigned long keyDownTime[numKeys];
unsigned long lastKeyTime;
unsigned long lastButtonCheck;

/* Working variables for LED state tracking. I track them separately from
 * the Trellis' own tracking so I can blink them and restore them individually.
 */
unsigned long ledState = 0;
unsigned long blinkSet = 0;
unsigned long lastBlink = 0;
#define BLINKRATE 250
#define BMASK( button ) (((unsigned short) 1) << (button & BUTTONMASK))

/* Arduino's software reset -- jump to 0 */
void (*resetFunc)(void) = 0;

/*
 * Prototypes to set/enforce argument default.
 */
void mySetLED( uint8_t n, bool state = true, bool update = false );
void myClrLED( uint8_t n, bool update = false );

/*
 * Compute the checksum of the EEPROM storage area as it currently stands.
 * We use NUMSEQ * sizeof(Pattern) + 1 bytes, with the first byte being
 * the stored, previously-computed checksum for comparison.
 * 
 * @returns Checksum for the EEPROM storage area.
 */
uint8_t getEECheck() {
  uint8_t ck = 0;
  short ll = sizeof(Pattern) * NUMSEQ;
  for (uint16_t i=1; i<=ll; ++i) {  
    ck += EEPROM[i] * (i % 16);
  }
  return ck;
}

/* 
 * Put a step of the current (RAM-copy) pattern on the drivers.
 * 
 * @param n The step of the current pattern to be shown.
 */
void showStep() {
  unsigned long rgb = seqData.pattern.steps[ seqData.nextStep ];
  white = (rgb >> 24) & 0xff;
  red = (rgb >> 16) & 0xff;
  green = (rgb >> 8) & 0xff;
  blue = rgb & 0xff;
  // Serial.print("showStep seq "); Serial.print(currentSequence); Serial.print(" step "); Serial.print( p->nextStep ); Serial.print(": ");
  setRGBW();
}

/*
 * Check to see if the next scheduled pattern step is eligible
 * (timing), and display it if so.
 */
void tickSequence( ) {
  if ( currentSequence < 0 ) return; /* assertion */
  /* Is it time for the next step? */
  if ( millis() < nextStepTime )
    return;
  /* Yes, yes it is. */
  patternRunners[ currentSequence ]->next();
  /* Set timing for next */
  nextStepTime = millis() + nextStepDelay;
}

/*
 * Load the specified sequence from EEPROM into RAM.
 * 
 * @param n The number of the sequence to load from EEPROM.
 */
void loadSequence( short n ) {
  uint16_t addr = 1 + n * sizeof(Pattern);
  EEPROM.get( addr, seqData.pattern );
}

class EEPROMRunner : public PatternRunner {
  private:
    int nPattern;
    
  public: 
    void start( int num ) {
      // Serial.print("EEPROMRunner::start"); Serial.println( num, DEC );
      /* Copy pattern from EEPROM to RAM */
      nPattern = num;
      loadSequence( num );
      /* Run the first step */
      seqData.nextStep = 0;
      showStep();
      /* Set timing for next step -- use pattern's setting to start */
      nextStepDelay = seqData.pattern.dly;
      nextStepTime = millis() + nextStepDelay;
    };

    void next() {
      // Serial.print("EEPROMRunner::next "); Serial.println( nPattern, DEC );
      seqData.nextStep += 1;
      if ( seqData.nextStep >= seqData.pattern.len ) {
        seqData.nextStep = 0;
      }
      showStep();
    };
  
    void stop() {
      /* Nothing to do here */
      // Serial.print("EEPROMRunner::stop "); Serial.println( nPattern, DEC );
    };
};

class ColorWheelRunner : public PatternRunner {
  private:
    int state;
    const int inc = 8;

  public:
    void start( int num ) {
      state = 0;
      red = green = blue = 0;
      nextStepDelay = 50;
      setRGBW();
      return NULL;
    }
    void next() {
      switch ( state ) {
        case 0: /* red up */
          red = red + inc;
          if ( red >= 255 ) {
            red = 255;
            ++state;
          }
          break;
        case 1: /* blue down */
          blue = blue - inc;
          if ( blue <= 0 ) {
            blue = 0;
            ++state;
          }
          break;
        case 2: /* green up */
          green = green + inc;
          if ( green >= 255 ) {
            green = 255;
            ++state;
          }
          break;
        case 3: /* red down */
          red = red - inc;
          if ( red <= 0 ) {
            red = 0;
            ++state;
          }
          break;
        case 4: /* blue up */
          blue = blue + inc;
          if ( blue >= 255 ) {
            blue = 255;
            ++state;
          }
          break;
        case 5: /* green down */
          green = green - inc;
          if ( green <= 0 ) {
            green = 0;
            state = 0; /* Start over */
          }
          break;
      }
      setRGBW();
    }
    void stop() {};
};

/*
 * Clear all Trellis LEDs and update display.
 */
void clearLEDs() {
  for (uint8_t i=0; i<numKeys; ++i) {
    trellis.clrLED(i);
    delay(30);
  }
  trellis.writeDisplay();
  ledState = 0;
  delay(30);
}

/*
 * Our own version of (Trellis) setLED, tracks LED state locally.
 * 
 * @param n The number of the LED (button) to set on or off.
 * @param state (optional) state of LED to set (default true = on)
 */
void mySetLED( uint8_t n, bool state, bool update ) {
  if (state) {
    trellis.setLED( n );
    ledState |= (1<<n);
  } else {
    trellis.clrLED( n );
    ledState &= ~(1<<n);
  }
  if ( update ) {
    trellis.writeDisplay();
    delay(30);
  }
}

/*
 * Our own clear tracks LED state locally.
 * 
 * @param n The number of the LED (button) to turn off.
 */
void myClrLED( uint8_t n, bool update ) {
  mySetLED( n, false, update );
}

/*
 * Use our local state to determine LED status. 
 * 
 * @param n The number of the LED for which to return status.
 * @returns The status, true=on, false=off.
 */
bool myIsLED( uint8_t n ) {
  return (ledState & (1<<n)) != 0;
}

/*
 * Add button to set of buttons that are blinked.
 * 
 * @param btn The number of the button to blink.
 */
void addBlinker( uint8_t btn ) {
  blinkSet = blinkSet | ( 1<<(btn & BUTTONMASK) );
}

/*
 * Set blinkers using a bit mask (bit set = blink).
 * 
 * @param mask Bitmask where 1 sets a button to blinking state, 0 means no change.
 */
void setBlinkers( unsigned short mask ) {
  blinkSet |= mask;
}

/*
 * Return blink condition of a button.
 * 
 * @param btn Button number
 * @returns Blink status of the button
 */
bool isBlinker( uint8_t btn ) {
  return ( blinkSet & (1<<btn) ) != 0;
}

/* 
 *  Clear blinkers using a bit mask (1 = stop blinking).
 *  
 *  @param mask Bitmask where 1 clears the blink state of the button, 0 means no change.
 */
void clearBlinkers( unsigned short mask ) {
  uint8_t k = 0;
  for (k=0; k<numKeys; ++k) {
    if ( mask & (1<<k) ) {
      blinkSet = blinkSet & ~(1<<k);
      mySetLED(k, (ledState & (1<<k)) != 0); /* Return LED to current on/off state */
    }
  }
  trellis.writeDisplay();
  delay(30);
}

/*
 * Stop blinking specified button.
 * 
 * Uses the tracked on/off state of the button to restore it
 * to its "natural" state (on or off).
 * 
 * @param btn Button number for which blinking is turned off.
 */
void clearBlinker( uint8_t btn ) {
  blinkSet = blinkSet & ~(1<<btn);
  mySetLED(btn, (ledState & (1<<btn)) != 0, true);
}

/* 
 * Make buttons blink (periodic task).
 */
void updateBlinkers() {
  /* Nothing yet */
  if ( millis() < lastBlink+BLINKRATE ) {
    return;
  }
  for (uint8_t i=0; i<numKeys; ++i) {
    if ( blinkSet & (1<<i) ) {
      if ( trellis.isLED(i) ) {
        trellis.clrLED(i);
      } else {
        trellis.setLED(i);
      }
    }
  }
  lastBlink = millis();
  trellis.writeDisplay();
  delay(30);
}

/*
 * Update LED drivers to current RGBW values.
 */
void setRGBW() {
  // Serial.print("setRGBW R="); Serial.print(red); Serial.print(", G="); Serial.print(green); Serial.print(", B="); Serial.println(blue);
  analogWrite(RGBPIN_RED, red);
  analogWrite(RGBPIN_GREEN, green);
  analogWrite(RGBPIN_BLUE, blue);  
}

/*
 * Increment RGBW parameter, with rounding to step size.
 * 
 * Contrains the result to byte-size values.
 * @param currVal The current value of the parameter.
 * @param incr Increment to apply (positive or negative).
 */
short incrementBrightness( short currVal, short incr ) {
  short aincr = incr < 0 ? -incr : incr;
  currVal = ((currVal + incr) / aincr) * aincr;
  if ( currVal < 0 ) {
    currVal = 0;
  } else if ( currVal > 255 ) {
    currVal = 255;
  }
  return currVal;
}

/*
 * Handle button-based color adjustment.
 * 
 * @param btn Button that's been pressed.
 */
void adjustColor( button_t btn ) {
  switch (btn) {
    case BUTTON_RED_UP:
      red = incrementBrightness( red, ColorIncrement );
      white = 0;
      break;
    case BUTTON_RED_DN:
      red = incrementBrightness( red, -ColorIncrement );
      white = 0;
      break;
    case BUTTON_GREEN_UP:
      green = incrementBrightness( green, ColorIncrement );
      white = 0;
      break;
    case BUTTON_GREEN_DN:
      green = incrementBrightness( green, -ColorIncrement );
      white = 0;
      break;
    case BUTTON_BLUE_UP:
      blue = incrementBrightness( blue, ColorIncrement );
      white = 0;
      break;
    case BUTTON_BLUE_DN:
      blue = incrementBrightness( blue, -ColorIncrement );
      white = 0;
      break;
    case BUTTON_WHITE_UP:
      white = incrementBrightness( white, ColorIncrement );
      red = blue = green = white;
      break;
    case BUTTON_WHITE_DN:
      white = incrementBrightness( white, -ColorIncrement );
      red = blue = green = white;
      break;
  }
  setRGBW();
}

/* 
 * Transition to IDLE state.
 * 
 * @param button The button that was pressed (not used here).
 */
void doIdle( button_t button ) {
  if ( currentSequence >= 0 || state == STATE_RUN ) {
    patternRunners[ currentSequence ]->stop();
  }
  currentSequence = -1;
  state = STATE_IDLE;
  clearBlinkers(0xffff);
}

/*
 * Change to power-on state.
 * 
 * @param button The button that was pressed (not used here).
 */
void doPowerOn( button_t button ) {
  blinkSet = 0;
  for (uint8_t k=0; k<numKeys; ++k) {
    keyDownTime[k] = 0;
  }
  lastKeyTime = millis();
  
  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    mySetLED(i, true, true);
  }
  // then turn them off (except 0)
  for (uint8_t i=8; i<numKeys; i++) {
    if ( i != BUTTON_POWER ) {
      mySetLED(i, false, true);
    }
  }

  /* Restore last RGB to LEDs */
  setRGBW();

  /* Transition to idle state */
  doIdle( button );
}

/*
 * Change to power off state.
 * 
 * @param button The button that was pressed (not used here).
 */
void doPowerOff( button_t button ) {
  currentSequence = -1;
  blinkSet = 0;
  digitalWrite(RGBPIN_RED, 0);
  digitalWrite(RGBPIN_GREEN, 0);
  digitalWrite(RGBPIN_BLUE, 0);
  clearLEDs();
  state = STATE_OFF;
}

/*
 * Button handler for sequence button press. Determines if using
 * internal canned/EEPROM sequence or user-provided function,
 * and dispatches accordingly.
 * 
 * @param button The bottom that was pressed (translated into sequence number to run).
 */
void doSequenceButton( button_t button ) {
  /* Stop the pattern if running */
  button = button & BUTTONMASK;
  int newSeq = button - 8;
  int oldSeq = currentSequence;
  if ( currentSequence >= 0 ) {
    doIdle( button );
    if ( oldSeq == newSeq ) {
      /* Hit same button as running sequence, so just stop */
      return;
    }
  }

  /* Start the new sequence */
  Serial.print("Starting new sequence "); Serial.println( newSeq );
  currentSequence = newSeq;
  addBlinker( button );
  patternRunners[currentSequence]->start( currentSequence );
  state = STATE_RUN;
}

/* 
 * Button handler to enter pattern edit mode.
 * 
 * @param button The button that was pressed (determines which sequence to edit).
 */
void doEnterEdit( button_t button ) {
  doIdle( button );
  // ??? Check if current pattern func is our EEPROM func, error flash and return if not (we can't edit).
  state = STATE_EDITSEQ;
  editButton = button & BUTTONMASK;
  loadSequence( editButton - 8 );
  mySetLED( editButton, true, true );
  setBlinkers( 0x00ff ); 
  seqData.nextStep = 0; /* We'll re-use this for our current programming step */
  showStep();
}

/*
 * Button handler to change timing (in run mode).
 * 
 * @param button The button that was pressed
 */
void doTiming( button_t button ) {
  // Serial.print("doTiming "); Serial.println(button,HEX);
  /* Fetch the current timing */
  if ( currentSequence < 0 ) return; /* assertion */
  if ( ( button & BUTTONMASK ) == BUTTON_WHITE_UP ) {
    if ( nextStepDelay >= MaxDelay ) {
      nextStepDelay = MaxDelay;
    } else {
      nextStepDelay += DelayIncrement;
    }
  } else if ( ( button & BUTTONMASK ) == BUTTON_WHITE_DN ) {
    if ( nextStepDelay <= DelayIncrement ) {
      nextStepDelay = 0;
    } else {
      nextStepDelay -= DelayIncrement;
    }
  }
  /* Save the delay and rewrite EEPROM, so it's stored ??? if it's an EEPROM pattern?*/
  seqData.pattern.dly = nextStepDelay;
  uint16_t eaddr = 1 + currentSequence * sizeof(Pattern);
  EEPROM.put( eaddr, seqData.pattern );
  EEPROM[0] = getEECheck(); /* Recompute checksum */
}

/*
 * Button handler for editing pattern (in edit mode).
 * 
 * @param button The button that was pressed (action).
 */
void doEditSeq( button_t button ) {
  if ( (button & BUTTONMASK) == editButton) {
    /* Save this step */
    Serial.print("Save step "); Serial.println(seqData.nextStep, DEC);
    unsigned long rgb = LONGRGB( white, red, green, blue );
    seqData.pattern.steps[seqData.nextStep] = rgb;
    seqData.nextStep += 1;
    if ( seqData.nextStep >= MAXSTEP || (button & LONGPRESS) ) {
      /* Stop editing */
      Serial.print("End of edit, saving pattern "); Serial.print( editButton-8 ); Serial.print(" length "); Serial.println( seqData.nextStep );
      
      /* Write currentPattern to EEPROM */
      seqData.pattern.len = seqData.nextStep; /* Make sure length is correct--may shorten! */
      seqData.pattern.dly = seqData.nextStep > 1 ? nextStepDelay : 0x7fff; /* force long delay if only one step in pattern */
      for (uint8_t i=seqData.nextStep; i<MAXSTEP; ++i) {
        seqData.pattern.steps[i] = 0;
      }
      uint16_t eaddr = 1 + (editButton-8) * sizeof(Pattern);
      EEPROM.put( eaddr, seqData.pattern );
      EEPROM[0] = getEECheck(); /* Recompute checksum */

      /* Clear Trellis and run pattern */
      clearBlinkers( 0xffff );
      doIdle(button);
      doSequenceButton(button);
    } else {
      showStep();
    }
  } else if ( button < 8 ) {
      doColorAdjustment( button );
      setRGBW();
  } else {
      flash( button & BUTTONMASK, 3, 50 );
  }
}

/*
 * Button handler for color buttons. Stops running sequence and
 * adjusts color.
 * 
 * @param button The button that was pressed (adjusts color channel).
 */
void doColorAdjustment( button_t button ) {
  doIdle( button );
  adjustColor( button & BUTTONMASK );
}

/* Button handler to reset EEPROM. Cheat-ish. It just mangles the
 * checksum and allows the next reset to re-write the contents.
 * 
 * @param button The button that was pressed (not used here).
 */
void doEEReset( button_t button ) {
  Serial.println("EEPROM reset!");
  EEPROM[0] = ~EEPROM[0];
  delay(250);
  resetFunc();
}

/* 
 * Flash button rapidly. Used to indicate error/invalid button press.
 * 
 * @param button The Trellis button to be flashed.
 * @param count Number of times to flash.
 * @param dly Delay (ms) between flashes.
 */
void flash( uint8_t button, int count, unsigned long dly ) {
  if (dly < 30) {
    dly = 30;
  }
  while (count > 0) {
    if ( trellis.isLED( button ) ) {
      trellis.clrLED( button );
      trellis.writeDisplay();
      delay( dly );
      trellis.setLED( button );
    } else {
      trellis.setLED( button );
      trellis.writeDisplay();
      delay( dly );
      trellis.clrLED( button );
    }
    trellis.writeDisplay();
    delay( dly );
    count -= 1;
  }
}

/* 
 * Get button press. 
 * 
 * Returns button number and flags as an unsigned 16-bit
 * value. The lower 8 bits are the button number, and the upper 8 are the
 * flags. The flags indicate short press, long press, or repeat. Holding
 * a button for one second or more generates a long press. For the first 8
 * buttons (0-7), it will also generate repeats. Nota bene: that means the
 * first 8 buttons will first return at least one repeat, more until the
 * button is released, followed by a long press when released.
 * 
 * @returns short
 *          Button number in the lower 8 bits, and flags in the upper.
 */
button_t getButton() {
  /* Change? */
  if (! trellis.readSwitches() ) {
    /* Nothing has changed, but see if any of 0-7 are down for long enough.
     *  If so, generate repeating events.
     */
    for (uint8_t key=0; key<8; ++key) {
      if ( keyDownTime[key] > 0 && ( keyDownTime[key] + 1000 ) < millis() ) {
        keyDownTime[key] = millis() - 800;
        return SHORTPRESS | KEYREPEAT | (button_t) key;
      }
    }
    return NOBUTTON;
  }
  for (uint8_t key=0; key<numKeys; ++key) {
    if (trellis.isKeyPressed(key)) {
      // Serial.print("Key "); Serial.print(key); Serial.println(" down");
      if (keyDownTime[key] == 0) {
        /* Key is newly-pressed */
        keyDownTime[key] = millis();
      } else {
        /* Key is still pressed */
      }
    } else if (keyDownTime[key] > 0) {
      // Serial.print("Key "); Serial.print(key); Serial.print(" up");
      if (keyDownTime[key]+1000 > millis()) {
        //Serial.println(", short press.");
        keyDownTime[key] = 0;
        lastKeyTime = millis();
        return SHORTPRESS | (button_t) key;
      } else {
        //Serial.println(", long press.");
        keyDownTime[key] = 0;
        lastKeyTime = millis();
        return LONGPRESS | (button_t) key;
      }
    } 
  }
  return NOBUTTON;
}

/*
 * Arduino setup routine (called by runtime).
 */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); while (!Serial) { ; }
  Serial.println(F("\n\n***RESET\ntoggledbits RGB Strip Driver " __DATE__ " " __TIME__));

  /* Initialize RGB outputs. */
  red = green = blue = white = 0;
  pinMode(RGBPIN_RED, OUTPUT);
  pinMode(RGBPIN_GREEN, OUTPUT);
  pinMode(RGBPIN_BLUE, OUTPUT);
  digitalWrite(RGBPIN_RED, 0);
  digitalWrite(RGBPIN_GREEN, 0);
  digitalWrite(RGBPIN_BLUE, 0);
  
  ledState = 0;
  blinkSet = 0;
  state = STATE_OFF;
  lastKeyTime = lastButtonCheck = 0;
  
  trellis.begin(0x70);
  clearLEDs();

  /* Initialize EEPROM if needed */
  mySetLED(0, true, true);
  Serial.println(F("Checking EEPROM..."));
  uint8_t ck = getEECheck();
  if ( EEPROM[0] != ck ) {
    unsigned short ll = sizeof(Pattern) * NUMSEQ;
    if ( ll >= EEPROM.length() ) { /* need ll+1 bytes */
      Serial.print("TOO LONG! NEED "); Serial.print(ll); Serial.print(" but only have "); Serial.println(EEPROM.length());
      ll = (EEPROM.length() / sizeof(Pattern)) * sizeof(Pattern);
    }
    Serial.print(F("EEPROM checksum mismatch, looking for ")); Serial.print(ck, HEX); Serial.print(" got "); Serial.println( EEPROM[0], HEX );
    mySetLED(1, true, true);
    for (uint16_t i=0; i<ll; ++i) {
      byte b = pgm_read_byte_near( ((byte *) &Seqs) + i );
      EEPROM[i+1] = b;
    }
    ck = getEECheck();
    Serial.print("EEPROM loaded, checksum is now "); Serial.println(ck, HEX);
    EEPROM[0] = ck;
  } else
    Serial.println(F("EEPROM checksum PASS!"));

  EEPROMRunner er = EEPROMRunner();
  for ( uint8_t i=0; i<8; ++i) {
    patternRunners[i] = &er;
  }
  /* Provide any overrides for the default pattern function here */
  patternRunners[4] = new ColorWheelRunner(); /* override for button 13/power button/pattern 5 */
  
  /* Finished. Clear the Trellis LEDs and return */
  clearLEDs();
  Serial.println("setup() finished.");
}

/* Quickie button/state transition table. */
typedef struct {
  int currentState; /* State to match */
  button_t button;  /* Button (mask) to match (LSB=button 1, MSB=button 16) */
  button_t flags;   /* Flags to match (match any) */
  void (*func)( button_t buttonWithFlags ); /* Button handler to call */
} T;
const T Trans[] = {
  { STATE_OFF, BMASK( BUTTON_RESET ), LONGPRESS, doEEReset },
  /* Button 13 (bottom left) is power button */
  { STATE_OFF, BMASK( BUTTON_POWER ), SHORTPRESS|LONGPRESS, doPowerOn },
  { STATE_IDLE, BMASK( BUTTON_POWER ), LONGPRESS, doPowerOff },
  /* Color buttons in idle stop sequence and change color */
  { STATE_IDLE, 0x00ff, SHORTPRESS, doColorAdjustment },
  /* Press 9-16 to run or stop sequence preset */
  { STATE_IDLE, 0xff00, SHORTPRESS, doSequenceButton },
  /* Long press 9-16 in idle state to edit sequence preset */
  { STATE_IDLE, 0xff00, LONGPRESS, doEnterEdit },
  /* Button 9-16 presses during edit are commands */
  { STATE_EDITSEQ, 0xff00, SHORTPRESS|LONGPRESS, doEditSeq },
  { STATE_EDITSEQ, 0x00ff, SHORTPRESS|LONGPRESS, doColorAdjustment },
  /* During run, long press on power is always power off */
  { STATE_RUN, BMASK( BUTTON_POWER ), LONGPRESS, doPowerOff },
  /* During run of pattern, short press changes sequence */
  { STATE_RUN, 0xff00, SHORTPRESS, doSequenceButton },
  /* During run, intensity buttons idle except white, which changes timing */
  { STATE_RUN, BMASK( BUTTON_WHITE_UP )|BMASK( BUTTON_WHITE_DN ), SHORTPRESS|LONGPRESS, doTiming }
};
#define NTRANS (sizeof(Trans) / sizeof(T))

/*
 * Runtime loop.
 */
void loop() {
  delay(10);
  
  updateBlinkers();

  /* doNextInSequence? */
  if ( state == STATE_RUN ) {
    tickSequence();  
  }

  if ( lastButtonCheck + 40 > millis() )
    return;
  button_t btn = getButton();
  lastButtonCheck = millis();
  if ( btn == NOBUTTON ) {
    if ( lastKeyTime + DIMDELAY <= millis() ) {
      trellis.setBrightness(1);
    }
    return;
  }
  trellis.setBrightness(15);
  button_t bBit = 1<<(btn & BUTTONMASK);
  button_t bFlags = btn & ~BUTTONMASK;
  
  // Serial.print("State "); Serial.print(state); Serial.print(", button "); Serial.print(btn); Serial.print(" ("); Serial.print(btn,HEX); Serial.println(")");
  // Serial.print("      bBit = "); Serial.print(bBit,HEX); Serial.print(", bFlags = "); Serial.println(bFlags,HEX);
  for (int k=0; k<NTRANS; ++k) {
    if ( Trans[k].currentState == state && Trans[k].button & bBit && Trans[k].flags & bFlags ) {
      // flash( btn, 1, 30 );
      Trans[k].func( btn );
      return;
    }
  }

  /* We didn't figure out what to do... */
  Serial.print(F("*** No action for state ")); Serial.print(state); Serial.print(F(" button ")); Serial.println(btn, HEX);
  flash( btn & BUTTONMASK, 3, 50 );
}
