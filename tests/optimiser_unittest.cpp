/*
 * chopt - Star Power optimiser for Clone Hero
 * Copyright (C) 2020 Raymond Wright
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

#include <iostream>

#include <algorithm>
#include <iterator>

#include "catch.hpp"

#include "optimiser.hpp"

static bool operator==(const Beat& lhs, const Beat& rhs)
{
    return lhs.value() == Approx(rhs.value()).margin(0.01);
}

static bool operator==(const Activation& lhs, const Activation& rhs)
{
    return std::tie(lhs.act_start, lhs.act_end, lhs.sp_start, lhs.sp_end)
        == std::tie(rhs.act_start, rhs.act_end, rhs.sp_start, rhs.sp_end);
}

TEST_CASE("path_summary produces the correct output", "Path summary")
{
    std::vector<Note> notes {{0}, {192}, {384}, {576}, {6144}};
    std::vector<StarPower> phrases {{0, 50}, {192, 50}, {384, 50}, {6144, 50}};
    std::vector<Solo> solos {{0, 50, 100}};
    NoteTrack note_track {notes, phrases, solos};
    ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
    const auto& points = track.points();

    SECTION("Overlap and ES are denoted correctly")
    {
        Path path {{{points.cbegin() + 2, points.cbegin() + 3, Beat {0.0},
                     Beat {0.0}}},
                   100};

        const char* desired_path_output
            = "Path: 2(+1)-ES1\n"
              "No SP score: 350\n"
              "Total score: 450\n"
              "Activation 1: Measure 1.5 to Measure 1.75";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No overlap is denoted correctly")
    {
        Path path {{{points.cbegin() + 3, points.cbegin() + 3, Beat {0.0},
                     Beat {0.0}}},
                   50};

        const char* desired_path_output
            = "Path: 3-ES1\n"
              "No SP score: 350\n"
              "Total score: 400\n"
              "Activation 1: Measure 1.75 to Measure 1.75";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No ES is denoted correctly")
    {
        Path path {{{points.cbegin() + 4, points.cbegin() + 4, Beat {0.0},
                     Beat {0.0}}},
                   50};

        const char* desired_path_output
            = "Path: 3(+1)\n"
              "No SP score: 350\n"
              "Total score: 400\n"
              "Activation 1: Measure 9 to Measure 9";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No SP is denoted correctly")
    {
        Path path {{}, 0};
        NoteTrack second_note_track {notes, {}, solos};
        ProcessedSong second_track {second_note_track, 192, {}, 1.0, 1.0};

        const char* desired_path_output = "Path: None\n"
                                          "No SP score: 350\n"
                                          "Total score: 350";

        REQUIRE(second_track.path_summary(path) == desired_path_output);
    }
}

TEST_CASE("optimal_path produces the correct path")
{
    SECTION("Simplest song with a non-empty path")
    {
        std::vector<Note> notes {{0}, {192}, {384}};
        std::vector<StarPower> phrases {{0, 50}, {192, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {{points.cbegin() + 2,
                                               points.cbegin() + 2, Beat {0.0},
                                               Beat {2.0}, Beat {18.0}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 50);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    SECTION("Simplest song with multiple acts")
    {
        std::vector<Note> notes {{0},
                                 {192},
                                 {384},
                                 {384, 0, NoteColour::Red},
                                 {384, 0, NoteColour::Yellow},
                                 {3840},
                                 {4032},
                                 {10368},
                                 {10368, 0, NoteColour::Red},
                                 {10368, 0, NoteColour::Yellow}};
        std::vector<StarPower> phrases {
            {0, 50}, {192, 50}, {3840, 50}, {4032, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {
            {points.cbegin() + 2, points.cbegin() + 2, Beat {0.0}, Beat {2.0},
             Beat {18.0}},
            {points.cbegin() + 5, points.cbegin() + 5, Beat {0.0}, Beat {54.0},
             Beat {70.0}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 300);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    SECTION("Simplest song with an act containing more than one note")
    {
        std::vector<Note> notes {{0}, {192}, {384}, {576}};
        std::vector<StarPower> phrases {{0, 50}, {192, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {{points.cbegin() + 2,
                                               points.cbegin() + 3, Beat {0.0},
                                               Beat {2.0}, Beat {18.0}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 100);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    SECTION("Simplest song with an act that must go as long as possible")
    {
        std::vector<Note> notes {{0}, {192}, {384}, {3360}};
        std::vector<StarPower> phrases {{0, 50}, {192, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {{points.cbegin() + 2,
                                               points.cbegin() + 3, Beat {0.0},
                                               Beat {2.0}, Beat {18.0}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 100);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    SECTION("Simplest song where greedy algorithm fails")
    {
        std::vector<Note> notes {
            {0}, {192}, {384}, {3840}, {3840, 0, NoteColour::Red}};
        std::vector<StarPower> phrases {{0, 50}, {192, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {{points.cbegin() + 3,
                                               points.cbegin() + 3, Beat {0.0},
                                               Beat {20.0}, Beat {36.0}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 100);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    SECTION("Simplest song where a phrase must be hit early")
    {
        std::vector<Note> notes {{0},    {192},   {384},  {3224},
                                 {9378}, {15714}, {15715}};
        std::vector<StarPower> phrases {
            {0, 50}, {192, 50}, {3224, 50}, {9378, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        const auto& points = track.points();
        std::vector<Activation> optimal_acts {
            {points.cbegin() + 2, points.cbegin() + 2, Beat {0.0},
             Beat {0.8958}, Beat {16.8958}},
            {points.cbegin() + 5, points.cbegin() + 6, Beat {0.0},
             Beat {81.84375}, Beat {97.84375}}};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 150);
        REQUIRE(opt_path.activations == optimal_acts);
    }

    // Naively the ideal path would be 2-1, but we have to squeeze the last SP
    // phrase early for the 2 to work and this makes the 1 impossible. So the
    // optimal path is really 3.
    SECTION("Simplest song where activations ending late matter")
    {
        std::vector<Note> notes {
            {0},     {192},   {384},   {3234, 1440}, {10944}, {10945}, {10946},
            {10947}, {10948}, {10949}, {10950},      {10951}, {10952}, {10953}};
        std::vector<StarPower> phrases {{0, 50}, {192, 50}, {3234, 50}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 750);
        REQUIRE(opt_path.activations.size() == 1);
    }

    // There was a bug where sustains at the start of an SP phrase right after
    // an activation/start of song had their early whammy discounted, if that
    // note didn't also grant SP. This affected a squeeze in GH3 Cult of
    // Personality. This test is to catch that.
    SECTION("Early whammy at start of an SP phrase is always counted")
    {
        std::vector<Note> notes {{0, 1420}, {1500}, {1600}};
        std::vector<StarPower> phrases {{0, 1550}};
        NoteTrack note_track {notes, phrases, {}};
        ProcessedSong track {note_track, 192, {}, 1.0, 1.0};
        Optimiser optimiser {&track};
        auto opt_path = optimiser.optimal_path();

        REQUIRE(opt_path.score_boost == 50);
        REQUIRE(opt_path.activations.size() == 1);
    }
}
