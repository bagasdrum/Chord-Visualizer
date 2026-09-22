#include "ChordVisualizerController.h"
#include "ChordVisualizerView.h"
#include "ChordVisualizerIDs.h"
#include "pluginterfaces/base/ustring.h"

namespace Steinberg {
namespace Vst {

ChordVisualizerController::ChordVisualizerController() {}
ChordVisualizerController::~ChordVisualizerController() {}

tresult PLUGIN_API ChordVisualizerController::initialize(FUnknown* context) {
    tresult result = EditController::initialize(context);
    if (result != kResultOk) return result;

    // Register Bypass parameter
    parameters.addParameter(
        STR16("Bypass"),
        STR16(""),
        1,
        0.0,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
        kParamBypass
    );

    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerController::terminate() {
    return EditController::terminate();
}

IPlugView* PLUGIN_API ChordVisualizerController::createView(FIDString name) {
    if (strcmp(name, ViewType::kEditor) == 0) {
        return new ChordVisualizerView(this);
    }
    return nullptr;
}

tresult PLUGIN_API ChordVisualizerController::setComponentState(IBStream* state) {
    return kResultOk;
}

} // namespace Vst
} // namespace Steinberg
