/*
Timidity VSTi
Copyright (C) 2021-2024  Datajake

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

#if REAPER_EXTENSIONS
bool Timidity::getParameterDisplayValue (VstInt32 index, char* text, float value)
{
	if (!text)
	{
		return false;
	}
	switch (index)
	{
	case kVolume:
		if (VolumeDisplay >= 0.5)
		{
			float2string (value*100, text, (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			dB2string (value, text, (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kVolumeDisplay:
		if (value >= 0.5)
		{
			vst_strncpy (text, "%", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "dB", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kImmediatePan:
		if (value >= 0.5)
		{
			vst_strncpy (text, "ON", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "OFF", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kMono:
		if (value >= 0.5)
		{
			vst_strncpy (text, "ON", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "OFF", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kTranspose:
		value = (value*24.0f)-12.0f;
		if (value >= 1 || value <= -1)
		{
			int2string ((VstInt32)value, text, (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "0", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kVoices:
		value = value*MAX_VOICES;
		int2string ((VstInt32)value, text, (kVstMaxParamStrLen*2)-1);
		break;
	case kFastDecay:
		if (value >= 0.5)
		{
			vst_strncpy (text, "ON", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "OFF", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kAntialiasing:
		if (value >= 0.5)
		{
			vst_strncpy (text, "ON", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "OFF", (kVstMaxParamStrLen*2)-1);
		}
		break;
	case kControlRate:
		value = value*sampleRate;
		int2string ((VstInt32)value, text, (kVstMaxParamStrLen*2)-1);
		break;
	case kPushMidi:
		if (value >= 0.5)
		{
			vst_strncpy (text, "ON", (kVstMaxParamStrLen*2)-1);
		}
		else
		{
			vst_strncpy (text, "OFF", (kVstMaxParamStrLen*2)-1);
		}
		break;
	}
	return true;
}

bool Timidity::string2parameterReplace (VstInt32 index, char* text)
{
	if (!text)
	{
		return false;
	}
	float value = (float)atof(text);
	switch (index)
	{
	case kVolume:
		if (VolumeDisplay >= 0.5)
		{
			value = value/100.0f;
		}
		else
		{
			value = (float)pow(10.0, value/20.0);
		}
		break;
	case kTranspose:
		value = (value+12.0f)/24.0f;
		break;
	case kVoices:
		value = value/MAX_VOICES;
		break;
	case kControlRate:
		value = value/sampleRate;
		break;
	}
	if (index == kVolume)
	{
		if (value > 10)
		{
			value = 10;
		}
		else if (value < 0)
		{
			value = 0;
		}
	}
	else
	{
		if (value > 1)
		{
			value = 1;
		}
		else if (value < 0)
		{
			value = 0;
		}
	}
	float2string (value, text, (kVstMaxParamStrLen*2)-1);
	return true;
}

bool Timidity::isEnumParameter (VstInt32 index)
{
	switch (index)
	{
	case kVolumeDisplay:
		return true;
	case kImmediatePan:
		return true;
	case kMono:
		return true;
	case kTranspose:
		return true;
	case kVoices:
		return true;
	case kFastDecay:
		return true;
	case kAntialiasing:
		return true;
	case kControlRate:
		return true;
	case kPushMidi:
		return true;
	}
	return false;
}

bool Timidity::automateParameter (VstInt32 index, float value, VstInt32 timestamp)
{
	VstParameterEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = kVstParameterType;
	ev.byteSize = sizeof(VstParameterEvent);
	ev.deltaFrames = timestamp;
	ev.index = index;
	ev.value = value;
	if (!ParameterQueue.EnqueueEvent((VstEvent*)&ev))
	{
		return false;
	}
	return true;
}

bool Timidity::parameterRange (VstInt32 index, double* range)
{
	if (!range)
	{
		return false;
	}
	switch (index)
	{
	case kVolume:
		range[0] = 0;
		range[1] = 10;
		break;
	}
	return true;
}

bool Timidity::renamePlug (char** text, const char* newName)
{
	if (!text || !newName)
	{
		return false;
	}
	*text = (char*)newName;
	return true;
}

void Timidity::adjustParameterIndex (VstInt32 index, VstInt32 adjust)
{
	VstInt32 idxadj[2] = { index, adjust };
	hostVendorSpecific (0xdeadbeef, audioMasterAutomate, idxadj, 0);
}
#endif
