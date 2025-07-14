# Introduction
This is a VST version of the Timidity MIDI player. The version of Timidity that is used in this project is based on the GSPlayer implementation, which is subsequently based on Timidity 0.2i by Tuukka Toivonen.

# Usage Notes
In order for the instrument to generate sound, it requires a set of Gravis Ultrasound patches to be loaded into the synth. There are several sets of instruments out there, and these include the original Gravis Ultrasound patch set, the instrument bank distributed with the GSPlayer MIDI plug-in, Pro Patches Lite, and EAWPATS. After downloading an instrument set, simply press the Load Configuration button in the plug-in's user interface, and browse to the appropriate Timidity configuration file. Note: some parameters require that instruments be reloaded in order for them to take effect, so some parameter changes can take slightly longer if a large instrument bank is being used.

# Automatable Parameters

* Volume: Synth master volume.
* VolumeDisplay: Sets the unit for displaying the aforementioned Volume parameter, either dB or %.
* ImmediatePan: Pans notes that are already playing. When this option is disabled, pan control messages won't take effect until the next note is played.
* Mono: Generates mono audio output.
* Transpose: Applies an offset to incoming MIDI notes.
* Voices: the maximum number of notes that can simultaneously play at a time.
* FastDecay: Makes notes decay slightly faster. When this option is enabled, the output can sound more like a Gravis Ultrasound.
* Antialiasing: Runs samples through an antialiasing filter during the loading process.
* PreResample: Resamples fixed pitch instruments during the loading process.
* DynamicLoad: Dynamically loads instruments as they are needed.
* ControlRate: The higher the value, the smoother envelopes and LFOs are rendered.
* PushMidi: Queue's MIDI events instead of processing them immediately. Queued events have sample accurate timing, while immediate events can have jittery playback with large audio buffers.

# What's with the GUI?
The user interfaces of most audio plug-ins draw graphics directly to the screen. The problem with this approach is that these controls aren't seen by assistive technology such as screen readers, which are designed to work with native operating system controls. This project aims to change that. The UI has been programmed from the ground up using raw Win32 with mostly standard controls, the only exception being the virtual MIDI keyboard.

## Guide to the GUI

### Settings

* Preset: sets the currently active preset. Even though this instrument only has a single preset, a framework has been set in place for working with multiple presets.
* Preset Name: sets the name of the currently active preset.
* The next set of options correspond to the previously mentioned automatable parameters, so they will be omitted here. It is worth noting that parameters such as Volume and Transpose have a static text label to the right of the control that shows the current value.
* Bypass: Stops processing audio and MIDI data. It is worth noting that this plug-in acts as a pass through device, as it takes audio in and mixes it with the synth output, as well as outputs MIDI back to the host. This is still the case when Bypass is enabled.

### Misc

* Refresh: refreshes the user interface with the plug-in's current settings and state.
* Load Configuration: loads a configuration file into the synth. The current configuration is shown to the right of this control.
* Panic Synth: stops all notes immediately.
* Reset Synth: stops all notes immediately and resets MIDI parameters. The number of currently active voices is shown to the right of this control.
* About: displays information about the VST plug-in, such as credits.
* Plug-in Statistics: shows information including the host sample rate and block size, as well as the number of active plug-in instances. The audio output level is shown to the right of this control.
* Host Info: displays information about the host the plug-in is running in.
* Hard Reset: Fully reinitializes the internal synthesis engine. The CPU load is shown to the right of this control.
* Forget Settings: resets all settings to there default values.
* Freeze Meters: This causes the active voice count, audio output level, and CPU load to stay at there last known values. When the user interface is refreshed by the user, these meters are updated regardless of this setting.
* Hide Parameter Display: hides the display of parameter values such as Volume and Transpose. The virtual MIDI keyboard is below this control.
* Channel Mixer: brings up a dialog where MIDI channels can be enabled or disabled. When a channel is disabled from this dialog, all notes are stopped and various controllers are reset on the relevant channel. This dialog also has All and None buttons for quickly enabling or disabling all channels, and there is a Close button to go back to the main window.
* Open Project Page: opens the project page in the user's default web browser. The user is asked if they are connected to the internet before attempting to load the browser.

## Virtual MIDI Keyboard Cheat Sheet

* Number Row: Select octave.
* Top Row: Sharp notes.
* Home Row: Flat notes.
* Bottom Row: Select velocity.
* Shift: Sustain.
* Up Arrow: Increase velocity.
* Down Arrow: Decrease velocity.
* Right Arrow: Increase octave.
* Left Arrow: Decrease octave.
* Page Down: Move to next instrument.
* Page Up: Move to previous instrument.
* End: Increase active channel.
* Home: Decrease active channel.
* =: Increase bend MSB.
* -: Decrease bend MSB.
* \: Increase bend LSB.
* `: Decrease bend LSB.
* Backspace: Reset pitch bend and program to current keyboard state.
* Space: Reset keyboard state.

# Extra Notes

* This is only a VST2 compatible plug-in. A VST3 version is not planned for various reasons.

# Building from Source
In order to build Timidity VSTi you will need

* Microsoft Visual Studio 2005
* Windows Server 2003 Platform SDK
* InnoSetup 5.4.3
* 7-Zip (any version)

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
* Me (Datajake), for writing the VST interface for this synth. It is about time someone did so.
* Gravis, for creating the Ultrasound series of sound cards back in the 90s.
* Old blogs and forums, for tips on VST development.
* Charles Petzold, for writing Programming Windows, Fifth Edition.
* Veli-Pekka Tätilä, for writing a set of recommended accessibility practices to keep in mind when designing VST user interfaces.
