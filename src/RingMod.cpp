#include "plugin.hpp"

using simd::float_4;


struct RingMod : VeridicalModule {
	enum ParamId {
		FREQ_PARAM,
		MIX_PARAM,
		FREQ_ATT_PARAM,
		MIX_ATT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FREQ_INPUT,
		MIX_INPUT,
		CARRIER_INPUT,
		IN_L_INPUT,
		IN_R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_L_OUTPUT,
		OUT_R_OUTPUT,
		OUTPUTS_LEN
	};

	float_4 oscPhase[PORT_MAX_CHANNELS / 4] = {};

	RingMod() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, 0);
		// Stopping at 4 kHz keeps the sum and difference products of ordinary
		// audio material inside the band most of the time. Above that a ring
		// modulator folds, and always has.
		configParam(FREQ_PARAM, std::log2(0.1f), std::log2(4000.f), std::log2(200.f), "Frequency", " Hz", 2.f);
		configParam(MIX_PARAM, 0.f, 1.f, 1.f, "Mix", "%", 0.f, 100.f);
		configParam(FREQ_ATT_PARAM, -1.f, 1.f, 0.f, "Frequency CV", "%", 0.f, 100.f);
		configParam(MIX_ATT_PARAM, -1.f, 1.f, 0.f, "Mix CV", "%", 0.f, 100.f);
		getParamQuantity(FREQ_ATT_PARAM)->randomizeEnabled = false;
		getParamQuantity(MIX_ATT_PARAM)->randomizeEnabled = false;

		configInput(FREQ_INPUT, "Frequency");
		getInputInfo(FREQ_INPUT)->description = "1V/octave at a fully open attenuverter";
		configInput(MIX_INPUT, "Mix");
		configInput(CARRIER_INPUT, "Carrier");
		getInputInfo(CARRIER_INPUT)->description = "Replaces the internal oscillator while patched";
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
		for (int g = 0; g < PORT_MAX_CHANNELS / 4; g++)
			oscPhase[g] = 0.f;
	}

	// Vectorised: a multiply and a crossfade per channel, and the internal
	// carrier is one phase accumulator that float_4 handles as easily as float.
	// Both sides share the carrier, so the module widens nothing that was not
	// already wide.
	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels());
		channels = std::max(channels, inputs[CARRIER_INPUT].getChannels());
		channels = std::max(channels, 1);
		outputs[OUT_L_OUTPUT].setChannels(channels);
		outputs[OUT_R_OUTPUT].setChannels(channels);

		bool external = inputs[CARRIER_INPUT].isConnected();
		float nyquist = 0.45f * args.sampleRate;

		for (int c = 0; c < channels; c += 4) {
			int g = c / 4;
			float_4 freq = modulated4(params[FREQ_PARAM], params[FREQ_ATT_PARAM], inputs[FREQ_INPUT], c, 10.f);
			freq = simd::fmin(dsp::exp2_taylor5(freq), nyquist);
			float_4 phase = oscPhase[g] + freq * args.sampleTime;
			phase -= simd::floor(phase);
			oscPhase[g] = phase;

			float_4 carrier = external
				? inputs[CARRIER_INPUT].getPolyVoltageSimd<float_4>(c) * 0.2f
				: simd::sin(2.f * float(M_PI) * phase);

			float_4 mix = simd::clamp(modulated4(params[MIX_PARAM], params[MIX_ATT_PARAM], inputs[MIX_INPUT], c, 1.f), 0.f, 1.f);

			float_4 left = inputs[IN_L_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 in[2] = {left, inputs[IN_R_INPUT].getNormalPolyVoltageSimd<float_4>(left, c)};
			for (int side = 0; side < 2; side++)
				simd::crossfade(in[side], in[side] * carrier, mix).store(outputs[OUT_L_OUTPUT + side].getVoltages(c));
		}
	}
};


struct RingModWidget : VeridicalWidget {
	RingModWidget(RingMod* module) {
		setModule(module);
		loadPanels("RingMod");

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 30.0)), module, RingMod::FREQ_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 30.0)), module, RingMod::FREQ_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 30.0)), module, RingMod::FREQ_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 56.0)), module, RingMod::MIX_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 56.0)), module, RingMod::MIX_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 56.0)), module, RingMod::MIX_INPUT));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 82.0)), module, RingMod::CARRIER_INPUT));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(8.315, 113.0)), module, RingMod::IN_L_INPUT));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(19.705, 113.0)), module, RingMod::IN_R_INPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(31.095, 113.0)), module, RingMod::OUT_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(42.485, 113.0)), module, RingMod::OUT_R_OUTPUT));
	}
};


Model* modelRingMod = createModel<RingMod, RingModWidget>("RingMod");
