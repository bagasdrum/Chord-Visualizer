#include "ChordVisualizerShared.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace Steinberg {
namespace Vst {

ChordVisualizerState g_sharedState;

namespace {

// -----------------------------------------------------------------------------
// Chord pattern engine
// -----------------------------------------------------------------------------

struct LocalChordPattern {
    const char* symbol;
    const char* quality;
    const char* formula;
    uint32_t mask;
    int priority;
};

// Build a 12-bit interval mask.
// Example: {0,4,7} = major triad.
constexpr uint32_t makeMask(std::initializer_list<int> intervals)
{
    uint32_t mask = 0;

    for (int interval : intervals) {
        int pc = interval % 12;
        if (pc < 0)
            pc += 12;

        mask |= (1u << pc);
    }

    return mask;
}

// -----------------------------------------------------------------------------
// Canonical chord vocabulary
//
// Priority is used only when multiple chord names produce exactly the same
// pitch-class set. Root-in-bass receives an additional bonus in evaluateChord().
// -----------------------------------------------------------------------------

static const LocalChordPattern kPatterns[] =
{
    // -------------------------------------------------------------------------
    // Basic triads
    // -------------------------------------------------------------------------

    { "",       "Major",              "1 - 3 - 5",
      makeMask({0,4,7}), 100 },

    { "m",      "Minor",              "1 - b3 - 5",
      makeMask({0,3,7}), 100 },

    { "dim",    "Diminished",         "1 - b3 - b5",
      makeMask({0,3,6}), 102 },

    { "aug",    "Augmented",          "1 - 3 - #5",
      makeMask({0,4,8}), 102 },

    { "sus2",   "Suspended 2",        "1 - 2 - 5",
      makeMask({0,2,7}), 101 },

    { "sus4",   "Suspended 4",        "1 - 4 - 5",
      makeMask({0,5,7}), 101 },

    { "5",      "Power Chord",        "1 - 5",
      makeMask({0,7}), 95 },

    // -------------------------------------------------------------------------
    // Sixth chords
    // -------------------------------------------------------------------------

    { "6",      "Major 6",            "1 - 3 - 5 - 6",
      makeMask({0,4,7,9}), 112 },

    { "m6",     "Minor 6",            "1 - b3 - 5 - 6",
      makeMask({0,3,7,9}), 113 },

    { "6/9",    "Major 6/9",          "1 - 2 - 3 - 5 - 6",
      makeMask({0,2,4,7,9}), 122 },

    { "m6/9",   "Minor 6/9",          "1 - 2 - b3 - 5 - 6",
      makeMask({0,2,3,7,9}), 123 },

    // -------------------------------------------------------------------------
    // Seventh chords
    // -------------------------------------------------------------------------

    { "7",      "Dominant 7",         "1 - 3 - 5 - b7",
      makeMask({0,4,7,10}), 130 },

    { "maj7",   "Major 7",            "1 - 3 - 5 - 7",
      makeMask({0,4,7,11}), 132 },

    { "m7",     "Minor 7",            "1 - b3 - 5 - b7",
      makeMask({0,3,7,10}), 132 },

    { "mMaj7",  "Minor Major 7",      "1 - b3 - 5 - 7",
      makeMask({0,3,7,11}), 134 },

    { "dim7",   "Diminished 7",       "1 - b3 - b5 - bb7",
      makeMask({0,3,6,9}), 136 },

    { "m7b5",   "Half-Diminished 7",  "1 - b3 - b5 - b7",
      makeMask({0,3,6,10}), 135 },

    { "7sus4",  "Dominant 7 Sus4",    "1 - 4 - 5 - b7",
      makeMask({0,5,7,10}), 131 },

    // -------------------------------------------------------------------------
    // Added 9 chords
    // -------------------------------------------------------------------------

    { "add9",   "Major Add 9",        "1 - 3 - 5 - 9",
      makeMask({0,2,4,7}), 116 },

    { "madd9",  "Minor Add 9",        "1 - b3 - 5 - 9",
      makeMask({0,2,3,7}), 117 },

    // -------------------------------------------------------------------------
    // Ninth chords
    // -------------------------------------------------------------------------

    { "9",      "Dominant 9",         "1 - 3 - 5 - b7 - 9",
      makeMask({0,2,4,7,10}), 145 },

    { "maj9",   "Major 9",            "1 - 3 - 5 - 7 - 9",
      makeMask({0,2,4,7,11}), 147 },

    { "m9",     "Minor 9",            "1 - b3 - 5 - b7 - 9",
      makeMask({0,2,3,7,10}), 147 },

    { "mMaj9",  "Minor Major 9",      "1 - b3 - 5 - 7 - 9",
      makeMask({0,2,3,7,11}), 149 },

    // -------------------------------------------------------------------------
    // Eleventh chords
    // -------------------------------------------------------------------------

    { "11",      "Dominant 11",       "1 - 3 - 5 - b7 - 9 - 11",
      makeMask({0,2,4,5,7,10}), 154 },

    { "maj11",   "Major 11",          "1 - 3 - 5 - 7 - 9 - 11",
      makeMask({0,2,4,5,7,11}), 156 },

    { "m11",     "Minor 11",          "1 - b3 - 5 - b7 - 9 - 11",
      makeMask({0,2,3,5,7,10}), 156 },

    // -------------------------------------------------------------------------
    // Thirteenth chords
    // -------------------------------------------------------------------------

    { "13",      "Dominant 13",       "1 - 3 - 5 - b7 - 9 - 13",
      makeMask({0,2,4,7,9,10}), 162 },

    { "maj13",   "Major 13",          "1 - 3 - 5 - 7 - 9 - 13",
      makeMask({0,2,4,7,9,11}), 164 },

    { "m13",     "Minor 13",          "1 - b3 - 5 - b7 - 9 - 13",
      makeMask({0,2,3,7,9,10}), 164 },

    // -------------------------------------------------------------------------
    // Altered dominant chords
    // -------------------------------------------------------------------------

    { "7b5",     "Dominant 7 Flat 5", "1 - 3 - b5 - b7",
      makeMask({0,4,6,10}), 142 },

    { "7#5",     "Dominant 7 Sharp 5","1 - 3 - #5 - b7",
      makeMask({0,4,8,10}), 142 },

    { "7b9",     "Dominant 7 Flat 9", "1 - 3 - 5 - b7 - b9",
      makeMask({0,1,4,7,10}), 150 },

    { "7#9",     "Dominant 7 Sharp 9","1 - 3 - 5 - b7 - #9",
      makeMask({0,3,4,7,10}), 150 },

    { "9b5",     "Dominant 9 Flat 5", "1 - 3 - b5 - b7 - 9",
      makeMask({0,2,4,6,10}), 152 },

    { "9#5",     "Dominant 9 Sharp 5","1 - 3 - #5 - b7 - 9",
      makeMask({0,2,4,8,10}), 152 },

    { "7(b5,b9)",
      "Dominant 7 Flat 5 Flat 9",
      "1 - 3 - b5 - b7 - b9",
      makeMask({0,1,4,6,10}), 153 },

    { "7(b5,#9)",
      "Dominant 7 Flat 5 Sharp 9",
      "1 - 3 - b5 - b7 - #9",
      makeMask({0,3,4,6,10}), 153 },

    { "7(#5,b9)",
      "Dominant 7 Sharp 5 Flat 9",
      "1 - 3 - #5 - b7 - b9",
      makeMask({0,1,4,8,10}), 153 },

    { "7(#5,#9)",
      "Dominant 7 Sharp 5 Sharp 9",
      "1 - 3 - #5 - b7 - #9",
      makeMask({0,3,4,8,10}), 153 },

    { "11b9",    "Dominant 11 Flat 9",
      "1 - 3 - 5 - b7 - b9 - 11",
      makeMask({0,1,4,5,7,10}), 158 },

    { "13b9",    "Dominant 13 Flat 9",
      "1 - 3 - 5 - b7 - b9 - 13",
      makeMask({0,1,4,7,9,10}), 166 },

    { "13#11",   "Dominant 13 Sharp 11",
      "1 - 3 - 5 - b7 - 9 - #11 - 13",
      makeMask({0,2,4,6,7,9,10}), 168 },

    // -------------------------------------------------------------------------
    // Altered major chords
    // -------------------------------------------------------------------------

    { "maj7b5",
      "Major 7 Flat 5",
      "1 - 3 - b5 - 7",
      makeMask({0,4,6,11}), 143 },

    { "maj7#5",
      "Major 7 Sharp 5",
      "1 - 3 - #5 - 7",
      makeMask({0,4,8,11}), 143 },

    { "maj9#11",
      "Major 9 Sharp 11",
      "1 - 3 - 5 - 7 - 9 - #11",
      makeMask({0,2,4,6,7,11}), 160 },

    { "maj13#11",
      "Major 13 Sharp 11",
      "1 - 3 - 5 - 7 - 9 - #11 - 13",
      makeMask({0,2,4,6,7,9,11}), 170 },

    // -------------------------------------------------------------------------
    // Altered minor chords
    // -------------------------------------------------------------------------

    { "m7#5",
      "Minor 7 Sharp 5",
      "1 - b3 - #5 - b7",
      makeMask({0,3,8,10}), 144 },

    { "m6add9",
      "Minor 6 Add 9",
      "1 - b3 - 5 - 6 - 9",
      makeMask({0,2,3,7,9}), 125 }
};

static constexpr int kPatternCount =
    static_cast<int>(sizeof(kPatterns) / sizeof(kPatterns[0]));

// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------

static void copyText(char* destination, size_t destinationSize, const char* source)
{
    if (!destination || destinationSize == 0)
        return;

    destination[0] = '\0';

    if (!source)
        return;

    std::strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

static int popcount12(uint32_t mask)
{
    int count = 0;

    for (int i = 0; i < 12; ++i) {
        if (mask & (1u << i))
            ++count;
    }

    return count;
}

static uint32_t rotatePitchClassMask(uint32_t sourceMask, int root)
{
    uint32_t rotated = 0;

    for (int interval = 0; interval < 12; ++interval) {
        int pitchClass = (root + interval) % 12;

        if (sourceMask & (1u << pitchClass))
            rotated |= (1u << interval);
    }

    return rotated;
}

static int intervalFromRoot(int root, int pitchClass)
{
    return (pitchClass - root + 12) % 12;
}

// Find the ordinal position of the bass interval in the chord.
// This is used for 1st/2nd/3rd/4th inversion labels.
static int inversionNumber(uint32_t chordMask, int bassInterval)
{
    if (bassInterval == 0)
        return 0;

    int position = 0;

    for (int interval = 1; interval < 12; ++interval) {
        if (chordMask & (1u << interval)) {
            ++position;

            if (interval == bassInterval)
                return position;
        }
    }

    return -1;
}

static void buildInversionText(
    char* destination,
    size_t destinationSize,
    uint32_t chordMask,
    int root,
    int bassPitchClass)
{
    int bassInterval = intervalFromRoot(root, bassPitchClass);

    if (bassInterval == 0) {
        copyText(destination, destinationSize, "Root Position");
        return;
    }

    int inversion = inversionNumber(chordMask, bassInterval);

    if (inversion >= 1 && inversion <= 4) {
        char buffer[64];

        const char* ordinal = "th";

        if (inversion == 1)
            ordinal = "st";
        else if (inversion == 2)
            ordinal = "nd";
        else if (inversion == 3)
            ordinal = "rd";

        std::snprintf(
            buffer,
            sizeof(buffer),
            "%d%s Inv (%s in Bass)",
            inversion,
            ordinal,
            kNoteNames[bassPitchClass]);

        copyText(destination, destinationSize, buffer);
    }
    else {
        char buffer[64];

        std::snprintf(
            buffer,
            sizeof(buffer),
            "Slash Chord (%s)",
            kNoteNames[bassPitchClass]);

        copyText(destination, destinationSize, buffer);
    }
}

static void buildChordQuality(
    char* destination,
    size_t destinationSize,
    const LocalChordPattern& pattern,
    int root,
    int bassPitchClass)
{
    int bassInterval = intervalFromRoot(root, bassPitchClass);

    if (bassInterval == 0) {
        copyText(destination, destinationSize, pattern.quality);
        return;
    }

    int inversion = inversionNumber(pattern.mask, bassInterval);

    if (inversion >= 1 && inversion <= 4) {
        char buffer[96];

        const char* ordinal = "th";

        if (inversion == 1)
            ordinal = "st";
        else if (inversion == 2)
            ordinal = "nd";
        else if (inversion == 3)
            ordinal = "rd";

        std::snprintf(
            buffer,
            sizeof(buffer),
            "%s (%d%s Inversion)",
            pattern.quality,
            inversion,
            ordinal);

        copyText(destination, destinationSize, buffer);
    }
    else {
        char buffer[96];

        std::snprintf(
            buffer,
            sizeof(buffer),
            "%s / %s Bass",
            pattern.quality,
            kNoteNames[bassPitchClass]);

        copyText(destination, destinationSize, buffer);
    }
}

} // anonymous namespace

// -----------------------------------------------------------------------------
// MIDI state
// -----------------------------------------------------------------------------

void ChordVisualizerState::triggerNoteOn(int pitch, float velocity)
{
    if (pitch < 0 || pitch >= 128)
        return;

    if (!noteActive[pitch].load()) {
        noteActive[pitch] = true;
        noteVelocity[pitch] = velocity;

        activeNotesCount++;

        evaluateChord();
    }
}

void ChordVisualizerState::triggerNoteOff(int pitch)
{
    if (pitch < 0 || pitch >= 128)
        return;

    if (noteActive[pitch].load()) {
        noteActive[pitch] = false;
        noteVelocity[pitch] = 0.0f;

        int count = activeNotesCount.load();

        activeNotesCount = (count > 0)
            ? (count - 1)
            : 0;

        evaluateChord();
    }
}

void ChordVisualizerState::clearAllNotes()
{
    for (int i = 0; i < 128; ++i) {
        noteActive[i] = false;
        noteVelocity[i] = 0.0f;
    }

    activeNotesCount = 0;

    evaluateChord();
}

// -----------------------------------------------------------------------------
// Chord evaluation
// -----------------------------------------------------------------------------

void ChordVisualizerState::evaluateChord()
{
    const int activeCount = activeNotesCount.load();

    // -------------------------------------------------------------------------
    // Idle
    // -------------------------------------------------------------------------

    if (activeCount == 0) {
        copyText(chordName, sizeof(chordName), "");
        copyText(chordQuality, sizeof(chordQuality), "");
        copyText(intervalsString, sizeof(intervalsString), "");
        copyText(notesString, sizeof(notesString), "");
        copyText(inversionString, sizeof(inversionString), "");
        copyText(rootNote, sizeof(rootNote), "");
        copyText(bassNote, sizeof(bassNote), "");

        isSlash = false;

        seqNumber++;
        return;
    }

    // -------------------------------------------------------------------------
    // Collect MIDI notes
    // -------------------------------------------------------------------------

    int lowestPitch = -1;

    uint32_t pitchClassMask = 0;

    std::vector<int> heldNotes;
    heldNotes.reserve(activeCount);

    for (int pitch = 0; pitch < 128; ++pitch) {
        if (noteActive[pitch].load()) {
            if (lowestPitch < 0)
                lowestPitch = pitch;

            const int pitchClass = pitch % 12;

            pitchClassMask |= (1u << pitchClass);

            heldNotes.push_back(pitch);
        }
    }

    if (lowestPitch < 0) {
        clearAllNotes();
        return;
    }

    const int bassPitchClass = lowestPitch % 12;

    copyText(
        bassNote,
        sizeof(bassNote),
        kNoteNames[bassPitchClass]);

    // -------------------------------------------------------------------------
    // Build Notes display
    //
    // Example:
    // C3 - E3 - G3 - B3
    //
    // The UI can convert "-" to its preferred visual separator.
    // -------------------------------------------------------------------------

    std::string notesStr;

    for (size_t i = 0; i < heldNotes.size(); ++i) {
        const int pitch = heldNotes[i];
        const int noteIndex = pitch % 12;
        const int octave = (pitch / 12) - 1;

        char buffer[16];

        std::snprintf(
            buffer,
            sizeof(buffer),
            "%s%d",
            kNoteNames[noteIndex],
            octave);

        if (i > 0)
            notesStr += " - ";

        notesStr += buffer;
    }

    copyText(
        notesString,
        sizeof(notesString),
        notesStr.c_str());

    // -------------------------------------------------------------------------
    // Single note
    // -------------------------------------------------------------------------

    if (activeCount == 1) {
        copyText(
            chordName,
            sizeof(chordName),
            kNoteNames[bassPitchClass]);

        copyText(
            chordQuality,
            sizeof(chordQuality),
            "Single Note");

        copyText(
            intervalsString,
            sizeof(intervalsString),
            "1");

        copyText(
            inversionString,
            sizeof(inversionString),
            "Root Position");

        copyText(
            rootNote,
            sizeof(rootNote),
            kNoteNames[bassPitchClass]);

        isSlash = false;

        seqNumber++;
        return;
    }

    // -------------------------------------------------------------------------
    // Find best chord/root combination
    // -------------------------------------------------------------------------

    int bestRoot = -1;
    const LocalChordPattern* bestPattern = nullptr;

    int bestScore = -100000;

    for (int root = 0; root < 12; ++root) {

        const uint32_t rotatedMask =
            rotatePitchClassMask(pitchClassMask, root);

        for (int i = 0; i < kPatternCount; ++i) {

            const LocalChordPattern& pattern = kPatterns[i];

            if (rotatedMask != pattern.mask)
                continue;

            int score = pattern.priority;

            // Strong preference for root in bass.
            if (root == bassPitchClass)
                score += 50;

            // Prefer more complex/explicit patterns when pitch sets are
            // otherwise ambiguous.
            score += popcount12(pattern.mask);

            // Prefer the actual bass/root interpretation over a theoretical
            // alternative root when both have identical priority.
            if (root == bassPitchClass)
                score += 10;

            if (score > bestScore) {
                bestScore = score;
                bestRoot = root;
                bestPattern = &pattern;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Chord found
    // -------------------------------------------------------------------------

    if (bestPattern && bestRoot >= 0) {

        copyText(
            rootNote,
            sizeof(rootNote),
            kNoteNames[bestRoot]);

        copyText(
            intervalsString,
            sizeof(intervalsString),
            bestPattern->formula);

        // Root position
        if (bestRoot == bassPitchClass) {

            char nameBuffer[64];

            std::snprintf(
                nameBuffer,
                sizeof(nameBuffer),
                "%s%s",
                kNoteNames[bestRoot],
                bestPattern->symbol);

            copyText(
                chordName,
                sizeof(chordName),
                nameBuffer);

            copyText(
                chordQuality,
                sizeof(chordQuality),
                bestPattern->quality);

            copyText(
                inversionString,
                sizeof(inversionString),
                "Root Position");

            isSlash = false;
        }

        // Inversion / slash chord
        else {

            char nameBuffer[64];

            std::snprintf(
                nameBuffer,
                sizeof(nameBuffer),
                "%s%s/%s",
                kNoteNames[bestRoot],
                bestPattern->symbol,
                kNoteNames[bassPitchClass]);

            copyText(
                chordName,
                sizeof(chordName),
                nameBuffer);

            buildChordQuality(
                chordQuality,
                sizeof(chordQuality),
                *bestPattern,
                bestRoot,
                bassPitchClass);

            buildInversionText(
                inversionString,
                sizeof(inversionString),
                bestPattern->mask,
                bestRoot,
                bassPitchClass);

            isSlash = true;
        }
    }

    // -------------------------------------------------------------------------
    // No exact match
    //
    // Do not guess a chord name. This prevents incorrect chord labels.
    // -------------------------------------------------------------------------

    else {

        char nameBuffer[64];

        std::snprintf(
            nameBuffer,
            sizeof(nameBuffer),
            "%s Unclassified",
            kNoteNames[bassPitchClass]);

        copyText(
            chordName,
            sizeof(chordName),
            nameBuffer);

        copyText(
            chordQuality,
            sizeof(chordQuality),
            "Unclassified Voicing");

        copyText(
            intervalsString,
            sizeof(intervalsString),
            "Unclassified");

        copyText(
            inversionString,
            sizeof(inversionString),
            "Unclassified Voicing");

        copyText(
            rootNote,
            sizeof(rootNote),
            kNoteNames[bassPitchClass]);

        isSlash = false;
    }

    seqNumber++;
}

} // namespace Vst
} // namespace Steinberg
