/*
Timidity VSTi
Copyright (C) 2021-2025  Datajake

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "Timidity.h"

void Timidity::initializeSettings (bool resetSynth)
{
	lock.acquire();
	vst_strncpy (ProgramName, "Default", kVstMaxProgNameLen-1);
	Volume = 1;
	VolumeDisplay = 0;
	ImmediatePan = 1;
	Mono = 0;
	Transpose = 0;
	Voices = DEFAULT_VOICES;
	FastDecay = 1;
	Antialiasing = 1;
	ControlRate = CONTROLS_PER_SECOND;
	PushMidi = 1;
	bypassed = false;
	memset(ConfigFile, 0, sizeof(ConfigFile));
	memset(ConfigName, 0, sizeof(ConfigName));
	for (VstInt32 i = 0; i < 16; i++)
	{
		ChannelEnabled[i] = true;
	}
	if (resetSynth)
	{
		suspend ();
		timid_unload_config(&synth);
		timid_free_default_instrument(&synth);
		if (Voices > MAX_VOICES)
		{
			Voices = MAX_VOICES;
		}
		else if (Voices < 1)
		{
			Voices = 1;
		}
		timid_set_max_voices(&synth, (VstInt32)Voices);
		if (ImmediatePan >= 0.5)
		{
			timid_set_immediate_panning(&synth, 1);
		}
		else
		{
			timid_set_immediate_panning(&synth, 0);
		}
		if (Mono >= 0.5)
		{
			timid_set_mono(&synth, 1);
		}
		else
		{
			timid_set_mono(&synth, 0);
		}
		if (FastDecay >= 0.5)
		{
			timid_set_fast_decay(&synth, 1);
		}
		else
		{
			timid_set_fast_decay(&synth, 0);
		}
		if (Antialiasing >= 0.5)
		{
			timid_set_antialiasing(&synth, 1);
		}
		else
		{
			timid_set_antialiasing(&synth, 0);
		}
		if (ControlRate > sampleRate)
		{
			ControlRate = sampleRate;
		}
		else if (ControlRate < sampleRate/MAX_CONTROL_RATIO)
		{
			ControlRate = sampleRate/MAX_CONTROL_RATIO;
		}
		timid_set_control_rate(&synth, (VstInt32)ControlRate);
	}
	lock.release();
}

bool Timidity::getBypass ()
{
	return bypassed;
}

bool Timidity::loadInstruments (char* filename, char* display)
{
	if (!filename || !display)
	{
		return false;
	}
	lock.acquire();
	if (timid_load_config(&synth, filename))
	{
		strncpy(ConfigFile, filename, sizeof(ConfigFile));
		strncpy(ConfigName, display, sizeof(ConfigName));
		lock.release();
		return true;
	}
	lock.release();
	return false;
}

void Timidity::enableChannel (VstInt32 channel, bool enable)
{
	lock.acquire();
	channel = channel & 0x0f;
	if (ChannelEnabled[channel] && !enable)
	{
		char data[3];
		data[0] = 0xb0 + (char)channel;
		data[1] = 0x40;
		data[2] = 0;
		sendMidi (data);
		data[1] = 0x7b;
		sendMidi (data);
		data[1] = 0x79;
		sendMidi (data);
		data[0] = 0xe0 + (char)channel;
		data[1] = 0;
		data[2] = 0x40;
		sendMidi (data);
	}
	ChannelEnabled[channel] = enable;
	lock.release();
}

bool Timidity::isChannelEnabled (VstInt32 channel)
{
	channel = channel & 0x0f;
	return ChannelEnabled[channel];
}

void Timidity::hardReset ()
{
	lock.acquire();
	clearSynth ();
	clearBuffer ();
	initSynth ();
	initBuffer ();
	lock.release();
}

VstInt32 Timidity::getActiveVoices ()
{
	return timid_get_active_voices(&synth);
}

VstInt32 Timidity::getMaxVoices ()
{
	return timid_get_max_voices(&synth);
}

void Timidity::getConfigName (char* text, VstInt32 size)
{
	if (!text)
	{
		return;
	}
	strncpy(text, ConfigName, size);
}

HostInfo* Timidity::getHostInfo ()
{
	return &hi;
}

double Timidity::getCPULoad ()
{
	return CPULoad;
}
