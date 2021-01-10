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

#include <algorithm>
#include <numeric>

#include "catch.hpp"

#include "points.hpp"

static bool operator==(const Beat& lhs, const Beat& rhs)
{
    return lhs.value() == Approx(rhs.value());
}

static bool operator==(const Position& lhs, const Position& rhs)
{
    return lhs.beat.value() == Approx(rhs.beat.value())
        && lhs.measure.value() == Approx(rhs.measure.value());
}

static std::vector<int> set_values(const PointSet& points)
{
    std::vector<int> values;
    values.reserve(static_cast<std::size_t>(
        std::distance(points.cbegin(), points.cend())));
    for (auto p = points.cbegin(); p < points.cend(); ++p) {
        values.push_back(p->value);
    }
    return values;
}

static std::vector<int> set_base_values(const PointSet& points)
{
    std::vector<int> base_values;
    base_values.reserve(static_cast<std::size_t>(
        std::distance(points.cbegin(), points.cend())));
    for (auto p = points.cbegin(); p < points.cend(); ++p) {
        base_values.push_back(p->base_value);
    }
    return base_values;
}

static std::vector<Beat> set_position_beats(const PointSet& points)
{
    std::vector<Beat> values;
    values.reserve(static_cast<std::size_t>(
        std::distance(points.cbegin(), points.cend())));
    for (auto p = points.cbegin(); p < points.cend(); ++p) {
        values.push_back(p->position.beat);
    }
    return values;
}

TEST_CASE("Non-hold notes")
{
    SECTION("Single notes give 50 points")
    {
        NoteTrack<NoteColour> track {{{768}, {960}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        std::vector<int> expected_values {50, 50};

        REQUIRE(set_values(points) == expected_values);
    }

    SECTION("Chords give multiples of 50 points")
    {
        NoteTrack<NoteColour> track {
            {{768, 0, NoteColour::Green}, {768, 0, NoteColour::Red}},
            {},
            {},
            192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        std::vector<int> expected_values {100};

        REQUIRE(set_values(points) == expected_values);
    }

    SECTION("GHL notes behave the same as 5 fret notes")
    {
        NoteTrack<GHLNoteColour> track {{{768}, {960}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        std::vector<int> expected_values {50, 50};

        REQUIRE(set_values(points) == expected_values);
    }
}

TEST_CASE("Hold notes")
{
    SECTION("Hold note points depend on resolution")
    {
        NoteTrack<NoteColour> track {{{768, 15}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet first_points {track, converter, 1.0, Second(0.0)};
        std::vector<int> first_expected_values {50, 3};
        std::vector<Beat> first_expected_beats {Beat(4.0), Beat(4.0026)};
        NoteTrack<NoteColour> second_track {{{768, 15}}, {}, {}, 200};
        TimeConverter second_converter {{}, 200};
        PointSet second_points {second_track, second_converter, 1.0,
                                Second(0.0)};
        std::vector<int> second_expected_values {50, 2};
        std::vector<Beat> second_expected_beats {Beat(3.84), Beat(3.8425)};

        REQUIRE(set_values(first_points) == first_expected_values);
        REQUIRE(set_position_beats(first_points) == first_expected_beats);
        REQUIRE(set_values(second_points) == second_expected_values);
        REQUIRE(set_position_beats(second_points) == second_expected_beats);
    }

    SECTION("Hold note points and chords")
    {
        NoteTrack<NoteColour> track {
            {{768, 8, NoteColour::Green}, {768, 8, NoteColour::Red}},
            {},
            {},
            192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        std::vector<int> expected_values {100, 2};
        std::vector<Beat> expected_beats {Beat(4.0), Beat(4.0026)};

        REQUIRE(set_values(points) == expected_values);
        REQUIRE(set_position_beats(points) == expected_beats);
    }

    SECTION("Resolutions below 25 do not enter an infinite loop")
    {
        NoteTrack<NoteColour> track {{{768, 2}}, {}, {}, 1};
        TimeConverter converter {{}, 1};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(std::distance(points.cbegin(), points.cend()) == 3);
    }

    SECTION("Sustains of uneven length are handled correctly")
    {
        NoteTrack<NoteColour> track {{{0, 1504, NoteColour::Green},
                                      {0, 1504, NoteColour::Red},
                                      {0, 736, NoteColour::Yellow}},
                                     {},
                                     {},
                                     192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        auto total_score = std::accumulate(
            points.cbegin(), points.cend(), 0,
            [](const auto& a, const auto& b) { return a + b.value; });

        REQUIRE(total_score == 686);
    }
}

TEST_CASE("Points are sorted")
{
    NoteTrack<NoteColour> track {{{768, 15}, {770, 0}}, {}, {}, 192};
    TimeConverter converter {{}, 192};
    PointSet points {track, converter, 1.0, Second(0.0)};
    const auto beats = set_position_beats(points);

    REQUIRE(std::is_sorted(beats.cbegin(), beats.cend()));
}

TEST_CASE("End of SP phrase points")
{
    NoteTrack<NoteColour> track {
        {{768}, {960}, {1152}}, {{768, 1}, {900, 50}, {1100, 53}}, {}, 192};
    TimeConverter converter {{}, 192};
    PointSet points {track, converter, 1.0, Second(0.0)};

    REQUIRE(points.cbegin()->is_sp_granting_note);
    REQUIRE(!std::next(points.cbegin())->is_sp_granting_note);
    REQUIRE(std::next(points.cbegin(), 2)->is_sp_granting_note);
}

TEST_CASE("Combo multiplier is taken into account")
{
    SECTION("Multiplier applies to non-holds")
    {
        std::vector<Note<NoteColour>> notes;
        notes.reserve(50);
        for (int i = 0; i < 50; ++i) {
            notes.push_back({192 * i});
        }
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        std::vector<int> expected_values;
        std::vector<int> expected_base_values;
        expected_values.reserve(50);
        expected_base_values.reserve(50);
        for (int i = 0; i < 50; ++i) {
            const auto mult = 1 + std::min((i + 1) / 10, 3);
            expected_values.push_back(50 * mult);
            expected_base_values.push_back(50);
        }

        REQUIRE(set_values(points) == expected_values);
        REQUIRE(set_base_values(points) == expected_base_values);
    }

    SECTION("Hold points are multiplied")
    {
        std::vector<Note<NoteColour>> notes;
        notes.reserve(50);
        for (int i = 0; i < 50; ++i) {
            notes.push_back({192 * i});
        }
        notes.push_back({9600, 192});

        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(std::prev(points.cend(), 2)->value == 4);
        REQUIRE(std::prev(points.cend(), 2)->base_value == 1);
    }

    SECTION("Later hold points in extended sustains are multiplied")
    {
        std::vector<Note<NoteColour>> notes;
        notes.reserve(10);
        for (int i = 0; i < 10; ++i) {
            notes.push_back({192 * i});
        }
        notes[0].length = 2000;

        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(std::prev(points.cend(), 2)->value == 2);
        REQUIRE(std::prev(points.cend(), 2)->base_value == 1);
    }

    SECTION("Drum notes have the multiplier handled correctly")
    {
        std::vector<Note<DrumNoteColour>> notes;
        notes.reserve(10);
        for (int i = 0; i < 9; ++i) {
            notes.push_back({192 * i, 0, DrumNoteColour::Red});
        }
        notes.push_back({192 * 7, 0, DrumNoteColour::Yellow});

        NoteTrack<DrumNoteColour> track {notes, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(std::prev(points.cend(), 1)->value == 100);
    }
}

TEST_CASE("Video lag is taken into account")
{
    const std::vector<Note<NoteColour>> notes {{192, 0}, {384, 192}};
    const NoteTrack<NoteColour> track {notes, {}, {}, 192};
    const TimeConverter converter {{}, 192};

    SECTION("Negative video lag is handled correctly")
    {
        PointSet points {track, converter, 1.0, Second(-0.20)};

        REQUIRE(points.cbegin()->position.beat == Beat(0.6));
        REQUIRE(points.cbegin()->hit_window_start.beat == Beat(0.46));
        REQUIRE(points.cbegin()->hit_window_end.beat == Beat(0.74));
        REQUIRE(std::next(points.cbegin(), 2)->position.beat == Beat(2.03385));
    }

    SECTION("Positive video lag is handled correctly")
    {
        PointSet points {track, converter, 1.0, Second(0.20)};

        REQUIRE(points.cbegin()->position.beat == Beat(1.4));
        REQUIRE(points.cbegin()->hit_window_start.beat == Beat(1.26));
        REQUIRE(points.cbegin()->hit_window_end.beat == Beat(1.54));
        REQUIRE(std::next(points.cbegin(), 2)->position.beat == Beat(2.03385));
    }

    SECTION("Tick points are not multiplied prematurely")
    {
        std::vector<Note<NoteColour>> other_notes {
            {192}, {193}, {194}, {195},      {196},
            {197}, {198}, {199}, {200, 200}, {400}};
        NoteTrack<NoteColour> other_track {other_notes, {}, {}, 192};
        PointSet points {other_track, converter, 1.0, Second(-0.40)};

        REQUIRE(std::prev(points.cend())->value == 100);
        REQUIRE(std::prev(points.cend(), 2)->value == 7);
    }
}

TEST_CASE("hit_window_start and hit_window_end are set correctly")
{
    TimeConverter converter {{{}, {{0, 150000}, {768, 200000}}}, 192};

    SECTION("Hit window starts for notes are correct")
    {
        std::vector<Note<NoteColour>> notes {{192}, {787}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(points.cbegin()->hit_window_start.beat == Beat(0.825));
        REQUIRE(std::next(points.cbegin())->hit_window_start.beat
                == Beat(3.89922));
    }

    SECTION("Hit window ends for notes are correct")
    {
        std::vector<Note<NoteColour>> notes {{192}, {749}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        REQUIRE(points.cbegin()->hit_window_end.beat == Beat(1.175));
        REQUIRE(std::next(points.cbegin())->hit_window_end.beat
                == Beat(4.10139));
    }

    SECTION("Hit window starts and ends for hold points are correct")
    {
        std::vector<Note<NoteColour>> notes {{672, 192}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};

        for (auto p = std::next(points.cbegin()); p < points.cend(); ++p) {
            REQUIRE(p->position.beat == p->hit_window_start.beat);
            REQUIRE(p->position.beat == p->hit_window_end.beat);
        }
    }

    SECTION("Squeeze setting is accounted for")
    {
        std::vector<Note<NoteColour>> notes {{192}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        PointSet points {track, converter, 0.5, Second(0.0)};

        REQUIRE(points.cbegin()->hit_window_start.beat == Beat(0.9125));
        REQUIRE(points.cbegin()->hit_window_end.beat == Beat(1.0875));
    }
}

TEST_CASE("next_non_hold_point is correct")
{
    std::vector<Note<NoteColour>> notes {{0}, {192, 192}};
    NoteTrack<NoteColour> track {notes, {}, {}, 192};

    PointSet points {track, {{}, 192}, 1.0, Second(0.0)};

    REQUIRE(points.next_non_hold_point(points.cbegin()) == points.cbegin());
    REQUIRE(points.next_non_hold_point(std::next(points.cbegin(), 2))
            == points.cend());
}

TEST_CASE("next_sp_granting_note is correct")
{
    std::vector<Note<NoteColour>> notes {{100, 0}, {200, 100}, {400, 0}};
    std::vector<StarPower> phrases {{200, 1}, {400, 1}};
    NoteTrack<NoteColour> track {notes, phrases, {}, 192};
    TimeConverter converter {{}, 192};

    PointSet points {track, converter, 1.0, Second(0.0)};

    REQUIRE(points.next_sp_granting_note(points.cbegin())
            == std::next(points.cbegin()));
    REQUIRE(points.next_sp_granting_note(std::next(points.cbegin()))
            == std::next(points.cbegin()));
    REQUIRE(points.next_sp_granting_note(std::next(points.cbegin(), 2))
            == std::prev(points.cend()));
}

TEST_CASE("Solo sections are added")
{
    std::vector<Solo> solos {{0, 576, 100}, {768, 1152, 200}};
    NoteTrack<NoteColour> track {{}, {}, solos, 192};
    PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
    std::vector<std::tuple<Position, int>> expected_solo_boosts {
        {{Beat(3.0), Measure(0.75)}, 100}, {{Beat(6.0), Measure(1.5)}, 200}};

    REQUIRE(points.solo_boosts() == expected_solo_boosts);
}

TEST_CASE("range_score is correct")
{
    NoteTrack<NoteColour> track {{{0, 192}, {386}}, {}, {}, 192};
    PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
    auto begin = points.cbegin();
    auto end = points.cend();

    REQUIRE(points.range_score(begin, begin) == 0);
    REQUIRE(points.range_score(begin, end) == 128);
    REQUIRE(points.range_score(begin + 1, end - 1) == 28);
}

TEST_CASE("colour_set is correct for 5 fret")
{
    std::vector<Note<NoteColour>> notes {{0},
                                         {0, 0, NoteColour::Red},
                                         {176, 100, NoteColour::Yellow},
                                         {500, 0, NoteColour::Blue}};
    NoteTrack<NoteColour> track {notes, {}, {}, 192};
    PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
    auto begin = points.cbegin();
    auto end = points.cend();

    REQUIRE(points.colour_set(begin) == "GR");
    REQUIRE(points.colour_set(begin + 1) == "Y");
    REQUIRE(points.colour_set(end - 1) == "B");
}

TEST_CASE("colour_set is correct for 6 fret")
{
    std::vector<Note<GHLNoteColour>> notes {
        {0},
        {0, 0, GHLNoteColour::WhiteMid},
        {176, 100, GHLNoteColour::BlackHigh},
        {500, 0, GHLNoteColour::Open}};
    NoteTrack<GHLNoteColour> track {notes, {}, {}, 192};
    PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
    auto begin = points.cbegin();
    auto end = points.cend();

    REQUIRE(points.colour_set(begin) == "W1W2");
    REQUIRE(points.colour_set(begin + 1) == "B3");
    REQUIRE(points.colour_set(end - 1) == "open");
}
