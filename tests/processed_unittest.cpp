/*
 * CHOpt - Star Power optimiser for Clone Hero
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

#include "catch.hpp"

#include "processed.hpp"

static bool operator==(const SpBar& lhs, const SpBar& rhs)
{
    return lhs.min() == Approx(rhs.min()) && lhs.max() == Approx(rhs.max());
}

TEST_CASE("3 arg total_available_sp counts SP correctly")
{
    std::vector<Note<NoteColour>> notes {{0},        {192},  {384},  {576},
                                         {768, 192}, {1152}, {1344}, {1536}};
    std::vector<StarPower> phrases {{0, 50}, {384, 50}, {768, 400}, {1344, 50}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong song {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = song.points();

    SECTION("Phrases are counted correctly")
    {
        REQUIRE(song.total_available_sp(Beat(0.0), points.cbegin(),
                                        points.cbegin() + 1)
                == SpBar {0.25, 0.25});
        REQUIRE(song.total_available_sp(Beat(0.0), points.cbegin(),
                                        points.cbegin() + 2)
                == SpBar {0.25, 0.25});
        REQUIRE(song.total_available_sp(Beat(0.5), points.cbegin() + 2,
                                        points.cbegin() + 3)
                == SpBar {0.25, 0.25});
    }

    SECTION("Whammy is counted correctly")
    {
        auto result = song.total_available_sp(Beat(4.0), points.cbegin() + 4,
                                              points.cbegin() + 5);
        REQUIRE(result.min() == Approx(0.0));
        REQUIRE(result.max() == Approx(0.00112847));
    }

    SECTION("Whammy is counted correctly even started mid hold")
    {
        auto result = song.total_available_sp(Beat(4.5), points.cend() - 3,
                                              points.cend() - 3);
        REQUIRE(result.min() == Approx(0.0));
        REQUIRE(result.max() == Approx(0.0166667));
    }

    SECTION("required_whammy_end is accounted for")
    {
        auto result = song.total_available_sp(Beat(4.0), points.cbegin() + 4,
                                              points.cbegin() + 5, Beat(4.02));

        REQUIRE(result.min() == Approx(0.000666667));
        REQUIRE(result.max() == Approx(0.00112847));

        result = song.total_available_sp(Beat(4.0), points.cbegin() + 4,
                                         points.cbegin() + 5, Beat(4.10));

        REQUIRE(result.min() == Approx(0.00112847));
        REQUIRE(result.max() == Approx(0.00112847));
    }

    SECTION("SP does not exceed full bar")
    {
        REQUIRE(song.total_available_sp(Beat(0.0), points.cbegin(),
                                        points.cend() - 1)
                == SpBar {1.0, 1.0});
    }

    SECTION("SP notes are counted from first_point when start is past middle")
    {
        REQUIRE(song.total_available_sp(Beat(0.05), points.cbegin(),
                                        points.cbegin() + 1)
                == SpBar {0.25, 0.25});
    }
}

TEST_CASE("total_available_sp_with_earliest_pos counts SP correctly and gives "
          "earliest posiiton")
{
    std::vector<Note<NoteColour>> notes {{0, 1459}, {1459}};
    std::vector<StarPower> phrases {{0, 100}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong song {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = song.points();

    const auto& [sp_bar, pos] = song.total_available_sp_with_earliest_pos(
        Beat(0.0), points.cbegin(), std::prev(points.cend()),
        std::prev(points.cend(), 2)->position);

    REQUIRE(sp_bar.max() == Approx(0.5));
    REQUIRE(pos.beat.value() == Approx(7.5));
}

TEST_CASE("is_candidate_valid works with no whammy")
{
    std::vector<Note<NoteColour>> notes {{0}, {1536}, {3072}, {6144}};
    NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin(),
                                   points.cbegin() + 3,
                                   {Beat(0.0), Measure(0.0)},
                                   {1.0, 1.0}};
    ProcessedSong second_track {note_track,  SyncTrack({{0, 3, 4}}, {}),
                                1.0,         1.0,
                                Second(0.0), Second(0.0)};
    const auto& second_points = second_track.points();
    ActivationCandidate second_candidate {second_points.cbegin(),
                                          second_points.cbegin() + 3,
                                          {Beat(0.0), Measure(0.0)},
                                          {1.0, 1.0}};

    SECTION("Full bar works with time signatures")
    {
        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
        REQUIRE(second_track.is_candidate_valid(second_candidate).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Half bar works with time signatures")
    {
        candidate.act_end = points.cbegin() + 2;
        candidate.sp_bar = {0.5, 0.5};
        second_candidate.act_end = second_points.cbegin() + 2;
        second_candidate.sp_bar = {0.5, 0.5};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
        REQUIRE(second_track.is_candidate_valid(second_candidate).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Below half bar never works")
    {
        candidate.act_end = points.cbegin() + 1;
        candidate.sp_bar.max() = 0.25;

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Check next point needs to not lie in activation")
    {
        candidate.act_end = points.cbegin() + 1;
        candidate.sp_bar.max() = 0.6;

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::surplus_sp);
    }

    SECTION("Check intermediate SP is accounted for")
    {
        std::vector<StarPower> phrases {{3000, 100}};
        NoteTrack<NoteColour> overlap_notes {notes, phrases, {}, 192};
        ProcessedSong overlap_track {overlap_notes, {},          1.0,
                                     1.0,           Second(0.0), Second(0.0)};
        const auto& overlap_points = overlap_track.points();
        ActivationCandidate overlap_candidate {overlap_points.cbegin(),
                                               overlap_points.cbegin() + 3,
                                               {Beat(0.0), Measure(0.0)},
                                               {0.8, 0.8}};

        REQUIRE(overlap_track.is_candidate_valid(overlap_candidate).validity
                == ActValidity::success);
    }

    SECTION("Check only reached intermediate SP is accounted for")
    {
        notes[2].position = 6000;
        std::vector<StarPower> phrases {{6000, 100}};
        NoteTrack<NoteColour> overlap_notes {notes, phrases, {}, 192};
        ProcessedSong overlap_track {overlap_notes, {},          1.0,
                                     1.0,           Second(0.0), Second(0.0)};
        const auto& overlap_points = overlap_track.points();
        ActivationCandidate overlap_candidate {overlap_points.cbegin(),
                                               overlap_points.cbegin() + 3,
                                               {Beat(0.0), Measure(0.0)},
                                               {0.8, 0.8}};

        REQUIRE(overlap_track.is_candidate_valid(overlap_candidate).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Last note's SP status is not ignored")
    {
        notes[3].position = 4000;
        std::vector<StarPower> phrases {{3072, 100}};
        NoteTrack<NoteColour> overlap_notes {notes, phrases, {}, 192};
        ProcessedSong overlap_track {overlap_notes, {},          1.0,
                                     1.0,           Second(0.0), Second(0.0)};
        const auto& overlap_points = overlap_track.points();
        ActivationCandidate overlap_candidate {overlap_points.cbegin(),
                                               overlap_points.cbegin() + 2,
                                               {Beat(0.0), Measure(0.0)},
                                               {0.5, 0.5}};

        REQUIRE(overlap_track.is_candidate_valid(overlap_candidate).validity
                == ActValidity::surplus_sp);
    }

    SECTION("SP bar does not exceed full bar")
    {
        std::vector<Note<NoteColour>> overlap_notes {{0}, {2}, {7000}};
        std::vector<StarPower> phrases {{0, 1}, {2, 1}};
        NoteTrack<NoteColour> overlap_note_track {
            overlap_notes, phrases, {}, 192};
        ProcessedSong overlap_track {overlap_note_track, {},         1.0, 1.0,
                                     Second(0.0),        Second(0.0)};
        const auto& overlap_points = overlap_track.points();
        ActivationCandidate overlap_candidate {overlap_points.cbegin(),
                                               overlap_points.cbegin() + 2,
                                               {Beat(0.0), Measure(0.0)},
                                               {1.0, 1.0}};

        REQUIRE(overlap_track.is_candidate_valid(overlap_candidate).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Earliest activation point is considered")
    {
        candidate.act_end = points.cbegin() + 1;
        candidate.sp_bar = {0.53125, 0.53125};
        candidate.earliest_activation_point = {Beat(-2.0), Measure(-0.5)};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    // There was a bug where if an optimal activation starts on an SP granting
    // note, and the note must be squeezed late, then the activation could be
    // drawn too far. This happened in the path for Das Neue bleibt beim Alten
    // from CSC November 2020.
    SECTION("Activations starting on an sp granting note have the correct end")
    {
        std::vector<Note<NoteColour>> overlap_notes {
            {384}, {384, 0, NoteColour::Red}, {5088}, {5136}};
        std::vector<StarPower> phrases {{384, 1}};
        NoteTrack<NoteColour> overlap_note_track {
            overlap_notes, phrases, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}}, {{0, 300000}}};
        ProcessedSong overlap_track {overlap_note_track, sync_track, 1.0, 1.0,
                                     Second(0.0),        Second(0.0)};
        const auto& overlap_points = overlap_track.points();
        ActivationCandidate overlap_candidate {overlap_points.cbegin(),
                                               overlap_points.cbegin() + 1,
                                               {Beat(2.24), Measure(0.56)},
                                               {0.5, 0.5}};

        const auto result = overlap_track.is_candidate_valid(
            overlap_candidate, 1.0, {Beat {27}, Measure {6.75}});

        REQUIRE(result.validity == ActValidity::success);
        REQUIRE(result.ending_position.beat.value() < 26.5);
    }
}

TEST_CASE("is_candidate_valid works with whammy")
{
    std::vector<Note<NoteColour>> notes {{0, 960}, {3840}, {6144}};
    std::vector<StarPower> phrases {{0, 7000}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin(),
                                   points.cend() - 2,
                                   {Beat(0.0), Measure(0.0)},
                                   {0.5, 0.5}};

    SECTION("Check whammy is counted")
    {
        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    // This comes up in Epidox: otherwise, chopt doesn't think you can squeeze
    // to the G after the B in the Robotic Buildup activation.
    SECTION("Check whammy from end of SP sustain before note is counted")
    {
        auto notes_copy = notes;
        notes_copy[1].position = 2880;
        NoteTrack<NoteColour> note_track_two {notes_copy, phrases, {}, 192};
        ProcessedSong track_two {note_track_two, {},         1.0, 1.0,
                                 Second(0.0),    Second(0.0)};
        const auto& points_two = track_two.points();
        ActivationCandidate candidate_two {points_two.cend() - 2,
                                           points_two.cend() - 1,
                                           {Beat(1.0), Measure(0.25)},
                                           {0.5, 0.5}};

        REQUIRE(track_two.is_candidate_valid(candidate_two).validity
                == ActValidity::success);
    }

    // There was a bug where an activation starting on an SP sustain would
    // double count the whammy around the note, giving impossible activations.
    // An example is the last activatiom of Soulless.
    SECTION("Whammy around the start of an SP sustain is not doubled counted")
    {
        std::vector<Note<NoteColour>> notes_two {
            {0}, {192}, {384, 192}, {5260}};
        std::vector<StarPower> phrases_two {{384, 1}};
        NoteTrack<NoteColour> note_track_two {notes_two, phrases_two, {}, 192};
        ProcessedSong track_two {note_track_two, {},         1.0, 1.0,
                                 Second(0.0),    Second(0.0)};
        const auto& points_two = track_two.points();
        ActivationCandidate candidate_two {points_two.cbegin() + 2,
                                           points_two.cend() - 1,
                                           {Beat(1.0), Measure(0.25)},
                                           {0.5, 0.5}};

        REQUIRE(track_two.is_candidate_valid(candidate_two).validity
                == ActValidity::insufficient_sp);
    }

    SECTION("Check compressed activations are counted")
    {
        candidate.sp_bar.max() = 0.9;
        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }
}

TEST_CASE("is_candidate_valid takes into account minimum SP")
{
    std::vector<Note<NoteColour>> notes {{0}, {1536}, {2304}, {3072}, {4608}};
    NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin(),
                                   points.cbegin() + 3,
                                   {Beat(0.0), Measure(0.0)},
                                   {0.5, 1.0}};

    SECTION("Lower SP is considered")
    {
        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    SECTION("Lower SP is only considered down to a half-bar")
    {
        candidate.act_end = points.cbegin() + 1;
        candidate.sp_bar.min() = 0.25;

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::surplus_sp);
    }
}

TEST_CASE("is_candidate_valid takes into account squeezing")
{
    SECTION("Front end and back end of the activation endpoints are considered")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3110}};
        NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 1,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    SECTION("Next note can be squeezed late to avoid going too far")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3034}, {3053}};
        NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 1,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    SECTION("Intermediate SP can be hit early")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3102}, {4608}};
        std::vector<StarPower> phrases {{3100, 100}};
        NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 2,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }

    SECTION("Intermediate SP can be hit late")
    {
        std::vector<Note<NoteColour>> notes {{0}, {768}, {6942}};
        std::vector<StarPower> phrases {{768, 100}};
        NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 2,
                                       {Beat(0.0), Measure(0.0)},
                                       {1.0, 1.0}};

        REQUIRE(track.is_candidate_valid(candidate).validity
                == ActValidity::success);
    }
}

TEST_CASE("is_candidate_valid handles very high BPM SP granting notes")
{
    std::vector<Note<NoteColour>> notes {{0}, {192}, {768}, {4608}, {5376}};
    std::vector<StarPower> phrases {{4608, 50}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    SyncTrack sync_track {{}, {{3840, 4000000}}};
    ProcessedSong track {note_track, sync_track,  1.0,
                         1.0,        Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin() + 2,
                                   points.cbegin() + 4,
                                   {Beat(0.0), Measure(0.0)},
                                   {0.5, 0.5}};

    REQUIRE(track.is_candidate_valid(candidate).validity
            == ActValidity::success);
}

TEST_CASE("is_candidate_valid takes into account squeeze param")
{
    SECTION("Front end and back end are restricted")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3110}};
        NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 1,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate, 0.5).validity
                == ActValidity::insufficient_sp);
        REQUIRE(track.is_candidate_valid(candidate, 1.0).validity
                == ActValidity::success);
    }

    SECTION("Intermediate SP front end is restricted")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3102}, {4608}};
        std::vector<StarPower> phrases {{3100, 100}};
        NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 2,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate, 0.5).validity
                == ActValidity::insufficient_sp);
        REQUIRE(track.is_candidate_valid(candidate, 1.0).validity
                == ActValidity::success);
    }

    SECTION("Intermediate SP back end is restricted")
    {
        std::vector<Note<NoteColour>> notes {{0}, {768}, {6942}};
        std::vector<StarPower> phrases {{768, 100}};
        NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 2,
                                       {Beat(0.0), Measure(0.0)},
                                       {1.0, 1.0}};

        REQUIRE(track.is_candidate_valid(candidate, 0.5).validity
                == ActValidity::insufficient_sp);
        REQUIRE(track.is_candidate_valid(candidate, 1.0).validity
                == ActValidity::success);
    }

    SECTION("Next note back end is restricted")
    {
        std::vector<Note<NoteColour>> notes {{0}, {3034}, {3053}};
        NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 1,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        REQUIRE(track.is_candidate_valid(candidate, 0.5).validity
                == ActValidity::surplus_sp);
        REQUIRE(track.is_candidate_valid(candidate, 1.0).validity
                == ActValidity::success);
    }

    SECTION("End position is finite if activation goes past last note")
    {
        std::vector<Note<NoteColour>> notes {{0}};
        NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
        ProcessedSong track {note_track, {},          1.0,
                             1.0,        Second(0.0), Second(0.0)};
        const auto& points = track.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin(),
                                       {Beat(0.0), Measure(0.0)},
                                       {1.0, 1.0}};
        auto result = track.is_candidate_valid(candidate, 1.0);

        REQUIRE(result.validity == ActValidity::success);
        REQUIRE(result.ending_position.beat.value() < 40.0);
    }
}

TEST_CASE("is_candidate_valid takes into account forced whammy")
{
    std::vector<Note<NoteColour>> notes {{0, 768}, {3072}, {3264}};
    std::vector<StarPower> phrases {{0, 3300}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin(),
                                   points.cend() - 2,
                                   {Beat(0.0), Measure(0.0)},
                                   {0.5, 0.5}};

    REQUIRE(track.is_candidate_valid(candidate, 1.0, {Beat(0.0), Measure(0.0)})
                .validity
            == ActValidity::success);
    REQUIRE(track.is_candidate_valid(candidate, 1.0, {Beat(4.0), Measure(1.0)})
                .validity
            == ActValidity::surplus_sp);
}

TEST_CASE("is_candidate_valid also takes account of whammy from end of SP "
          "sustain before note is counted")
{
    std::vector<Note<NoteColour>> notes {{0, 960}, {2880}, {6144}};
    std::vector<StarPower> phrases {{0, 7000}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cend() - 2,
                                   points.cend() - 1,
                                   {Beat(1.0), Measure(0.25)},
                                   {0.5, 0.5}};

    REQUIRE(track.is_candidate_valid(candidate, 0.0).validity
            == ActValidity::success);
}

// This is to stop a bug that appears in Edd Instrument Solo from CTH2: the last
// note is an SP sustain and the last activation was shown as ending during it,
// when it should go past the end of the song.
TEST_CASE("is_candidate_valid takes account of overlapped phrase at end if "
          "last note is whammy")
{
    std::vector<Note<NoteColour>> notes {{0}, {192}, {384}, {3456, 192}};
    std::vector<StarPower> phrases {{0, 1}, {192, 1}, {3456, 1}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin() + 2,
                                   points.cend() - 1,
                                   {Beat(1.0), Measure(0.25)},
                                   {0.5, 0.5}};

    auto result = track.is_candidate_valid(candidate, 0.0);

    REQUIRE(result.ending_position.beat > Beat(20.0));
}

// This is to stop a bug that appears in Time Traveler from CB: some of the
// mid-sustain activations were misdrawn. The exact problem is that if we must
// activate past the earliest activation point in order for the activation to
// work, then the minimum sp after hitting the point is not clamped to 0, which
// caused the endpoint to be too early.
TEST_CASE("is_candidate_valid correctly clamps low SP")
{
    std::vector<Note<NoteColour>> notes {{0, 6720}};
    std::vector<StarPower> phrases {{0, 1}};
    NoteTrack<NoteColour> note_track {notes, phrases, {}, 192};
    SyncTrack sync_track {{{0, 1, 4}}, {}};
    ProcessedSong track {note_track, sync_track,  0.0,
                         0.0,        Second(0.0), Second(0.0)};
    const auto& points = track.points();
    ActivationCandidate candidate {points.cbegin() + 500,
                                   points.cbegin() + 750,
                                   {Beat(0.0), Measure(0.0)},
                                   {1.0, 1.0}};

    auto result = track.is_candidate_valid(candidate, 0.0);

    REQUIRE(result.ending_position.beat > Beat(27.3));
}

TEST_CASE("adjusted_hit_window_* functions return correct values")
{
    std::vector<Note<NoteColour>> notes {{0}};
    NoteTrack<NoteColour> note_track {notes, {}, {}, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();

    SECTION("adjusted_hit_window_start returns correct values")
    {
        REQUIRE(
            track.adjusted_hit_window_start(points.cbegin(), 0.5).beat.value()
            == Approx(-0.07));
        REQUIRE(
            track.adjusted_hit_window_start(points.cbegin(), 1.0).beat.value()
            == Approx(-0.14));
    }

    SECTION("adjusted_hit_window_end returns correct values")
    {
        REQUIRE(track.adjusted_hit_window_end(points.cbegin(), 0.5).beat.value()
                == Approx(0.07));
        REQUIRE(track.adjusted_hit_window_end(points.cbegin(), 1.0).beat.value()
                == Approx(0.14));
    }
}

TEST_CASE("Video lag is taken account of")
{
    SECTION("Effect on whammy is taken account of")
    {
        std::vector<Note<NoteColour>> notes {{192}, {384, 192}, {768}};
        std::vector<StarPower> phrases {{384, 1}};
        NoteTrack<NoteColour> track {notes, phrases, {}, 192};
        ProcessedSong song {track, {}, 0.0, 0.0, Second(0.0), Second(-0.1)};
        const auto& points = song.points();

        auto result = song.total_available_sp(Beat(0.0), points.cbegin(),
                                              std::prev(points.cend()));

        REQUIRE(result.max() == Approx(0.29));
    }

    SECTION("Effect on notes is taken account of")
    {
        std::vector<Note<NoteColour>> notes {{768}, {3840}};
        NoteTrack<NoteColour> track {notes, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}, {3840, 2, 4}}, {}};
        ProcessedSong song {track, sync_track,  0.0,
                            0.0,   Second(0.0), Second(0.1)};
        const auto& points = song.points();
        ActivationCandidate candidate {points.cbegin(),
                                       points.cbegin() + 1,
                                       {Beat(0.0), Measure(0.0)},
                                       {0.5, 0.5}};

        auto result = song.is_candidate_valid(candidate);

        REQUIRE(result.validity == ActValidity::insufficient_sp);
    }
}

TEST_CASE("path_summary produces the correct output")
{
    std::vector<Note<NoteColour>> notes {{0}, {192}, {384}, {576}, {6144}};
    std::vector<StarPower> phrases {{0, 50}, {192, 50}, {384, 50}, {6144, 50}};
    std::vector<Solo> solos {{0, 50, 100}};
    NoteTrack<NoteColour> note_track {notes, phrases, solos, 192};
    ProcessedSong track {note_track, {}, 1.0, 1.0, Second(0.0), Second(0.0)};
    const auto& points = track.points();

    SECTION("Overlap and ES are denoted correctly")
    {
        Path path {{{points.cbegin() + 2, points.cbegin() + 3, Beat {0.0},
                     Beat {0.0}}},
                   100};

        const char* desired_path_output = "No SP score: 350\n"
                                          "Total score: 450\n"
                                          "Average multiplier: 1.400x";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No overlap is denoted correctly")
    {
        Path path {{{points.cbegin() + 3, points.cbegin() + 3, Beat {0.0},
                     Beat {0.0}}},
                   50};

        const char* desired_path_output = "No SP score: 350\n"
                                          "Total score: 400\n"
                                          "Average multiplier: 1.200x";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No ES is denoted correctly")
    {
        Path path {{{points.cbegin() + 4, points.cbegin() + 4, Beat {0.0},
                     Beat {0.0}}},
                   50};

        const char* desired_path_output = "No SP score: 350\n"
                                          "Total score: 400\n"
                                          "Average multiplier: 1.200x";

        REQUIRE(track.path_summary(path) == desired_path_output);
    }

    SECTION("No SP is denoted correctly")
    {
        Path path {{}, 0};
        NoteTrack<NoteColour> second_note_track {notes, {}, solos, 192};
        ProcessedSong second_track {second_note_track, {},         1.0, 1.0,
                                    Second(0.0),       Second(0.0)};

        const char* desired_path_output = "No SP score: 350\n"
                                          "Total score: 350\n"
                                          "Average multiplier: 1.000x";

        REQUIRE(second_track.path_summary(path) == desired_path_output);
    }
}
