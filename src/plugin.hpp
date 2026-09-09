#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;

extern Model* modelFlanger;
extern Model* modelChorus;
extern Model* modelPhaser;
extern Model* modelRingMod;
extern Model* modelEnvFollower;


/** Knob plus attenuverted CV. Ten volts through a fully open attenuverter
moves the result by `range` in the knob's own units, so a 0..1 knob takes
range 1 and a bipolar one takes 2. Callers clamp, because the sensible limit
differs from one parameter to the next.
*/
inline float modulated(Param& knob, Param& atten, Input& cv, int channel, float range) {
	return knob.getValue() + atten.getValue() * cv.getPolyVoltage(channel) * range * 0.1f;
}

/** Four channels at a time, for the modules whose inner loop is vectorised. */
inline simd::float_4 modulated4(Param& knob, Param& atten, Input& cv, int channel, float range) {
	return knob.getValue() + atten.getValue() * cv.getPolyVoltageSimd<simd::float_4>(channel) * (range * 0.1f);
}


/** Panel choice is per module instance, so it lives on the Module and gets
serialised with the patch. Rack 2 has no per-module theming of its own.
*/
struct VeridicalModule : Module {
	enum Theme {
		DARK_THEME,
		LIGHT_THEME
	};

	int theme = DARK_THEME;

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "theme", json_integer(theme));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* themeJ = json_object_get(rootJ, "theme");
		if (themeJ)
			theme = clamp((int) json_integer_value(themeJ), 0, 1);
	}
};


struct VeridicalWidget : ModuleWidget {
	SvgPanel* svgPanel = NULL;
	std::shared_ptr<window::Svg> panelSvg[2];
	int shownTheme = -1;

	void loadPanels(const std::string& name) {
		panelSvg[VeridicalModule::DARK_THEME] = window::Svg::load(asset::plugin(pluginInstance, "res/" + name + "-dark.svg"));
		panelSvg[VeridicalModule::LIGHT_THEME] = window::Svg::load(asset::plugin(pluginInstance, "res/" + name + "-light.svg"));
		svgPanel = new SvgPanel;
		svgPanel->setBackground(panelSvg[VeridicalModule::DARK_THEME]);
		shownTheme = VeridicalModule::DARK_THEME;
		setPanel(svgPanel);
	}

	void step() override {
		VeridicalModule* m = dynamic_cast<VeridicalModule*>(module);
		int wanted = m ? m->theme : VeridicalModule::DARK_THEME;
		if (wanted != shownTheme) {
			shownTheme = wanted;
			svgPanel->setBackground(panelSvg[wanted]);
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		VeridicalModule* m = dynamic_cast<VeridicalModule*>(module);
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexPtrSubmenuItem("Panel", {"Dark", "Light"}, &m->theme));
	}
};


/** The knobs, jacks and switch are drawn in res/ rather than taken from Rack's
component library, so one set of artwork has to sit on both panel colours.
*/
struct VeridicalKnob : app::SvgKnob {
	VeridicalKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		shadow->opacity = 0.f;
		setSvg(window::Svg::load(asset::plugin(pluginInstance, "res/Knob.svg")));
	}
};

struct VeridicalTrim : app::SvgKnob {
	VeridicalTrim() {
		minAngle = -0.75f * M_PI;
		maxAngle = 0.75f * M_PI;
		shadow->opacity = 0.f;
		setSvg(window::Svg::load(asset::plugin(pluginInstance, "res/KnobSmall.svg")));
	}
};

struct VeridicalPort : app::SvgPort {
	VeridicalPort() {
		shadow->opacity = 0.f;
		setSvg(window::Svg::load(asset::plugin(pluginInstance, "res/Port.svg")));
	}
};

struct VeridicalSwitch : app::SvgSwitch {
	VeridicalSwitch() {
		shadow->opacity = 0.f;
		addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/Switch3_0.svg")));
		addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/Switch3_1.svg")));
		addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/Switch3_2.svg")));
	}
};
