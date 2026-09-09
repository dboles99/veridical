# Veridical

Five modulation effects for VCV Rack 2, by eddie_dodd.

They are ordinary effects. A flanger flanges, a phaser phases, and the labels
mean what they say. The series exists because the obvious ones are missing:
searching the library for a flanger returns three modules and none of them is a
flanger, and no phaser in the library has been rebuilt since 2025.

## Modules

**Flanger** — short modulated delay, roughly 0.1 to 10 ms. Feedback is bipolar,
so negative feedback gives the hollow sound rather than the resonant one.

**Chorus** — longer delay, 10 to 30 ms, with two, three or four voices. Each
voice runs its own LFO phase.

**Phaser** — a cascade of one-pole allpass sections, switchable between four,
six and eight stages, with feedback.

**Ring Mod** — an internal sine carrier, replaced by whatever you patch into the
carrier input.

**Env Follower** — attack and release follower with an envelope output and a
gate output. It is here so the other four can be driven from audio.

## Everything is polyphonic and stereo

All five follow the polyphonic channel count of their input and process up to
sixteen channels. Right input is normalled to left, so a mono source feeds both
sides without a mult.

Every continuous parameter has a CV input and an attenuverter. The knob sets the
base value, the attenuverter scales the CV, and the two are summed.

## Panels

Each module has a dark and a light panel. Right-click the module and pick one.
The choice is saved with the patch, per module.

Colours are documented in [docs/palette.md](docs/palette.md).

## Building

Requires the Rack SDK. Point `RACK_DIR` at it:

    RACK_DIR=/path/to/Rack-SDK make

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE).
