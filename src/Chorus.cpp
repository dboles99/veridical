#include "plugin.hpp"
#include "DelayLine.hpp"


struct Chorus : VeridicalModule {
	enum ParamId {
		RATE_PARAM,
		DEPTH_PARAM,
		MIX_PARAM,
		RATE_ATT_PARAM,
		DEPTH_ATT_PARAM,
		MIX_ATT_PARAM,
		VOICES_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RATE_INPUT,
		DEPTH_INPUT,
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
	float centre = 960.f;
	float swing = 480.f;

	Chorus() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, 0);
		configParam(RATE_PARAM, std::log2(0.02f), std::log2(5.f), std::log2(0.6f), "Rate", " Hz", 2.f);
		configParam(DEPTH_PARAM, 0.f, 1.f, 0.5f, "Depth", "%", 0.f, 100.f);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0.f, 100.f);
		configParam(RATE_ATT_PARAM, -1.f, 1.f, 0.f, "Rate CV", "%", 0.f, 100.f);
		configParam(DEPTH_ATT_PARAM, -1.f, 1.f, 0.f, "Depth CV", "%", 0.f, 100.f);
		configParam(MIX_ATT_PARAM, -1.f, 1.f, 0.f, "Mix CV", "%", 0.f, 100.f);
		getParamQuantity(RATE_ATT_PARAM)->randomizeEnabled = false;
		getParamQuantity(DEPTH_ATT_PARAM)->randomizeEnabled = false;
		getParamQuantity(MIX_ATT_PARAM)->randomizeEnabled = false;
		configSwitch(VOICES_PARAM, 0.f, 2.f, 1.f, "Voices", {"2", "3", "4"});

		configInput(RATE_INPUT, "Rate");
		getInputInfo(RATE_INPUT)->description = "1V/octave at a fully open attenuverter";
		configInput(DEPTH_INPUT, "Depth");
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
		// 20 ms nominal, plus or minus 10 ms at full depth.
		centre = 0.020f * e.sampleRate;
		swing = 0.010f * e.sampleRate;
		int capacity = (int) std::ceil(centre + swing) + 4;
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

	// Scalar for the same reason as the Flanger: the taps are gathers. Here
	// there are two to four of them per side, so it is even less worth
	// pretending they are a vector.
	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels());
		channels = std::max(channels, 1);
		outputs[OUT_L_OUTPUT].setChannels(channels);
		outputs[OUT_R_OUTPUT].setChannels(channels);

		int voices = 2 + (int) params[VOICES_PARAM].getValue();
		float voiceSpacing = 1.f / voices;

		for (int c = 0; c < channels; c++) {
			float rate = modulated(params[RATE_PARAM], params[RATE_ATT_PARAM], inputs[RATE_INPUT], c, 10.f);
			rate = clamp(dsp::exp2_taylor5(rate), 0.005f, 20.f);
			float phase = lfoPhase[c] + rate * args.sampleTime;
			phase -= std::floor(phase);
			lfoPhase[c] = phase;

			float depth = clamp(modulated(params[DEPTH_PARAM], params[DEPTH_ATT_PARAM], inputs[DEPTH_INPUT], c, 1.f), 0.f, 1.f);
			float mix = clamp(modulated(params[MIX_PARAM], params[MIX_ATT_PARAM], inputs[MIX_INPUT], c, 1.f), 0.f, 1.f);
			float excursion = swing * depth;

			float left = inputs[IN_L_INPUT].getPolyVoltage(c);
			float in[2] = {left, inputs[IN_R_INPUT].getNormalPolyVoltage(left, c)};

			for (int side = 0; side < 2; side++) {
				DelayLine& line = lines[side][c];
				line.write(in[side]);

				float wet = 0.f;
				for (int v = 0; v < voices; v++) {
					// Voices are spread around the cycle, and the right side
					// sits a quarter turn behind the left.
					float p = phase + v * voiceSpacing + 0.25f * side;
					wet += line.readLinear(centre + excursion * std::sin(2.f * M_PI * p));
				}
				wet /= voices;

				outputs[OUT_L_OUTPUT + side].setVoltage(crossfade(in[side], wet, mix), c);
			}
		}
	}
};


struct ChorusWidget : VeridicalWidget {
	ChorusWidget(Chorus* module) {
		setModule(module);
		loadPanels("Chorus");

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 24.0)), module, Chorus::RATE_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 24.0)), module, Chorus::RATE_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 24.0)), module, Chorus::RATE_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 46.0)), module, Chorus::DEPTH_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 46.0)), module, Chorus::DEPTH_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 46.0)), module, Chorus::DEPTH_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 68.0)), module, Chorus::MIX_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 68.0)), module, Chorus::MIX_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 68.0)), module, Chorus::MIX_INPUT));

		addParam(createParamCentered<VeridicalSwitch>(mm2px(Vec(11.8, 90.0)), module, Chorus::VOICES_PARAM));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(8.315, 113.0)), module, Chorus::IN_L_INPUT));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(19.705, 113.0)), module, Chorus::IN_R_INPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(31.095, 113.0)), module, Chorus::OUT_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(42.485, 113.0)), module, Chorus::OUT_R_OUTPUT));
	}
};


Model* modelChorus = createModel<Chorus, ChorusWidget>("Chorus");
