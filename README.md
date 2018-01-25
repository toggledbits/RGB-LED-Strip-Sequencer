# RGB LED Strip Sequencer using Arduino & Adafruit Trellis #

![toggledbits](https://github.com/toggledbits/RGB-LED-Strip-Sequencer/raw/master/Images/cover.jpg "RGB LED Strip Sequencer (Arduino and Adafruit Trellis)")

## Background ##

My wife had been asking me to install LED strips on my boys' hutch-style desks, which were a little dark.
The boys told me they wanted cool color LEDs, not just the white strips that their Mom and I use.
I bought a couple of RGB LED strips on Amazon, most of which seem to come as kits with a power supply 
and controller. But having used the included controllers before, I know that their canned patterns
would quickly become boring to my boys. So, I thought it would better if I built something, using an
Arduino Nano or something similar.

As I thought about the project a little more, I realized that it could be a great opportunity to give them
an intro to programming live projects with a useful result. I've been teaching them programming for a few
months now, but we've mostly been printing to a shell stdout or web page. Here, I had an opportunity to let
them program to blink lights! Any way they want!

So, the project quickly morphed from a 30-minute job getting LED strips to stick to the desk frame to several
weeks of tinkering with Arduino Sketch, circuit design, board layout, and 3D printing. My concept was to build
an Arduino-based LED sequencer that they could use to create simple patterns from a user interface of some kind,
or as their skills grew, implement their patterns within a Sketch that acted like a framework: they just supply
their one part at first, but later they can venture into any part of the code they want.

This repository contains what is currently my version 1.1 effort. 

## What It Does ##

[See it in action on YouTube!](https://youtu.be/xU6wqpSIGhU)

The Arduino drives three MOSFETs to drive the color channels of the LED strip. An [Adafruit Trellis](https://www.adafruit.com/product/1616) 4x4 button 
matrix is used to create the user interface. Color-keying of the button LEDs helps provide visual cues to function
(e.g. changing the red channel intensity is done using the red buttons, green channel using green buttons, etc.).

To turn the project on, long-press the lower-left button. The project lights up all LEDs, then turns off the
bottom two rows except the lower-left (power) button.

The two top rows of buttons change the color intensity of the red, green, and blue channels. Pressing the white
buttons changes the LED strip color to white (all channels driven equally) and varies the intensity in the same
manner as the individual color buttons.

Eight preset patterns are stored in the Arduino's EEPROM. A pattern can be played by pressing one of the eight
buttons in the lower two rows of the pad. The button blinks while the pattern is running. Pressing the button
again stops the pattern. Pressing a different pattern button starts that new pattern. Pressing the intensity
buttons (top two rows) stops the pattern at its current step and changes the color accordingly.

The EEPROM-based patterns can be edited using the keypad interface. A long-press (more than one second) on a
pattern button puts the project into editing mode for that pattern. When in editing mode, the upper two rows
of buttons blink, and the LED strip shows the color of the current step. Pressing the intensity buttons modifies
the color for that step. When a satisfactory color is set, a short press on the pattern button advances to the next step.
These steps are continued until editing completes, either by short press on the 16th step (max 16 steps per pattern), 
or by long-pressing the pattern button at any point. The modified pattern then automatically runs.

To turn the project off, long-press the lower left button.

A "factory reset" of the project, restoring the original default patterns to EEPROM, can be done by pressing
and holding the bottom left and bottom right buttons until the top two LEDs light. Release the buttons, and the
project will restore defaults to EEPROM.

## Operation and Development ##

### Driving the LED Strip ###

The first thing I did was wire up a proto board using MOSFETs to drive the color channels. I used IRLB8721s because
I had a bunch around and they would be fine for driving fairly hefty wattages of LEDs. The MOSFETs are driven
by PWM-capable digital outputs on the Ardunio (D9-11, specifically), allowing brightness control.

![toggledbits](https://github.com/toggledbits/RGB-LED-Strip-Sequencer/raw/master/Images/proto-board.jpg "The proto board")

### Discovering a User Interface ###

For an interface, I had often used the uniquitous 16x2 LCD with four or five tactile switches for projects, 
but I felt that that kind of interface was going to be slow and clumsy for this purpose. 
What I really thought would work well was a 
matrix of lighted buttons like they use on MIDI controllers. I thought it would be easy to create an interface
on it, and the boys would think it was really cool-looking (shout out to Thomas Dolby for
that--it's a long story, maybe ask me over a beer).

I don't recall how I found it, but I ended up stumbling across the Adafruit Trellis. It's (as of this writing)
a $9.95 board (plus $4.95 for the silicone keypad--not sure why they sell it separately) with a 4x4 matrix of
addressable switches. The board doesn't come with LEDs, but you can provide your own and solder them to pads
provided. The Arduino library lets you check button pressed and turn LEDs on and off with simple function calls, and 
the board uses two-wire (I2C) communication. You can connect up to four together to create an 8x8 matrix.

I ordered one, and started playing with it as soon as it arrived. Perfect. I soon had it working on the Arduino,
and from there, imagining an interface and getting it to work took only a couple of hours.

With the user interface shaping up, I connected everything together, supplied power to the LED strip, and played
with my new toy. This was going to work great for the boys.
Except, I could not imagine putting a spaghetti bundle of three boards and wiring on their desks. That was
fraught with peril. I needed an enclosure, but that wasn't going to be enough.
The solder-globbing style of proto board work left me feeling like it was going to be
a permanently unpolished project. The case my hide the worst of it, but I wanted the project to
look as polished on the inside as it was on outside. The driver board had to be cleaned up.

### Version 1.0 is Born ###

I started designing the driver board as a proper printed circuit board (PCB). It seemed obvious to me to make it a
shield for the Arduino, so the connections between the two could be many and firm. After Googling out a few diagrams,
I had a board properly dimensioned with headers in the right places to be a shield. A quick print at actual size
confirmed the fit. Adding the driver circuit to the board from there was easy, as my driver circuit is simple and even the
small size of the Arduino footprint left plenty of room.

The next consideration was simplifying the power supplies. In the prototype, the LEDs were separately powered, and the
Arduino drew its power from the USB port. It had to work standalone from a single supply. I double-checked the specs for 
the Uno, and 12V supply wasn't going to be a problem. Checking the schematic, I could see there was a VIN header pin that would
give me the supply voltage, but there was a potential problem: the PJ202 jack on the Arduino has a protection diode connected between it and that VIN pin, 
and that diode (and probably the traces on the Arduino) were unlikely to handle the potential power demand that the LED strip
would make on them. The schematic quickly confirmed, however, that I could supply power *to* the Arduino on
the VIN pin, and have the 12V supply connect directly to the shield. This left the matter of getting a 5V supply to the
Trellis, but that's easy because the Arduino supplies 5V from its on-board regulated supply on a header pin. So, in the final result,
12V is supplied to the shield, which sends 12V to the Arduino over VIN, which sends 5V back to the shield, which sends that 5V up to
the Trellis. And, the shield traces are all large enough to handle the larger currents the LED strips require. Done.

Right before I sent the boards out to be fabricated, I realized that I could make holes for stand-offs, so the Trellis could
mount to the shield. This would make a sandwich: Arduino on the bottom, shield connected to it, and Trellis connected to shield
on top. Perfect. I just had to move two traces slightly to get the Trellis mounting holes where they needed to be, and off the v1.0 
board went to the board house.

When I got the boards back, everything went together perfectly. I tested the board standalone, driving it with my bench
power supply, and it seemed to work fine. When I hooked it up to the Arduino, everything worked perfectly. So there it was,
version 1.0.

Check out the [YouTube video](https://youtu.be/SFQtTEeX06Y).

![toggledbits](https://github.com/toggledbits/RGB-LED-Strip-Sequencer/raw/master/Images/shield-and-trellis.jpg "Trellis on Shield")

It took me a few attempts to get the 3D-printed enclosure just perfect. I've only recently started using Fusion 360, and I find it
quite intuitive and easy to work in. I really love it. It's much easier to use and more powerful than my previous go-to tool,
123D Design (also AutoDesk, now discontinued). The lid was actually a snap, because the holes for the Trellis' buttons
are very precisely spaced and Fusion 360 has tools for exactly
that kind of thing. It took me less than 30 minutes to design a lid, and that design changed very little throughout. The body,
however, had holes in two of its vertical sides, and getting those positioned so that they ended up lining up right with my three-board
sandwich took a couple of attempts. It took about 4 hours to print a test body with 200u layers, so it wasn't that it was hard, 
each test print just took time (shout out to [Josef Prusa and crew](https://www.prusa3d.com/) here--this was my first time using 
my new Prusa i3 MK2S Christmas present on a real project, and it performed beautifully).

The final design looks clean and fits OK. The Arduino is inserted first, on its own, into the bottom of the body. The
shield with the Trellis mounted is then inserted into the Arduino. The lid is then pressed on over the Trellis, and is
held by friction. I could tweak it to death, but I have other ideas about how I want to modify the project that will require
big changes to the enclosure, so it's not worth my time at the moment to sweat over a millimeter here or there.

![toggledbits](https://github.com/toggledbits/RGB-LED-Strip-Sequencer/raw/master/Images/test-print.jpg "An early test print")

### Today: Version 1.1 ###

As happens with all prototypes, you discover a few mistakes along the way. In the process of building the first unit for
my older boy, and writing the Instructable to describe the process, I realized that I made two mistakes I would need to
corrected right away:

* I needed a protection diode to prevent any power directly into the Arduino from trying to power the LEDs. That is, if
the project was connected to USB for Sketch updating and the 12V supply was not connected, the Arduino would be sending
USB power at 5V up to the shield on the VIN pin, and the shield would attempt to power the LEDs with it. To prevent that,
a diode facing the Arduino has been added to the shield, which blocks any backfeeding of the shield from the Arduino. The
Arduino can be safely connected to USB, and will even power the Trellis, but it won't power the LED circuit on the shield.
* I have not yet provided a way in the Sketch code to control the rate at which patterns are run. This is a relatively
easy enhancement to the UI.

So, as it stands today, the project consists of:

1. The custom-designed Arduino shield that creates all of the interfaces for the circuit: input, output, and power;
1. The Arduino Sketch that drives the shield and the Adafruit Trellis;
1. The enclosure designed in Fusion 360 to fit the current hardware configuration.

## What's Next ##

My current thoughts about the next revision of this project are:

1. A version of the driver that accepts a Nano or Feather directly. This will "shorten" the sandwich and make a more compact design.
1. WiFi-enable it so my home automation system can monitor and control it.
1. Modify the case design to that the boards are inserted from the bottom. This will make fitting them into the case easier.

## Build One! ##

If you want to build one, check out the [YouTube video](https://youtu.be/SFQtTEeX06Y) and [Instructable](#) I made about building this project.

This Github repository contains all of the files you need: schematics, board layouts (if you decide to build the shield instead of just
using a proto board or breadboard), the Arduino Sketch, and the enclosure model (STL) files.

The enclosure models (STLs) are also [on Thingiverse](https://www.thingiverse.com/thing:2768560).