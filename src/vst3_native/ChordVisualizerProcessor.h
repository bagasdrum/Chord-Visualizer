#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "ChordVisualizerShared.h"

namespace Steinberg {
namespace Vst {

class ChordVisualizerProcessor : public AudioEffect {
public:
    ChordVisualizerProcessor();
    ~ChordVisualizerProcessor() override;

    static FUnknown* createInstance(void*) {
        return (IAudioProcessor*)new ChordVisualizerProcessor();
    }

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                          SpeakerArrangement* outputs, int32 numOuts) override;
    tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) override;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API process(ProcessData& data) override;
    uint32 PLUGIN_API getLatencySamples() override;
    tresult PLUGIN_API getControllerClassId(TUID classId) override;

    tresult PLUGIN_API setState(IBStream* state) override;
    tresult PLUGIN_API getState(IBStream* state) override;

private:
    void noteOn(int pitch, float velocity);
    void noteOff(int pitch);

    bool mBypass{false};
    double mSampleRate{44100.0};
};

} // namespace Vst
} // namespace Steinberg
