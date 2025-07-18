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
#include "gmnames.h"

VstInt32 Timidity::getMidiProgramName (VstInt32 channel, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || !mpn)
	return 0;
	VstInt32 prg = mpn->thisProgramIndex;
	if (prg < 0 || prg >= 128)
	return 0;
	fillProgram (channel, prg, mpn);
	if (channel == 9)
	return 1;
	return 128L;
}

VstInt32 Timidity::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || !mpn)
	return -1;
	VstInt32 prg = timid_channel_get_program(&synth, channel);
	channelPrograms[channel] = prg;
	mpn->thisProgramIndex = prg;
	fillProgram (channel, prg, mpn);
	return prg;
}

void Timidity::fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || prg < 0 || prg >= 128 || !mpn)
	return;
	mpn->midiBankMsb =
	mpn->midiBankLsb = -1;
	mpn->reserved = 0;
	mpn->flags = 0;

	if (channel == 9)	// drums
	{
		vst_strncpy (mpn->name, "Standard", kVstMaxNameLen-1);
		mpn->midiProgram = 0;
		mpn->parentCategoryIndex = 0;
	}
	else
	{
		vst_strncpy (mpn->name, GmNames[prg], kVstMaxNameLen-1);
		mpn->midiProgram = (char)prg;
		mpn->parentCategoryIndex = -1;	// for now

		for (VstInt32 i = 0; i < kNumGmCategories; i++)
		{
			if (prg >= GmCategoriesFirstIndices[i] && prg < GmCategoriesFirstIndices[i + 1])
			{
				mpn->parentCategoryIndex = i;
				break;
			}
		}
	}
}

VstInt32 Timidity::getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* cat)
{
	if (channel < 0 || channel >= 16 || !cat)
	return 0;
	cat->parentCategoryIndex = -1;	// -1:no parent category
	cat->flags = 0;					// reserved, none defined yet, zero.
	VstInt32 category = cat->thisCategoryIndex;
	if (channel == 9)
	{
		vst_strncpy (cat->name, "Drums", kVstMaxNameLen-1);
		return 1;
	}
	if (category >= 0 && category < kNumGmCategories)
	vst_strncpy (cat->name, GmCategories[category], kVstMaxNameLen-1);
	else
	cat->name[0] = 0;
	return kNumGmCategories;
}

bool Timidity::hasMidiProgramsChanged (VstInt32 channel)
{
	if (channel < 0 || channel >= 16)
	return false;
	if (timid_channel_get_program(&synth, channel) != channelPrograms[channel])
	{
		return true;
	}
	return false;
}

bool Timidity::getMidiKeyName (VstInt32 channel, MidiKeyName* key)
// struct will be filled with information for 'thisProgramIndex' and 'thisKeyNumber'
// if keyName is "" the standard name of the key will be displayed.
// if false is returned, no MidiKeyNames defined for 'thisProgramIndex'.
{
	if (channel < 0 || channel >= 16 || !key)
	return false;
	// key->thisProgramIndex;		// >= 0. fill struct for this program index.
	// key->thisKeyNumber;			// 0 - 127. fill struct for this key number.
	key->keyName[0] = 0;
	key->reserved = 0;				// zero
	key->flags = 0;					// reserved, none defined yet, zero.
	return false;
}
