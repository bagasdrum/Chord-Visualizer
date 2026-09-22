#include "ChordVisualizerShared.h"
#include <cstdio>
#include <algorithm>

namespace Steinberg {
namespace Vst {

ChordVisualizerState g_sharedState;

void ChordVisualizerState::triggerNoteOn(int pitch, float velocity) {
    if (pitch >= 0 && pitch < 128) {
        if (!noteActive[pitch].load()) {
            noteActive[pitch] = true;
            noteVelocity[pitch] = velocity;
            activeNotesCount++;
            evaluateChord();
        }
    }
}

void ChordVisualizerState::triggerNoteOff(int pitch) {
    if (pitch >= 0 && pitch < 128) {
        if (noteActive[pitch].load()) {
            noteActive[pitch] = false;
            noteVelocity[pitch] = 0.0f;
            int count = activeNotesCount.load();
            activeNotesCount = (count > 0) ? (count - 1) : 0;
            evaluateChord();
        }
    }
}

void ChordVisualizerState::clearAllNotes() {
    for (int i = 0; i < 128; ++i) {
        noteActive[i] = false;
        noteVelocity[i] = 0.0f;
    }
    activeNotesCount = 0;
    evaluateChord();
}

void ChordVisualizerState::evaluateChord() {
    int activeCount = activeNotesCount.load();
    if (activeCount == 0) {
        chordName[0] = '\0';
        chordQuality[0] = '\0';
        intervalsString[0] = '\0';
        notesString[0] = '\0';
        inversionString[0] = '\0';
        isSlash = false;
        rootNote[0] = '\0';
        bassNote[0] = '\0';
        seqNumber++;
        return;
    }

    int lowestPitch = -1;
    uint32_t pitchClassMask = 0;
    std::vector<int> heldNotes;

    for (int p = 0; p < 128; ++p) {
        if (noteActive[p].load()) {
            if (lowestPitch == -1) lowestPitch = p;
            pitchClassMask |= (1 << (p % 12));
            heldNotes.push_back(p);
        }
    }

    int bassPitchClass = (lowestPitch >= 0) ? (lowestPitch % 12) : 0;
    strncpy(bassNote, kNoteNames[bassPitchClass], sizeof(bassNote) - 1);

    // Build Notes string (e.g. "C4 · E4 · G4 · B4")
    std::string notesStr = "";
    for (size_t i = 0; i < heldNotes.size(); ++i) {
        int pitch = heldNotes[i];
        int noteIdx = pitch % 12;
        int octave = (pitch / 12) - 1;
        char buf[16];
        snprintf(buf, sizeof(buf), "%s%d", kNoteNames[noteIdx], octave);
        if (i > 0) notesStr += " - ";
        notesStr += buf;
    }
    strncpy(notesString, notesStr.c_str(), sizeof(notesString) - 1);

    if (activeCount == 1) {
        strncpy(chordName, kNoteNames[bassPitchClass], sizeof(chordName) - 1);
        strncpy(chordQuality, "Single Note", sizeof(chordQuality) - 1);
        strncpy(intervalsString, "1", sizeof(intervalsString) - 1);
        strncpy(inversionString, "Root Position", sizeof(inversionString) - 1);
        strncpy(rootNote, kNoteNames[bassPitchClass], sizeof(rootNote) - 1);
        isSlash = false;
        seqNumber++;
        return;
    }

    // Match against chord database for all 12 candidate roots
    int bestRoot = -1;
    const ChordPatternDef* bestMatch = nullptr;
    int bestScore = -1;

    for (int r = 0; r < 12; ++r) {
        uint32_t rotated = 0;
        for (int b = 0; b < 12; ++b) {
            if (pitchClassMask & (1 << ((r + b) % 12))) {
                rotated |= (1 << b);
            }
        }

        for (int pIdx = 0; pIdx < kNumChordPatterns; ++pIdx) {
            const auto& pattern = kChordPatterns[pIdx];
            if (rotated == pattern.mask) {
                int score = pattern.priority;
                if (r == bassPitchClass) score += 20; // Root in bass bonus
                if (score > bestScore) {
                    bestScore = score;
                    bestRoot = r;
                    bestMatch = &pattern;
                }
            }
        }
    }

    if (bestMatch && bestRoot >= 0) {
        strncpy(rootNote, kNoteNames[bestRoot], sizeof(rootNote) - 1);
        strncpy(intervalsString, bestMatch->formula, sizeof(intervalsString) - 1);

        if (bestRoot == bassPitchClass) {
            // Root Position
            char nameBuf[64];
            snprintf(nameBuf, sizeof(nameBuf), "%s%s", kNoteNames[bestRoot], bestMatch->symbol);
            strncpy(chordName, nameBuf, sizeof(chordName) - 1);
            strncpy(chordQuality, bestMatch->quality, sizeof(chordQuality) - 1);
            strncpy(inversionString, "Root Position", sizeof(inversionString) - 1);
            isSlash = false;
        } else {
            // Inversion / Slash Chord
            char nameBuf[64];
            snprintf(nameBuf, sizeof(nameBuf), "%s%s/%s",
                     kNoteNames[bestRoot], bestMatch->symbol, kNoteNames[bassPitchClass]);
            strncpy(chordName, nameBuf, sizeof(chordName) - 1);
            isSlash = true;

            int bassInterval = (bassPitchClass - bestRoot + 12) % 12;
            if (bassInterval == 4 || bassInterval == 3) {
                char qBuf[64];
                snprintf(qBuf, sizeof(qBuf), "%s (1st Inversion)", bestMatch->quality);
                strncpy(chordQuality, qBuf, sizeof(chordQuality) - 1);
                char invBuf[64];
                snprintf(invBuf, sizeof(invBuf), "1st Inv (%s in Bass)", kNoteNames[bassPitchClass]);
                strncpy(inversionString, invBuf, sizeof(inversionString) - 1);
            } else if (bassInterval == 7) {
                char qBuf[64];
                snprintf(qBuf, sizeof(qBuf), "%s (2nd Inversion)", bestMatch->quality);
                strncpy(chordQuality, qBuf, sizeof(chordQuality) - 1);
                char invBuf[64];
                snprintf(invBuf, sizeof(invBuf), "2nd Inv (%s in Bass)", kNoteNames[bassPitchClass]);
                strncpy(inversionString, invBuf, sizeof(inversionString) - 1);
            } else if (bassInterval == 10 || bassInterval == 11) {
                char qBuf[64];
                snprintf(qBuf, sizeof(qBuf), "%s (3rd Inversion)", bestMatch->quality);
                strncpy(chordQuality, qBuf, sizeof(chordQuality) - 1);
                char invBuf[64];
                snprintf(invBuf, sizeof(invBuf), "3rd Inv (%s in Bass)", kNoteNames[bassPitchClass]);
                strncpy(inversionString, invBuf, sizeof(inversionString) - 1);
            } else {
                char qBuf[64];
                snprintf(qBuf, sizeof(qBuf), "%s / %s Bass", bestMatch->quality, kNoteNames[bassPitchClass]);
                strncpy(chordQuality, qBuf, sizeof(chordQuality) - 1);
                char invBuf[64];
                snprintf(invBuf, sizeof(invBuf), "Slash Chord (%s)", kNoteNames[bassPitchClass]);
                strncpy(inversionString, invBuf, sizeof(inversionString) - 1);
            }
        }
    } else {
        // Extended cluster
        char clusterBuf[64];
        snprintf(clusterBuf, sizeof(clusterBuf), "%s Cluster (%d notes)",
                 kNoteNames[bassPitchClass], activeCount);
        strncpy(chordName, clusterBuf, sizeof(chordName) - 1);
        strncpy(chordQuality, "Extended Cluster Voicing", sizeof(chordQuality) - 1);
        strncpy(intervalsString, "Free Voicing", sizeof(intervalsString) - 1);
        strncpy(inversionString, "Free Voicing", sizeof(inversionString) - 1);
        isSlash = false;
    }

    seqNumber++;
}

} // namespace Vst
} // namespace Steinberg
