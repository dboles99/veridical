// Drives every module's process() loop offline and asserts the output is
// audible, finite, bounded and stereo where it should be. Links against
// libRack.dll from a Rack installation; see tools/dspcheck.sh.

#include "../src/plugin.cpp"
#include "../src/Flanger.cpp"
#include "../src/Chorus.cpp"
#include "../src/Phaser.cpp"
#include "../src/RingMod.cpp"
#include "../src/EnvFollower.cpp"

#include <cstdio>
#include <cmath>
#include <vector>

static int failures = 0;

static void report(const char* what, bool ok) {
	std::printf("%-56s %s\n", what, ok ? "ok" : "FAIL");
	if (!ok)
		failures++;
}

struct Trace {
	std::vector<float> in, left, right;
	bool finite = true;

	double peak(const std::vector<float>& v) const {
		double p = 0.0;
		for (size_t i = 0; i < v.size(); i++)
			p = std::max(p, (double) std::fabs(v[i]));
		return p;
	}
	double rms(const std::vector<float>& v) const {
		double s = 0.0;
		for (size_t i = 0; i < v.size(); i++)
			s += (double) v[i] * v[i];
		return v.empty() ? 0.0 : std::sqrt(s / v.size());
	}
	double difference(const std::vector<float>& a, const std::vector<float>& b) const {
		double s = 0.0;
		for (size_t i = 0; i < a.size(); i++)
			s += (double)(a[i] - b[i]) * (a[i] - b[i]);
		return a.empty() ? 0.0 : std::sqrt(s / a.size());
	}
};

struct Rig {
	int inL, inR, outL, outR;
	int cv;  // the CV input used for the attenuverter check
};

template <typename M>
static void configure(M& m, float sampleRate, const Rig& rig, int channels, bool patchRight) {
	rack::engine::Module::SampleRateChangeEvent sr;
	sr.sampleRate = sampleRate;
	sr.sampleTime = 1.f / sampleRate;
	m.onSampleRateChange(sr);
	m.inputs[rig.inL].channels = channels;
	if (patchRight)
		m.inputs[rig.inR].channels = channels;
	for (size_t i = 0; i < m.outputs.size(); i++)
		m.outputs[i].channels = channels;
}

/** Sine into the left input only, so the right side exercises the normal. */
template <typename M>
static Trace run(M& m, float sampleRate, const Rig& rig, float amplitude, float seconds,
	int cvId = -1, float cvVolts = 0.f) {
	configure(m, sampleRate, rig, 1, false);
	if (cvId >= 0) {
		m.inputs[cvId].channels = 1;
		m.inputs[cvId].setVoltage(cvVolts);
	}

	rack::engine::Module::ProcessArgs args;
	args.sampleRate = sampleRate;
	args.sampleTime = 1.f / sampleRate;
	args.frame = 0;

	Trace t;
	double phase = 0.0;
	int samples = (int)(sampleRate * seconds);
	int skip = (int)(sampleRate / 5);
	for (int i = 0; i < samples; i++) {
		float in = amplitude * (float) std::sin(2.0 * M_PI * phase);
		phase += 220.0 / sampleRate;
		if (phase >= 1.0)
			phase -= 1.0;
		m.inputs[rig.inL].setVoltage(in);
		m.process(args);
		args.frame++;
		if (i < skip)
			continue;
		float l = m.outputs[rig.outL].getVoltage();
		float r = m.outputs[rig.outR].getVoltage();
		if (!std::isfinite(l) || !std::isfinite(r))
			t.finite = false;
		t.in.push_back(in);
		t.left.push_back(l);
		t.right.push_back(r);
	}
	return t;
}

template <typename M>
static void poly(const char* name, const Rig& rig) {
	M m;
	configure(m, 48000.f, rig, PORT_MAX_CHANNELS, true);
	rack::engine::Module::ProcessArgs args;
	args.sampleRate = 48000.f;
	args.sampleTime = 1.f / 48000.f;
	args.frame = 0;

	double phase[PORT_MAX_CHANNELS] = {};
	double energy[PORT_MAX_CHANNELS] = {};
	bool finite = true;
	for (int i = 0; i < 48000; i++) {
		for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
			// A different pitch per channel, so a lane mistake shows up as a
			// dead or duplicated channel rather than looking plausible.
			float v = 5.f * (float) std::sin(2.0 * M_PI * phase[c]);
			m.inputs[rig.inL].setVoltage(v, c);
			m.inputs[rig.inR].setVoltage(v, c);
			phase[c] += (110.0 + 37.0 * c) / 48000.0;
			if (phase[c] >= 1.0)
				phase[c] -= 1.0;
		}
		m.process(args);
		args.frame++;
		if (i > 9600) {
			for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
				float v = m.outputs[rig.outR].getVoltage(c);
				if (!std::isfinite(v))
					finite = false;
				energy[c] += (double) v * v;
			}
		}
	}
	bool allLive = true;
	for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
		if (energy[c] < 1.0)
			allLive = false;
	}
	char buf[160];
	std::snprintf(buf, sizeof buf, "%s: 16 channels all live and finite", name);
	report(buf, finite && allLive
		&& m.outputs[rig.outL].getChannels() == PORT_MAX_CHANNELS
		&& m.outputs[rig.outR].getChannels() == PORT_MAX_CHANNELS);
}

template <typename M>
static void check(const char* name, const Rig& rig, bool expectWidth) {
	char buf[160];
	{
		M m;
		Trace t = run(m, 48000.f, rig, 5.f, 2.f);
		std::snprintf(buf, sizeof buf, "%s: output is finite", name);
		report(buf, t.finite);
		std::snprintf(buf, sizeof buf, "%s: left not silent (rms %.3f V)", name, t.rms(t.left));
		report(buf, t.rms(t.left) > 0.5);
		std::snprintf(buf, sizeof buf, "%s: right normalled from left (rms %.3f V)", name, t.rms(t.right));
		report(buf, t.rms(t.right) > 0.5);
		std::snprintf(buf, sizeof buf, "%s: differs from input (rms %.3f V)", name, t.difference(t.left, t.in));
		report(buf, t.difference(t.left, t.in) > 0.05);
		std::snprintf(buf, sizeof buf, "%s: bounded (peak %.2f V)", name, t.peak(t.left));
		report(buf, t.peak(t.left) < 12.0);

		double width = t.difference(t.left, t.right);
		if (expectWidth) {
			std::snprintf(buf, sizeof buf, "%s: sides differ (rms %.3f V)", name, width);
			report(buf, width > 0.1);
		}
		else {
			std::snprintf(buf, sizeof buf, "%s: sides match (rms %.4f V)", name, width);
			report(buf, width < 1e-6);
		}
	}
	{
		M m;
		for (size_t i = 0; i < m.params.size(); i++)
			m.params[i].setValue(m.paramQuantities[i]->getMaxValue());
		Trace t = run(m, 48000.f, rig, 5.f, 10.f);
		std::snprintf(buf, sizeof buf, "%s: stable at max settings (peak %.2f V)", name, t.peak(t.left));
		report(buf, t.finite && t.peak(t.left) < 15.0);
	}
	{
		M m;
		Trace t = run(m, 48000.f, rig, 0.f, 1.f);
		std::snprintf(buf, sizeof buf, "%s: quiet with no input (peak %.4f V)", name, t.peak(t.left));
		report(buf, t.peak(t.left) < 1e-3);
	}
	for (int i = 0; i < 2; i++) {
		float sr = i ? 192000.f : 44100.f;
		M m;
		Trace t = run(m, sr, rig, 5.f, 1.f);
		std::snprintf(buf, sizeof buf, "%s: works at %g Hz (rms %.3f V)", name, sr, t.rms(t.left));
		report(buf, t.finite && t.rms(t.left) > 0.5 && t.peak(t.left) < 12.0);
	}
	{
		// A closed attenuverter has to make the CV input inert, otherwise the
		// knob is not really in sole control of anything.
		M a, b;
		Trace plain = run(a, 48000.f, rig, 5.f, 0.5f);
		Trace driven = run(b, 48000.f, rig, 5.f, 0.5f, rig.cv, 7.3f);
		double drift = plain.difference(plain.left, driven.left);
		std::snprintf(buf, sizeof buf, "%s: closed attenuverter ignores CV (rms %.6f V)", name, drift);
		report(buf, drift == 0.0);
	}
	poly<M>(name, rig);
}

int main() {
	Rig flanger = {Flanger::IN_L_INPUT, Flanger::IN_R_INPUT, Flanger::OUT_L_OUTPUT, Flanger::OUT_R_OUTPUT, Flanger::RATE_INPUT};
	Rig chorus = {Chorus::IN_L_INPUT, Chorus::IN_R_INPUT, Chorus::OUT_L_OUTPUT, Chorus::OUT_R_OUTPUT, Chorus::RATE_INPUT};
	Rig phaser = {Phaser::IN_L_INPUT, Phaser::IN_R_INPUT, Phaser::OUT_L_OUTPUT, Phaser::OUT_R_OUTPUT, Phaser::RATE_INPUT};
	Rig ring = {RingMod::IN_L_INPUT, RingMod::IN_R_INPUT, RingMod::OUT_L_OUTPUT, RingMod::OUT_R_OUTPUT, RingMod::FREQ_INPUT};
	Rig env = {EnvFollower::IN_L_INPUT, EnvFollower::IN_R_INPUT, EnvFollower::ENV_L_OUTPUT, EnvFollower::ENV_R_OUTPUT, EnvFollower::ATTACK_INPUT};

	check<Flanger>("Flanger", flanger, true);
	check<Chorus>("Chorus", chorus, true);
	check<Phaser>("Phaser", phaser, true);
	check<RingMod>("RingMod", ring, false);
	check<EnvFollower>("EnvFollower", env, false);

	char buf[160];
	{
		EnvFollower m;
		Trace t = run(m, 48000.f, env, 5.f, 2.f);
		std::snprintf(buf, sizeof buf, "EnvFollower: ENV tracks 10Vpp (peak %.2f V)", t.peak(t.left));
		report(buf, t.finite && t.peak(t.left) > 9.0 && t.peak(t.left) <= 10.0);
		float gateL = m.outputs[EnvFollower::GATE_L_OUTPUT].getVoltage();
		float gateR = m.outputs[EnvFollower::GATE_R_OUTPUT].getVoltage();
		std::snprintf(buf, sizeof buf, "EnvFollower: both gates high (%.1f, %.1f V)", gateL, gateR);
		report(buf, gateL > 9.9f && gateR > 9.9f);
	}
	{
		EnvFollower m;
		Trace t = run(m, 48000.f, env, 0.f, 1.f);
		std::snprintf(buf, sizeof buf, "EnvFollower: ENV rests at zero (peak %.4f V)", t.peak(t.left));
		report(buf, t.peak(t.left) < 1e-4);
		std::snprintf(buf, sizeof buf, "EnvFollower: gate low (%.1f V)", m.outputs[EnvFollower::GATE_L_OUTPUT].getVoltage());
		report(buf, m.outputs[EnvFollower::GATE_L_OUTPUT].getVoltage() < 0.1f);
	}

	std::printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
	return failures ? 1 : 0;
}
