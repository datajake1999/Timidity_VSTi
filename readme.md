# Introduction
This is a VST version of the Timidity MIDI player. The version of Timidity that is used in this project is based on the GSPlayer implementation, which is subsequently based on Timidity 0.2i by Tuukka Toivonen.

# Usage notes
In order for the instrument to generate sound, it requires a set of Gravis Ultrasound patches to be loaded into the synth. There are several sets of instruments out there, and these include the original Gravis Ultrasound patch set, the instrument bank distributed with the GSPlayer MIDI plug-in, Pro Patches Lite, and EAWPATS. After downloading an instrument set, simply press the Load Configuration button in the plug-in's user interface, and browse to the appropriate Timidity configuration file. Note: some parameters require that instruments be reloaded in order for them to take effect, so some parameter changes can take slightly longer if a large instrument bank is being used.

# Automatable parameters

* Volume: Synth master volume.
* VolumeDisplay: Sets the unit for displaying the aforementioned Volume parameter, either dB or %.
* ImmediatePan: Pans notes that are already playing. When this option is disabled, pan control messages won't take effect until the next note is played.
* Mono: Generates mono audio output.
* Transpose: Applies an offset to Incoming MIDI notes.
* Voices: the maximum number of notes that can simultaneously play at a time.
* FastDecay: Makes notes decay slightly faster. When this option is enabled, the output can sound more like a Gravis Ultrasound.
* Antialiasing: Runs samples through an antialiasing filter during the loading process.
* ControlRate: The higher the value, the smoother envelopes and LFOs are rendered.
* PushMidi: Queue's MIDI events Instead of processing them Immediately. Queued events have sample accurate timing, while immediate events can have jittery playback with large audio buffers.

# What's with the GUI?

The User Interfaces of most audio plugins draw graphics directly to the screen. The problem with this approach Is that these controls aren't seen by assistive technology such as screen readers, which are designed to work with native Operating System controls. This project aims to change that. The UI has been programmed from the ground up using raw Win32 with mostly standard controls, The only exception being the Virtual MIDI Keyboard.

## Virtual MIDI Keyboard cheat sheet

* Number row: Select octave.
* Top row: Sharp notes.
* Home row: Flat notes.
* Bottom row: Select velocity.
* Shift: Sustain.
* Up arrow: Increase velocity.
* Down arrow: Decrease velocity.
* Right arrow: Increase octave.
* Left arrow: Decrease octave.
* Page down: Move to next Instrument.
* Page up: Move to previous Instrument.
* End: Increase active channel.
* Home: Decrease active channel.
* =: Increase bend MSB.
* -: Decrease bend MSB.
* \: Increase bend LSB.
* `: Decrease bend LSB.
* Backspace: Reset pitch bend and program to current keyboard state.
* Space: Reset keyboard state.

# Extra notes

* This is only a VST2 compatible plug-in. A VST3 version Is not planned for various reasons.

# Building from source
In order to build Timidity VSTi you will need

* Microsoft Visual Studio 2005
* Windows Server 2003 Platform SDK
* InnoSetup 5.4.3
* 7-Zip

After installing, just run build.bat. You may need to change the paths defined in this script to correspond with your installation of the build tools.

# License
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

# Credits

* Tuukka Toivonen and other's, for writing the original version of Timidity.
* Y.Nagamidori, for creating GSPlayer.
* HIMS, for using GSPlayer as the underlying playback engine for media files on their older Braille Sense note takers that ran Windows CE. This is how I was introduced to Timidity in the first place, however I didn't know it at the time.
* Me, for writing the VST interface for this synth. It is about time someone did so.
* Gravis, for creating the Ultrasound Series of sound cards back in the 90s.
* Old blogs and forums, for tips on VST development.
