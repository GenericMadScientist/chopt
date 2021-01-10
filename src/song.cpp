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

#include <climits>
#include <filesystem>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>

#include "nowide_wrapper.hpp"

#include "song.hpp"

static bool starts_with_prefix(const std::string& string,
                               std::string_view prefix)
{
    return string.substr(0, prefix.size()) == prefix;
}

static bool ends_with_suffix(const std::string& string, std::string_view suffix)
{
    if (string.size() < suffix.size()) {
        return false;
    }
    return string.substr(string.size() - suffix.size()) == suffix;
}

Song Song::from_filename(const std::string& filename)
{
    std::string ini_file;
    const std::filesystem::path song_path {filename};
    const auto song_directory = song_path.parent_path();
    const auto ini_path = song_directory / "song.ini";
    nowide::ifstream ini_in {ini_path.string()};
    if (ini_in.is_open()) {
        ini_file = std::string {std::istreambuf_iterator<char>(ini_in),
                                std::istreambuf_iterator<char>()};
    }
    const auto ini = parse_ini(ini_file);

    if (ends_with_suffix(filename, ".chart")) {
        nowide::ifstream in {filename};
        if (!in.is_open()) {
            throw std::invalid_argument("File did not open");
        }
        std::string contents {std::istreambuf_iterator<char>(in),
                              std::istreambuf_iterator<char>()};
        return Song::from_chart(parse_chart(contents), ini);
    }
    if (ends_with_suffix(filename, ".mid")) {
        nowide::ifstream in {filename, std::ios::binary};
        if (!in.is_open()) {
            throw std::invalid_argument("File did not open");
        }
        std::vector<std::uint8_t> buffer {std::istreambuf_iterator<char>(in),
                                          std::istreambuf_iterator<char>()};
        return Song::from_midi(parse_midi(buffer), ini);
    }
    throw std::invalid_argument("file should be .chart or .mid");
}

// Takes a sequence of points where some note type/event is turned on, and a
// sequence where said type is turned off, and returns a tuple of intervals
// where the event is on.
static std::vector<std::tuple<int, int>>
combine_solo_events(const std::vector<int>& on_events,
                    const std::vector<int>& off_events)
{
    std::vector<std::tuple<int, int>> ranges;

    auto on_iter = on_events.cbegin();
    auto off_iter = off_events.cbegin();

    while (on_iter < on_events.cend() && off_iter < off_events.cend()) {
        if (*on_iter >= *off_iter) {
            ++off_iter;
            continue;
        }
        ranges.emplace_back(*on_iter, *off_iter);
        while (on_iter < on_events.cend() && *on_iter < *off_iter) {
            ++on_iter;
        }
    }

    return ranges;
}

template <typename T>
static std::vector<Solo>
form_solo_vector(const std::vector<int>& solo_on_events,
                 const std::vector<int>& solo_off_events,
                 const std::vector<Note<T>>& notes, bool is_midi)
{
    constexpr int SOLO_NOTE_VALUE = 100;

    std::vector<Solo> solos;

    for (auto [start, end] :
         combine_solo_events(solo_on_events, solo_off_events)) {
        std::set<int> positions_in_solo;
        for (const auto& note : notes) {
            if ((note.position >= start && note.position < end)
                || (note.position == end && !is_midi)) {
                positions_in_solo.insert(note.position);
            }
        }
        if (positions_in_solo.empty()) {
            continue;
        }
        auto note_count = static_cast<int>(positions_in_solo.size());
        solos.push_back({start, end, SOLO_NOTE_VALUE * note_count});
    }

    return solos;
}

template <typename T>
static std::optional<Note<T>> note_from_note_colour(int position, int length,
                                                    int fret_type)
{
    if constexpr (std::is_same_v<T, NoteColour>) {
        static const std::array<std::optional<NoteColour>, 8> COLOURS {
            NoteColour::Green, NoteColour::Red,    NoteColour::Yellow,
            NoteColour::Blue,  NoteColour::Orange, std::nullopt,
            std::nullopt,      NoteColour::Open};
        if (static_cast<std::size_t>(fret_type) >= COLOURS.size()) {
            return std::nullopt;
        }
        const auto colour = COLOURS.at(static_cast<std::size_t>(fret_type));
        if (!colour.has_value()) {
            return std::nullopt;
        }
        return Note<NoteColour> {position, length, *colour};
    } else if constexpr (std::is_same_v<T, GHLNoteColour>) {
        static const std::array<std::optional<GHLNoteColour>, 9> COLOURS {
            GHLNoteColour::WhiteLow,
            GHLNoteColour::WhiteMid,
            GHLNoteColour::WhiteHigh,
            GHLNoteColour::BlackLow,
            GHLNoteColour::BlackMid,
            std::nullopt,
            std::nullopt,
            GHLNoteColour::Open,
            GHLNoteColour::BlackHigh};
        if (static_cast<std::size_t>(fret_type) >= COLOURS.size()) {
            return std::nullopt;
        }
        const auto colour = COLOURS.at(static_cast<std::size_t>(fret_type));
        if (!colour.has_value()) {
            return std::nullopt;
        }
        return Note<GHLNoteColour> {position, length, *colour};
    } else if constexpr (std::is_same_v<T, DrumNoteColour>) {
        const std::map<int, DrumNoteColour> COLOURS {
            {0, DrumNoteColour::Kick},
            {1, DrumNoteColour::Red},
            {2, DrumNoteColour::Yellow},
            {3, DrumNoteColour::Blue},
            {4, DrumNoteColour::Green},
            {66, DrumNoteColour::YellowCymbal},
            {67, DrumNoteColour::BlueCymbal},
            {68, DrumNoteColour::GreenCymbal}};
        (void)length;
        const auto colour_iter = COLOURS.find(fret_type);
        if (colour_iter == COLOURS.end()) {
            return std::nullopt;
        }
        return Note<DrumNoteColour> {position, 0, colour_iter->second};
    }
}

static std::string
get_with_default(const std::map<std::string, std::string>& map,
                 const std::string& key, std::string default_value)
{
    const auto iter = map.find(key);
    if (iter == map.end()) {
        return default_value;
    }
    return iter->second;
}

static std::optional<std::tuple<Difficulty, Instrument>>
diff_inst_from_header(const std::string& header)
{
    using namespace std::literals;

    static const std::array<std::tuple<std::string_view, Difficulty>, 4>
        DIFFICULTIES {std::tuple {"Easy"sv, Difficulty::Easy},
                      {"Medium"sv, Difficulty::Medium},
                      {"Hard"sv, Difficulty::Hard},
                      {"Expert"sv, Difficulty::Expert}};
    static const std::array<std::tuple<std::string_view, Instrument>, 8>
        INSTRUMENTS {std::tuple {"Single"sv, Instrument::Guitar},
                     {"DoubleGuitar"sv, Instrument::GuitarCoop},
                     {"DoubleBass"sv, Instrument::Bass},
                     {"DoubleRhythm"sv, Instrument::Rhythm},
                     {"Keyboard"sv, Instrument::Keys},
                     {"GHLGuitar"sv, Instrument::GHLGuitar},
                     {"GHLBass"sv, Instrument::GHLBass},
                     {"Drums"sv, Instrument::Drums}};
    // NOLINT is required because following clang-tidy here causes the VS2017
    // compile to fail.
    auto diff_iter = std::find_if( // NOLINT
        DIFFICULTIES.cbegin(), DIFFICULTIES.cend(), [&](const auto& pair) {
            return starts_with_prefix(header, std::get<0>(pair));
        });
    if (diff_iter == DIFFICULTIES.cend()) {
        return std::nullopt;
    }
    auto inst_iter = std::find_if( // NOLINT
        INSTRUMENTS.cbegin(), INSTRUMENTS.cend(), [&](const auto& pair) {
            return ends_with_suffix(header, std::get<0>(pair));
        });
    if (inst_iter == INSTRUMENTS.cend()) {
        return std::nullopt;
    }
    return std::tuple {std::get<1>(*diff_iter), std::get<1>(*inst_iter)};
}

static std::vector<Note<DrumNoteColour>>
apply_cymbal_events(const std::vector<Note<DrumNoteColour>>& notes)
{
    std::set<unsigned int> deletion_spots;
    for (auto i = 0U; i < notes.size(); ++i) {
        const auto& cymbal_note = notes[i];
        DrumNoteColour non_cymbal_colour = DrumNoteColour::Kick;
        switch (cymbal_note.colour) {
        case DrumNoteColour::YellowCymbal:
            non_cymbal_colour = DrumNoteColour::Yellow;
            break;
        case DrumNoteColour::BlueCymbal:
            non_cymbal_colour = DrumNoteColour::Blue;
            break;
        case DrumNoteColour::GreenCymbal:
            non_cymbal_colour = DrumNoteColour::Green;
            break;
        case DrumNoteColour::Red:
        case DrumNoteColour::Yellow:
        case DrumNoteColour::Blue:
        case DrumNoteColour::Green:
        case DrumNoteColour::Kick:
            continue;
        }
        bool delete_cymbal = true;
        for (auto j = 0U; j < notes.size(); ++j) {
            const auto& non_cymbal_note = notes[j];
            if (non_cymbal_note.position != cymbal_note.position) {
                continue;
            }
            if (non_cymbal_note.colour != non_cymbal_colour) {
                continue;
            }
            delete_cymbal = false;
            deletion_spots.insert(j);
        }
        if (delete_cymbal) {
            deletion_spots.insert(i);
        }
    }
    std::vector<Note<DrumNoteColour>> new_notes;
    for (auto i = 0U; i < notes.size(); ++i) {
        if (deletion_spots.count(i) == 0) {
            new_notes.push_back(notes[i]);
        }
    }
    return new_notes;
}

template <typename T>
static NoteTrack<T> note_track_from_section(const ChartSection& section,
                                            int resolution)
{
    std::vector<Note<T>> notes;
    for (const auto& note_event : section.note_events) {
        const auto note = note_from_note_colour<T>(
            note_event.position, note_event.length, note_event.fret);
        if (note.has_value()) {
            notes.push_back(*note);
        }
    }
    if constexpr (std::is_same_v<T, DrumNoteColour>) {
        notes = apply_cymbal_events(notes);
    }

    std::vector<StarPower> sp;
    for (const auto& phrase : section.sp_events) {
        if (phrase.key == 2) {
            sp.push_back(StarPower {phrase.position, phrase.length});
        }
    }

    std::vector<int> solo_on_events;
    std::vector<int> solo_off_events;
    for (const auto& event : section.events) {
        if (event.data == "solo") {
            solo_on_events.push_back(event.position);
        } else if (event.data == "soloend") {
            solo_off_events.push_back(event.position);
        }
    }
    std::sort(solo_on_events.begin(), solo_on_events.end());
    std::sort(solo_off_events.begin(), solo_off_events.end());
    auto solos
        = form_solo_vector(solo_on_events, solo_off_events, notes, false);

    return NoteTrack<T> {std::move(notes), std::move(sp), std::move(solos),
                         resolution};
}

std::vector<Instrument> Song::instruments() const
{
    std::set<Instrument> instrument_set;
    for (const auto& [key, val] : m_five_fret_tracks) {
        instrument_set.insert(std::get<0>(key));
    }
    for (const auto& [key, val] : m_six_fret_tracks) {
        instrument_set.insert(std::get<0>(key));
    }
    if (!m_drum_note_tracks.empty()) {
        instrument_set.insert(Instrument::Drums);
    }

    std::vector<Instrument> instruments {instrument_set.cbegin(),
                                         instrument_set.cend()};
    std::sort(instruments.begin(), instruments.end());
    return instruments;
}

static bool is_six_fret_instrument(Instrument instrument)
{
    return instrument == Instrument::GHLGuitar
        || instrument == Instrument::GHLBass;
}

std::vector<Difficulty> Song::difficulties(Instrument instrument) const
{
    std::vector<Difficulty> difficulties;
    if (instrument == Instrument::Drums) {
        for (const auto& [key, val] : m_drum_note_tracks) {
            difficulties.push_back(key);
        }
    } else if (is_six_fret_instrument(instrument)) {
        for (const auto& [key, val] : m_six_fret_tracks) {
            if (std::get<0>(key) == instrument) {
                difficulties.push_back(std::get<1>(key));
            }
        }
    } else {
        for (const auto& [key, val] : m_five_fret_tracks) {
            if (std::get<0>(key) == instrument) {
                difficulties.push_back(std::get<1>(key));
            }
        }
    }
    std::sort(difficulties.begin(), difficulties.end());
    return difficulties;
}

Song Song::from_chart(const Chart& chart, const IniValues& ini)
{
    Song song;

    song.m_name = ini.name;
    song.m_artist = ini.artist;
    song.m_charter = ini.charter;

    for (const auto& section : chart.sections) {
        if (section.name == "Song") {
            try {
                song.m_resolution = std::stoi(get_with_default(
                    section.key_value_pairs, "Resolution", "192"));
            } catch (const std::invalid_argument&) {
                // CH just ignores this kind of parsing mistake.
                // TODO: Use from_chars instead to avoid having to use
                // exceptions as control flow.
            }
        } else if (section.name == "SyncTrack") {
            std::vector<BPM> bpms;
            for (const auto& bpm : section.bpm_events) {
                bpms.push_back({bpm.position, bpm.bpm});
            }
            std::vector<TimeSignature> tses;
            for (const auto& ts : section.ts_events) {
                if (static_cast<std::size_t>(ts.denominator)
                    >= (CHAR_BIT * sizeof(int))) {
                    throw ParseError("Invalid Time Signature denominator");
                }
                tses.push_back(
                    {ts.position, ts.numerator, 1 << ts.denominator});
            }
            song.m_sync_track = SyncTrack {std::move(tses), std::move(bpms)};
        } else {
            auto pair = diff_inst_from_header(section.name);
            if (!pair.has_value()) {
                continue;
            }
            auto [diff, inst] = *pair;
            if (is_six_fret_instrument(inst)) {
                auto note_track = note_track_from_section<GHLNoteColour>(
                    section, song.m_resolution);
                if (!note_track.notes().empty()) {
                    song.m_six_fret_tracks.insert({{inst, diff}, note_track});
                }
            } else if (inst == Instrument::Drums) {
                auto note_track = note_track_from_section<DrumNoteColour>(
                    section, song.m_resolution);
                if (!note_track.notes().empty()) {
                    song.m_drum_note_tracks.insert({diff, note_track});
                }
            } else {
                auto note_track = note_track_from_section<NoteColour>(
                    section, song.m_resolution);
                if (!note_track.notes().empty()) {
                    song.m_five_fret_tracks.insert({{inst, diff}, note_track});
                }
            }
        }
    }

    if (song.m_five_fret_tracks.empty() && song.m_six_fret_tracks.empty()
        && song.m_drum_note_tracks.empty()) {
        throw ParseError("Chart has no notes");
    }

    return song;
}

// Like combine_solo_events, but never skips on events to suit Midi parsing and
// checks if there is an unmatched on event.
// The tuples are a pair of the form (position, rank), where events later in the
// file have a higher rank. This is in case of the Note Off event being right
// after the corresponding Note On event in the file, but at the same tick.
static std::vector<std::tuple<int, int>>
combine_note_on_off_events(const std::vector<std::tuple<int, int>>& on_events,
                           const std::vector<std::tuple<int, int>>& off_events)
{
    std::vector<std::tuple<int, int>> ranges;

    auto on_iter = on_events.cbegin();
    auto off_iter = off_events.cbegin();

    while (on_iter < on_events.cend() && off_iter < off_events.cend()) {
        if (*on_iter >= *off_iter) {
            ++off_iter;
            continue;
        }
        ranges.emplace_back(std::get<0>(*on_iter), std::get<0>(*off_iter));
        ++on_iter;
        ++off_iter;
    }

    if (on_iter != on_events.cend()) {
        throw ParseError("on event has no corresponding off event");
    }

    return ranges;
}

template <typename T>
static std::optional<Difficulty> difficulty_from_key(std::uint8_t key)
{
    if constexpr (std::is_same_v<
                      T, NoteColour> || std::is_same_v<T, DrumNoteColour>) {
        constexpr int EASY_GREEN = 60;
        constexpr int EASY_ORANGE = 64;
        constexpr int EXPERT_GREEN = 96;
        constexpr int EXPERT_ORANGE = 100;
        constexpr int HARD_GREEN = 84;
        constexpr int HARD_ORANGE = 88;
        constexpr int MEDIUM_GREEN = 72;
        constexpr int MEDIUM_ORANGE = 76;

        if (key >= EXPERT_GREEN && key <= EXPERT_ORANGE) {
            return {Difficulty::Expert};
        }
        if (key >= HARD_GREEN && key <= HARD_ORANGE) {
            return {Difficulty::Hard};
        }
        if (key >= MEDIUM_GREEN && key <= MEDIUM_ORANGE) {
            return {Difficulty::Medium};
        }
        if (key >= EASY_GREEN && key <= EASY_ORANGE) {
            return {Difficulty::Easy};
        }
        return std::nullopt;
    } else if constexpr (std::is_same_v<T, GHLNoteColour>) {
        constexpr int EASY_OPEN = 58;
        constexpr int EASY_BLACK_HIGH = 64;
        constexpr int EXPERT_OPEN = 94;
        constexpr int EXPERT_BLACK_HIGH = 100;
        constexpr int HARD_OPEN = 82;
        constexpr int HARD_BLACK_HIGH = 88;
        constexpr int MEDIUM_OPEN = 70;
        constexpr int MEDIUM_BLACK_HIGH = 76;

        if (key >= EXPERT_OPEN && key <= EXPERT_BLACK_HIGH) {
            return {Difficulty::Expert};
        }
        if (key >= HARD_OPEN && key <= HARD_BLACK_HIGH) {
            return {Difficulty::Hard};
        }
        if (key >= MEDIUM_OPEN && key <= MEDIUM_BLACK_HIGH) {
            return {Difficulty::Medium};
        }
        if (key >= EASY_OPEN && key <= EASY_BLACK_HIGH) {
            return {Difficulty::Easy};
        }
        return std::nullopt;
    }
}

template <typename T> static T colour_from_key(std::uint8_t key)
{
    if constexpr (std::is_same_v<T, NoteColour>) {
        constexpr int EASY_GREEN = 60;
        constexpr int EASY_ORANGE = 64;
        constexpr int EXPERT_GREEN = 96;
        constexpr int EXPERT_ORANGE = 100;
        constexpr int HARD_GREEN = 84;
        constexpr int HARD_ORANGE = 88;
        constexpr int MEDIUM_GREEN = 72;
        constexpr int MEDIUM_ORANGE = 76;

        constexpr std::array<NoteColour, 5> NOTE_COLOURS {
            NoteColour::Green, NoteColour::Red, NoteColour::Yellow,
            NoteColour::Blue, NoteColour::Orange};

        if (key >= EXPERT_GREEN && key <= EXPERT_ORANGE) {
            key -= EXPERT_GREEN;
        } else if (key >= HARD_GREEN && key <= HARD_ORANGE) {
            key -= HARD_GREEN;
        } else if (key >= MEDIUM_GREEN && key <= MEDIUM_ORANGE) {
            key -= MEDIUM_GREEN;
        } else if (key >= EASY_GREEN && key <= EASY_ORANGE) {
            key -= EASY_GREEN;
        } else {
            throw ParseError("Invalid key for note");
        }

        return NOTE_COLOURS.at(key);
    } else if constexpr (std::is_same_v<T, GHLNoteColour>) {
        constexpr int EASY_OPEN = 58;
        constexpr int EASY_BLACK_HIGH = 64;
        constexpr int EXPERT_OPEN = 94;
        constexpr int EXPERT_BLACK_HIGH = 100;
        constexpr int HARD_OPEN = 82;
        constexpr int HARD_BLACK_HIGH = 88;
        constexpr int MEDIUM_OPEN = 70;
        constexpr int MEDIUM_BLACK_HIGH = 76;

        constexpr std::array<GHLNoteColour, 7> GHL_NOTE_COLOURS {
            GHLNoteColour::Open,     GHLNoteColour::WhiteLow,
            GHLNoteColour::WhiteMid, GHLNoteColour::WhiteHigh,
            GHLNoteColour::BlackLow, GHLNoteColour::BlackMid,
            GHLNoteColour::BlackHigh};

        if (key >= EXPERT_OPEN && key <= EXPERT_BLACK_HIGH) {
            key -= EXPERT_OPEN;
        } else if (key >= HARD_OPEN && key <= HARD_BLACK_HIGH) {
            key -= HARD_OPEN;
        } else if (key >= MEDIUM_OPEN && key <= MEDIUM_BLACK_HIGH) {
            key -= MEDIUM_OPEN;
        } else if (key >= EASY_OPEN && key <= EASY_BLACK_HIGH) {
            key -= EASY_OPEN;
        } else {
            throw ParseError("Invalid key for note");
        }

        return GHL_NOTE_COLOURS.at(key);
    } else if constexpr (std::is_same_v<T, DrumNoteColour>) {
        constexpr int EASY_KICK = 60;
        constexpr int EASY_GREEN = 64;
        constexpr int EXPERT_KICK = 96;
        constexpr int EXPERT_GREEN = 100;
        constexpr int HARD_KICK = 84;
        constexpr int HARD_GREEN = 88;
        constexpr int MEDIUM_KICK = 72;
        constexpr int MEDIUM_GREEN = 76;

        constexpr std::array<DrumNoteColour, 5> DRUM_NOTE_COLOURS {
            DrumNoteColour::Kick, DrumNoteColour::Red,
            DrumNoteColour::YellowCymbal, DrumNoteColour::BlueCymbal,
            DrumNoteColour::GreenCymbal};

        if (key >= EXPERT_KICK && key <= EXPERT_GREEN) {
            key -= EXPERT_KICK;
        } else if (key >= HARD_KICK && key <= HARD_GREEN) {
            key -= HARD_KICK;
        } else if (key >= MEDIUM_KICK && key <= MEDIUM_GREEN) {
            key -= MEDIUM_KICK;
        } else if (key >= EASY_KICK && key <= EASY_GREEN) {
            key -= EASY_KICK;
        } else {
            throw ParseError("Invalid key for note");
        }

        return DRUM_NOTE_COLOURS.at(key);
    }
}

static bool is_open_event_sysex(const SysexEvent& event)
{
    constexpr std::array<std::tuple<std::size_t, int>, 6> REQUIRED_BYTES {
        std::tuple {0, 0x50}, {1, 0x53}, {2, 0}, {3, 0}, {5, 1}, {7, 0xF7}};
    constexpr std::array<std::tuple<std::size_t, int>, 2> UPPER_BOUNDS {
        std::tuple {4, 3}, {6, 1}};
    constexpr int SYSEX_DATA_SIZE = 8;

    if (event.data.size() != SYSEX_DATA_SIZE) {
        return false;
    }
    if (std::any_of(REQUIRED_BYTES.cbegin(), REQUIRED_BYTES.cend(),
                    [&](const auto& pair) {
                        return event.data[std::get<0>(pair)]
                            != std::get<1>(pair);
                    })) {
        return false;
    }
    return std::all_of(
        UPPER_BOUNDS.cbegin(), UPPER_BOUNDS.cend(), [&](const auto& pair) {
            return event.data[std::get<0>(pair)] <= std::get<1>(pair);
        });
}

static SyncTrack read_first_midi_track(const MidiTrack& track)
{
    constexpr int SET_TEMPO_ID = 0x51;
    constexpr int TIME_SIG_ID = 0x58;

    std::vector<BPM> tempos;
    std::vector<TimeSignature> time_sigs;
    for (const auto& event : track.events) {
        const auto* meta_event = std::get_if<MetaEvent>(&event.event);
        if (meta_event == nullptr) {
            continue;
        }
        switch (meta_event->type) {
        case SET_TEMPO_ID: {
            if (meta_event->data.size() < 3) {
                throw ParseError("Tempo meta event too short");
            }
            const auto us_per_quarter = meta_event->data[0] << 16
                | meta_event->data[1] << 8 | meta_event->data[2];
            const auto bpm = 60000000000 / us_per_quarter;
            tempos.push_back({event.time, static_cast<int>(bpm)});
            break;
        }
        case TIME_SIG_ID:
            if (meta_event->data.size() < 2) {
                throw ParseError("Tempo meta event too short");
            }
            if (meta_event->data[1] >= (CHAR_BIT * sizeof(int))) {
                throw ParseError("Time sig denominator too large");
            }
            time_sigs.push_back(
                {event.time, meta_event->data[0], 1 << meta_event->data[1]});
            break;
        }
    }

    return {std::move(time_sigs), std::move(tempos)};
}

template <typename T> struct InstrumentMidiTrack {
    std::map<std::tuple<Difficulty, T>, std::vector<std::tuple<int, int>>>
        note_on_events;
    std::map<std::tuple<Difficulty, T>, std::vector<std::tuple<int, int>>>
        note_off_events;
    std::map<Difficulty, std::vector<std::tuple<int, int>>> open_on_events;
    std::map<Difficulty, std::vector<std::tuple<int, int>>> open_off_events;
    std::vector<std::tuple<int, int>> yellow_tom_on_events;
    std::vector<std::tuple<int, int>> yellow_tom_off_events;
    std::vector<std::tuple<int, int>> blue_tom_on_events;
    std::vector<std::tuple<int, int>> blue_tom_off_events;
    std::vector<std::tuple<int, int>> green_tom_on_events;
    std::vector<std::tuple<int, int>> green_tom_off_events;
    std::vector<std::tuple<int, int>> solo_on_events;
    std::vector<std::tuple<int, int>> solo_off_events;
    std::vector<std::tuple<int, int>> sp_on_events;
    std::vector<std::tuple<int, int>> sp_off_events;
};

template <typename T>
static void add_sysex_event(InstrumentMidiTrack<T>& track,
                            const SysexEvent& event, int time, int rank)
{
    constexpr std::array<Difficulty, 4> OPEN_EVENT_DIFFS {
        Difficulty::Easy, Difficulty::Medium, Difficulty::Hard,
        Difficulty::Expert};
    constexpr int SYSEX_ON_INDEX = 6;

    if (!is_open_event_sysex(event)) {
        return;
    }
    Difficulty diff = OPEN_EVENT_DIFFS.at(event.data[4]);
    if (event.data[SYSEX_ON_INDEX] == 0) {
        track.open_off_events[diff].push_back({time, rank});
    } else {
        track.open_on_events[diff].push_back({time, rank});
    }
}

template <typename T>
static void add_note_off_event(InstrumentMidiTrack<T>& track,
                               const std::array<std::uint8_t, 2>& data,
                               int time, int rank)
{
    constexpr int YELLOW_TOM_ID = 110;
    constexpr int BLUE_TOM_ID = 111;
    constexpr int GREEN_TOM_ID = 112;
    constexpr int SOLO_NOTE_ID = 103;
    constexpr int SP_NOTE_ID = 116;

    const auto diff = difficulty_from_key<T>(data[0]);
    if (diff.has_value()) {
        const auto colour = colour_from_key<T>(data[0]);
        track.note_off_events[{*diff, colour}].push_back({time, rank});
    } else if (data[0] == YELLOW_TOM_ID) {
        track.yellow_tom_off_events.push_back({time, rank});
    } else if (data[0] == BLUE_TOM_ID) {
        track.blue_tom_off_events.push_back({time, rank});
    } else if (data[0] == GREEN_TOM_ID) {
        track.green_tom_off_events.push_back({time, rank});
    } else if (data[0] == SOLO_NOTE_ID) {
        track.solo_off_events.push_back({time, rank});
    } else if (data[0] == SP_NOTE_ID) {
        track.sp_off_events.push_back({time, rank});
    }
}

template <typename T>
static void add_note_on_event(InstrumentMidiTrack<T>& track,
                              const std::array<std::uint8_t, 2>& data, int time,
                              int rank)
{
    constexpr int YELLOW_TOM_ID = 110;
    constexpr int BLUE_TOM_ID = 111;
    constexpr int GREEN_TOM_ID = 112;
    constexpr int SOLO_NOTE_ID = 103;
    constexpr int SP_NOTE_ID = 116;

    // Velocity 0 Note On events are counted as Note Off events.
    if (data[1] == 0) {
        add_note_off_event(track, data, time, rank);
        return;
    }

    const auto diff = difficulty_from_key<T>(data[0]);
    if (diff.has_value()) {
        const auto colour = colour_from_key<T>(data[0]);
        track.note_on_events[{*diff, colour}].push_back({time, rank});
    } else if (data[0] == YELLOW_TOM_ID) {
        track.yellow_tom_on_events.push_back({time, rank});
    } else if (data[0] == BLUE_TOM_ID) {
        track.blue_tom_on_events.push_back({time, rank});
    } else if (data[0] == GREEN_TOM_ID) {
        track.green_tom_on_events.push_back({time, rank});
    } else if (data[0] == SOLO_NOTE_ID) {
        track.solo_on_events.push_back({time, rank});
    } else if (data[0] == SP_NOTE_ID) {
        track.sp_on_events.push_back({time, rank});
    }
}

template <typename T>
static InstrumentMidiTrack<T>
read_instrument_midi_track(const MidiTrack& midi_track)
{
    constexpr int NOTE_OFF_ID = 0x80;
    constexpr int NOTE_ON_ID = 0x90;
    constexpr int UPPER_NIBBLE_MASK = 0xF0;

    InstrumentMidiTrack<T> event_track;

    int rank = 0;
    for (const auto& event : midi_track.events) {
        ++rank;
        const auto* midi_event = std::get_if<MidiEvent>(&event.event);
        if (midi_event == nullptr) {
            const auto* sysex_event = std::get_if<SysexEvent>(&event.event);
            if (sysex_event != nullptr) {
                add_sysex_event(event_track, *sysex_event, event.time, rank);
            }
            continue;
        }
        switch (midi_event->status & UPPER_NIBBLE_MASK) {
        case NOTE_OFF_ID:
            add_note_off_event(event_track, midi_event->data, event.time, rank);
            break;
        case NOTE_ON_ID:
            add_note_on_event(event_track, midi_event->data, event.time, rank);
            break;
        }
    }

    if (event_track.sp_on_events.empty()
        && event_track.solo_on_events.size() > 1) {
        std::swap(event_track.sp_off_events, event_track.solo_off_events);
        std::swap(event_track.sp_on_events, event_track.solo_on_events);
    }

    return event_track;
}

static std::map<Difficulty, NoteTrack<NoteColour>>
note_tracks_from_midi(const MidiTrack& midi_track, int resolution)
{
    const auto event_track = read_instrument_midi_track<NoteColour>(midi_track);

    std::map<Difficulty, std::vector<std::tuple<int, int>>> open_events;
    for (const auto& [diff, open_ons] : event_track.open_on_events) {
        if (event_track.open_off_events.count(diff) == 0) {
            throw ParseError("No open Note Off events");
        }
        const auto& open_offs = event_track.open_off_events.at(diff);
        open_events[diff] = combine_note_on_off_events(open_ons, open_offs);
    }

    std::map<Difficulty, std::vector<Note<NoteColour>>> notes;
    for (const auto& [key, note_ons] : event_track.note_on_events) {
        const auto& [diff, colour] = key;
        if (event_track.note_off_events.count(key) == 0) {
            throw ParseError("No corresponding Note Off events");
        }
        const auto& note_offs = event_track.note_off_events.at(key);
        for (const auto& [pos, end] :
             combine_note_on_off_events(note_ons, note_offs)) {
            const auto note_length = end - pos;
            auto note_colour = colour;
            for (const auto& [open_start, open_end] : open_events[diff]) {
                if (pos >= open_start && pos < open_end) {
                    note_colour = NoteColour::Open;
                }
            }
            notes[diff].push_back({pos, note_length, note_colour});
        }
    }

    std::vector<StarPower> sp_phrases;
    for (const auto& [start, end] : combine_note_on_off_events(
             event_track.sp_on_events, event_track.sp_off_events)) {
        sp_phrases.push_back({start, end - start});
    }

    std::map<Difficulty, NoteTrack<NoteColour>> note_tracks;
    for (const auto& [diff, note_set] : notes) {
        std::vector<int> solo_ons;
        std::vector<int> solo_offs;
        solo_ons.reserve(event_track.solo_on_events.size());
        for (const auto& [pos, rank] : event_track.solo_on_events) {
            solo_ons.push_back(pos);
        }
        solo_offs.reserve(event_track.solo_off_events.size());
        for (const auto& [pos, rank] : event_track.solo_off_events) {
            solo_offs.push_back(pos);
        }
        auto solos = form_solo_vector(solo_ons, solo_offs, note_set, true);
        note_tracks.emplace(diff,
                            NoteTrack<NoteColour> {note_set, sp_phrases,
                                                   std::move(solos),
                                                   resolution});
    }

    return note_tracks;
}

static std::map<Difficulty, NoteTrack<GHLNoteColour>>
ghl_note_tracks_from_midi(const MidiTrack& midi_track, int resolution)
{
    const auto event_track
        = read_instrument_midi_track<GHLNoteColour>(midi_track);

    std::map<Difficulty, std::vector<Note<GHLNoteColour>>> notes;
    for (const auto& [key, note_ons] : event_track.note_on_events) {
        const auto& [diff, colour] = key;
        if (event_track.note_off_events.count(key) == 0) {
            throw ParseError("No corresponding Note Off events");
        }
        const auto& note_offs = event_track.note_off_events.at(key);
        for (const auto& [pos, end] :
             combine_note_on_off_events(note_ons, note_offs)) {
            const auto note_length = end - pos;
            notes[diff].push_back({pos, note_length, colour});
        }
    }

    std::vector<StarPower> sp_phrases;
    for (const auto& [start, end] : combine_note_on_off_events(
             event_track.sp_on_events, event_track.sp_off_events)) {
        sp_phrases.push_back({start, end - start});
    }

    std::map<Difficulty, NoteTrack<GHLNoteColour>> note_tracks;
    for (const auto& [diff, note_set] : notes) {
        std::vector<int> solo_ons;
        std::vector<int> solo_offs;
        solo_ons.reserve(event_track.solo_on_events.size());
        for (const auto& [pos, rank] : event_track.solo_on_events) {
            solo_ons.push_back(pos);
        }
        solo_offs.reserve(event_track.solo_off_events.size());
        for (const auto& [pos, rank] : event_track.solo_off_events) {
            solo_offs.push_back(pos);
        }
        auto solos = form_solo_vector(solo_ons, solo_offs, note_set, true);
        note_tracks.emplace(diff,
                            NoteTrack<GHLNoteColour> {note_set, sp_phrases,
                                                      std::move(solos),
                                                      resolution});
    }

    return note_tracks;
}

static std::map<Difficulty, NoteTrack<DrumNoteColour>>
drum_note_tracks_from_midi(const MidiTrack& midi_track, int resolution)
{
    const auto event_track
        = read_instrument_midi_track<DrumNoteColour>(midi_track);

    const auto yellow_tom_events = combine_note_on_off_events(
        event_track.yellow_tom_on_events, event_track.yellow_tom_off_events);
    const auto blue_tom_events = combine_note_on_off_events(
        event_track.blue_tom_on_events, event_track.blue_tom_off_events);
    const auto green_tom_events = combine_note_on_off_events(
        event_track.green_tom_on_events, event_track.green_tom_off_events);

    std::map<Difficulty, std::vector<Note<DrumNoteColour>>> notes;
    for (const auto& [key, note_ons] : event_track.note_on_events) {
        const auto& [diff, colour] = key;
        if (event_track.note_off_events.count(key) == 0) {
            throw ParseError("No corresponding Note Off events");
        }
        const auto& note_offs = event_track.note_off_events.at(key);
        for (const auto& [pos, end] :
             combine_note_on_off_events(note_ons, note_offs)) {
            auto note_colour = colour;
            if (note_colour == DrumNoteColour::YellowCymbal) {
                for (const auto& [open_start, open_end] : yellow_tom_events) {
                    if (pos >= open_start && pos < open_end) {
                        note_colour = DrumNoteColour::Yellow;
                    }
                }
            } else if (note_colour == DrumNoteColour::BlueCymbal) {
                for (const auto& [open_start, open_end] : blue_tom_events) {
                    if (pos >= open_start && pos < open_end) {
                        note_colour = DrumNoteColour::Blue;
                    }
                }
            } else if (note_colour == DrumNoteColour::GreenCymbal) {
                for (const auto& [open_start, open_end] : green_tom_events) {
                    if (pos >= open_start && pos < open_end) {
                        note_colour = DrumNoteColour::Green;
                    }
                }
            }
            notes[diff].push_back({pos, 0, note_colour});
        }
    }

    std::vector<StarPower> sp_phrases;
    for (const auto& [start, end] : combine_note_on_off_events(
             event_track.sp_on_events, event_track.sp_off_events)) {
        sp_phrases.push_back({start, end - start});
    }

    std::map<Difficulty, NoteTrack<DrumNoteColour>> note_tracks;
    for (const auto& [diff, note_set] : notes) {
        std::vector<int> solo_ons;
        std::vector<int> solo_offs;
        solo_ons.reserve(event_track.solo_on_events.size());
        for (const auto& [pos, rank] : event_track.solo_on_events) {
            solo_ons.push_back(pos);
        }
        solo_offs.reserve(event_track.solo_off_events.size());
        for (const auto& [pos, rank] : event_track.solo_off_events) {
            solo_offs.push_back(pos);
        }
        auto solos = form_solo_vector(solo_ons, solo_offs, note_set, true);
        note_tracks.emplace(diff,
                            NoteTrack<DrumNoteColour> {note_set, sp_phrases,
                                                       std::move(solos),
                                                       resolution});
    }

    return note_tracks;
}

static std::optional<Instrument> midi_section_instrument(const MidiTrack& track)
{
    const std::map<std::string, Instrument> INSTRUMENTS {
        {"PART GUITAR", Instrument::Guitar},
        {"T1 GEMS", Instrument::Guitar},
        {"PART GUITAR COOP", Instrument::GuitarCoop},
        {"PART BASS", Instrument::Bass},
        {"PART RHYTHM", Instrument::Rhythm},
        {"PART KEYS", Instrument::Keys},
        {"PART GUITAR GHL", Instrument::GHLGuitar},
        {"PART BASS GHL", Instrument::GHLBass},
        {"PART DRUMS", Instrument::Drums}};

    if (track.events.empty()) {
        return std::nullopt;
    }
    std::string track_name;
    for (const auto& event : track.events) {
        const auto* meta_event = std::get_if<MetaEvent>(&event.event);
        if (meta_event == nullptr) {
            continue;
        }
        if (meta_event->type != 3) {
            continue;
        }
        track_name
            = std::string {meta_event->data.cbegin(), meta_event->data.cend()};
        break;
    }
    const auto iter = INSTRUMENTS.find(track_name);
    if (iter == INSTRUMENTS.end()) {
        return std::nullopt;
    }
    return iter->second;
}

Song Song::from_midi(const Midi& midi, const IniValues& ini)
{
    if (midi.ticks_per_quarter_note == 0) {
        throw ParseError("Resolution must be > 0");
    }

    Song song;
    song.m_is_from_midi = true;
    song.m_resolution = midi.ticks_per_quarter_note;
    song.m_name = ini.name;
    song.m_artist = ini.artist;
    song.m_charter = ini.charter;

    if (midi.tracks.empty()) {
        return song;
    }

    song.m_sync_track = read_first_midi_track(midi.tracks[0]);

    for (const auto& track : midi.tracks) {
        const auto inst = midi_section_instrument(track);
        if (!inst.has_value()) {
            continue;
        }
        if (is_six_fret_instrument(*inst)) {
            auto tracks = ghl_note_tracks_from_midi(track, song.m_resolution);
            for (auto& [diff, note_track] : tracks) {
                song.m_six_fret_tracks.emplace(std::tuple {*inst, diff},
                                               std::move(note_track));
            }
        } else if (*inst == Instrument::Drums) {
            song.m_drum_note_tracks
                = drum_note_tracks_from_midi(track, song.m_resolution);
        } else {
            auto tracks = note_tracks_from_midi(track, song.m_resolution);
            for (auto& [diff, note_track] : tracks) {
                song.m_five_fret_tracks.emplace(std::tuple {*inst, diff},
                                                std::move(note_track));
            }
        }
    }

    return song;
}
