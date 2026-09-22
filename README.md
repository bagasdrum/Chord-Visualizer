# ChordScope

Minimal Windows x64 VST3 MIDI chord visualizer for FL Studio.

## Design goals
- MIDI chord detection only; no audio analysis and no synthesizer.
- Very small native Win32/GDI editor: text display + 12 test-note buttons.
- Test-note clicks automatically release after 3 seconds.
- MIDI notes received from the VST3 Event Input follow normal Note On/Note Off and are not timed out by the UI test timer.
- Stereo audio is passed through when the host provides an audio input/output arrangement.
- One plugin instance is the intended use case.
- No React, WebView2, Web Audio, Gemini, Node.js, or frontend build is required.

## Build
The GitHub Actions workflow builds the Windows x64 VST3 on a Windows runner using CMake and the Steinberg VST3 SDK.

## Important FL Studio note
A VST3 audio/Mixer routing path does not automatically guarantee that MIDI Event Input is delivered to the plugin. The final integration must be tested in FL Studio with the exact MIDI routing used by the project.
