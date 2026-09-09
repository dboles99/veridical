#pragma once
#include <rack.hpp>


/** Ring buffer with fractional reads, one per polyphony channel.

Capacity is rounded up to a power of two so the wrap is a mask. The Flanger and
the Chorus are the only users; the Phaser sweeps allpass sections instead and
never touches this.
*/
struct DelayLine {
	std::vector<float> buf;
	int size = 0;
	int mask = 0;
	int w = 0;

	void setCapacity(int samples) {
		int n = 8;
		while (n < samples)
			n *= 2;
		if (n == size)
			return;
		size = n;
		mask = n - 1;
		buf.assign(n, 0.f);
		w = 0;
	}

	void clear() {
		std::fill(buf.begin(), buf.end(), 0.f);
		w = 0;
	}

	void write(float x) {
		w = (w + 1) & mask;
		buf[w] = x;
	}

	/** `d` is in samples and must be at least 1. */
	float readLinear(float d) const {
		int i = (int) d;
		float f = d - i;
		int a = (w - i + size) & mask;
		int b = (a - 1 + size) & mask;
		return buf[a] + (buf[b] - buf[a]) * f;
	}

	/** Catmull-Rom, for the flanger where the delay is a few samples long and
	linear interpolation audibly lowpasses the wet signal. `d` must be at
	least 2 so the newer neighbour is a real sample.
	*/
	float readCubic(float d) const {
		int i = (int) d;
		float f = d - i;
		int i0 = (w - i + size) & mask;
		float ym1 = buf[(i0 + 1) & mask];
		float y0 = buf[i0];
		float y1 = buf[(i0 - 1 + size) & mask];
		float y2 = buf[(i0 - 2 + size) & mask];

		float c1 = 0.5f * (y1 - ym1);
		float c2 = ym1 - 2.5f * y0 + 2.f * y1 - 0.5f * y2;
		float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
		return ((c3 * f + c2) * f + c1) * f + y0;
	}
};
