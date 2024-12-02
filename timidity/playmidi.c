/*

TiMidity -- Experimental MIDI to WAVE converter
Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

playmidi.c -- random stuff in need of rearrangement

*/

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32_WCE
#include <string.h>
#endif

#include "timid.h"

static void adjust_amplification(Timid *tm, int value)
{
    tm->master_volume = (double)(value) / 100.0L;
}

static void reset_voices(Timid *tm)
{
    int i;
    for (i=0; i<MAX_VOICES; i++)
		tm->voice[i].status=VOICE_FREE;
}

/* Process the Reset All Controllers event */
static void reset_controllers(Timid *tm, int c)
{
    tm->channel[c].volume=90; /* Some standard says, although the SCC docs say 0. */
    tm->channel[c].expression=127; /* SCC-1 does this. */
    tm->channel[c].sustain=0;
    tm->channel[c].mono=0;
    tm->channel[c].pitchbend=0x2000;
    tm->channel[c].pitchfactor=0; /* to be computed */
}

static void reset_midi(Timid *tm)
{
    int i;
    for (i=0; i<16; i++)
    {
        reset_controllers(tm, i);
        /* The rest of these are unaffected by the Reset All Controllers event */
        tm->channel[i].program=tm->default_program;
        tm->channel[i].panning=NO_PANNING;
        tm->channel[i].pitchsens=2;
        tm->channel[i].bank=0; /* tone bank or drum set */
        tm->rpn_msb[i]=0xff;
        tm->rpn_lsb[i]=0xff;
    }
    reset_voices(tm);
    tm->lost_notes = 0;
    tm->cut_notes = 0;
}

static void select_sample(Timid *tm, int v, Instrument *ip)
{
    int32 f, cdiff, diff;
    int s,i;
    Sample *sp, *closest;
    
    s=ip->samples;
    sp=ip->sample;
    
    if (s==1)
    {
        tm->voice[v].sample=sp;
        return;
    }
    
    f=tm->voice[v].orig_frequency;
    for (i=0; i<s; i++)
    {
        if (sp->low_freq <= f && sp->high_freq >= f)
        {
            tm->voice[v].sample=sp;
            return;
        }
        sp++;
    }
    
    /*
    No suitable sample found! We'll select the sample whose root
    frequency is closest to the one we want. (Actually we should
    probably convert the low, high, and root frequencies to MIDI note
    values and compare those.) */
    
    cdiff=0x7FFFFFFF;
    closest=sp=ip->sample;
    for(i=0; i<s; i++)
    {
        diff=sp->root_freq - f;
        if (diff<0) diff=-diff;
        if (diff<cdiff)
        {
            cdiff=diff;
            closest=sp;
        }
        sp++;
    }
    tm->voice[v].sample=closest;
    return;
}

static void recompute_freq(Timid *tm, int v)
{
    int
    sign=(tm->voice[v].sample_increment < 0), /* for bidirectional loops */
    pb=tm->channel[tm->voice[v].channel].pitchbend;
    double a;
    
    if (!tm->voice[v].sample->sample_rate)
    return;
    
    if (tm->voice[v].vibrato_control_ratio)
    {
        /* This instrument has vibrato. Invalidate any precomputed
        sample_increments. */
        
        int i=VIBRATO_SAMPLE_INCREMENTS;
        while (i--)
        tm->voice[v].vibrato_sample_increment[i]=0;
    }
    
    if (pb==0x2000 || pb<0 || pb>0x3FFF)
    tm->voice[v].frequency=tm->voice[v].orig_frequency;
    else
    {
        pb-=0x2000;
        if (!(tm->channel[tm->voice[v].channel].pitchfactor))
        {
            /* Damn. Somebody bent the pitch. */
            int32 i=pb*tm->channel[tm->voice[v].channel].pitchsens;
            if (pb<0)
            i=-i;
            tm->channel[tm->voice[v].channel].pitchfactor=
            bend_fine[(i>>5) & 0xFF] * bend_coarse[i>>13];
        }
        if (pb>0)
        tm->voice[v].frequency=
        (int32)(tm->channel[tm->voice[v].channel].pitchfactor *
        (double)(tm->voice[v].orig_frequency));
        else
        tm->voice[v].frequency=
        (int32)((double)(tm->voice[v].orig_frequency) /
        tm->channel[tm->voice[v].channel].pitchfactor);
    }
    
    a = FSCALE(((double)(tm->voice[v].sample->sample_rate) *
    (double)(tm->voice[v].frequency)) /
    ((double)(tm->voice[v].sample->root_freq) *
    (double)(tm->play_mode.rate)),
    FRACTION_BITS);
    
    if (sign)
    a = -a; /* need to preserve the loop direction */
    
    tm->voice[v].sample_increment = (int32)(a);
}

static void recompute_amp(Timid *tm, int v)
{
    int32 tempamp;
    
    /* TODO: use fscale */
    
    tempamp= (tm->voice[v].velocity *
    tm->channel[tm->voice[v].channel].volume *
    tm->channel[tm->voice[v].channel].expression); /* 21 bits */
    
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        if (tm->voice[v].panning > 60 && tm->voice[v].panning < 68)
        {
            tm->voice[v].panned=PANNED_CENTER;
            
            tm->voice[v].left_amp=
            FSCALENEG((double)(tempamp) * tm->voice[v].sample->volume * tm->master_volume,
            21);
        }
        else if (tm->voice[v].panning<5)
        {
            tm->voice[v].panned = PANNED_LEFT;
            
            tm->voice[v].left_amp=
            FSCALENEG((double)(tempamp) * tm->voice[v].sample->volume * tm->master_volume,
            20);
        }
        else if (tm->voice[v].panning>123)
        {
            tm->voice[v].panned = PANNED_RIGHT;
            
            tm->voice[v].left_amp= /* left_amp will be used */
            FSCALENEG((double)(tempamp) * tm->voice[v].sample->volume * tm->master_volume,
            20);
        }
        else
        {
            tm->voice[v].panned = PANNED_MYSTERY;
            
            tm->voice[v].left_amp=
            FSCALENEG((double)(tempamp) * tm->voice[v].sample->volume * tm->master_volume,
            27);
            tm->voice[v].right_amp=tm->voice[v].left_amp * (tm->voice[v].panning);
            tm->voice[v].left_amp *= (double)(127-tm->voice[v].panning);
        }
    }
    else
    {
        tm->voice[v].panned=PANNED_CENTER;
        
        tm->voice[v].left_amp=
        FSCALENEG((double)(tempamp) * tm->voice[v].sample->volume * tm->master_volume,
        21);
    }
}

static void start_note(Timid *tm, MidiEvent *e, int i)
{
    Instrument *ip;
    int j;
    
    if (ISDRUMCHANNEL(tm, e->channel))
    {
        if (!tm->drumset[tm->channel[e->channel].bank] && !tm->drumset[0])
        return; /* No drumset? Then we can't play. */
        if (!(ip=tm->drumset[tm->channel[e->channel].bank]->tone[e->a].instrument))
        {
            if (!(ip=tm->drumset[0]->tone[e->a].instrument))
            return; /* No instrument? Then we can't play. */
        }
        
        if (ip->sample->note_to_use) /* Do we have a fixed pitch? */
        tm->voice[i].orig_frequency=freq_table[(int)(ip->sample->note_to_use)];
        else
        tm->voice[i].orig_frequency=freq_table[e->a & 0x7F];
        
        /* drums are supposed to have only one sample */
        tm->voice[i].sample=ip->sample;
    }
    else
    {
        if (!tm->tonebank[tm->channel[e->channel].bank] && !tm->tonebank[0])
        return; /* No tonebank? Then we can't play. */
        if (tm->channel[e->channel].program==SPECIAL_PROGRAM)
        ip=tm->default_instrument;
        else if (!(ip=tm->tonebank[tm->channel[e->channel].bank]->
        tone[tm->channel[e->channel].program].instrument))
        {
            if (!(ip=tm->tonebank[0]->tone[tm->channel[e->channel].program].instrument))
            return; /* No instrument? Then we can't play. */
        }
        
        if (ip->sample->note_to_use) /* Fixed-pitch instrument? */
        tm->voice[i].orig_frequency=freq_table[(int)(ip->sample->note_to_use)];
        else
        tm->voice[i].orig_frequency=freq_table[e->a & 0x7F];
        select_sample(tm, i, ip);
    }
    
    tm->voice[i].status=VOICE_ON;
    tm->voice[i].channel=e->channel;
    tm->voice[i].note=e->a;
    tm->voice[i].velocity=e->b;
    tm->voice[i].sample_offset=0;
    tm->voice[i].sample_increment=0; /* make sure it isn't negative */
    
    tm->voice[i].tremolo_phase=0;
    tm->voice[i].tremolo_phase_increment=tm->voice[i].sample->tremolo_phase_increment;
    tm->voice[i].tremolo_sweep=tm->voice[i].sample->tremolo_sweep_increment;
    tm->voice[i].tremolo_sweep_position=0;
    
    tm->voice[i].vibrato_sweep=tm->voice[i].sample->vibrato_sweep_increment;
    tm->voice[i].vibrato_sweep_position=0;
    tm->voice[i].vibrato_control_ratio=tm->voice[i].sample->vibrato_control_ratio;
    tm->voice[i].vibrato_control_counter=tm->voice[i].vibrato_phase=0;
    for (j=0; j<VIBRATO_SAMPLE_INCREMENTS; j++)
    tm->voice[i].vibrato_sample_increment[j]=0;
    
    if (tm->channel[e->channel].panning != NO_PANNING)
    tm->voice[i].panning=tm->channel[e->channel].panning;
    else
    tm->voice[i].panning=tm->voice[i].sample->panning;
    
    recompute_freq(tm, i);
    recompute_amp(tm, i);
    if (tm->voice[i].sample->modes & MODES_ENVELOPE)
    {
        /* Ramp up from 0 */
        tm->voice[i].envelope_stage=0;
        tm->voice[i].envelope_volume=0;
        tm->voice[i].control_counter=0;
        recompute_envelope(tm, i);
        apply_envelope_to_amp(tm, i);
    }
    else
    {
        tm->voice[i].envelope_increment=0;
        apply_envelope_to_amp(tm, i);
    }
}

static void kill_note(Timid *tm, int i)
{
    tm->voice[i].status=VOICE_DIE;
}

/* Only one instance of a note can be playing on a single channel. */
static void note_on(Timid *tm, MidiEvent *e)
{
    int i=tm->voices, lowest=-1;
    int32 lv=0x7FFFFFFF, v;
    
    while (i--)
    {
        if (tm->voice[i].status == VOICE_FREE)
        lowest=i; /* Can't get a lower volume than silence */
        else if (tm->voice[i].channel==e->channel &&
        (tm->voice[i].note==e->a || tm->channel[tm->voice[i].channel].mono))
        kill_note(tm, i);
    }
    
    if (lowest != -1)
    {
        /* Found a free voice. */
        start_note(tm,e,lowest);
        return;
    }
    
    /* Look for the decaying note with the lowest volume */
    i=tm->voices;
    while (i--)
    {
        if ((tm->voice[i].status!=VOICE_ON) &&
        (tm->voice[i].status!=VOICE_DIE))
        {
            v=tm->voice[i].left_mix;
            if ((tm->voice[i].panned==PANNED_MYSTERY) && (tm->voice[i].right_mix>v))
            v=tm->voice[i].right_mix;
            if (v<lv)
            {
                lv=v;
                lowest=i;
            }
        }
    }
    
    if (lowest != -1)
    {
        /* This can still cause a click, but if we had a free voice to
        spare for ramping down this note, we wouldn't need to kill it
        in the first place... Still, this needs to be fixed. Perhaps
        we could use a reserve of voices to play dying notes only. */
        
        tm->cut_notes++;
        tm->voice[lowest].status=VOICE_FREE;
        start_note(tm,e,lowest);
    }
    else
    tm->lost_notes++;
}

static void finish_note(Timid *tm, int i)
{
    if (tm->voice[i].sample->modes & MODES_ENVELOPE)
    {
        /* We need to get the envelope out of Sustain stage */
        tm->voice[i].envelope_stage=3;
        tm->voice[i].status=VOICE_OFF;
        recompute_envelope(tm, i);
        apply_envelope_to_amp(tm, i);
    }
    else
    {
        /* Set status to OFF so resample_voice() will let this voice out
        of its loop, if any. In any case, this voice dies when it
        hits the end of its data (ofs>=data_length). */
        tm->voice[i].status=VOICE_OFF;
    }
}

static void note_off(Timid *tm, MidiEvent *e)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].status==VOICE_ON &&
    tm->voice[i].channel==e->channel &&
    tm->voice[i].note==e->a)
    {
        if (tm->channel[e->channel].sustain)
        {
            tm->voice[i].status=VOICE_SUSTAINED;
        }
        else
        finish_note(tm, i);
        return;
    }
}

/* Process the All Notes Off event */
static void all_notes_off(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].status==VOICE_ON &&
    tm->voice[i].channel==c)
    {
        if (tm->channel[c].sustain)
        {
            tm->voice[i].status=VOICE_SUSTAINED;
        }
        else
        finish_note(tm, i);
    }
}

/* Process the All Sounds Off event */
static void all_sounds_off(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].channel==c &&
    tm->voice[i].status != VOICE_FREE &&
    tm->voice[i].status != VOICE_DIE)
    {
        kill_note(tm, i);
    }
}

static void adjust_pressure(Timid *tm, MidiEvent *e)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].status==VOICE_ON &&
    tm->voice[i].channel==e->channel &&
    tm->voice[i].note==e->a)
    {
        tm->voice[i].velocity=e->b;
        recompute_amp(tm, i);
        apply_envelope_to_amp(tm, i);
        return;
    }
}

static void adjust_panning(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if ((tm->voice[i].channel==c) &&
    (tm->voice[i].status==VOICE_ON || tm->voice[i].status==VOICE_SUSTAINED))
    {
        tm->voice[i].panning=tm->channel[c].panning;
        recompute_amp(tm, i);
        apply_envelope_to_amp(tm, i);
    }
}

static void drop_sustain(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].status==VOICE_SUSTAINED && tm->voice[i].channel==c)
    finish_note(tm, i);
}

static void adjust_pitchbend(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].status!=VOICE_FREE && tm->voice[i].channel==c)
    {
        recompute_freq(tm, i);
    }
}

static void adjust_volume(Timid *tm, int c)
{
    int i=tm->voices;
    while (i--)
    if (tm->voice[i].channel==c &&
    (tm->voice[i].status==VOICE_ON || tm->voice[i].status==VOICE_SUSTAINED))
    {
        recompute_amp(tm, i);
        apply_envelope_to_amp(tm, i);
    }
}

static void play_midi(Timid *tm, MidiEvent *e)
{
    if (e)
    {
        switch(e->type)
        {
            
            /* Effects affecting a single note */
            
        case ME_NOTEON:
            if (!(e->b)) /* Velocity 0? */
            note_off(tm, e);
            else
            note_on(tm, e);
            break;
            
        case ME_NOTEOFF:
            note_off(tm, e);
            break;
            
        case ME_KEYPRESSURE:
            adjust_pressure(tm, e);
            break;
            
            /* Effects affecting a single channel */
            
        case ME_PITCH_SENS:
            tm->channel[e->channel].pitchsens=
            e->a;
            tm->channel[e->channel].pitchfactor=0;
            break;
            
        case ME_PITCHWHEEL:
            tm->channel[e->channel].pitchbend=
            e->a + e->b * 128;
            tm->channel[e->channel].pitchfactor=0;
            /* Adjust pitch for notes already playing */
            adjust_pitchbend(tm, e->channel);
            break;
            
        case ME_MAINVOLUME:
            tm->channel[e->channel].volume=e->a;
            adjust_volume(tm, e->channel);
            break;
            
        case ME_PAN:
            tm->channel[e->channel].panning=e->a;
            if (tm->adjust_panning_immediately)
            adjust_panning(tm, e->channel);
            break;
            
        case ME_EXPRESSION:
            tm->channel[e->channel].expression=e->a;
            adjust_volume(tm, e->channel);
            break;
            
        case ME_PROGRAM:
            if (ISDRUMCHANNEL(tm, e->channel))
            {
                /* Change drum set */
                if (tm->drumset[e->a])
                {
                    tm->channel[e->channel].bank=e->a;
                }
            }
            else
            {
                tm->channel[e->channel].program=e->a;
            }
            break;
            
        case ME_SUSTAIN:
            tm->channel[e->channel].sustain=e->a;
            if (!e->a)
            drop_sustain(tm, e->channel);
            break;
            
        case ME_RESET_CONTROLLERS:
            reset_controllers(tm, e->channel);
            break;
            
        case ME_ALL_NOTES_OFF:
            all_notes_off(tm, e->channel);
            break;
            
        case ME_ALL_SOUNDS_OFF:
            all_sounds_off(tm, e->channel);
            break;
            
        case ME_MONO:
            tm->channel[e->channel].mono=1;
            all_notes_off(tm, e->channel);
            break;
            
        case ME_POLY:
            tm->channel[e->channel].mono=0;
            all_notes_off(tm, e->channel);
            break;
            
        case ME_TONE_BANK:
            if (!ISDRUMCHANNEL(tm, e->channel))
            {
                if (tm->tonebank[e->a])
                {
                    tm->channel[e->channel].bank=e->a;
                }
            }
            break;
        }
    }
}

static void do_compute_data(Timid *tm, int32 *buffer, int32 count)
{
    int i;
    memset(buffer, 0,
    (tm->play_mode.encoding & PE_MONO) ? (count * 4) : (count * 8));
    for (i=0; i<tm->voices; i++)
    {
        if(tm->voice[i].status != VOICE_FREE)
            mix_voice(tm, buffer, i, count);
    }
}

void timid_init(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    tm->default_program=DEFAULT_PROGRAM;
    tm->antialiasing_allowed=1;
#ifdef FAST_DECAY
    tm->fast_decay=1;
#else
    tm->fast_decay=0;
#endif
    tm->voices=DEFAULT_VOICES;
    tm->play_mode.rate=DEFAULT_RATE;
    tm->control_rate=CONTROLS_PER_SECOND;
    tm->control_ratio = tm->play_mode.rate/tm->control_rate;
    tm->drumchannels=DEFAULT_DRUMCHANNELS;
    tm->adjust_panning_immediately=1;
    init_tables(tm);
    reset_midi(tm);
    adjust_amplification(tm, DEFAULT_AMPLIFICATION);
}

int timid_load_config(Timid *tm, char *filename)
{
    char directory[256];
    char *separator;
    if (!tm || !filename)
    {
        return 0;
    }
    timid_unload_config(tm);
    strcpy(directory, filename);
    separator = strrchr(directory, PATH_SEP);
    if (separator) *separator = '\0';
    add_to_pathlist(tm, directory);
    if (read_config_file(tm, filename) == 0)
    {
        if (load_instruments(tm) == 0)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }
    return 0;
}

void timid_unload_config(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    free_instruments(tm);
    free_pathlist(tm);
    memset(tm->last_config, 0, sizeof(tm->last_config));
}

void timid_reload_config(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    if (strlen(tm->last_config))
    {
        char temp[1024];
        strncpy(temp, tm->last_config, 1023);
        temp[1023]='\0';
        timid_load_config(tm, temp);
    }
}

void timid_write_midi(Timid *tm, uint8 byte1, uint8 byte2, uint8 byte3)
{
    uint8 type = byte1 & 0xf0;
    uint8 channel = byte1 & 0x0f;
    MidiEvent ev;
    if (!tm)
    {
        return;
    }
    memset(&ev, 0, sizeof(ev));
    ev.channel = channel;
    switch(type)
    {
    case 0x80:
        ev.type = ME_NOTEOFF;
        ev.a = byte2;
        ev.b = byte3;
        play_midi(tm, &ev);
        break;
    case 0x90:
        ev.type = ME_NOTEON;
        ev.a = byte2;
        ev.b = byte3;
        play_midi(tm, &ev);
        break;
    case 0xa0:
        ev.type = ME_KEYPRESSURE;
        ev.a = byte2;
        ev.b = byte3;
        play_midi(tm, &ev);
        break;
    case 0xc0:
        ev.type = ME_PROGRAM;
        ev.a = byte2;
        ev.b = byte3;
        play_midi(tm, &ev);
        break;
    case 0xe0:
        ev.type = ME_PITCHWHEEL;
        ev.a = byte2;
        ev.b = byte3;
        play_midi(tm, &ev);
        break;
    case 0xb0:
        switch(byte2)
        {
        case 0x00:
            ev.type = ME_TONE_BANK;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x06:
            switch((tm->rpn_msb[channel]<<8) | tm->rpn_lsb[channel])
            {
            case 0x0000:
                ev.type = ME_PITCH_SENS;
                ev.a = byte3;
                play_midi(tm, &ev);
                break;
            case 0x7f7f:
                ev.type = ME_PITCH_SENS;
                ev.a = 2;
                play_midi(tm, &ev);
                break;
            }
            tm->rpn_msb[channel]=0xff;
            tm->rpn_lsb[channel]=0xff;
            break;
        case 0x07:
            ev.type = ME_MAINVOLUME;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x0a:
            ev.type = ME_PAN;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x0b:
            ev.type = ME_EXPRESSION;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x40:
            ev.type = ME_SUSTAIN;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x64:
            tm->rpn_msb[channel] = byte3;
            break;
        case 0x65:
            tm->rpn_lsb[channel] = byte3;
            break;
        case 0x78:
            ev.type = ME_ALL_SOUNDS_OFF;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x79:
            ev.type = ME_RESET_CONTROLLERS;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x7b:
            ev.type = ME_ALL_NOTES_OFF;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x7e:
            ev.type = ME_MONO;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        case 0x7f:
            ev.type = ME_POLY;
            ev.a = byte3;
            play_midi(tm, &ev);
            break;
        }
        break;
    }
}

void timid_write_midi_packed(Timid *tm, uint32 data)
{
    uint8 byte1 = data & 0xff;
    uint8 byte2 = (data >> 8) & 0x7f;
    uint8 byte3 = (data >> 16) & 0x7f;
    if (!tm)
    {
        return;
    }
    timid_write_midi(tm, byte1, byte2, byte3);
}

void timid_write_sysex(Timid *tm, uint8 *buffer, int32 count)
{
    const uint8 gm_reset_array[6] = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
    const uint8 gm2_reset_array[6] = {0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7};
    const uint8 gs_reset_array[11] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
    const uint8 xg_reset_array[9] = {0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7};
    if (!tm || !buffer)
    {
        return;
    }
    if (buffer[0] != 0xF0 || buffer[count-1] != 0xF7)
    {
        return;
    }
    if (count == 6 && memcmp(&gm_reset_array[0], buffer, 6) == 0)
    {
        reset_midi(tm);
    }
    else if (count == 6 && memcmp(&gm2_reset_array[0], buffer, 6) == 0)
    {
        reset_midi(tm);
    }
    else if (count == 11 && memcmp(&gs_reset_array[0], buffer, 11) == 0)
    {
        reset_midi(tm);
    }
    else if (count == 9 && memcmp(&xg_reset_array[0], buffer, 9) == 0)
    {
        reset_midi(tm);
    }
}

void timid_render_char(Timid *tm, uint8 *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            temp[0] = temp[0] >> (32 - 8 - GUARD_BITS);
            if (temp[0] > 127)
            {
                temp[0] = 127;
            }
            else if (temp[0] < -128)
            {
                temp[0] = -128;
            }
            buffer[i*2+0] = (uint8)temp[0] + 128;
            temp[1] = temp[1] >> (32 - 8 - GUARD_BITS);
            if (temp[1] > 127)
            {
                temp[1] = 127;
            }
            else if (temp[1] < -128)
            {
                temp[1] = -128;
            }
            buffer[i*2+1] = (uint8)temp[1] + 128;
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            temp = temp >> (32 - 8 - GUARD_BITS);
            if (temp > 127)
            {
                temp = 127;
            }
            else if (temp < -128)
            {
                temp = -128;
            }
            buffer[i] = (uint8)temp + 128;
        }
    }
}

void timid_render_short(Timid *tm, int16 *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            temp[0] = temp[0] >> (32 - 16 - GUARD_BITS);
            if (temp[0] > 32767)
            {
                temp[0] = 32767;
            }
            else if (temp[0] < -32768)
            {
                temp[0] = -32768;
            }
            buffer[i*2+0] = (int16)temp[0];
            temp[1] = temp[1] >> (32 - 16 - GUARD_BITS);
            if (temp[1] > 32767)
            {
                temp[1] = 32767;
            }
            else if (temp[1] < -32768)
            {
                temp[1] = -32768;
            }
            buffer[i*2+1] = (int16)temp[1];
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            temp = temp >> (32 - 16 - GUARD_BITS);
            if (temp > 32767)
            {
                temp = 32767;
            }
            else if (temp < -32768)
            {
                temp = -32768;
            }
            buffer[i] = (int16)temp;
        }
    }
}

void timid_render_24(Timid *tm, int24 *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            temp[0] = temp[0] >> (32 - 24 - GUARD_BITS);
            if (temp[0] > 8388607)
            {
                temp[0] = 8388607;
            }
            else if (temp[0] < -8388608)
            {
                temp[0] = -8388608;
            }
            buffer[i*2+0].data[0] = temp[0] & 0xff;
            buffer[i*2+0].data[1] = (temp[0] >> 8) & 0xff;
            buffer[i*2+0].data[2] = (temp[0] >> 16) & 0xff;
            temp[1] = temp[1] >> (32 - 24 - GUARD_BITS);
            if (temp[1] > 8388607)
            {
                temp[1] = 8388607;
            }
            else if (temp[1] < -8388608)
            {
                temp[1] = -8388608;
            }
            buffer[i*2+1].data[0] = temp[1] & 0xff;
            buffer[i*2+1].data[1] = (temp[1] >> 8) & 0xff;
            buffer[i*2+1].data[2] = (temp[1] >> 16) & 0xff;
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            temp = temp >> (32 - 24 - GUARD_BITS);
            if (temp > 8388607)
            {
                temp = 8388607;
            }
            else if (temp < -8388608)
            {
                temp = -8388608;
            }
            buffer[i].data[0] = temp & 0xff;
            buffer[i].data[1] = (temp >> 8) & 0xff;
            buffer[i].data[2] = (temp >> 16) & 0xff;
        }
    }
}

void timid_render_long(Timid *tm, int32 *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            if (temp[0] > 268435455)
            {
                temp[0] = 268435455;
            }
            else if (temp[0] < -268435456)
            {
                temp[0] = -268435456;
            }
            temp[0] = temp[0] << GUARD_BITS;
            buffer[i*2+0] = temp[0];
            if (temp[1] > 268435455)
            {
                temp[1] = 268435455;
            }
            else if (temp[1] < -268435456)
            {
                temp[1] = -268435456;
            }
            temp[1] = temp[1] << GUARD_BITS;
            buffer[i*2+1] = temp[1];
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            if (temp > 268435455)
            {
                temp = 268435455;
            }
            else if (temp < -268435456)
            {
                temp = -268435456;
            }
            temp = temp << GUARD_BITS;
            buffer[i] = temp;
        }
    }
}

void timid_render_float(Timid *tm, float *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            buffer[i*2+0] = temp[0] / (float)268435456;
            buffer[i*2+1] = temp[1] / (float)268435456;
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            buffer[i] = temp / (float)268435456;
        }
    }
}

void timid_render_double(Timid *tm, double *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            buffer[i*2+0] = temp[0] / (double)268435456;
            buffer[i*2+1] = temp[1] / (double)268435456;
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            buffer[i] = temp / (double)268435456;
        }
    }
}

void timid_render_ulaw(Timid *tm, uint8 *buffer, int32 count)
{
    int i;
    if (!tm || !buffer)
    {
        return;
    }
    if (!(tm->play_mode.encoding & PE_MONO))
    {
        for (i=0; i<count; i++)
        {
            int32 temp[2];
            do_compute_data(tm, &temp[0], 1);
            temp[0] = temp[0] >> (32 - 13 - GUARD_BITS);
            if (temp[0] > 4095)
            {
                temp[0] = 4095;
            }
            else if (temp[0] < -4096)
            {
                temp[0] = -4096;
            }
            buffer[i*2+0] = _l2u[temp[0]];
            temp[1] = temp[1] >> (32 - 13 - GUARD_BITS);
            if (temp[1] > 4095)
            {
                temp[1] = 4095;
            }
            else if (temp[1] < -4096)
            {
                temp[1] = -4096;
            }
            buffer[i*2+1] = _l2u[temp[1]];
        }
    }
    else
    {
        for (i=0; i<count; i++)
        {
            int32 temp;
            do_compute_data(tm, &temp, 1);
            temp = temp >> (32 - 13 - GUARD_BITS);
            if (temp > 4095)
            {
                temp = 4095;
            }
            else if (temp < -4096)
            {
                temp = -4096;
            }
            buffer[i] = _l2u[temp];
        }
    }
}

void timid_panic(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
}

void timid_reset(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    reset_midi(tm);
}

void timid_set_amplification(Timid *tm, int amplification)
{
    if (!tm)
    {
        return;
    }
    if (amplification > MAX_AMPLIFICATION)
    {
        amplification=MAX_AMPLIFICATION;
    }
    else if (amplification < 0)
    {
        amplification=0;
    }
    reset_voices(tm);
    adjust_amplification(tm, amplification);
}

void timid_set_max_voices(Timid *tm, int voices)
{
    if (!tm)
    {
        return;
    }
    if (voices > MAX_VOICES)
    {
        voices=MAX_VOICES;
    }
    else if (voices < 1)
    {
        voices=1;
    }
    reset_voices(tm);
    tm->voices = voices;
}

void timid_set_immediate_panning(Timid *tm, int value)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    tm->adjust_panning_immediately = value;
}

void timid_set_mono(Timid *tm, int value)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    tm->play_mode.encoding = value;
}

void timid_set_fast_decay(Timid *tm, int value)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    tm->fast_decay = value;
    timid_reload_config(tm);
}

void timid_set_antialiasing(Timid *tm, int value)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    tm->antialiasing_allowed = value;
    timid_reload_config(tm);
}

void timid_set_sample_rate(Timid *tm, int rate)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    if (rate > MAX_OUTPUT_RATE)
    {
        rate=MAX_OUTPUT_RATE;
    }
    else if (rate < MIN_OUTPUT_RATE)
    {
        rate=MIN_OUTPUT_RATE;
    }
    tm->play_mode.rate = rate;
    if (tm->control_rate > tm->play_mode.rate)
    {
        tm->control_rate=tm->play_mode.rate;
    }
    else if (tm->control_rate < tm->play_mode.rate / MAX_CONTROL_RATIO)
    {
        tm->control_rate=tm->play_mode.rate / MAX_CONTROL_RATIO;
    }
    tm->control_ratio = tm->play_mode.rate/tm->control_rate;
    if (tm->control_ratio > MAX_CONTROL_RATIO)
    {
        tm->control_ratio=MAX_CONTROL_RATIO;
    }
    else if (tm->control_ratio < 1)
    {
        tm->control_ratio=1;
    }
    timid_reload_config(tm);
}

void timid_set_control_rate(Timid *tm, int rate)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    tm->control_rate = rate;
    if (tm->control_rate > tm->play_mode.rate)
    {
        tm->control_rate=tm->play_mode.rate;
    }
    else if (tm->control_rate < tm->play_mode.rate / MAX_CONTROL_RATIO)
    {
        tm->control_rate=tm->play_mode.rate / MAX_CONTROL_RATIO;
    }
    tm->control_ratio = tm->play_mode.rate/tm->control_rate;
    if (tm->control_ratio > MAX_CONTROL_RATIO)
    {
        tm->control_ratio=MAX_CONTROL_RATIO;
    }
    else if (tm->control_ratio < 1)
    {
        tm->control_ratio=1;
    }
    timid_reload_config(tm);
}

void timid_set_default_program(Timid *tm, int program)
{
    if (!tm)
    {
        return;
    }
    tm->default_program = program & 0x7f;
}

int timid_set_default_instrument(Timid *tm, char *filename)
{
    if (!tm || !filename)
    {
        return 0;
    }
    if (set_default_instrument(tm, filename) == 0)
    {
        return 1;
    }
    return 0;
}

void timid_free_default_instrument(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    reset_voices(tm);
    free_default_instrument(tm);
}

void timid_get_config_name(Timid *tm, char *buffer, int32 count)
{
    if (!tm || !buffer)
    {
        return;
    }
    if (strlen(tm->last_config))
    {
        strncpy(buffer, tm->last_config, count);
    }
}

int timid_get_amplification(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return (int)(tm->master_volume * 100.0L);
}

int timid_get_active_voices(Timid *tm)
{
    int count = 0;
    int i;
    if (!tm)
    {
        return 0;
    }
    for (i=0; i<tm->voices; i++)
    {
        if(tm->voice[i].status != VOICE_FREE)
            count++;
    }
    return count;
}

int timid_get_max_voices(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->voices;
}

int timid_get_immediate_panning(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->adjust_panning_immediately;
}

int timid_get_mono(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->play_mode.encoding;
}

int timid_get_fast_decay(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->fast_decay;
}

int timid_get_antialiasing(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->antialiasing_allowed;
}

int timid_get_sample_rate(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->play_mode.rate;
}

int timid_get_control_rate(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->control_rate;
}

int timid_get_default_program(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->default_program;
}

int timid_get_lost_notes(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->lost_notes;
}

int timid_get_cut_notes(Timid *tm)
{
    if (!tm)
    {
        return 0;
    }
    return tm->cut_notes;
}

int timid_get_current_program(Timid *tm, int c)
{
    if (!tm)
    {
        return 0;
    }
    c = c & 0x0f;
    if (ISDRUMCHANNEL(tm, c))
    {
        return tm->channel[c].bank;
    }
    else
    {
        return tm->channel[c].program;
    }
}

void timid_close(Timid *tm)
{
    if (!tm)
    {
        return;
    }
    reset_midi(tm);
    timid_unload_config(tm);
    free_default_instrument(tm);
    free_tables(tm);
}
