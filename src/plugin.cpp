#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelFlanger);
	p->addModel(modelChorus);
	p->addModel(modelPhaser);
	p->addModel(modelRingMod);
	p->addModel(modelEnvFollower);
}
