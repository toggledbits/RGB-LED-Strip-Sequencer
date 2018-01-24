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
 * A2, A4, and A5 - Connect to Trellis INT, SDA, and SCL respectively. 5V and GND
 * are connected as usual. This is the "reference" (example) connection for the
 * Trellis. The INT pin (A2) is not used and may be omitted.
 * 
 * Digital 9, 10, and 11 connect to the red, green, and blue MOSFET driver gates.
 * The LED strip leads for each color (cathodes) are connected to the MOSFET drains.
 * The MOSFET drains are connected to +12V (for a 12V LED strip). LED voltage ground
 * MUST be connected in common with the Arduino/5V system ground (same reference).
 * 
 * If a white channel is used, the standard #define expects it on pin 6. Connection
 * is the same as the RGB channels.
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
 * When the Uno powers up, it initializes the Trellis library and various globals for
 * its button and sequence processing, and waits in the "OFF" state (no LEDs on). The 
 * Trellis is unlit at this point. Press and hold the lower-left Trellis button to turn
 * the unit ON. The system will power up the Trellis display, lighting the 8 color con-
 * trol buttons (0-7) and the lower-left (12) button (as a power indicator, redundant).
 * The system is now in IDLE state. In this state, the color output of the LED drivers
 * can be modified using buttons 0-7 (for example, buttons 0 and 4 raise and lower,
 * respectively, the brightness of the red LED strip channel if everything is connected
 * correctly). To run a pattern, briefly press one of the pattern buttons 8-15 (the
 * lower two rows of buttons). There are eight-preprogrammed sequences or patterns
 * defined. Stop a running pattern by either pressing the same pattern button again
 * (the button of the running pattern is the blinking one, so press the blinking one).
 * You can also press the RGBW brightness buttons (0-7) at any time to stop the running
 * sequence at the current step.
 * 
 * Patterns can be edited, and the edited pattern is stored in EEPROM. To edit a pattern,
 * "long-press" (more than one second) any of the pattern buttons except 12 (more below).
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
 * Since the LED strip I'm using doesn't have a white channel, I reuse the white bright-
 * ness buttons to control sequence timing. Use the up and down white buttons to increase
 * and decrease the inter-step delay (each unit/press is about 128ms, so timing varies
 * from 128ms to 2,176ms--you can't set a delay of zero).
 * 
 * To turn the system off, press and hold button 12 (lower left). Because of this, pre-
 * defined pattern 5 cannot be edited, as the long-press is used for power off in pref-
 * erence to entering edit mode. Maybe a two-button sequence could be implemented instead,
 * but for now, this is good enough.
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
#define RGBPIN_WHITE 6

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
  /* 4 */  { 1, 32767, 0xFFFFFFFF },
  /* 5 */  { 1, 32767, 0x00FF0000 },
  /* 6 */  { 1, 32767, 0x0000FF00 },
  /* 7 */  { 1, 32767, 0x000000FF },
};
#define NUMSEQ (sizeof Seqs / sizeof(Pattern))

/* Working variables for pattern run and edit */
Pattern currentPattern; /* Current pattern running (or not) */
unsigned long nextStepTime;
short nextStep;
short currentSequence;

/* Working variables for RGB drivers */
short red, green, blue, white; /* yes, signed */
const short incr = 16;
#define LS(b,n) (((unsigned long)(b & 0xff))<<n)
#define LONGRGB( w, r, g, b ) (LS(w,24)|LS(r,16)|LS(g,8)|(b&0xff))

/* Working variables for system state */
#define STATE_OFF 0
#define STATE_IDLE 1
#define STATE_GETEDITBTN 2
#define STATE_EDITSEQ 3
uint8_t state = STATE_OFF;
uint8_t editButton;

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

/* Working variables for LED state tracking */
unsigned long ledState = 0;
unsigned long blinkSet = 0;
unsigned long lastBlink = 0;
#define BLINKRATE 250
#define BMASK( button ) (((unsigned short) 1) << (button & BUTTONMASK))

/*
 * Prototype to set/enforce argument default.
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
 * Put a step of the current pattern on the drivers.
 * 
 * @param n The step of the current pattern to be shown.
 */
void showStep( short n ) {
  unsigned long rgb = currentPattern.steps[n];
  white = (rgb >> 24) & 0xff;
  red = (rgb >> 16) & 0xff;
  green = (rgb >> 8) & 0xff;
  blue = rgb & 0xff;
  setRGBW();
  Serial.print("Seq "); Serial.print(currentSequence); Serial.print(" step "); Serial.print(n); Serial.print(": ");
  Serial.print("W="); Serial.print(white); Serial.print(", ");
  Serial.print("R="); Serial.print(red); Serial.print(", G="); Serial.print(green); Serial.print(", B="); Serial.println(blue);
}

/*
 * Check to see if the next scheduled pattern step is eligible
 * (timing), and display it if so.
 */
void tickSequence() {
  /* If we're not running a sequence, just return */
  if ( currentSequence < 0 ) {
    return;
  }
  /* Is it time for the next step? */
  if ( millis() < nextStepTime )
    return;
  /* Yes, yes it is. */
  nextStep += 1;
  if ( nextStep >= currentPattern.len ) {
    nextStep = 0;
  }
  showStep( nextStep );
  nextStepTime = millis() + currentPattern.dly;
}

/*
 * Clear all Trellis LEDs and update display.
 */
void clearLEDs() {
  for (uint8_t i=0; i<numKeys; ++i) {
    trellis.clrLED(i);
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
  analogWrite(RGBPIN_RED, red);
  analogWrite(RGBPIN_GREEN, green);
  analogWrite(RGBPIN_BLUE, blue);  
  /* analogWrite(RGBPIN_WHITE, white); */
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
 * Handle button-based color adjustment. ??? switch statement?
 * 
 * @param btn Button that's been pressed.
 */
void adjustColor( button_t btn ) {
  switch (btn) {
    case BUTTON_RED_UP:
      red = incrementBrightness( red, incr );
      break;
    case BUTTON_RED_DN:
      red = incrementBrightness( red, -incr );
      break;
    case BUTTON_GREEN_UP:
      green = incrementBrightness( green, incr );
      break;
    case BUTTON_GREEN_DN:
      green = incrementBrightness( green, -incr );
      break;
    case BUTTON_BLUE_UP:
      blue = incrementBrightness( blue, incr );
      break;
    case BUTTON_BLUE_DN:
      blue = incrementBrightness( blue, -incr );
      break;
    case BUTTON_WHITE_UP:
      white = incrementBrightness( white, incr );
      red = blue = green = white;
      break;
    case BUTTON_WHITE_DN:
      white = incrementBrightness( white, -incr );
      red = blue = green = white;
      break;
  }
  setRGBW();
  Serial.print("Adjust to R="); Serial.print(red); Serial.print(", G="); Serial.print(green); Serial.print(", B="); Serial.println(blue);
}

/* 
 * Transition to IDLE state.
 * 
 * @param button The button that was pressed (not used here).
 */
void doIdle( button_t button ) {
  currentSequence = -1;
  clearBlinkers(0xffff);
  state = STATE_IDLE;
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
  /* digitalWrite(RGBPIN_WHITE, 0); */
  clearLEDs();
  state = STATE_OFF;
}

/*
 * Load the specified sequence from EEPROM into RAM.
 * 
 * @param n The number of the sequence to load from EEPROM.
 */
void loadSequence( short n ) {
  uint16_t addr = 1 + n * sizeof(Pattern);
  Serial.print("Loading EEPROM pattern "); Serial.println(n, DEC);
  EEPROM.get( addr, currentPattern );
  Serial.print("Pattern length is "); Serial.println(currentPattern.len);
  Serial.print("Pattern delay is "); Serial.println(currentPattern.dly);
}

/*
 * Stop the running sequence, if any.
 */
void stopSequence() {
  if (currentSequence < 0) {
    return;
  }
  doIdle( 0 );
}

/*
 * Button handler to start (or stop) sequence.
 * 
 * @param button The button that was pressed (translated into sequence to run).
 */
void doSequence( button_t button ) {
  button = button & BUTTONMASK;
  if ( button < 8 ) {
    doIdle( button );
    return;
  }
  int newSeq = button - 8;
  if ( currentSequence >= 0 ) {
    int oldSeq = currentSequence;
    stopSequence();
    /* If same button as current sequence, toggle off to idle */
    if ( newSeq == oldSeq ) {
      return;
    }
  }
  addBlinker( button );

  /* Copy pattern from EEPROM to RAM */
  loadSequence( newSeq );

  /* Get started */
  currentSequence = newSeq;
  nextStep = 0;
  showStep( nextStep );
  nextStepTime = millis() + currentPattern.dly;
}

/* 
 * Button handler to enter pattern edit mode.
 * 
 * @param button The button that was pressed (determines which sequence to edit).
 */
void doEnterEdit( button_t button ) {
  Serial.println("doEnterEdit");
  stopSequence();
  state = STATE_EDITSEQ;
  editButton = button & BUTTONMASK;
  loadSequence( editButton - 8 );
  mySetLED( editButton, true, true );
  setBlinkers( 0x00ff ); 
  nextStep = 0; /* We'll re-use this for our current programming step */
  showStep( nextStep );
}

/*
 * Button handler for editing pattern (in edit mode).
 * 
 * @param button The button that was pressed (action).
 */
void doEditSeq( button_t button ) {
  Serial.print("doEditSeq "); Serial.println(button, HEX);
  if ( (button & BUTTONMASK) == editButton) {
    /* Save this step */
    Serial.print("Save step "); Serial.println(nextStep, DEC);
    unsigned long rgb = LONGRGB( white, red, green, blue );
    currentPattern.steps[nextStep] = rgb;
    nextStep += 1;
    if ( nextStep >= MAXSTEP || (button & LONGPRESS) ) {
      /* Stop editing */
      Serial.print("End of edit, saving pattern "); Serial.print( editButton-8 ); Serial.print(" length "); Serial.println( nextStep );
      
      /* Write currentPattern to EEPROM */
      currentPattern.len = nextStep; /* Make sure length is correct--may shorten! */
      currentPattern.dly = nextStep > 1 ? white * 8 : 0x7fff; /* force long delay if only one step in pattern */
      for (uint8_t i=nextStep; i<MAXSTEP; ++i) {
        currentPattern.steps[i] = 0;
      }
      uint16_t eaddr = 1 + (editButton-8) * sizeof(Pattern);
      EEPROM.put( eaddr, currentPattern );
      EEPROM[0] = getEECheck(); /* Recompute checksum */

      /* Clear Trellis and run pattern */
      clearBlinkers( 0x00ff );
      doIdle(button);
      doSequence(button);
    } else {
      showStep(nextStep);
    }
  } else if ( button < 8 ) {
      doColorAdjustment( button );
      setRGBW();
  } else {
      Serial.print("In edit, button "); Serial.println(button);
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
  stopSequence();
  adjustColor( button & BUTTONMASK );
}

/* Button handler to reset EEPROM. Cheat-ish. It just mangles the
 * checksum and allows the next reset to re-write the contents.
 * 
 * @param button The button that was pressed (not used here).
 */
void (*resetFunc)(void) = 0; // Arduino reset function at address zero
void doEEReset( button_t button ) {
  Serial.println("EEPROM reset!");
  EEPROM[0] = ~EEPROM[0];
  delay(250);
  resetFunc();
}

/*
 * Arduino setup routine (called by runtime).
 */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); while (!Serial) { ; }
  Serial.println(F("RGB Strip Driver " __DATE__ " " __TIME__));

  /* Initialize RGB outputs. These are sinks. */
  pinMode(RGBPIN_RED, OUTPUT);
  pinMode(RGBPIN_GREEN, OUTPUT);
  pinMode(RGBPIN_BLUE, OUTPUT);
  /* pinMode(RGBPIN_WHITE, OUTPUT); */
  digitalWrite(RGBPIN_RED, 0);
  digitalWrite(RGBPIN_GREEN, 0);
  digitalWrite(RGBPIN_BLUE, 0);
  /* digitalWrite(RGBPIN_WHITE, 0); */

  /* We're not using interrupts from the Trellis
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  */
  
  ledState = 0;
  blinkSet = 0;
  state = STATE_OFF;
  
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
  
  clearLEDs();
  Serial.println("setup() finished.");
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
    for (uint8_t key=0; key<7; ++key) {
      if ( keyDownTime[key] > 0 && ( keyDownTime[key] + 1000 ) < millis() ) {
        keyDownTime[key] = millis() - 800;
        return SHORTPRESS | KEYREPEAT | (button_t) key;
      }
    }
    return NOBUTTON;
  }
  for (uint8_t key=0; key<numKeys; ++key) {
    if (trellis.isKeyPressed(key)) {
      Serial.print("Key "); Serial.print(key);
      Serial.println(" down");
      if (keyDownTime[key] == 0) {
        /* Key is newly-pressed */
        keyDownTime[key] = millis();
      } else {
        /* Key is still pressed */
      }
    } else if (keyDownTime[key] > 0) {
      Serial.print("Key "); Serial.print(key);
      Serial.print(" up");
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

void xramp( short *pv, int dir) {
  if ( dir < 0 ) {
    while (*pv > 0) {
      *pv = *pv - 1;
      setRGBW();
      delay(10);
    }
  } else {
    while (*pv < 255) {
      *pv = *pv + 1;
      setRGBW();
      delay(10);
    }
  }
}

void colorWheel(unsigned short button) {
  uint8_t r, g, b;
  red = blue = green = 0; 
  setRGBW();
  xramp( &red, 1 );
  while (true) {
    xramp( &green, 1 );
    xramp( &red, -1 );
    xramp( &blue, 1 );
    xramp( &green, -1 );
    xramp( &red, 1 );
    xramp( &blue, -1 );
  }
}

/* Quickie button/state transition table. */
typedef struct {
  int currentState; /* State to match */
  button_t button;  /* Button to match */
  button_t flags;   /* Flags to match (match any) */
  void (*func)( button_t buttonWithFlags ); /* Button handler to call */
} T;
const T Trans[] = {
  { STATE_OFF, BMASK( BUTTON_RESET ), LONGPRESS, doEEReset },
  /* Button 12 (bottom left) is power button */
  { STATE_OFF, BMASK( BUTTON_POWER ), SHORTPRESS|LONGPRESS, doPowerOn },
  { STATE_IDLE, BMASK( BUTTON_POWER ), LONGPRESS, doPowerOff },
  /* Color buttons in idle stop sequence and change color */
  { STATE_IDLE, 0x00ff, SHORTPRESS, doColorAdjustment },
  /* Press 8-15 to run or stop sequence preset */
  { STATE_IDLE, 0xff00, SHORTPRESS, doSequence },
  /* Long press 8-15 in idle state to edit sequence preset */
  { STATE_IDLE, 0xff00, LONGPRESS, doEnterEdit },
  /* Button 8-15 presses during edit are commands */
  { STATE_EDITSEQ, 0xff00, SHORTPRESS|LONGPRESS, doEditSeq },
  { STATE_EDITSEQ, 0x00ff, SHORTPRESS|LONGPRESS, doColorAdjustment }
};
#define NTRANS (sizeof(Trans) / sizeof(T))

/*
 * Runtime loop.
 */
void loop() {
  delay(30);
  
  updateBlinkers();

  /* doNextInSequence? */
  if ( state != STATE_OFF ) {
    tickSequence();  
  }
  
  button_t btn = getButton();
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
