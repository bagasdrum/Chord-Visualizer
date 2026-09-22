#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Steinberg {
namespace Vst {

enum ChordVisualizerParamTags {
    kParamBypass = 100,
    kParamOctaveRange = 101,
    kParamBaseOctave = 102
};

// Processor CID: {4B8D1056-9A2E-4F61-8B34-5C9E21471001}
static DECLARE_UID(ChordVisualizerProcessorUID, 0x4B8D1056, 0x9A2E4F61, 0x8B345C9E, 0x21471001);

// Controller CID: {4B8D1056-9A2E-4F61-8B34-5C9E21471002}
static DECLARE_UID(ChordVisualizerControllerUID, 0x4B8D1056, 0x9A2E4F61, 0x8B345C9E, 0x21471002);

} // namespace Vst
} // namespace Steinberg
