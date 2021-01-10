/*
 * CHOpt - Star Power optimiser for Clone Hero
 * Copyright (C) 2020, 2021 Raymond Wright
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHOPT_SONGPARTS_HPP
#define CHOPT_SONGPARTS_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

enum class Difficulty { Easy, Medium, Hard, Expert };

enum class Instrument {
    Guitar,
    GuitarCoop,
    Bass,
    Rhythm,
    Keys,
    GHLGuitar,
    GHLBass,
    Drums
};

enum class NoteColour { Green, Red, Yellow, Blue, Orange, Open };

enum class GHLNoteColour {
    WhiteLow,
    WhiteMid,
    WhiteHigh,
    BlackLow,
    BlackMid,
    BlackHigh,
    Open
};

enum class DrumNoteColour {
    Red,
    Yellow,
    Blue,
    Green,
    YellowCymbal,
    BlueCymbal,
    GreenCymbal,
    Kick
};

struct TimeSignature {
    int position;
    int numerator;
    int denominator;
};

struct BPM {
    int position;
    int bpm;
};

template <typename T> struct Note {
    int position {0};
    int length {0};
    T colour = {};
};

struct StarPower {
    int position;
    int length;
};

struct Solo {
    int start;
    int end;
    int value;
};

class ParseError : public std::runtime_error {
public:
    ParseError(const char* what)
        : std::runtime_error {what}
    {
    }
};

// Invariants:
// notes() will always return a vector of sorted notes.
// notes() will not return a vector with two notes of the same colour with the
// same position.
// resolution() will be greater than zero.
// sp_phrases() will always return a vector of sorted SP phrases.
// sp_phrases() will only return phrases with a note in their range.
// sp_phrases() will return non-overlapping phrases.
// solos() will always return a vector of sorted solos.
template <typename T> class NoteTrack {
private:
    std::vector<Note<T>> m_notes;
    std::vector<StarPower> m_sp_phrases;
    std::vector<Solo> m_solos;
    int m_resolution;
    int m_base_score;

    int compute_base_score()
    {
        constexpr auto BASE_NOTE_VALUE = 50;
        constexpr auto BASE_SUSTAIN_DENSITY = 25;

        auto base_score = static_cast<int>(BASE_NOTE_VALUE * m_notes.size());
        auto total_ticks = 0;
        auto current_pos = -1;
        auto first_note_length = 0;
        auto current_total_ticks = 0;
        auto is_disjoint_chord = false;
        for (const auto& note : m_notes) {
            if (note.position != current_pos) {
                if (is_disjoint_chord) {
                    total_ticks += current_total_ticks;
                } else {
                    total_ticks += first_note_length;
                }
                first_note_length = note.length;
                current_total_ticks = note.length;
                current_pos = note.position;
                is_disjoint_chord = false;
            } else {
                if (note.length != first_note_length) {
                    is_disjoint_chord = true;
                }
                current_total_ticks += note.length;
            }
        }
        if (is_disjoint_chord) {
            total_ticks += current_total_ticks;
        } else {
            total_ticks += first_note_length;
        }

        base_score += (total_ticks * BASE_SUSTAIN_DENSITY + m_resolution - 1)
            / m_resolution;
        return base_score;
    }

public:
    NoteTrack(std::vector<Note<T>> notes,
              const std::vector<StarPower>& sp_phrases, std::vector<Solo> solos,
              int resolution)
        : m_resolution {resolution}
    {
        if (m_resolution <= 0) {
            throw ParseError("Resolution non-positive");
        }

        std::stable_sort(notes.begin(), notes.end(),
                         [](const auto& lhs, const auto& rhs) {
                             return std::tie(lhs.position, lhs.colour)
                                 < std::tie(rhs.position, rhs.colour);
                         });

        if (!notes.empty()) {
            auto prev_note = notes.cbegin();
            for (auto p = notes.cbegin() + 1; p < notes.cend(); ++p) {
                if (p->position != prev_note->position
                    || p->colour != prev_note->colour) {
                    m_notes.push_back(*prev_note);
                }
                prev_note = p;
            }
            m_notes.push_back(*prev_note);
        }

        std::vector<int> sp_starts;
        std::vector<int> sp_ends;
        sp_starts.reserve(sp_phrases.size());
        sp_ends.reserve(sp_phrases.size());

        for (const auto& phrase : sp_phrases) {
            sp_starts.push_back(phrase.position);
            sp_ends.push_back(phrase.position + phrase.length);
        }

        std::sort(sp_starts.begin(), sp_starts.end());
        std::sort(sp_ends.begin(), sp_ends.end());

        std::vector<StarPower> new_sp_phrases;
        new_sp_phrases.reserve(sp_phrases.size());
        for (auto i = 0U; i < sp_phrases.size(); ++i) {
            auto start = sp_starts[i];
            if (i > 0) {
                start = std::max(sp_starts[i], sp_ends[i - 1]);
            }
            const auto length = sp_ends[i] - start;
            new_sp_phrases.push_back({start, length});
        }

        for (const auto& phrase : new_sp_phrases) {
            const auto first_note = std::lower_bound(
                m_notes.cbegin(), m_notes.cend(), phrase.position,
                [](const auto& lhs, const auto& rhs) {
                    return lhs.position < rhs;
                });
            if ((first_note != m_notes.cend())
                && (first_note->position < (phrase.position + phrase.length))) {
                m_sp_phrases.push_back(phrase);
            }
        }

        std::stable_sort(solos.begin(), solos.end(),
                         [](const auto& lhs, const auto& rhs) {
                             return lhs.start < rhs.start;
                         });
        m_solos = std::move(solos);

        m_base_score = compute_base_score();
    }

    [[nodiscard]] const std::vector<Note<T>>& notes() const { return m_notes; }
    [[nodiscard]] const std::vector<StarPower>& sp_phrases() const
    {
        return m_sp_phrases;
    }
    [[nodiscard]] const std::vector<Solo>& solos() const { return m_solos; }
    [[nodiscard]] int resolution() const { return m_resolution; }
    [[nodiscard]] int base_score() const { return m_base_score; }
    [[nodiscard]] NoteTrack<T> trim_sustains(int speed) const
    {
        constexpr int DEFAULT_RESOLUTION = 192;
        constexpr int DEFAULT_SPEED = 100;
        constexpr int DEFAULT_SUST_CUTOFF = 64;

        auto trimmed_track = *this;
        auto sust_cutoff = (DEFAULT_SUST_CUTOFF * m_resolution * speed)
            / (DEFAULT_SPEED * DEFAULT_RESOLUTION);

        for (auto& note : trimmed_track.m_notes) {
            if (note.length <= sust_cutoff) {
                note.length = 0;
            }
        }

        trimmed_track.m_base_score = trimmed_track.compute_base_score();

        // We need to do this because for speeds below 100%, the sustains are
        // trimmed the same as 100% speed but the base score can be higher.
        if (speed < DEFAULT_SPEED) {
            sust_cutoff
                = (DEFAULT_SUST_CUTOFF * m_resolution) / DEFAULT_RESOLUTION;

            for (auto& note : trimmed_track.m_notes) {
                if (note.length <= sust_cutoff) {
                    note.length = 0;
                }
            }
        }

        return trimmed_track;
    }
};

// Invariants:
// bpms() are sorted by position.
// bpms() never has two BPMs with the same position.
// bpms() is never empty.
// time_sigs() are sorted by position.
// time_sigs() never has two TimeSignatures with the same position.
// time_sigs() is never empty.
class SyncTrack {
private:
    std::vector<TimeSignature> m_time_sigs;
    std::vector<BPM> m_bpms;

public:
    SyncTrack()
        : SyncTrack({}, {})
    {
    }
    SyncTrack(std::vector<TimeSignature> time_sigs, std::vector<BPM> bpms);
    [[nodiscard]] const std::vector<TimeSignature>& time_sigs() const
    {
        return m_time_sigs;
    }
    [[nodiscard]] const std::vector<BPM>& bpms() const { return m_bpms; }
    // Return the SyncTrack for a speedup of speed% (normal speed is 100).
    [[nodiscard]] SyncTrack speedup(int speed) const;
};

#endif
