#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

namespace Steinberg {
namespace Vst {

static const char* const kNoteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

struct ChordPatternDef {
    uint32_t mask;
    const char* symbol;
    const char* quality;
    const char* formula;
    int priority;
};

static const ChordPatternDef kChordPatterns[] = {
    // 6-7 notes
    { 0b101011010101, "13", "Dominant Thirteenth", "1 - 3 - 5 - b7 - 9 - 13", 100 },
    { 0b011000010101, "13", "Dominant Thirteenth (no5)", "1 - 3 - b7 - 9 - 13", 98 },
    { 0b011000010001, "13", "Dominant Thirteenth (Shell)", "1 - 3 - b7 - 13", 97 },
    { 0b101010110101, "m11", "Minor Eleventh", "1 - b3 - 5 - b7 - 9 - 11", 95 },
    { 0b101001010101, "11", "Dominant Eleventh", "1 - 3 - 5 - b7 - 9 - 11", 95 },
    // 5 notes
    { 0b001010010101, "6/9", "Six Nine", "1 - 3 - 5 - 6 - 9", 90 },
    { 0b100010010101, "maj9", "Major Ninth", "1 - 3 - 5 - 7 - 9", 90 },
    { 0b010010010101, "9", "Dominant Ninth", "1 - 3 - 5 - b7 - 9", 90 },
    { 0b010010100101, "m9", "Minor Ninth", "1 - b3 - 5 - b7 - 9", 90 },
    { 0b010010011001, "7#9", "Hendrix Altered (7#9)", "1 - 3 - 5 - b7 - #9", 92 },
    { 0b010010010011, "7b9", "Dominant Minor Ninth", "1 - 3 - 5 - b7 - b9", 90 },
    // 4 notes
    { 0b100010010001, "maj7", "Major Seventh", "1 - 3 - 5 - 7", 80 },
    { 0b010010010001, "7", "Dominant Seventh", "1 - 3 - 5 - b7", 80 },
    { 0b010010001001, "m7", "Minor Seventh", "1 - b3 - 5 - b7", 80 },
    { 0b100010001001, "m(maj7)", "Minor Major Seventh", "1 - b3 - 5 - 7", 78 },
    { 0b010001001001, "m7b5", "Half-Diminished (m7b5)", "1 - b3 - b5 - b7", 80 },
    { 0b001001001001, "dim7", "Diminished Seventh", "1 - b3 - b5 - bb7", 80 },
    { 0b010100010001, "7#5", "Augmented Seventh", "1 - 3 - #5 - b7", 78 },
    { 0b001010010001, "6", "Major Sixth", "1 - 3 - 5 - 6", 75 },
    { 0b001010001001, "m6", "Minor Sixth", "1 - b3 - 5 - 6", 75 },
    { 0b000010010101, "add9", "Major Add Nine", "1 - 3 - 5 - 9", 75 },
    { 0b000010001101, "m(add9)", "Minor Add Nine", "1 - b3 - 5 - 9", 75 },
    { 0b010010100001, "7sus4", "Dominant Seventh Sus4", "1 - 4 - 5 - b7", 75 },
    // 3 notes (Triads) - Priority 70 to accurately favor Major & Minor triads
    { 0b000010010001, "", "Major Triad", "1 - 3 - 5", 70 },
    { 0b000010001001, "m", "Minor Triad", "1 - b3 - 5", 70 },
    { 0b000010100001, "sus4", "Suspended Fourth", "1 - 4 - 5", 65 },
    { 0b000010000101, "sus2", "Suspended Second", "1 - 2 - 5", 65 },
    { 0b000001001001, "dim", "Diminished Triad", "1 - b3 - b5", 68 },
    { 0b000100010001, "aug", "Augmented Triad", "1 - 3 - #5", 68 },
    // 2 notes (Dyads)
    { 0b000010000001, "5", "Power Chord (Fifth)", "1 - 5", 40 },
    { 0b000000010001, " (no5)", "Major Third Dyad", "1 - 3", 30 },
    { 0b000000001001, "m (no5)", "Minor Third Dyad", "1 - b3", 30 }
};
static const int kNumChordPatterns = sizeof(kChordPatterns) / sizeof(kChordPatterns[0]);

struct ChordVisualizerState {
    std::atomic<uint32_t> seqNumber{0};
    
    // MIDI note states (0-127)
    std::atomic<int> activeNotesCount{0};
    std::atomic<bool> noteActive[128]{};
    std::atomic<float> noteVelocity[128]{};

    // Chord Analysis strings (empty by default, exactly matching Web UI detectChord returning null)
    char chordName[64]{""};
    char chordQuality[64]{""};
    char intervalsString[64]{""};
    char notesString[64]{""};
    char inversionString[64]{"Root Position"};
    char rootNote[8]{""};
    char bassNote[8]{""};
    bool isSlash{false};

    // Engine flags
    std::atomic<bool> isBypassed{false};

    // UI Customization Settings requested by user
    std::atomic<bool> isLightTheme{false};     // Toggle Theme: Dark (false) vs Light (true)
    std::atomic<int> octaveRange{3};          // 2, 3, 4 octaves
    std::atomic<int> baseOctave{3};           // Base octave (C2 or C3)
    std::atomic<int> currentScalePercent{100}; // 75, 100, 125, 150 %

    void triggerNoteOn(int pitch, float velocity = 0.8f);
    void triggerNoteOff(int pitch);
    void clearAllNotes();
    void evaluateChord();
};

extern ChordVisualizerState g_sharedState;

} // namespace Vst
} // namespace Steinberg
