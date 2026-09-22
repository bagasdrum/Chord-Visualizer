#include "ChordVisualizerProcessor.h"
#include "ChordVisualizerIDs.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Steinberg {
namespace Vst {

ChordVisualizerProcessor::ChordVisualizerProcessor() {
    setControllerClass(ChordVisualizerControllerUID);
}

ChordVisualizerProcessor::~ChordVisualizerProcessor() {}

tresult PLUGIN_API ChordVisualizerProcessor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;

    // 1. Audio Stereo Input (optional for FX) & Output (always stereo)
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

    // 2. MIDI Event Input (16 channels) & Output (16 channels)
    addEventInput(STR16("MIDI In"), 16, kMain, BusInfo::kDefaultActive);
    addEventOutput(STR16("MIDI Out"), 16, kMain, BusInfo::kDefaultActive);

    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerProcessor::terminate() {
    return AudioEffect::terminate();
}

tresult PLUGIN_API ChordVisualizerProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                              SpeakerArrangement* outputs, int32 numOuts) {
    // Output must be stereo
    if (numOuts != 1 || outputs[0] != SpeakerArr::kStereo) {
        return kResultFalse;
    }
    // Accept Generator / Instrument (0 audio inputs) in Channel Rack
    if (numIns == 0) {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    // Accept FX Slot / Analyzer (1 stereo or mono input) in Mixer Track
    if (numIns == 1 && (inputs[0] == SpeakerArr::kStereo || inputs[0] == SpeakerArr::kMono)) {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    return kResultFalse;
}

tresult PLUGIN_API ChordVisualizerProcessor::setupProcessing(ProcessSetup& newSetup) {
    mSampleRate = (newSetup.sampleRate > 8000.0) ? newSetup.sampleRate : 44100.0;
    return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API ChordVisualizerProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
        return kResultTrue;
    return kResultFalse;
}

tresult PLUGIN_API ChordVisualizerProcessor::setActive(TBool state) {
    if (!state) g_sharedState.clearAllNotes();
    return AudioEffect::setActive(state);
}

uint32 PLUGIN_API ChordVisualizerProcessor::getLatencySamples() {
    return 0; // Guaranteed zero latency
}

tresult PLUGIN_API ChordVisualizerProcessor::getControllerClassId(TUID classId) {
    memcpy(classId, ChordVisualizerControllerUID, sizeof(TUID));
    return kResultOk;
}

void ChordVisualizerProcessor::noteOn(int pitch, float velocity) {
    if (pitch < 0 || pitch >= 128) return;
    g_sharedState.triggerNoteOn(pitch, velocity);
}

void ChordVisualizerProcessor::noteOff(int pitch) {
    if (pitch < 0 || pitch >= 128) return;
    g_sharedState.triggerNoteOff(pitch);
}

tresult PLUGIN_API ChordVisualizerProcessor::process(ProcessData& data) {
    // ChordScope is an analyzer, not a synth. MIDI is observed from the VST3
    // event input and forwarded unchanged. Audio is transparent when an audio
    // input/output arrangement is provided by the host.
    if (data.inputParameterChanges) {
        int32 numParams = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParams; ++i) {
            IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue) continue;
            const int32 numPoints = queue->getPointCount();
            if (numPoints <= 0) continue;
            ParamValue value{};
            int32 sampleOffset{};
            if (queue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue &&
                queue->getParameterId() == kParamBypass) {
                mBypass = value >= 0.5;
                g_sharedState.isBypassed = mBypass;
            }
        }
    }

    if (data.inputEvents) {
        const int32 eventCount = data.inputEvents->getEventCount();
        for (int32 i = 0; i < eventCount; ++i) {
            Event event{};
            if (data.inputEvents->getEvent(i, event) != kResultOk) continue;

            if (event.type == Event::kNoteOnEvent) {
                if (event.noteOn.velocity > 0.0f) noteOn(event.noteOn.pitch, event.noteOn.velocity);
                else noteOff(event.noteOn.pitch);
            } else if (event.type == Event::kNoteOffEvent) {
                noteOff(event.noteOff.pitch);
            }

            if (data.outputEvents) data.outputEvents->addEvent(event);
        }
    }

    if (data.numOutputs > 0 && data.numSamples > 0 && data.outputs[0].channelBuffers32) {
        const int32 outChannels = data.outputs[0].numChannels;
        for (int32 ch = 0; ch < outChannels; ++ch) {
            float* out = data.outputs[0].channelBuffers32[ch];
            if (!out) continue;

            if (data.numInputs > 0 && data.inputs[0].channelBuffers32 &&
                ch < data.inputs[0].numChannels && data.inputs[0].channelBuffers32[ch]) {
                const float* in = data.inputs[0].channelBuffers32[ch];
                if (out != in) std::memcpy(out, in, static_cast<size_t>(data.numSamples) * sizeof(float));
            } else {
                std::memset(out, 0, static_cast<size_t>(data.numSamples) * sizeof(float));
            }
        }
    }

    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerProcessor::setState(IBStream* state) {
    if (!state) return kInvalidArgument;
    return kResultOk;
}

tresult PLUGIN_API ChordVisualizerProcessor::getState(IBStream* state) {
    if (!state) return kInvalidArgument;
    return kResultOk;
}

} // namespace Vst
} // namespace Steinberg
