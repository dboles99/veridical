#include "plugin.hpp"
#include "DelayLine.hpp"


struct Flanger : VeridicalModule {
	enum ParamId {
		RATE_PARAM,
		DEPTH_PARAM,
		FEEDBACK_PARAM,
		MIX_PARAM,
		RATE_ATT_PARAM,
		DEPTH_ATT_PARAM,
		FEEDBACK_ATT_PARAM,
		MIX_ATT_PARAM,
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

	DelayLine lines[2][PORT_MAX_CHANNELS];
	float lfoPhase[PORT_MAX_CHANNELS] = {};
	float shortest = 4.f;
	float longest = 480.f;

	Flanger() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, 0);
		configParam(RATE_PARAM, std::log2(0.02f), std::log2(10.f), std::log2(0.4f), "Rate", " Hz", 2.f);
		configParam(DEPTH_PARAM, 0.f, 1.f, 0.6f, "Depth", "%", 0.f, 100.f);
		configParam(FEEDBACK_PARAM, -1.f, 1.f, 0.35f, "Feedback", "%", 0.f, 100.f);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0.f, 100.f);

		const char* attNames[] = {"Rate CV", "Depth CV", "Feedback CV", "Mix CV"};
		for (int i = 0; i < 4; i++) {
			configParam(RATE_ATT_PARAM + i, -1.f, 1.f, 0.f, attNames[i], "%", 0.f, 100.f);
			getParamQuantity(RATE_ATT_PARAM + i)->randomizeEnabled = false;
		}

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

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		shortest = std::max(2.f, 0.0001f * e.sampleRate);
		longest = 0.010f * e.sampleRate;
		assert(longest > shortest);
		int capacity = (int) std::ceil(longest) + 4;
		for (int side = 0; side < 2; side++) {
			for (int c = 0; c < PORT_MAX_CHANNELS; c++)
				lines[side][c].setCapacity(capacity);
		}
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (int side = 0; side < 2; side++) {
			for (int c = 0; c < PORT_MAX_CHANNELS; c++)
				lines[side][c].clear();
		}
		for (int c = 0; c < PORT_MAX_CHANNELS; c++)
			lfoPhase[c] = 0.f;
	}

	// Scalar. Every channel and side reads its own ring buffer at its own
	// fractional index, which is a gather; float_4 would only help the
	// arithmetic either side of it and would cost the loads and stores to set
	// up.
	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels());
		channels = std::max(channels, 1);
		outputs[OUT_L_OUTPUT].setChannels(channels);
		outputs[OUT_R_OUTPUT].setChannels(channels);

		float span = longest - shortest;

		for (int c = 0; c < channels; c++) {
			float rate = modulated(params[RATE_PARAM], params[RATE_ATT_PARAM], inputs[RATE_INPUT], c, 10.f);
			rate = clamp(dsp::exp2_taylor5(rate), 0.005f, 20.f);
			float phase = lfoPhase[c] + rate * args.sampleTime;
			phase -= std::floor(phase);
			lfoPhase[c] = phase;

			float depth = clamp(modulated(params[DEPTH_PARAM], params[DEPTH_ATT_PARAM], inputs[DEPTH_INPUT], c, 1.f), 0.f, 1.f);
			float fb = clamp(modulated(params[FEEDBACK_PARAM], params[FEEDBACK_ATT_PARAM], inputs[FEEDBACK_INPUT], c, 2.f), -1.f, 1.f);
			float mix = clamp(modulated(params[MIX_PARAM], params[MIX_ATT_PARAM], inputs[MIX_INPUT], c, 1.f), 0.f, 1.f);

			float g = 0.85f * fb;
			// A comb resonates to 1/(1-|g|) times its input, which at the top
			// of the range is thirty volts. Trimming what goes into the loop
			// holds the peak near 2x instead. Nothing here clips, so nothing
			// here aliases.
			float trim = std::min(1.f, 2.2f * (1.f - std::fabs(g)));

			float left = inputs[IN_L_INPUT].getPolyVoltage(c);
			float in[2] = {left, inputs[IN_R_INPUT].getNormalPolyVoltage(left, c)};

			for (int side = 0; side < 2; side++) {
				// A quarter cycle between the sides is what sends the sweep
				// across the stereo field instead of leaving it in the middle.
				float p = phase + 0.25f * side;
				p -= std::floor(p);
				float sweep = 0.5f - 0.5f * std::cos(2.f * M_PI * p);

				DelayLine& line = lines[side][c];
				float wet = line.readCubic(shortest + span * depth * sweep);
				line.write(clamp(in[side] * trim + g * wet, -60.f, 60.f));
				outputs[OUT_L_OUTPUT + side].setVoltage(crossfade(in[side], wet, mix), c);
			}
		}
	}
};


struct FlangerWidget : VeridicalWidget {
	FlangerWidget(Flanger* module) {
		setModule(module);
		loadPanels("Flanger");

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 24.0)), module, Flanger::RATE_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 24.0)), module, Flanger::RATE_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 24.0)), module, Flanger::RATE_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 46.0)), module, Flanger::DEPTH_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 46.0)), module, Flanger::DEPTH_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 46.0)), module, Flanger::DEPTH_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 68.0)), module, Flanger::FEEDBACK_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 68.0)), module, Flanger::FEEDBACK_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 68.0)), module, Flanger::FEEDBACK_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 90.0)), module, Flanger::MIX_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 90.0)), module, Flanger::MIX_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 90.0)), module, Flanger::MIX_INPUT));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(8.315, 113.0)), module, Flanger::IN_L_INPUT));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(19.705, 113.0)), module, Flanger::IN_R_INPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(31.095, 113.0)), module, Flanger::OUT_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(42.485, 113.0)), module, Flanger::OUT_R_OUTPUT));
	}
};


Model* modelFlanger = createModel<Flanger, FlangerWidget>("Flanger");
