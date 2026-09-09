# eddie_dodd's Veridical Series

Five effects for VCV Rack 2. Each one does what its label says and nothing else:
a flanger, a chorus, a phaser, a ring modulator and an envelope follower. There
are no hidden modes, no menu-only parameters and no macro knobs.

All five are stereo, all five are polyphonic to sixteen channels, and every
continuous control has a CV input with its own attenuverter. The right input is
normalled to the left, so a mono source feeds both sides without a mult.

## Modules

### Flanger (10 HP)

A short modulated delay, 0.1 to 10 ms, read with cubic interpolation because at
those lengths linear interpolation audibly dulls the wet signal.

| Control | Range |
|---|---|
| Rate | 0.02 to 10 Hz |
| Depth | how much of the 0.1 to 10 ms span the sweep covers |
| Feedback | bipolar; negative gives the hollow, thinned version |
| Mix | dry to wet |

The left and right sweeps sit a quarter cycle apart.

### Chorus (10 HP)

A longer delay, 20 ms nominal and up to 10 ms either side of that, with two,
three or four voices spread evenly around the LFO cycle. No feedback, so it
stays clean at any setting.

| Control | Range |
|---|---|
| Rate | 0.02 to 5 Hz |
| Depth | excursion, up to 10 ms |
| Mix | dry to wet |
| Voices | 2, 3 or 4 |

### Phaser (10 HP)

Four, six or eight first-order allpass sections in series, swept from 120 Hz
upwards over six octaves at full depth, with feedback around the chain.

| Control | Range |
|---|---|
| Rate | 0.02 to 8 Hz |
| Depth | how far up the sweep travels |
| Feedback | 0 to maximum resonance |
| Mix | dry to wet |
| Stages | 4, 6 or 8 |

Feedback around an allpass chain has nothing damping it, so the input into the
loop is trimmed to keep the resonant peak near twice unity instead of ten times
it. Below about half feedback the trim does nothing at all.

### Ring Mod (10 HP)

Multiplies the input by a sine carrier. Patch anything into CARRIER and it
replaces the internal oscillator.

| Control | Range |
|---|---|
| Freq | 0.1 Hz to 4 kHz, 1V/oct from the CV input at a fully open attenuverter |
| Mix | dry to ring modulated |

The carrier stops at 4 kHz. Higher than that and the sum and difference
products of ordinary material fold back into the audible band, which is
inherent to ring modulation rather than a fault, but there is no reason to
encourage it.

### Env Follower (10 HP)

Rectifies and smooths each side separately, with independent attack and release
times.

| Control | Range |
|---|---|
| Attack | 0.1 ms to 1 s |
| Release | 1 ms to 5 s |
| Gain | 0 to 4x |

ENV runs 0 to 10 V and reaches full scale on a 10 Vpp input at unity gain. GATE
goes high above 1 V and low again below 0.5 V. This is the one module without a
bypass route, because its outputs measure the input rather than carrying a
treated copy of it.

## Panels

Every module has a dark and a light panel. Right click and pick one under
"Panel"; the choice is saved with the patch, per module instance. Dark is the
default.

The colours, for anyone editing the artwork:

| | Dark | Light |
|---|---|---|
| Panel | `#101010` | `#edeae3` |
| Module name | `#b4b4b4` | `#2e2e2e` |
| Control labels | `#9a9a9a` | `#3a3a3a` |
| Tick marks, wordmark | `#6f6f6f` | `#5f5f5f` |

The knobs, attenuverters, jacks and switch are drawn for this plugin rather than
taken from Rack's component library, so one set of artwork has to sit on both
grounds. Knob face `#8b9a8c`, outline and pointer `#4f5a50`. The outline is what
separates the knob from the panel in both directions, so it is structural.

`scripts/genpanels.py` regenerates the ten panel SVGs. It needs `fonttools` and
a copy of DejaVu Sans; label text is emitted as outlines because nanosvg, which
is what Rack renders SVG with, ignores `<text>` entirely.

## Building

Set `RACK_DIR` to your copy of the Rack 2 SDK:

    RACK_DIR=../Rack-SDK make

`tools/panelcheck.sh` parses the panels with Rack's own nanosvg and checks the
page size, the HP, that nothing overflows and that the hidden `components`
layer is not being drawn. `tools/dspcheck.sh` builds a small program that drives
every module's `process()` offline and asserts the output is audible, finite,
bounded, stereo and correct across sixteen channels and three sample rates. It
links against `libRack.dll`, so it needs a Rack installation rather than just
the SDK.

## Licence

Copyright (C) 2026 eddie_dodd. Released under the GPL, version 3 or later; see
LICENSE.

That covers the panel artwork as well as the code. The panel SVGs, the knobs, the
attenuverters, the jacks and the switch are all original work for this plugin and
are GPL-3.0-or-later like everything else, so they can be redistributed and
modified, including into variants I have not made.

Panel lettering is outlined from DejaVu Sans, which is free to use and
redistribute under the Bitstream Vera and Arev licences.
