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


   timid.h
*/

/* Application developers wishing to integrate this library can skip to the public API, which is in the "extern "C"" block */

#ifndef TIMID_H
#define TIMID_H

#include <stdio.h>

#include "config.h"

typedef struct {
  char *path;
  void *next;
} PathList;

/* Noise modes for open_file */
#define OF_SILENT	0
#define OF_NORMAL	1
#define OF_VERBOSE	2

/* Order of the FIR filter = 20 should be enough ! */
#define ORDER 20
#define ORDER2 ORDER/2

#ifndef PI
#define PI   3.14159265
#endif

typedef struct {
  int32
    loop_start, loop_end, data_length,
    sample_rate, low_freq, high_freq, root_freq;
  int32
    envelope_rate[6], envelope_offset[6];
  FLOAT_T
    volume;
  sample_t *data;
  int32 
    tremolo_sweep_increment, tremolo_phase_increment, 
    vibrato_sweep_increment, vibrato_control_ratio;
  uint8
    tremolo_depth, vibrato_depth,
    modes;
  int8
    panning, note_to_use;
} Sample;

/* Bits in modes: */
#define MODES_16BIT	(1<<0)
#define MODES_UNSIGNED	(1<<1)
#define MODES_LOOPING	(1<<2)
#define MODES_PINGPONG	(1<<3)
#define MODES_REVERSE	(1<<4)
#define MODES_SUSTAIN	(1<<5)
#define MODES_ENVELOPE	(1<<6)

typedef struct {
  int samples;
  Sample *sample;
} Instrument;

typedef struct {
  char *name;
  Instrument *instrument;
  int note, amp, pan, strip_loop, strip_envelope, strip_tail;
} ToneBankElement;

/* A hack to delay instrument loading until after reading the
   entire MIDI file. */
#define MAGIC_LOAD_INSTRUMENT ((Instrument *)(-1))

typedef struct {
  ToneBankElement tone[128];
} ToneBank;

#define SPECIAL_PROGRAM -1

/* Data format encoding bits */

#define PE_MONO 	0x01  /* versus stereo */
#define PE_SIGNED	0x02  /* versus unsigned */
#define PE_16BIT 	0x04  /* versus 8-bit */
#define PE_ULAW 	0x08  /* versus linear */
#define PE_BYTESWAP	0x10  /* versus the other way */

typedef struct {
  int32 rate, encoding;
} PlayMode;

typedef struct {
  int32 time;
  uint8 channel, type, a, b;
} MidiEvent;

/* Midi events */
#define ME_NONE 	0
#define ME_NOTEON	1
#define ME_NOTEOFF	2
#define ME_KEYPRESSURE	3
#define ME_MAINVOLUME	4
#define ME_PAN		5
#define ME_SUSTAIN	6
#define ME_EXPRESSION	7
#define ME_PITCHWHEEL	8
#define ME_PROGRAM	9
#define ME_MONO	10
#define ME_PITCH_SENS	11

#define ME_ALL_SOUNDS_OFF	12
#define ME_RESET_CONTROLLERS	13
#define ME_ALL_NOTES_OFF	14
#define ME_TONE_BANK	15

#define ME_POLY	16

#define ME_TEMPO	17

#define ME_EOT		99

typedef struct {
  int
    bank, program, volume, sustain, panning, pitchbend, expression, 
    mono, /* one note only on this channel */
    pitchsens;
  /* chorus, reverb... Coming soon to a 300-MHz, eight-way superscalar
     processor near you */
  FLOAT_T
    pitchfactor; /* precomputed pitch bend factor to save some fdiv's */
} Channel;

/* Causes the instrument's default panning to be used. */
#define NO_PANNING -1

typedef struct {
  uint8
    status, channel, note, velocity;
  Sample *sample;
  int32
    orig_frequency, frequency,
    sample_offset, sample_increment,
    envelope_volume, envelope_target, envelope_increment,
    tremolo_sweep, tremolo_sweep_position,
    tremolo_phase, tremolo_phase_increment,
    vibrato_sweep, vibrato_sweep_position;
  
  final_volume_t left_mix, right_mix;

  FLOAT_T
    left_amp, right_amp, tremolo_volume;
  int32
    vibrato_sample_increment[VIBRATO_SAMPLE_INCREMENTS];
  int
    vibrato_phase, vibrato_control_ratio, vibrato_control_counter,
    envelope_stage, control_counter, panning, panned;

} Voice;

/* Voice status options: */
#define VOICE_FREE 0
#define VOICE_ON 1
#define VOICE_SUSTAINED 2
#define VOICE_OFF 3
#define VOICE_DIE 4

/* Voice panned options: */
#define PANNED_MYSTERY 0
#define PANNED_LEFT 1
#define PANNED_RIGHT 2
#define PANNED_CENTER 3
/* Anything but PANNED_MYSTERY only uses the left volume */

#define ISDRUMCHANNEL(tm, c) ((tm->drumchannels & (1<<(c))))
#define ISQUIETCHANNEL(tm, c) ((tm->quietchannels & (1<<(c))))

typedef struct {
  MidiEvent event;
  void *next;
} MidiEventList;

#ifdef LOOKUP_SINE
FLOAT_T sine(int x);
#else
#include <math.h>
#define sine(x) (sin((2*PI/1024.0) * (x)))
#endif

#define SINE_CYCLE_LENGTH 1024
extern int32 freq_table[];
extern FLOAT_T vol_table[];
extern FLOAT_T bend_fine[];
extern FLOAT_T bend_coarse[];
extern uint8 *_l2u; /* 13-bit PCM to 8-bit u-law */
extern uint8 _l2u_[]; /* used in LOOKUP_HACK */
#ifdef LOOKUP_HACK
extern int16 _u2l[];
#endif

typedef struct {
  char current_filename[1024];
  /* The paths in this list will be tried whenever we're reading a file */
  PathList *pathlist;
  ToneBank *tonebank[128];
  ToneBank *drumset[128];
  /* This is a special instrument, used for all melodic programs */
  Instrument *default_instrument;
  /* This is only used for tracks that don't specify a program */
  int default_program;
  int antialiasing_allowed;
  int pre_resampling_allowed;
  int fast_decay;
  int dynamic_loading;
  PlayMode play_mode;
  int32 common_buffer[AUDIO_BUFFER_SIZE*2]; /* stereo samples */
  int32 *buffer_pointer;
  Channel channel[16];
  Voice voice[MAX_VOICES];
  int32 control_rate;
  int32 control_ratio;
  FLOAT_T master_volume;
  int32 drumchannels;
  int32 quietchannels;
  int32 lost_notes;
  int32 cut_notes;
  int adjust_panning_immediately;
  int voices;
  uint8 rpn_msb[16];
  uint8 rpn_lsb[16];
  MidiEvent *event_list;
  MidiEvent *current_event;
  int32 sample_count;
  int32 current_sample;
  FILE* fp_midi;
  int32 events_midi;
  char song_title[256];
  char song_copyright[256];
  char last_smf[1024];
  /* to avoid some unnecessary parameter passing */
  MidiEventList *evlist;
  int32 event_count;
  FILE *fp;
  int32 at;
  /* These would both fit into 32 bits, but they are often added in
  large multiples, so it's simpler to have two roomy ints */
  int32 sample_increment;
  int32 sample_correction;
  sample_t resample_buffer[AUDIO_BUFFER_SIZE];
#ifdef LOOKUP_HACK
  int32 *mixup;
#ifdef LOOKUP_INTERPOLATION
  int8 *iplookup;
#endif
#endif
  char def_instr_name[256];
  char last_config[1024];
} Timid;

typedef struct {
  int8 data[3];
} int24;

FILE *open_file(Timid *tm, char *name, int decompress, int noise_mode);
void add_to_pathlist(Timid *tm, char *s);
void free_pathlist(Timid *tm);
void close_file(FILE *fp);
void skip(FILE *fp, size_t len);
void *safe_malloc(size_t count);
void antialiasing(Sample *sp, int32 output_rate);
int load_missing_instruments(Timid *tm);
void free_instruments(Timid *tm);
int set_default_instrument(Timid *tm, char *name);
void free_default_instrument(Timid *tm);
void mix_voice(Timid *tm, int32 *buf, int v, int32 c);
int recompute_envelope(Timid *tm, int v);
void apply_envelope_to_amp(Timid *tm, int v);
MidiEvent *read_midi_file(Timid *tm, FILE *mfp, int32 *count, int32 *sp);
sample_t *resample_voice(Timid *tm, int v, int32 *countptr);
void pre_resample(Timid *tm, Sample *sp);
void init_tables(Timid *tm);
void free_tables(Timid *tm);
int read_config_file(Timid *tm, char *name);

#ifdef __cplusplus
extern "C" {
#endif

/* Audio format identifiers for timid_play_smf */

#define AU_CHAR	1
#define AU_SHORT	2
#define AU_24	3
#define AU_LONG	4
#define AU_FLOAT	5
#define AU_DOUBLE	6
#define AU_ULAW	7

/* General API notes: Unless otherwise indicated, functions that return a value will return non-0 on success, and 0 on failure. Time and duration is represented in milliseconds. API functions for getting strings return the length of the string */

/* Initialize an instance of Timidity. This should be called after allocating memory for a Timid struct, and before any other API function calls */
void timid_init(Timid *tm);

/* Manage Timidity configurations (sample sets) */
int timid_load_config(Timid *tm, char *filename);
void timid_unload_config(Timid *tm);
int timid_reload_config(Timid *tm);

/* High level input API */
void timid_channel_note_on(Timid *tm, uint8 channel, uint8 note, uint8 velocity);
void timid_channel_note_off(Timid *tm, uint8 channel, uint8 note);
void timid_channel_key_pressure(Timid *tm, uint8 channel, uint8 note, uint8 velocity);
void timid_channel_set_volume(Timid *tm, uint8 channel, uint8 volume);
void timid_channel_set_pan(Timid *tm, uint8 channel, uint8 pan);
void timid_channel_set_expression(Timid *tm, uint8 channel, uint8 expression);
void timid_channel_set_sustain(Timid *tm, uint8 channel, uint8 sustain);
void timid_channel_set_pitch_wheel(Timid *tm, uint8 channel, uint16 pitch);
void timid_channel_set_pitch_range(Timid *tm, uint8 channel, uint8 range);
void timid_channel_set_program(Timid *tm, uint8 channel, uint8 program);
void timid_channel_set_bank(Timid *tm, uint8 channel, uint8 bank);
void timid_channel_mono_mode(Timid *tm, uint8 channel);
void timid_channel_poly_mode(Timid *tm, uint8 channel);
void timid_channel_all_notes_off(Timid *tm, uint8 channel);
void timid_channel_all_sounds_off(Timid *tm, uint8 channel);
void timid_channel_reset_controllers(Timid *tm, uint8 channel);
void timid_channel_control_change(Timid *tm, uint8 channel, uint8 controller, uint8 value);

/* Low level input API */
void timid_write_midi(Timid *tm, uint8 byte1, uint8 byte2, uint8 byte3);
void timid_write_midi_packed(Timid *tm, uint32 data);
void timid_write_sysex(Timid *tm, uint8 *buffer, int32 count);

/* Audio output functions */
void timid_render_char(Timid *tm, uint8 *buffer, int32 count);
void timid_render_short(Timid *tm, int16 *buffer, int32 count);
void timid_render_24(Timid *tm, int24 *buffer, int32 count);
void timid_render_long(Timid *tm, int32 *buffer, int32 count);
void timid_render_float(Timid *tm, float *buffer, int32 count);
void timid_render_double(Timid *tm, double *buffer, int32 count);
void timid_render_ulaw(Timid *tm, uint8 *buffer, int32 count);

/* Stop all notes with release */
void timid_all_notes_off(Timid *tm);
/* Stop all notes with quick fade out, helps avoid clicks */
void timid_all_sounds_off(Timid *tm);
/* Reset all MIDI controllers */
void timid_reset_controllers(Timid *tm);
/* Stop all notes immediately */
void timid_panic(Timid *tm);
/* Stop all notes immediately, and reset all MIDI parameters */
void timid_reset(Timid *tm);

/* MIDI file player, only supports standard MIDI files */
int timid_load_smf(Timid *tm, char *filename);
int timid_reload_smf(Timid *tm);
/* The following function returns 1 if more audio is to be rendered, and 0 once the track is finished and all notes have stopped */
int timid_play_smf(Timid *tm, int32 type, uint8 *buffer, int32 count); /* count is in samples */
/* The return value for the following functions is the new current time */
/* Absolute seeking */
int timid_seek_smf(Timid *tm, int32 time);
/* Relative seeking */
int timid_fast_forward_smf(Timid *tm, int32 time);
int timid_rewind_smf(Timid *tm, int32 time);
/* Quick ways to restart or stop a track without reloading or unloading */
int timid_restart_smf(Timid *tm);
int timid_stop_smf(Timid *tm);
void timid_unload_smf(Timid *tm);

/* Setters */
/* Amplification is represented in percent */
void timid_set_amplification(Timid *tm, int amplification);
/* The number of voices is clamped between 1 and MAX_VOICES */
void timid_set_max_voices(Timid *tm, int voices);
/* The value argument for the following functions should be treated as a boolean */
void timid_set_immediate_panning(Timid *tm, int value);
/* Renders mono audio buffers if enabled, interleaved stereo otherwise */
void timid_set_mono(Timid *tm, int value);
/* These next few functions reload the current sample bank before returning */
void timid_set_fast_decay(Timid *tm, int value);
void timid_set_antialiasing(Timid *tm, int value);
void timid_set_pre_resample(Timid *tm, int value);
void timid_set_dynamic_instrument_load(Timid *tm, int value);
void timid_set_sample_rate(Timid *tm, int rate);
void timid_set_control_rate(Timid *tm, int rate);
/* Sets the default MIDI program, takes effect on next MIDI reset */
void timid_set_default_program(Timid *tm, int program);
void timid_set_drum_channel(Timid *tm, int channel, int enable);
void timid_set_quiet_channel(Timid *tm, int channel, int enable);

/* Restore default settings */
void timid_restore_defaults(Timid *tm);

/* Force all instruments to be loaded */
int timid_force_instrument_load(Timid *tm);

/* Manage default instruments. These functions take effect on the next MIDI reset */
int timid_set_default_instrument(Timid *tm, char *filename);
void timid_free_default_instrument(Timid *tm);

/* Getters */
int timid_get_config_name(Timid *tm, char *buffer, int32 count);
int timid_get_amplification(Timid *tm);
int timid_get_active_voices(Timid *tm);
int timid_get_max_voices(Timid *tm);
int timid_get_immediate_panning(Timid *tm);
int timid_get_mono(Timid *tm);
int timid_get_fast_decay(Timid *tm);
int timid_get_antialiasing(Timid *tm);
int timid_get_pre_resample(Timid *tm);
int timid_get_dynamic_instrument_load(Timid *tm);
int timid_get_sample_rate(Timid *tm);
int timid_get_control_rate(Timid *tm);
int timid_get_default_program(Timid *tm);
int timid_get_drum_channel_enabled(Timid *tm, int channel);
int timid_get_quiet_channel_enabled(Timid *tm, int channel);
int timid_get_lost_notes(Timid *tm);
int timid_get_cut_notes(Timid *tm);

/* Get values from a particular MIDI channel */
int timid_channel_get_volume(Timid *tm, int channel);
int timid_channel_get_pan(Timid *tm, int channel);
int timid_channel_get_expression(Timid *tm, int channel);
int timid_channel_get_sustain(Timid *tm, int channel);
int timid_channel_get_pitch_wheel(Timid *tm, int channel);
int timid_channel_get_pitch_range(Timid *tm, int channel);
int timid_channel_get_program(Timid *tm, int channel);
/* The following function returns -1 for drum channels */
int timid_channel_get_bank(Timid *tm, int channel);
int timid_channel_get_mono(Timid *tm, int channel);

/* These are for the MIDI file player */
int timid_get_smf_name(Timid *tm, char *buffer, int32 count);
int timid_get_event_count(Timid *tm);
int timid_get_sample_count(Timid *tm);
int timid_get_duration(Timid *tm);
int timid_get_current_time(Timid *tm);
int timid_get_current_sample_position(Timid *tm);
int timid_get_bitrate(Timid *tm);
int timid_get_song_title(Timid *tm, char *buffer, int32 count);
int timid_get_song_copyright(Timid *tm, char *buffer, int32 count);

/* Utility functions */
int timid_millis2samples(Timid *tm, int millis);
int timid_samples2millis(Timid *tm, int samples);

/* Close an instance of Timidity. This should be called after all other API function calls */
void timid_close(Timid *tm);

#ifdef __cplusplus
}
#endif

#endif
