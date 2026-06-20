/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

/* sound.cpp: implementation of the sound class. */

#include "types.h"

#ifdef DRZ80
#include "DrZ80_support.h"
#else
#if defined(CZ80)
#include "cz80_support.h"
#else
#include "z80.h"
#endif
#endif

#include <math.h>

#define Machine (&m_emuInfo)

#include "neopopsound.h"
#include "neopop_blip.h"

int sndCycles = 0;

int neopop_audio_accurate = 0; /* 0 = fast per-sample, 1 = band-limited Blip */

/* Re-derive the Blip synth parameters from the live chip register state, then
 * advance the band-limited synth by 'cycles' chip cycles. Keeping the Blip path
 * a pure observer of toneChip/noiseChip avoids duplicating the register decode. */
static void neopop_blip_sync_from_chips(void)
{
   int c;

   for (c = 0; c < 3; c++)
      neopop_blip_sync_tone(c,
            neopop_sound_tone_divider(c),
            neopop_sound_tone_volume(c));

   neopop_blip_sync_noise(
         neopop_sound_noise_divider(),
         neopop_sound_noise_volume(),
         neopop_sound_noise_feedback_periodic());
}

void soundStep(int cycles)
{
   sndCycles+= cycles;

   if (neopop_audio_accurate)
   {
      neopop_blip_sync_from_chips();
      neopop_blip_run(cycles);
   }
}

/*
 *
 * Neogeo Pocket Sound system
 *
 */

unsigned int	ngpRunning;

/* Loadstate sound-debug counters (shown on the NGP screen). T3 = Timer3 IRQs,
 * Z80 = sound-Z80 cycles executed, SW = WriteSoundChip calls. After a load they
 * reveal where the sound chain breaks (timer / Z80 run / sequencer write). */
unsigned int g_dbg_t3 = 0, g_dbg_z80run = 0, g_dbg_sndw = 0;

void ngpSoundStart(void)
{
   ngpRunning = 1;	/* ? */
#if defined(DRZ80) || defined(CZ80)
   Z80_Reset();
#else
   z80Init();
   z80SetRunning(1);
#endif
}

/* Execute all gained cycles (divided by 2) */
void ngpSoundExecute(void)
{
#if defined(DRZ80) || defined(CZ80)
   int toRun = sndCycles/2;
   if(ngpRunning) {
      Z80_Execute(toRun);
      g_dbg_z80run += (toRun > 0) ? (unsigned int)toRun : 0;
   }
   sndCycles -= toRun;
#else
   int		elapsed;
   while(sndCycles > 0)
   {
      elapsed = z80Step();
      sndCycles-= (2*elapsed);
   }
#endif
}

/* Switch sound system off */
void ngpSoundOff(void)
{
   ngpRunning = 0;
#if defined(DRZ80) || defined(CZ80)

#else
   z80SetRunning(0);
#endif
}

/* Generate interrupt to ngp sound system */
void ngpSoundInterrupt(void)
{
   if (ngpRunning)
   {
#if defined(DRZ80) || defined(CZ80)
      Z80_Cause_Interrupt(0x100); /* Z80_IRQ_INT??? */
#else
      z80Interrupt(0);
#endif
   }
}
