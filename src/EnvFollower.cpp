#include "plugin.hpp"

using simd::float_4;


struct EnvFollower : VeridicalModule {
	enum ParamId {
		ATTACK_PARAM,
		RELEASE_PARAM,
		GAIN_PARAM,
		ATTACK_ATT_PARAM,
		RELEASE_ATT_PARAM,
		GAIN_ATT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ATTACK_INPUT,
		RELEASE_INPUT,
		GAIN_INPUT,
		IN_L_INPUT,
		IN_R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENV_L_OUTPUT,
		ENV_R_OUTPUT,
		GATE_L_OUTPUT,
		GATE_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENV_LIGHT,
		LIGHTS_LEN
	};

	static const int GROUPS = PORT_MAX_CHANNELS / 4;

	float_4 env[2][GROUPS] = {};
	float_4 gate[2][GROUPS] = {};
	dsp::ClockDivider lightDivider;

	EnvFollower() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ATTACK_PARAM, std::log2(0.0001f), std::log2(1.f), std::log2(0.005f), "Attack", " s", 2.f);
		configParam(RELEASE_PARAM, std::log2(0.001f), std::log2(5.f), std::log2(0.15f), "Release", " s", 2.f);
		configParam(GAIN_PARAM, 0.f, 4.f, 1.f, "Gain", "x");
		configParam(ATTACK_ATT_PARAM, -1.f, 1.f, 0.f, "Attack CV", "%", 0.f, 100.f);
		configParam(RELEASE_ATT_PARAM, -1.f, 1.f, 0.f, "Release CV", "%", 0.f, 100.f);
		configParam(GAIN_ATT_PARAM, -1.f, 1.f, 0.f, "Gain CV", "%", 0.f, 100.f);
		for (int i = 0; i < 3; i++)
			getParamQuantity(ATTACK_ATT_PARAM + i)->randomizeEnabled = false;

		configInput(ATTACK_INPUT, "Attack");
		configInput(RELEASE_INPUT, "Release");
		configInput(GAIN_INPUT, "Gain");
		configInput(IN_L_INPUT, "Left audio");
		configInput(IN_R_INPUT, "Right audio");
		getInputInfo(IN_R_INPUT)->description = "Normalled to the left input";
		configOutput(ENV_L_OUTPUT, "Left envelope");
		getOutputInfo(ENV_L_OUTPUT)->description = "0 to 10V; reaches 10V on a 10Vpp input at unity gain";
		configOutput(ENV_R_OUTPUT, "Right envelope");
		configOutput(GATE_L_OUTPUT, "Left gate");
		configOutput(GATE_R_OUTPUT, "Right gate");
		configLight(ENV_LIGHT, "Envelope");
		// No configBypass. The outputs measure the input rather than carrying a
		// treated copy of it, so there is nothing sensible to pass through.

		lightDivider.setDivision(32);
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		for (int side = 0; side < 2; side++) {
			for (int g = 0; g < GROUPS; g++) {
				env[side][g] = 0.f;
				gate[side][g] = 0.f;
			}
		}
	}

	// Vectorised. The attack/release branch is a select rather than a jump, so
	// all four channels take the same path through the code no matter which way
	// each one is moving.
	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels());
		channels = std::max(channels, 1);
		for (int i = 0; i < OUTPUTS_LEN; i++)
			outputs[i].setChannels(channels);

		for (int c = 0; c < channels; c += 4) {
			int g = c / 4;

			float_4 attack = modulated4(params[ATTACK_PARAM], params[ATTACK_ATT_PARAM], inputs[ATTACK_INPUT], c, 10.f);
			float_4 release = modulated4(params[RELEASE_PARAM], params[RELEASE_ATT_PARAM], inputs[RELEASE_INPUT], c, 10.f);
			attack = simd::clamp(dsp::exp2_taylor5(attack), 5e-5f, 10.f);
			release = simd::clamp(dsp::exp2_taylor5(release), 5e-5f, 10.f);
			float_4 rising = 1.f - simd::exp(-args.sampleTime / attack);
			float_4 falling = 1.f - simd::exp(-args.sampleTime / release);
			float_4 gain = simd::clamp(modulated4(params[GAIN_PARAM], params[GAIN_ATT_PARAM], inputs[GAIN_INPUT], c, 4.f), 0.f, 4.f);

			float_4 left = inputs[IN_L_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 in[2] = {left, inputs[IN_R_INPUT].getNormalPolyVoltageSimd<float_4>(left, c)};

			for (int side = 0; side < 2; side++) {
				float_4 rectified = simd::fabs(in[side]) * gain;
				float_4 e = env[side][g];
				e += (rectified - e) * simd::ifelse(rectified > e, rising, falling);
				env[side][g] = e;

				float_4 level = simd::clamp(2.f * e, 0.f, 10.f);
				level.store(outputs[ENV_L_OUTPUT + side].getVoltages(c));

				gate[side][g] = simd::ifelse(level > 1.f, float_4(10.f),
					simd::ifelse(level < 0.5f, float_4(0.f), gate[side][g]));
				gate[side][g].store(outputs[GATE_L_OUTPUT + side].getVoltages(c));
			}
		}

		if (lightDivider.process()) {
			float dt = args.sampleTime * lightDivider.getDivision();
			lights[ENV_LIGHT].setBrightnessSmooth(outputs[ENV_L_OUTPUT].getVoltage() / 10.f, dt);
		}
	}
};


struct EnvFollowerWidget : VeridicalWidget {
	EnvFollowerWidget(EnvFollower* module) {
		setModule(module);
		loadPanels("EnvFollower");

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 24.0)), module, EnvFollower::ATTACK_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 24.0)), module, EnvFollower::ATTACK_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 24.0)), module, EnvFollower::ATTACK_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 46.0)), module, EnvFollower::RELEASE_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 46.0)), module, EnvFollower::RELEASE_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 46.0)), module, EnvFollower::RELEASE_INPUT));

		addParam(createParamCentered<VeridicalKnob>(mm2px(Vec(11.8, 68.0)), module, EnvFollower::GAIN_PARAM));
		addParam(createParamCentered<VeridicalTrim>(mm2px(Vec(25.4, 68.0)), module, EnvFollower::GAIN_ATT_PARAM));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(39.0, 68.0)), module, EnvFollower::GAIN_INPUT));

		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(19.705, 91.0)), module, EnvFollower::IN_L_INPUT));
		addInput(createInputCentered<VeridicalPort>(mm2px(Vec(31.095, 91.0)), module, EnvFollower::IN_R_INPUT));

		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(14.01, 101.0)), module, EnvFollower::ENV_LIGHT));

		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(8.315, 113.0)), module, EnvFollower::ENV_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(19.705, 113.0)), module, EnvFollower::ENV_R_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(31.095, 113.0)), module, EnvFollower::GATE_L_OUTPUT));
		addOutput(createOutputCentered<VeridicalPort>(mm2px(Vec(42.485, 113.0)), module, EnvFollower::GATE_R_OUTPUT));
	}
};


Model* modelEnvFollower = createModel<EnvFollower, EnvFollowerWidget>("EnvFollower");
