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

#include "songparts.hpp"

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

TEST_CASE("NoteTrack ctor maintains invariants")
{
    SECTION("Notes are sorted")
    {
        std::vector<Note<NoteColour>> notes {{768}, {384}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        std::vector<Note<NoteColour>> sorted_notes {{384}, {768}};

        REQUIRE(track.notes() == sorted_notes);
    }

    SECTION("Notes of the same colour and position are merged")
    {
        std::vector<Note<NoteColour>> notes {{768, 0}, {768, 768}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        std::vector<Note<NoteColour>> required_notes {{768, 768}};

        REQUIRE(track.notes() == required_notes);

        std::vector<Note<NoteColour>> second_notes {{768, 768}, {768, 0}};
        NoteTrack<NoteColour> second_track {second_notes, {}, {}, 192};
        std::vector<Note<NoteColour>> second_required_notes {{768, 0}};

        REQUIRE(second_track.notes() == second_required_notes);
    }

    SECTION("Notes of different colours are dealt with separately")
    {
        std::vector<Note<NoteColour>> notes {{768, 0, NoteColour::Green},
                                             {768, 0, NoteColour::Red},
                                             {768, 768, NoteColour::Green}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        std::vector<Note<NoteColour>> required_notes {
            {768, 768, NoteColour::Green}, {768, 0, NoteColour::Red}};

        REQUIRE(track.notes() == required_notes);
    }

    SECTION("Resolution is positive")
    {
        std::vector<Note<NoteColour>> notes {{768}};
        REQUIRE_THROWS_AS(
            [&] { return NoteTrack<NoteColour>(notes, {}, {}, 0); }(),
            ParseError);
    }

    SECTION("Empty SP phrases are culled")
    {
        std::vector<Note<NoteColour>> notes {{768}};
        std::vector<StarPower> phrases {{0, 100}, {700, 100}, {1000, 100}};
        NoteTrack<NoteColour> track {notes, phrases, {}, 192};
        std::vector<StarPower> required_phrases {{700, 100}};

        REQUIRE(track.sp_phrases() == required_phrases);
    }

    SECTION("SP phrases are sorted")
    {
        std::vector<Note<NoteColour>> notes {{768}, {1000}};
        std::vector<StarPower> phrases {{1000, 1}, {768, 1}};
        NoteTrack<NoteColour> track {notes, phrases, {}, 192};
        std::vector<StarPower> required_phrases {{768, 1}, {1000, 1}};

        REQUIRE(track.sp_phrases() == required_phrases);
    }

    SECTION("SP phrases do not overlap")
    {
        std::vector<Note<NoteColour>> notes {{768}, {1000}, {1500}};
        std::vector<StarPower> phrases {{768, 1000}, {900, 150}};
        NoteTrack<NoteColour> track {notes, phrases, {}, 192};
        std::vector<StarPower> required_phrases {{768, 282}, {1050, 718}};

        REQUIRE(track.sp_phrases() == required_phrases);
    }

    SECTION("Solos are sorted")
    {
        std::vector<Note<NoteColour>> notes {{0}, {768}};
        std::vector<Solo> solos {{768, 868, 100}, {0, 100, 100}};
        NoteTrack<NoteColour> track {notes, {}, solos, 192};
        std::vector<Solo> required_solos {{0, 100, 100}, {768, 868, 100}};

        REQUIRE(track.solos() == required_solos);
    }
}

TEST_CASE("Base score for average multiplier is correct")
{
    SECTION("Base score is correct for songs without sustains")
    {
        std::vector<Note<NoteColour>> notes {
            {192}, {384}, {384, 0, NoteColour::Red}};

        NoteTrack<NoteColour> track {notes, {}, {}, 192};

        REQUIRE(track.base_score() == 150);
    }

    SECTION("Base score is correct for songs with sustains")
    {
        std::vector<Note<NoteColour>> notes_one {{192, 192}};
        std::vector<Note<NoteColour>> notes_two {{192, 92}};
        std::vector<Note<NoteColour>> notes_three {{192, 93}};

        NoteTrack<NoteColour> track_one {notes_one, {}, {}, 192};
        NoteTrack<NoteColour> track_two {notes_two, {}, {}, 192};
        NoteTrack<NoteColour> track_three {notes_three, {}, {}, 192};

        REQUIRE(track_one.base_score() == 75);
        REQUIRE(track_two.base_score() == 62);
        REQUIRE(track_three.base_score() == 63);
    }

    SECTION("Base score is correct for songs with chord sustains")
    {
        std::vector<Note<NoteColour>> notes {{192, 192},
                                             {192, 192, NoteColour::Red}};

        NoteTrack<NoteColour> track {notes, {}, {}, 192};

        REQUIRE(track.base_score() == 125);
    }

    SECTION("Base score is correct for other resolutions")
    {
        std::vector<Note<NoteColour>> notes {{192, 192}};

        NoteTrack<NoteColour> track {notes, {}, {}, 480};

        REQUIRE(track.base_score() == 60);
    }

    SECTION("Fractional ticks from multiple holds are added together correctly")
    {
        std::vector<Note<NoteColour>> notes {{0, 100}, {192, 100}};

        NoteTrack<NoteColour> track {notes, {}, {}, 192};

        REQUIRE(track.base_score() == 127);
    }

    SECTION("Disjoint chords are handled correctly")
    {
        std::vector<Note<NoteColour>> notes {
            {0, 384}, {0, 384, NoteColour::Red}, {0, 192, NoteColour::Yellow}};

        NoteTrack<NoteColour> track {notes, {}, {}, 192};

        REQUIRE(track.base_score() == 275);
    }
}

TEST_CASE("trim_sustains is correct")
{
    const std::vector<Note<NoteColour>> notes {{0, 65}, {200, 70}, {400, 140}};
    const NoteTrack<NoteColour> track {notes, {}, {}, 200};

    SECTION("100% speed")
    {
        auto new_track = track.trim_sustains(100);
        const auto& new_notes = new_track.notes();

        REQUIRE(new_notes[0].length == 0);
        REQUIRE(new_notes[1].length == 70);
        REQUIRE(new_notes[2].length == 140);
        REQUIRE(new_track.base_score() == 177);
    }

    SECTION("50% speed")
    {
        auto new_track = track.trim_sustains(50);
        const auto& new_notes = new_track.notes();

        REQUIRE(new_notes[0].length == 0);
        REQUIRE(new_notes[1].length == 70);
        REQUIRE(new_notes[2].length == 140);
        REQUIRE(new_track.base_score() == 185);
    }

    SECTION("200% speed")
    {
        auto new_track = track.trim_sustains(200);
        const auto& new_notes = new_track.notes();

        REQUIRE(new_notes[0].length == 0);
        REQUIRE(new_notes[1].length == 0);
        REQUIRE(new_notes[2].length == 140);
        REQUIRE(new_track.base_score() == 168);
    }
}

TEST_CASE("SyncTrack ctor maintains invariants")
{
    SECTION("BPMs are sorted by position")
    {
        SyncTrack track {{}, {{0, 150000}, {2000, 200000}, {1000, 225000}}};
        std::vector<BPM> expected_bpms {
            {0, 150000}, {1000, 225000}, {2000, 200000}};

        REQUIRE(track.bpms() == expected_bpms);
    }

    SECTION("No two BPMs have the same position")
    {
        SyncTrack track {{}, {{0, 150000}, {0, 200000}}};
        std::vector<BPM> expected_bpms {{0, 200000}};

        REQUIRE(track.bpms() == expected_bpms);
    }

    SECTION("bpms() is never empty")
    {
        SyncTrack track;
        std::vector<BPM> expected_bpms {{0, 120000}};

        REQUIRE(track.bpms() == expected_bpms);
    }

    SECTION("TimeSignatures are sorted by position")
    {
        SyncTrack track {{{0, 4, 4}, {2000, 3, 3}, {1000, 2, 2}}, {}};
        std::vector<TimeSignature> expected_tses {
            {0, 4, 4}, {1000, 2, 2}, {2000, 3, 3}};

        REQUIRE(track.time_sigs() == expected_tses);
    }

    SECTION("No two TimeSignatures have the same position")
    {
        SyncTrack track {{{0, 4, 4}, {0, 3, 4}}, {}};
        std::vector<TimeSignature> expected_tses {{0, 3, 4}};

        REQUIRE(track.time_sigs() == expected_tses);
    }

    SECTION("time_sigs() is never empty")
    {
        SyncTrack track;
        std::vector<TimeSignature> expected_tses {{0, 4, 4}};

        REQUIRE(track.time_sigs() == expected_tses);
    }

    SECTION("BPMs must not be zero or negative")
    {
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{}, {{192, 0}}};
                          })(),
                          ParseError);
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{}, {{192, -1}}};
                          })(),
                          ParseError);
    }

    SECTION("Time Signatures must be positive/positive")
    {
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{{0, 0, 4}}, {}};
                          })(),
                          ParseError);
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{{0, -1, 4}}, {}};
                          })(),
                          ParseError);
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{{0, 4, 0}}, {}};
                          })(),
                          ParseError);
        REQUIRE_THROWS_AS(([&] {
                              return SyncTrack {{{0, 4, -1}}, {}};
                          })(),
                          ParseError);
    }
}

TEST_CASE("Speedup returns the correct SyncTrack")
{
    const SyncTrack sync_track {{{0, 4, 4}}, {{0, 120000}, {192, 240000}}};
    const std::vector<BPM> expected_bpms {{0, 180000}, {192, 360000}};
    const std::vector<TimeSignature> expected_tses {{0, 4, 4}};

    const auto speedup = sync_track.speedup(150);

    REQUIRE(speedup.bpms() == expected_bpms);
    REQUIRE(speedup.time_sigs() == expected_tses);
}
