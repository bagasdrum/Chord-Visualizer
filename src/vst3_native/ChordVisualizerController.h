#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "ChordVisualizerShared.h"

namespace Steinberg {
namespace Vst {

class ChordVisualizerController : public EditController {
public:
    ChordVisualizerController();
    ~ChordVisualizerController() override;

    static FUnknown* createInstance(void*) {
        return (IEditController*)new ChordVisualizerController();
    }

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;
    IPlugView* PLUGIN_API createView(FIDString name) override;
    tresult PLUGIN_API setComponentState(IBStream* state) override;
};

} // namespace Vst
} // namespace Steinberg
