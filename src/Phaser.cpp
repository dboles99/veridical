#include "plugin.hpp"

using simd::float_4;


struct Phaser : VeridicalModule {
	enum ParamId {
		RATE_PARAM,
		DEPTH_PARAM,
		FEEDBACK_PARAM,
		MIX_PARAM,
		RATE_ATT_PARAM,
		DEPTH_ATT_PARAM,
		FEEDBACK_ATT_PARAM,
		MIX_ATT_PARAM,
		STAGES_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RATE_INPUT,
		DEPTH_INPUT,
		FEEDBACK_INPUT,
		MIX_INPUT,
		IN_L_INPUT,
		IN_R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_L_OUTPUT,
		OUT_R_OUTPUT,
		OUTPUTS_LEN
	};

	static const int MAX_STAGES = 8;
	static const int GROUPS = PORT_MAX_CHANNELS / 4;

	float_4 allpass[2][MAX_STAGES][GROUPS] = {};
	float_4 recycled[2][GROUPS] = {};
	float_4 lfoPhase[GROUPS] = {};

	Phaser() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, 0);
		configParam(RATE_PARAM, std::log2(0.02f), std::log2(8.f), std::log2(0.3f), "Rate", " Hz", 2.f);
		configParam(DEPTH_PARAM, 0.f, 1.f, 0.7f, "Depth", "%", 0.f, 100.f);
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.4f, "Feedback", "%", 0.f, 100.f);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0.f, 100.f);
		configParam(RATE_ATT_PARAM, -1.f, 1.f, 0.f, "Rate CV", "%", 0.f, 100.f);
		configParam(DEPTH_ATT_PARAM, -1.f, 1.f, 0.f, "Depth CV", "%", 0.f, 100.f);
		configParam(FEEDBACK_ATT_PARAM, -1.f, 1.f, 0.f, "Feedback CV", "%", 0.f, 100.f);
		configParam(MIX_ATT_PARAM, -1.f, 1.f, 0.f, "Mix CV", "%", 0.f, 100.f);
		for (int i = 0; i < 4; i++)
			getParamQuantity(RATE_ATT_PARAM + i)->randomizeEnabled = false;
		configSwitch(STAGES_PARAM, 0.f, 2.f, 1.f, "Stages", {"4", "6", "8"});

		configInput(RATE_INPUT, "Rate");
		getInputInfo(RATE_INPUT)->description = "1V/octave at a fully open attenuverter";
		configInput(DEPTH_INPUT, "Depth");
		configInput(FEEDBACK_INPUT, "Feedback");
		configInput(MIX_INPUT, "Mix");
		configInput(IN_L_INPUT, "Left audio");
		configInput(IN_R_INPUT, "Right audio");
		getInputInfo(IN_R_INPUT)->description = "Normalled to the left input";
		configOutput(OUT_L_OUTPUT, "Left audio");
		configOutput(OUT_R_OUTPUT, "Right audio");
		configBypass(IN_L_INPUT, OUT_L_OUTPUT);
		configBypass(IN_R_INPUT, OUT_R_OUTPUT);
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (int side = 0; side < 2; side++) {
			for (int g = 0; g < GROUPS; g++) {
				for (int s = 0; s < MAX_STAGES; s++)
					allpass[side][s][g] = 0.f;
				recycled[side][g] = 0.f;
			}
		}
		for (int g = 0; g < GROUPS; g++)
			lfoPhase[g] = 0.f;
	}

	// Vectorised. The allpass cascade is the same two-line recurrence on every
	// channel with nothing but the coefficient differing, so four channels ride
	// through it together with no shuffling.
	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels());
		channels = std::max(channels, 1);
		outputs[OUT_L_OUTPUT].setChannels(channels);
		outputs[OUT_R_OUTPUT].setChannels(channels);

		int stages = 4 + 2 * (int) params[STAGES_PARAM].getValue();
		assert(stages <= MAX_STAGES);
		float ceiling = 0.45f * args.sampleRate;

		for (int c = 0; c < channels; c += 4) {
			int g = c / 4;

			float_4 rate = modulated4(params[RATE_PARAM], params[RATE_ATT_PARAM], inputs[RATE_INPUT], c, 10.f);
			rate = simd::clamp(dsp::exp2_taylor5(rate), 0.005f, 20.f);
			float_4 phase = lfoPhase[g] + rate * args.sampleTime;
			phase -= simd::floor(phase);
			lfoPhase[g] = phase;

			float_4 depth = simd::clamp(modulated4(params[DEPTH_PARAM], params[DEPTH_ATT_PARAM], inputs[DEPTH_INPUT], c, 1.f), 0.f, 1.f);
			float_4 fb = simd::clamp(modulated4(params[FEEDBACK_PARAM], params[FEEDBACK_ATT_PARAM], inputs[FEEDBACK_INPUT], c, 1.f), 0.f, 1.f);
			float_4 mix = simd::clamp(modulated4(params[MIX_PARAM], params[MIX_ATT_PARAM], inputs[MIX_INPUT], c, 1.f), 0.f, 1.f);

			// An allpass chain has unity magnitude everywhere, so the loop
			// resonates to 1/(1-k) with nothing to damp it. Trim the input to
			// hold that near 2x; below about half feedback the trim is 1 and
			// the module is transparent.
			float_4 k = 0.85f * fb;
			float_4 trim = simd::fmin(float_4(1.f), 2.2f * (1.f - k));

			float_4 left = inputs[IN_L_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 in[2] = {left, inputs[IN_R_INPUT].getNormalPolyVoltageSimd<float_4>(left, c)};

			for (int side = 0; side < 2; side++) {
				float_4 p = phase + 0.25f * side;
				p -= simd::floor(p);
				float_4 sweep = 0.5f - 0.5f * simd::cos(2.f * float(M_PI) * p);
				// Six octaves up from 120 Hz puts the top of the sweep near
				// 7.7 kHz, which is where a phaser stops sounding like one.
				float_4 corner = simd::fmin(120.f * dsp::exp2_taylor5(6.f * depth * sweep), ceiling);
				// Bilinear transform of (w - s)/(w + s). The coefficient is
				// negative, which puts the pole near DC; get the sign the
				// wrong way round and the section shifts phase up at Nyquist
				// instead of at the corner, which sounds like almost nothing.
				float_4 t = simd::tan(float(M_PI) * corner * args.sampleTime);
				float_4 a = (t - 1.f) / (t + 1.f);

				float_4 x = in[side] * trim + k * recycled[side][g];
				for (int s = 0; s < stages; s++) {
					float_4 y = a * x + allpass[side][s][g];
					allpass[side][s][g] = x - a * y;
					x = y;
				}
				recycled[side][g] = simd::clamp(x, -60.f, 60.f);

				simd::crossfade(in[side], x, mix).store(outputs[OUT_L_OUTPUT + side].getVoltages(c));
			}
		}
	}
};


struct PhaserWidget : VeridicalWidget {
	PhaserWidget(Phaser* module) {
		setModule(module);
		loadPanels("Phaser");

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 24.0)), module, Phaser::RATE_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 24.0)), module, Phaser::RATE_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 24.0)), module, Phaser::RATE_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 41.0)), module, Phaser::DEPTH_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 41.0)), module, Phaser::DEPTH_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 41.0)), module, Phaser::DEPTH_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 58.0)), module, Phaser::FEEDBACK_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 58.0)), module, Phaser::FEEDBACK_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 58.0)), module, Phaser::FEEDBACK_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 75.0)), module, Phaser::MIX_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 75.0)), module, Phaser::MIX_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 75.0)), module, Phaser::MIX_INPUT));

		addParam(createParamCentered<VeridicalSwitch>(mm2px(Vec(11.8, 92.0)), module, Phaser::STAGES_PARAM));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(8.315, 113.0)), module, Phaser::IN_L_INPUT));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(19.705, 113.0)), module, Phaser::IN_R_INPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(31.095, 113.0)), module, Phaser::OUT_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(42.485, 113.0)), module, Phaser::OUT_R_OUTPUT));
	}
};


Model* modelPhaser = createModel<Phaser, PhaserWidget>("Phaser");
