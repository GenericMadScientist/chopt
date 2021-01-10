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

#include "catch.hpp"

#include "song.hpp"

static bool operator==(const BPM& lhs, const BPM& rhs)
{
    return std::tie(lhs.position, lhs.bpm) == std::tie(rhs.position, rhs.bpm);
}

template <typename T>
static bool operator==(const Note<T>& lhs, const Note<T>& rhs)
{
    return std::tie(lhs.position, lhs.length, lhs.colour)
        == std::tie(rhs.position, rhs.length, rhs.colour);
}

static bool operator==(const Solo& lhs, const Solo& rhs)
{
    return std::tie(lhs.start, lhs.end, lhs.value)
        == std::tie(rhs.start, rhs.end, rhs.value);
}

static bool operator==(const StarPower& lhs, const StarPower& rhs)
{
    return std::tie(lhs.position, lhs.length)
        == std::tie(rhs.position, rhs.length);
}

static bool operator==(const TimeSignature& lhs, const TimeSignature& rhs)
{
    return std::tie(lhs.position, lhs.numerator, lhs.denominator)
        == std::tie(rhs.position, rhs.numerator, rhs.denominator);
}

template <typename T>
static bool operator==(const NoteTrack<T>& lhs, const NoteTrack<T>& rhs)
{
    return std::tie(lhs.notes(), lhs.sp_phrases(), lhs.solos())
        == std::tie(rhs.notes(), rhs.sp_phrases(), rhs.solos());
}

TEST_CASE("Chart -> Song has correct value for is_from_midi")
{
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};
    std::vector<ChartSection> sections {expert_single};
    const Chart chart {sections};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(!song.is_from_midi());
}

TEST_CASE("Chart reads resolution")
{
    ChartSection sync_track {"SyncTrack", {}, {}, {}, {}, {}, {}};
    ChartSection events {"Events", {}, {}, {}, {}, {}, {}};
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};

    SECTION("Default is 192 Res")
    {
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};

        const auto resolution = Song::from_chart(chart, {}).resolution();

        REQUIRE(resolution == 192);
    }

    SECTION("Default is overriden by specified value")
    {
        ChartSection header {
            "Song", {{"Resolution", "200"}, {"Offset", "100"}}, {}, {}, {}, {},
            {}};
        std::vector<ChartSection> sections {header, expert_single};
        const Chart chart {sections};

        const auto resolution = Song::from_chart(chart, {}).resolution();

        REQUIRE(resolution == 200);
    }

    SECTION("Bad values are ignored")
    {
        ChartSection header {
            "Song", {{"Resolution", "\"480\""}}, {}, {}, {}, {}, {}};
        std::vector<ChartSection> sections {header, expert_single};
        const Chart chart {sections};

        const auto resolution = Song::from_chart(chart, {}).resolution();

        REQUIRE(resolution == 192);
    }
}

TEST_CASE("Chart header values besides resolution are discarded")
{
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};
    ChartSection header_section {"Song",
                                 {{"Name", "\"TestName\""},
                                  {"Artist", "\"GMS\""},
                                  {"Charter", "\"NotGMS\""}},
                                 {},
                                 {},
                                 {},
                                 {},
                                 {}};
    std::vector<ChartSection> sections {header_section, expert_single};
    const Chart chart {sections};
    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.name() != "TestName");
    REQUIRE(song.artist() != "GMS");
    REQUIRE(song.charter() != "NotGMS");
}

TEST_CASE("Ini values are used for converting from .chart files")
{
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};
    ChartSection header_section {"Song", {}, {}, {}, {}, {}, {}};
    std::vector<ChartSection> sections {header_section, expert_single};
    const Chart chart {sections};
    const IniValues ini {"TestName", "GMS", "NotGMS"};
    const auto song = Song::from_chart(chart, ini);

    REQUIRE(song.name() == "TestName");
    REQUIRE(song.artist() == "GMS");
    REQUIRE(song.charter() == "NotGMS");
}

TEST_CASE("Chart reads sync track correctly")
{
    ChartSection sync_track {"SyncTrack", {}, {{0, 200000}},           {},
                             {},          {}, {{0, 4, 2}, {768, 4, 1}}};
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};
    std::vector<ChartSection> sections {sync_track, expert_single};
    const Chart chart {sections};
    std::vector<TimeSignature> time_sigs {{0, 4, 4}, {768, 4, 2}};
    std::vector<BPM> bpms {{0, 200000}};

    const auto chart_sync_track = Song::from_chart(chart, {}).sync_track();

    REQUIRE(chart_sync_track.time_sigs() == time_sigs);
    REQUIRE(chart_sync_track.bpms() == bpms);
}

TEST_CASE("Large time sig denominators cause an exception")
{
    ChartSection sync_track {"SyncTrack", {}, {}, {}, {}, {}, {{0, 4, 32}}};
    ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                {{768, 0, 0}},  {}, {}};
    std::vector<ChartSection> sections {sync_track, expert_single};
    const Chart chart {sections};

    REQUIRE_THROWS_AS([&] { return Song::from_chart(chart, {}); }(),
                      ParseError);
}

TEST_CASE("Chart reads easy note track correctly")
{
    ChartSection easy_single {"EasySingle",    {}, {}, {}, {{768, 0, 0}},
                              {{768, 2, 100}}, {}};
    std::vector<ChartSection> sections {easy_single};
    const Chart chart {sections};
    NoteTrack<NoteColour> note_track {
        {{768, 0, NoteColour::Green}}, {{768, 100}}, {}, 192};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.guitar_note_track(Difficulty::Easy) == note_track);
}

TEST_CASE("Invalid note values are ignored")
{
    ChartSection expert_single {
        "ExpertSingle", {}, {}, {}, {{768, 0, 0}, {768, 13, 0}}, {}, {}};
    std::vector<ChartSection> sections {expert_single};
    const Chart chart {sections};
    NoteTrack<NoteColour> note_track {
        {{768, 0, NoteColour::Green}}, {}, {}, 192};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.guitar_note_track(Difficulty::Expert) == note_track);
}

TEST_CASE("SP phrases are read correctly from Chart")
{
    ChartSection expert_single {"ExpertSingle",  {}, {}, {}, {{768, 0, 0}},
                                {{768, 1, 100}}, {}};
    std::vector<ChartSection> sections {expert_single};
    const Chart chart {sections};
    NoteTrack<NoteColour> note_track {
        {{768, 0, NoteColour::Green}}, {{768, 100}}, {}, 192};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.guitar_note_track(Difficulty::Expert).sp_phrases().empty());
}

TEST_CASE("Chart does not need sections in usual order")
{
    SECTION("Non note sections need not be present")
    {
        ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                    {{768, 0, 0}},  {}, {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};

        REQUIRE_NOTHROW([&] { return Song::from_chart(chart, {}); }());
    }

    SECTION("At least one non-empty note section must be present")
    {
        ChartSection expert_single {"ExpertSingle",  {}, {}, {}, {},
                                    {{768, 2, 100}}, {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};

        REQUIRE_THROWS_AS([] { return Song::from_chart({}, {}); }(),
                          ParseError);
        REQUIRE_THROWS_AS([&] { return Song::from_chart(chart, {}); }(),
                          ParseError);
    }

    SECTION("Non note sections can be in any order")
    {
        ChartSection expert_single {"ExpertSingle", {}, {}, {},
                                    {{768, 0, 0}},  {}, {}};
        ChartSection sync_track {"SyncTrack", {}, {{0, 200000}}, {}, {},
                                 {},          {}};
        ChartSection header {"Song", {{"Resolution", "200"}}, {}, {}, {}, {},
                             {}};
        std::vector<ChartSection> sections {sync_track, expert_single, header};
        const Chart chart {sections};
        std::vector<Note<NoteColour>> notes {{768}};
        std::vector<BPM> bpms {{0, 200000}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.resolution() == 200);
        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes() == notes);
        REQUIRE(song.sync_track().bpms() == bpms);
    }
}

TEST_CASE("Only first non-empty part of note sections matter")
{
    SECTION("Later non-empty sections are ignored")
    {
        ChartSection expert_single_one {"ExpertSingle", {}, {}, {},
                                        {{768, 0, 0}},  {}, {}};
        ChartSection expert_single_two {"ExpertSingle", {}, {}, {},
                                        {{768, 1, 0}},  {}, {}};
        std::vector<ChartSection> sections {expert_single_one,
                                            expert_single_two};
        const Chart chart {sections};
        std::vector<Note<NoteColour>> notes {{768, 0, NoteColour::Green}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes() == notes);
    }

    SECTION("Leading empty sections are ignored")
    {
        ChartSection expert_single_one {"ExpertSingle", {}, {}, {}, {}, {}, {}};
        ChartSection expert_single_two {"ExpertSingle", {}, {}, {},
                                        {{768, 1, 0}},  {}, {}};
        std::vector<ChartSection> sections {expert_single_one,
                                            expert_single_two};
        const Chart chart {sections};
        std::vector<Note<NoteColour>> notes {{768, 0, NoteColour::Red}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes() == notes);
    }
}

TEST_CASE("Solos are read properly")
{
    SECTION("Expected solos are read properly")
    {
        ChartSection expert_single {
            "ExpertSingle",
            {},
            {},
            {{0, "solo"}, {200, "soloend"}, {300, "solo"}, {400, "soloend"}},
            {{100, 0, 0}, {300, 0, 0}, {400, 0, 0}},
            {},
            {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};
        std::vector<Solo> required_solos {{0, 200, 100}, {300, 400, 200}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos()
                == required_solos);
    }

    SECTION("Chords are not counted double")
    {
        ChartSection expert_single {"ExpertSingle",
                                    {},
                                    {},
                                    {{0, "solo"}, {200, "soloend"}},
                                    {{100, 0, 0}, {100, 1, 0}},
                                    {},
                                    {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};
        std::vector<Solo> required_solos {{0, 200, 100}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos()
                == required_solos);
    }

    SECTION("Empty solos are ignored")
    {
        ChartSection expert_single {
            "ExpertSingle", {}, {}, {{100, "solo"}, {200, "soloend"}},
            {{0, 0, 0}},    {}, {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos().empty());
    }

    SECTION("Repeated solo starts and ends don't matter")
    {
        ChartSection expert_single {
            "ExpertSingle",
            {},
            {},
            {{0, "solo"}, {100, "solo"}, {200, "soloend"}, {300, "soloend"}},
            {{100, 0, 0}},
            {},
            {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};
        std::vector<Solo> required_solos {{0, 200, 100}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos()
                == required_solos);
    }

    SECTION("Solo markers are sorted")
    {
        ChartSection expert_single {
            "ExpertSingle", {}, {}, {{384, "soloend"}, {0, "solo"}},
            {{192, 0, 0}},  {}, {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};
        std::vector<Solo> required_solos {{0, 384, 100}};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos()
                == required_solos);
    }

    SECTION("Solos with no soloend event are ignored")
    {
        ChartSection expert_single {"ExpertSingle", {}, {}, {{0, "solo"}},
                                    {{192, 0, 0}},  {}, {}};
        std::vector<ChartSection> sections {expert_single};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).solos().empty());
    }
}

TEST_CASE("instruments returns the supported instruments")
{
    ChartSection guitar {"ExpertSingle", {}, {}, {}, {{192, 0, 0}}, {}, {}};
    ChartSection bass {"ExpertDoubleBass", {}, {}, {}, {}, {}, {}};
    ChartSection drums {"ExpertDrums", {}, {}, {}, {{192, 0, 0}}, {}, {}};
    std::vector<ChartSection> sections {guitar, bass, drums};
    const Chart chart {sections};
    std::vector<Instrument> instruments {Instrument::Guitar, Instrument::Drums};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.instruments() == instruments);
}

TEST_CASE("difficulties returns the difficulties for an instrument")
{
    ChartSection guitar {"ExpertSingle", {}, {}, {}, {{192, 0, 0}}, {}, {}};
    ChartSection hard_guitar {"HardSingle", {}, {}, {}, {{192, 0, 0}}, {}, {}};
    ChartSection drums {"ExpertDrums", {}, {}, {}, {{192, 0, 0}}, {}, {}};
    std::vector<ChartSection> sections {guitar, hard_guitar, drums};
    const Chart chart {sections};
    std::vector<Difficulty> guitar_difficulties {Difficulty::Hard,
                                                 Difficulty::Expert};
    std::vector<Difficulty> drum_difficulties {Difficulty::Expert};

    const auto song = Song::from_chart(chart, {});

    REQUIRE(song.difficulties(Instrument::Guitar) == guitar_difficulties);
    REQUIRE(song.difficulties(Instrument::Drums) == drum_difficulties);
}

TEST_CASE("Other 5 fret instruments are read from Chart")
{
    SECTION("Guitar Co-op is read")
    {
        ChartSection expert_double {"ExpertDoubleGuitar", {}, {}, {},
                                    {{192, 0, 0}},        {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE_NOTHROW(
            [&] { return song.guitar_coop_note_track(Difficulty::Expert); }());
    }

    SECTION("Bass is read")
    {
        ChartSection expert_double {"ExpertDoubleBass", {}, {}, {},
                                    {{192, 0, 0}},      {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE_NOTHROW(
            [&] { return song.bass_note_track(Difficulty::Expert); }());
    }

    SECTION("Rhythm is read")
    {
        ChartSection expert_double {"ExpertDoubleRhythm", {}, {}, {},
                                    {{192, 0, 0}},        {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE_NOTHROW(
            [&] { return song.rhythm_note_track(Difficulty::Expert); }());
    }

    SECTION("Keys is read")
    {
        ChartSection expert_double {"ExpertKeyboard", {}, {}, {},
                                    {{192, 0, 0}},    {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};

        const auto song = Song::from_chart(chart, {});

        REQUIRE_NOTHROW(
            [&] { return song.keys_note_track(Difficulty::Expert); }());
    }
}

TEST_CASE("6 fret instruments are read correctly from .chart")
{
    SECTION("6 fret guitar is read correctly")
    {
        ChartSection expert_double {"ExpertGHLGuitar",          {}, {}, {},
                                    {{192, 0, 0}, {384, 3, 0}}, {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};
        std::vector<Note<GHLNoteColour>> notes {
            {192, 0, GHLNoteColour::WhiteLow},
            {384, 0, GHLNoteColour::BlackLow}};

        const auto song = Song::from_chart(chart, {});
        const auto& track = song.ghl_guitar_note_track(Difficulty::Expert);

        REQUIRE(track.notes() == notes);
    }

    SECTION("6 fret bass is read correctly")
    {
        ChartSection expert_double {
            "ExpertGHLBass", {}, {}, {}, {{192, 0, 0}, {384, 3, 0}}, {}, {}};
        std::vector<ChartSection> sections {expert_double};
        const Chart chart {sections};
        std::vector<Note<GHLNoteColour>> notes {
            {192, 0, GHLNoteColour::WhiteLow},
            {384, 0, GHLNoteColour::BlackLow}};

        const auto song = Song::from_chart(chart, {});
        const auto& track = song.ghl_bass_note_track(Difficulty::Expert);

        REQUIRE(track.notes() == notes);
    }
}

TEST_CASE("Drums are read correctly from .chart")
{
    ChartSection expert_drums {
        "ExpertDrums",
        {},
        {},
        {},
        {{192, 1, 0}, {384, 2, 0}, {384, 66, 0}, {576, 5, 0}},
        {},
        {}};
    std::vector<ChartSection> sections {expert_drums};
    const Chart chart {sections};
    std::vector<Note<DrumNoteColour>> notes {
        {192, 0, DrumNoteColour::Red}, {384, 0, DrumNoteColour::YellowCymbal}};

    const auto song = Song::from_chart(chart, {});
    const auto& track = song.drum_note_track(Difficulty::Expert);

    REQUIRE(track.notes() == notes);
}

TEST_CASE("Invalid drum notes are ignored")
{
    ChartSection drums {
        "ExpertDrums", {}, {}, {}, {{192, 1, 0}, {192, 6, 0}}, {}, {}};
    std::vector<ChartSection> sections {drums};
    const Chart chart {sections};

    const auto song = Song::from_chart(chart, {});
    const auto& track = song.drum_note_track(Difficulty::Expert);

    REQUIRE(track.notes().size() == 1);
}

TEST_CASE("Midi -> Song has correct value for is_from_midi")
{
    const Midi midi {192, {}};

    const auto song = Song::from_midi(midi, {});

    REQUIRE(song.is_from_midi());
}

TEST_CASE("Midi resolution is read correctly")
{
    SECTION("Midi's resolution is read")
    {
        const Midi midi {200, {}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.resolution() == 200);
    }

    SECTION("Resolution > 0 invariant is upheld")
    {
        const Midi midi {0, {}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }
}

TEST_CASE("First track is read correctly")
{
    SECTION("Tempos are read correctly")
    {
        MidiTrack tempo_track {{{0, {MetaEvent {0x51, {6, 0x1A, 0x80}}}},
                                {1920, {MetaEvent {0x51, {4, 0x93, 0xE0}}}}}};
        const Midi midi {192, {tempo_track}};
        SyncTrack tempos {{}, {{0, 150000}, {1920, 200000}}};

        const auto song = Song::from_midi(midi, {});
        const auto& sync_track = song.sync_track();

        REQUIRE(sync_track.bpms() == tempos.bpms());
        REQUIRE(sync_track.time_sigs() == tempos.time_sigs());
    }

    SECTION("Too short tempo events cause an exception")
    {
        MidiTrack tempo_track {{{0, {MetaEvent {0x51, {6, 0x1A}}}}}};
        const Midi midi {192, {tempo_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }

    SECTION("Time signatures are read correctly")
    {
        MidiTrack ts_track {{{0, {MetaEvent {0x58, {6, 2, 24, 8}}}},
                             {1920, {MetaEvent {0x58, {3, 3, 24, 8}}}}}};
        const Midi midi {192, {ts_track}};
        SyncTrack tses {{{0, 6, 4}, {1920, 3, 8}}, {}};

        const auto song = Song::from_midi(midi, {});
        const auto& sync_track = song.sync_track();

        REQUIRE(sync_track.bpms() == tses.bpms());
        REQUIRE(sync_track.time_sigs() == tses.time_sigs());
    }

    SECTION("Time signatures with large denominators cause an exception")
    {
        MidiTrack ts_track {{{0, {MetaEvent {0x58, {6, 32, 24, 8}}}}}};
        const Midi midi {192, {ts_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }

    SECTION("Too short time sig events cause an exception")
    {
        MidiTrack ts_track {{{0, {MetaEvent {0x58, {6}}}}}};
        const Midi midi {192, {ts_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }

    SECTION("Song name is not read from midi")
    {
        MidiTrack name_track {{{0, {MetaEvent {1, {72, 101, 108, 108, 111}}}}}};
        const Midi midi {192, {name_track}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.name() != "Hello");
    }
}

TEST_CASE("Ini values are used when converting from .mid files")
{
    const Midi midi {192, {}};
    const IniValues ini {"TestName", "GMS", "NotGMS"};

    const auto song = Song::from_midi(midi, ini);

    REQUIRE(song.name() == "TestName");
    REQUIRE(song.artist() == "GMS");
    REQUIRE(song.charter() == "NotGMS");
}

TEST_CASE("Notes are read correctly")
{
    SECTION("Notes of every difficulty are read")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {768, {MidiEvent {0x90, {84, 64}}}},
                               {768, {MidiEvent {0x90, {72, 64}}}},
                               {768, {MidiEvent {0x90, {60, 64}}}},
                               {960, {MidiEvent {0x80, {96, 0}}}},
                               {960, {MidiEvent {0x80, {84, 0}}}},
                               {960, {MidiEvent {0x80, {72, 0}}}},
                               {960, {MidiEvent {0x80, {60, 0}}}}}};
        const Midi midi {192, {note_track}};
        const std::vector<Note<NoteColour>> green_note {
            {768, 192, NoteColour::Green}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Easy).notes() == green_note);
        REQUIRE(song.guitar_note_track(Difficulty::Medium).notes()
                == green_note);
        REQUIRE(song.guitar_note_track(Difficulty::Hard).notes() == green_note);
        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes()
                == green_note);
    }

    SECTION("Notes are read from PART GUITAR")
    {
        MidiTrack other_track {{{768, {MidiEvent {0x90, {96, 64}}}},
                                {960, {MidiEvent {0x80, {96, 0}}}}}};
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {97, 64}}}},
                               {960, {MidiEvent {0x80, {97, 0}}}}}};
        const Midi midi {192, {other_track, note_track}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes()[0].colour
                == NoteColour::Red);
    }

    SECTION("PART GUITAR event need note be the first event")
    {
        MidiTrack note_track {
            {{0, {MetaEvent {0x7F, {0x05, 0x0F, 0x09, 0x08, 0x40}}}},
             {0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                           0x41, 0x52}}}},
             {768, {MidiEvent {0x90, {97, 64}}}},
             {960, {MidiEvent {0x80, {97, 0}}}}}};
        const Midi midi {192, {note_track}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes()[0].colour
                == NoteColour::Red);
    }

    SECTION("Guitar notes are also read from T1 GEMS")
    {
        MidiTrack other_track {{{768, {MidiEvent {0x90, {96, 64}}}},
                                {960, {MidiEvent {0x80, {96, 0}}}}}};
        MidiTrack note_track {
            {{0, {MetaEvent {3, {0x54, 0x31, 0x20, 0x47, 0x45, 0x4D, 0x53}}}},
             {768, {MidiEvent {0x90, {97, 64}}}},
             {960, {MidiEvent {0x80, {97, 0}}}}}};
        const Midi midi {192, {other_track, note_track}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes()[0].colour
                == NoteColour::Red);
    }

    SECTION("Note On events must have a corresponding Note Off event")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {960, {MidiEvent {0x80, {96, 64}}}},
                               {1152, {MidiEvent {0x90, {96, 64}}}}}};
        const Midi midi {192, {note_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }

    SECTION("Corresponding Note Off events are after Note On events")
    {
        MidiTrack note_track {{
            {0,
             {MetaEvent {3,
                         {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                          0x41, 0x52}}}},
            {480, {MidiEvent {0x80, {96, 64}}}},
            {480, {MidiEvent {0x90, {96, 64}}}},
            {960, {MidiEvent {0x80, {96, 64}}}},
            {960, {MidiEvent {0x90, {96, 64}}}},
            {1440, {MidiEvent {0x80, {96, 64}}}},
        }};
        const Midi midi {480, {note_track}};

        const auto song = Song::from_midi(midi, {});
        const auto& notes = song.guitar_note_track(Difficulty::Expert).notes();

        REQUIRE(notes.size() == 2);
        REQUIRE(notes[0].length == 480);
    }

    SECTION("Note On events with velocity 0 count as Note Off events")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {960, {MidiEvent {0x90, {96, 0}}}}}};
        const Midi midi {192, {note_track}};

        REQUIRE_NOTHROW([&] { return Song::from_midi(midi, {}); }());
    }

    SECTION(
        "Note On events with no intermediate Note Off events are not merged")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {769, {MidiEvent {0x90, {96, 64}}}},
                               {800, {MidiEvent {0x80, {96, 64}}}},
                               {801, {MidiEvent {0x80, {96, 64}}}}}};
        const Midi midi {192, {note_track}};

        const auto song = Song::from_midi(midi, {});
        const auto& notes = song.guitar_note_track(Difficulty::Expert).notes();

        REQUIRE(notes.size() == 2);
    }

    SECTION("Each Note On event consumes the following Note Off event")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {769, {MidiEvent {0x90, {96, 64}}}},
                               {800, {MidiEvent {0x80, {96, 64}}}},
                               {1000, {MidiEvent {0x80, {96, 64}}}}}};
        const Midi midi {192, {note_track}};

        const auto song = Song::from_midi(midi, {});
        const auto& notes = song.guitar_note_track(Difficulty::Expert).notes();

        REQUIRE(notes.size() == 2);
        REQUIRE(notes[1].length > 0);
    }

    SECTION("Note Off events can be 0 ticks after the Note On events")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {768, {MidiEvent {0x80, {96, 64}}}}}};
        const Midi midi {192, {note_track}};

        const auto song = Song::from_midi(midi, {});
        const auto& notes = song.guitar_note_track(Difficulty::Expert).notes();

        REQUIRE(notes.size() == 1);
    }

    SECTION("ParseError thrown if NoteOn has no corresponding NoteOff track")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}}}};
        const Midi midi {192, {note_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }

    SECTION("Open notes are read correctly")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                           0x41, 0x52}}}},
             {768, {MidiEvent {0x90, {96, 64}}}},
             {768, {SysexEvent {{0x50, 0x53, 0, 0, 3, 1, 1, 0xF7}}}},
             {770, {SysexEvent {{0x50, 0x53, 0, 0, 3, 1, 0, 0xF7}}}},
             {960, {MidiEvent {0x90, {96, 0}}}}}};
        const Midi midi {192, {note_track}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).notes()[0].colour
                == NoteColour::Open);
    }

    SECTION("ParseError thrown if open Note Ons have no Note Offs")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                           0x41, 0x52}}}},
             {768, {MidiEvent {0x90, {96, 64}}}},
             {768, {SysexEvent {{0x50, 0x53, 0, 0, 3, 1, 1, 0xF7}}}},
             {960, {MidiEvent {0x90, {96, 0}}}}}};
        const Midi midi {192, {note_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }
}

// Note that a note at the very end of a solo event is not considered part of
// the solo for a .mid, but it is for a .chart.
TEST_CASE("Solos are read from .mid correctly")
{
    MidiTrack note_track {{{0,
                            {MetaEvent {3,
                                        {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                         0x55, 0x49, 0x54, 0x41, 0x52}}}},
                           {768, {MidiEvent {0x90, {103, 64}}}},
                           {768, {MidiEvent {0x90, {96, 64}}}},
                           {900, {MidiEvent {0x90, {97, 64}}}},
                           {900, {MidiEvent {0x80, {103, 64}}}},
                           {960, {MidiEvent {0x80, {96, 0}}}},
                           {960, {MidiEvent {0x80, {97, 64}}}}}};
    const Midi midi {192, {note_track}};
    const std::vector<Solo> solos {{768, 900, 100}};

    const auto song = Song::from_midi(midi, {});

    REQUIRE(song.guitar_note_track(Difficulty::Expert).solos() == solos);
}

TEST_CASE("Star Power is read")
{
    SECTION("A single phrase is read")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {116, 64}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {900, {MidiEvent {0x80, {116, 64}}}},
                               {960, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};
        const std::vector<StarPower> sp_phrases {{768, 132}};

        const auto song = Song::from_midi(midi, {});

        REQUIRE(song.guitar_note_track(Difficulty::Expert).sp_phrases()
                == sp_phrases);
    }

    SECTION("A Note Off event is required for every phrase")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                             0x55, 0x49, 0x54, 0x41, 0x52}}}},
                               {768, {MidiEvent {0x90, {116, 64}}}},
                               {768, {MidiEvent {0x90, {96, 64}}}},
                               {960, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};

        REQUIRE_THROWS_AS([&] { return Song::from_midi(midi, {}); }(),
                          ParseError);
    }
}

TEST_CASE(".mids with multiple solos and no Star Power have solos read as SP")
{
    MidiTrack note_track {{{0,
                            {MetaEvent {3,
                                        {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                         0x55, 0x49, 0x54, 0x41, 0x52}}}},
                           {768, {MidiEvent {0x90, {103, 64}}}},
                           {768, {MidiEvent {0x90, {96, 64}}}},
                           {800, {MidiEvent {0x80, {96, 64}}}},
                           {900, {MidiEvent {0x80, {103, 64}}}},
                           {950, {MidiEvent {0x90, {103, 64}}}},
                           {960, {MidiEvent {0x90, {97, 64}}}},
                           {1000, {MidiEvent {0x80, {97, 64}}}},
                           {1000, {MidiEvent {0x80, {103, 64}}}}}};
    const Midi midi {192, {note_track}};

    const auto song = Song::from_midi(midi, {});
    const auto& track = song.guitar_note_track(Difficulty::Expert);

    REQUIRE(track.solos().empty());
    REQUIRE(track.sp_phrases().size() == 2);
}

// This should be done by NoteTrack's trim_sustains method, because we may want
// to trim sustains for slowdowns.
TEST_CASE("Short midi sustains are not trimmed")
{
    MidiTrack note_track {{{0,
                            {MetaEvent {3,
                                        {0x50, 0x41, 0x52, 0x54, 0x20, 0x47,
                                         0x55, 0x49, 0x54, 0x41, 0x52}}}},
                           {0, {MidiEvent {0x90, {96, 64}}}},
                           {65, {MidiEvent {0x80, {96, 0}}}},
                           {100, {MidiEvent {0x90, {96, 64}}}},
                           {170, {MidiEvent {0x80, {96, 0}}}}}};
    const Midi midi {200, {note_track}};
    const auto song = Song::from_midi(midi, {});
    const auto& notes = song.guitar_note_track(Difficulty::Expert).notes();

    REQUIRE(notes[0].length == 65);
    REQUIRE(notes[1].length == 70);
}

TEST_CASE("Other 5 fret instruments are read from .mid")
{
    SECTION("Guitar Co-op is read")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                           0x41, 0x52, 0x20, 0x43, 0x4F, 0x4F, 0x50}}}},
             {0, {MidiEvent {0x90, {96, 64}}}},
             {65, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});

        REQUIRE_NOTHROW(
            [&] { return song.guitar_coop_note_track(Difficulty::Expert); }());
    }

    SECTION("Bass is read")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {
                  3, {0x50, 0x41, 0x52, 0x54, 0x20, 0x42, 0x41, 0x53, 0x53}}}},
             {0, {MidiEvent {0x90, {96, 64}}}},
             {65, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});

        REQUIRE_NOTHROW(
            [&] { return song.bass_note_track(Difficulty::Expert); }());
    }

    SECTION("Rhythm is read")
    {
        MidiTrack note_track {{{0,
                                {MetaEvent {3,
                                            {0x50, 0x41, 0x52, 0x54, 0x20, 0x52,
                                             0x48, 0x59, 0x54, 0x48, 0x4D}}}},
                               {0, {MidiEvent {0x90, {96, 64}}}},
                               {65, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});

        REQUIRE_NOTHROW(
            [&] { return song.rhythm_note_track(Difficulty::Expert); }());
    }

    SECTION("Keys is read")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {
                  3, {0x50, 0x41, 0x52, 0x54, 0x20, 0x4B, 0x45, 0x59, 0x53}}}},
             {0, {MidiEvent {0x90, {96, 64}}}},
             {65, {MidiEvent {0x80, {96, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});

        REQUIRE_NOTHROW(
            [&] { return song.keys_note_track(Difficulty::Expert); }());
    }
}

TEST_CASE("6 fret instruments are read correctly from .mid")
{
    SECTION("6 fret guitar is read correctly")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x47, 0x55, 0x49, 0x54,
                           0x41, 0x52, 0x20, 0x47, 0x48, 0x4C}}}},
             {0, {MidiEvent {0x90, {94, 64}}}},
             {65, {MidiEvent {0x80, {94, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});
        const auto& track = song.ghl_guitar_note_track(Difficulty::Expert);

        std::vector<Note<GHLNoteColour>> notes {{0, 65, GHLNoteColour::Open}};

        REQUIRE(track.notes() == notes);
    }

    SECTION("6 fret bass is read correctly")
    {
        MidiTrack note_track {
            {{0,
              {MetaEvent {3,
                          {0x50, 0x41, 0x52, 0x54, 0x20, 0x42, 0x41, 0x53, 0x53,
                           0x20, 0x47, 0x48, 0x4C}}}},
             {0, {MidiEvent {0x90, {94, 64}}}},
             {65, {MidiEvent {0x80, {94, 0}}}}}};
        const Midi midi {192, {note_track}};
        const auto song = Song::from_midi(midi, {});
        const auto& track = song.ghl_bass_note_track(Difficulty::Expert);

        std::vector<Note<GHLNoteColour>> notes {{0, 65, GHLNoteColour::Open}};

        REQUIRE(track.notes() == notes);
    }
}

TEST_CASE("Drums are read correctly from .mid")
{
    MidiTrack note_track {{
        {0,
         {MetaEvent {
             3, {0x50, 0x41, 0x52, 0x54, 0x20, 0x44, 0x52, 0x55, 0x4D, 0x53}}}},
        {0, {MidiEvent {0x90, {98, 64}}}},
        {0, {MidiEvent {0x90, {110, 64}}}},
        {65, {MidiEvent {0x80, {98, 0}}}},
        {65, {MidiEvent {0x80, {110, 0}}}},
    }};
    const Midi midi {192, {note_track}};
    const auto song = Song::from_midi(midi, {});
    const auto& track = song.drum_note_track(Difficulty::Expert);

    std::vector<Note<DrumNoteColour>> notes {{0, 0, DrumNoteColour::Yellow}};

    REQUIRE(track.notes() == notes);
}
