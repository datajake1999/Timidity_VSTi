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

void Timidity::setSampleRate (float sampleRate)
{
	lock.acquire();
	AudioEffectX::setSampleRate (sampleRate);
	if (ControlRate > sampleRate)
	{
		ControlRate = sampleRate;
	}
	else if (ControlRate < sampleRate/MAX_CONTROL_RATIO)
	{
		ControlRate = sampleRate/MAX_CONTROL_RATIO;
	}
	timid_set_sample_rate(&synth, (VstInt32)sampleRate);
	lock.release();
}

void Timidity::setBlockSize (VstInt32 blockSize)
{
	lock.acquire();
	clearBuffer ();
	AudioEffectX::setBlockSize (blockSize);
	initBuffer ();
	lock.release();
}

void Timidity::setBlockSizeAndSampleRate (VstInt32 blockSize, float sampleRate)
{
	setBlockSize (blockSize);
	setSampleRate (sampleRate);
}

void Timidity::initSynth ()
{
	memset(&synth, 0, sizeof(synth));
	timid_init(&synth);
	timid_set_sample_rate(&synth, (VstInt32)sampleRate);
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
	loadInstruments (ConfigFile, ConfigName);
}

void Timidity::initBuffer ()
{
	buffer = new float[2*blockSize];
	if (buffer)
	{
		memset(buffer, 0, (2*blockSize)*sizeof(float));
	}
}

void Timidity::clearSynth ()
{
	timid_close(&synth);
}

void Timidity::clearBuffer ()
{
	if (buffer)
	{
		memset(buffer, 0, (2*blockSize)*sizeof(float));
		delete[] buffer;
		buffer = NULL;
	}
}

bool Timidity::getErrorText (char* text)
{
	if (!text)
	{
		return false;
	}
	if (!buffer)
	{
		sprintf(text, "Error initializing buffer.\n");
		return true;
	}
	return false;
}

void Timidity::suspend ()
{
	lock.acquire();
	MidiQueue.Flush(true);
#if REAPER_EXTENSIONS
	ParameterQueue.Flush(true);
#endif
	timid_panic(&synth);
	lock.release();
}

void Timidity::resume ()
{
	lock.acquire();
	AudioEffectX::resume ();
	timid_reset(&synth);
	lock.release();
}

float Timidity::getVu ()
{
	if (bypassed)
	{
		return 0;
	}
	return (float)fabs((vu[0] + vu[1]) / 2);
}

#if !VST_FORCE_DEPRECATED
void Timidity::process (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	processTemplate (inputs, outputs, sampleFrames);
}

#endif
void Timidity::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	processTemplate (inputs, outputs, sampleFrames);
}

#if VST_2_4_EXTENSIONS
void Timidity::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	processTemplate (inputs, outputs, sampleFrames);
}

#endif
template <class sampletype>
void Timidity::processTemplate (sampletype** inputs, sampletype** outputs, VstInt32 sampleFrames)
{
	sampletype* in1 = inputs[0];
	sampletype* in2 = inputs[1];
	sampletype* out1 = outputs[0];
	sampletype* out2 = outputs[1];
	double begin;
	double end;

	lock.acquire();
	if (bypassed || !buffer || !out1 || !out2)
	{
		begin = GetCPUTime();
		if (out1)
		{
			if (in1)
			{
				memcpy(out1, in1, sampleFrames*sizeof(sampletype));
			}
			else
			{
				memset(out1, 0, sampleFrames*sizeof(sampletype));
			}
		}
		if (out2)
		{
			if (in2)
			{
				memcpy(out2, in2, sampleFrames*sizeof(sampletype));
			}
			else
			{
				memset(out2, 0, sampleFrames*sizeof(sampletype));
			}
		}
		end = GetCPUTime();
		calculateCPULoad (begin, end, sampleFrames);
		lock.release();
		return;
	}

	if (sampleFrames > blockSize)
	{
		sampleFrames = blockSize;
	}

	begin = GetCPUTime();
	VstInt32 totalFrames = sampleFrames;
	VstInt32 renderedFrames = 0;
	float *bufferPointer = buffer;
	while (totalFrames > 0)
	{
		while (MidiQueue.HasEvents() && MidiQueue.GetEventTime() <= renderedFrames)
		{
			processEvent (MidiQueue.GetNextEvent());
		}
		VstInt32 currentFrames = MidiQueue.GetEventTime() -renderedFrames;
		if (currentFrames > totalFrames || currentFrames <= 0)
		{
			currentFrames = totalFrames;
		}
		timid_render_float(&synth, bufferPointer, currentFrames);
		if (Mono >= 0.5)
		{
			bufferPointer += currentFrames;
		}
		else
		{
			bufferPointer += 2*currentFrames;
		}
		renderedFrames += currentFrames;
		totalFrames -= currentFrames;
	}
	while (MidiQueue.HasEvents())
	{
		processEvent (MidiQueue.GetNextEvent());
	}
	for (VstInt32 i = 0; i < sampleFrames; i++)
	{
#if REAPER_EXTENSIONS
		while (ParameterQueue.HasEvents() && ParameterQueue.GetEventTime() <= i)
		{
			processEvent (ParameterQueue.GetNextEvent());
		}
#endif
		if (out1)
		{
			if (Mono >= 0.5)
			{
				out1[i] = buffer[i];
			}
			else
			{
				out1[i] = buffer[i*2+0];
			}
			out1[i] = out1[i] * Volume;
#ifdef DEMO
			if (time(NULL) >= startTime + 600)
			{
				out1[i] += ((rand() / (sampletype)RAND_MAX) / (sampletype)256);
			}
#endif
			if (in1)
			{
				out1[i] += in1[i];
			}
			vu[0] = out1[i];
		}
		if (out2)
		{
			if (Mono >= 0.5)
			{
				out2[i] = buffer[i];
			}
			else
			{
				out2[i] = buffer[i*2+1];
			}
			out2[i] = out2[i] * Volume;
#ifdef DEMO
			if (time(NULL) >= startTime + 600)
			{
				out2[i] += ((rand() / (sampletype)RAND_MAX) / (sampletype)256);
			}
#endif
			if (in2)
			{
				out2[i] += in2[i];
			}
			vu[1] = out2[i];
		}
	}
#if REAPER_EXTENSIONS
	while (ParameterQueue.HasEvents())
	{
		processEvent (ParameterQueue.GetNextEvent());
	}
#endif
	end = GetCPUTime();
	calculateCPULoad (begin, end, sampleFrames);
	lock.release();
}

void Timidity::calculateCPULoad (double begin, double end, VstInt32 numsamples)
{
	double GenerateDuration = (end - begin) / GetCPUFrequency();
	double BufferDuration = numsamples * (1.0 / sampleRate);
	CPULoad = (GenerateDuration / BufferDuration) * 100.0;
}

VstInt32 Timidity::processEvents (VstEvents* ev)
{
	lock.acquire();
	if (bypassed || !ev)
	{
		if (ev && hi.ReceiveEvents)
		{
			sendVstEventsToHost (ev);
		}
		lock.release();
		return 0;
	}

	if (PushMidi >= 0.5)
	{
		VstInt32 eventCount = ev->numEvents;
		if (eventCount > EVBUFSIZE)
		{
			eventCount = EVBUFSIZE;
		}
		for (VstInt32 i = 0; i < eventCount; i++)
		{
			if (ev->events[i]->type == kVstMidiType)
			{
				if (!MidiQueue.EnqueueEvent(ev->events[i]))
				{
					break;
				}
			}
#if VST_2_4_EXTENSIONS
			else if (ev->events[i]->type == kVstSysExType)
			{
				processEvent (ev->events[i]);
			}
#endif
		}
	}
	else
	{
		for (VstInt32 i = 0; i < ev->numEvents; i++)
		{
			processEvent (ev->events[i]);
		}
	}
	if (hi.ReceiveEvents)
	{
		sendVstEventsToHost (ev);
	}
	lock.release();
	return 1;
}

void Timidity::processEvent (VstEvent* ev)
{
	if (!ev)
	{
		return;
	}
	if (ev->type == kVstMidiType)
	{
		VstMidiEvent* event = (VstMidiEvent*)ev;
		char* midiData = event->midiData;
		sendMidi (midiData);
	}
#if VST_2_4_EXTENSIONS
	else if (ev->type == kVstSysExType)
	{
		VstMidiSysexEvent* event = (VstMidiSysexEvent*)ev;
		timid_write_sysex(&synth, (unsigned char*)event->sysexDump, event->dumpBytes);
	}
#endif
#if REAPER_EXTENSIONS
	else if (ev->type == kVstParameterType)
	{
		VstParameterEvent* event = (VstParameterEvent*)ev;
		setParameter (event->index, event->value);
	}
#endif
}

void Timidity::sendMidi (char* data)
{
	if (!data)
	{
		return;
	}
	unsigned char byte1 = data[0] & 0xff;
	unsigned char byte2 = data[1] & 0x7f;
	unsigned char byte3 = data[2] & 0x7f;
	unsigned char type = byte1 & 0xf0;
	unsigned char channel = byte1 & 0x0f;
	if (!ChannelEnabled[channel])
	{
		return;
	}
	if (Transpose >= 1 || Transpose <= -1)
	{
		if (type == 0x80 || type == 0x90)
		{
			if (channel != 9)
			{
				VstInt32 note = byte2 + (VstInt32)Transpose;
				if (note > 127)
				{
					note = 127;
				}
				else if (note < 0)
				{
					note = 0;
				}
				byte2 = (unsigned char)note;
			}
		}
	}
	timid_write_midi(&synth, byte1, byte2, byte3);
}

VstInt32 Timidity::startProcess ()
{
	lock.acquire();
	if (buffer)
	{
		memset(buffer, 0, (2*blockSize)*sizeof(float));
		lock.release();
		return 1;
	}
	lock.release();
	return 0;
}

VstInt32 Timidity::stopProcess ()
{
	lock.acquire();
	if (buffer)
	{
		memset(buffer, 0, (2*blockSize)*sizeof(float));
		lock.release();
		return 1;
	}
	lock.release();
	return 0;
}
