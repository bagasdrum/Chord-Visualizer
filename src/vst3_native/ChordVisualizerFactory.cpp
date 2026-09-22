#include "ChordVisualizerProcessor.h"
#include "ChordVisualizerController.h"
#include "ChordVisualizerIDs.h"
#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "ChordScope"

BEGIN_FACTORY_DEF("ChordScope",
                  "https://github.com/",
                  "")

    // Audio Effect & Instrument Component
    // Category "Fx|Instrument|Analyzer|Tools" allows loading in BOTH:
    // 1) FL Studio Channel Rack (as Instrument / Generator with direct keyboard play)
    // 2) FL Studio Mixer Track (as Audio FX Slot with 16-ch MIDI In/Out analyzer)
    DEF_CLASS2(INLINE_UID(0x4B8D1056, 0x9A2E4F61, 0x8B345C9E, 0x21471001),
               Steinberg::PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               stringPluginName,
               Steinberg::Vst::kDistributable,
               "Fx|Instrument|Analyzer|Tools",
               "1.0.0",
               kVstVersionString,
               Steinberg::Vst::ChordVisualizerProcessor::createInstance)

    // Edit Controller Component
    DEF_CLASS2(INLINE_UID(0x4B8D1056, 0x9A2E4F61, 0x8B345C9E, 0x21471002),
               Steinberg::PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               stringPluginName " Controller",
               0,
               "",
               "1.0.0",
               kVstVersionString,
               Steinberg::Vst::ChordVisualizerController::createInstance)

END_FACTORY
