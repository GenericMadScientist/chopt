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

#include "imagebuilder.hpp"

template <typename T>
static bool operator==(const DrawnNote<T>& lhs, const DrawnNote<T>& rhs)
{
    return lhs.beat == Approx(rhs.beat) && lhs.length == Approx(rhs.length)
        && lhs.colour == rhs.colour && lhs.is_sp_note == rhs.is_sp_note;
}

static bool operator==(const DrawnRow& lhs, const DrawnRow& rhs)
{
    return lhs.start == Approx(rhs.start) && lhs.end == Approx(rhs.end);
}

TEST_CASE("Track type is stored correctly")
{
    SECTION("Five fret gets the right track type")
    {
        NoteTrack<NoteColour> track {{}, {}, {}, 192};
        ImageBuilder builder {track, {}};

        REQUIRE(builder.track_type() == TrackType::FiveFret);
    }

    SECTION("Six fret gets the right track type")
    {
        NoteTrack<GHLNoteColour> track {{}, {}, {}, 192};
        ImageBuilder builder {track, {}};

        REQUIRE(builder.track_type() == TrackType::SixFret);
    }

    SECTION("Drums gets the right track type")
    {
        NoteTrack<DrumNoteColour> track {{}, {}, {}, 192};
        ImageBuilder builder {track, {}};

        REQUIRE(builder.track_type() == TrackType::Drums);
    }
}

TEST_CASE("Notes are handled correclty")
{
    SECTION("Non-SP non-sustains are handled correctly")
    {
        NoteTrack<NoteColour> track {
            {{0}, {768, 0, NoteColour::Red}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnNote<NoteColour>> expected_notes {
            {0.0, 0.0, NoteColour::Green, false},
            {4.0, 0.0, NoteColour::Red, false}};

        REQUIRE(builder.notes() == expected_notes);
    }

    SECTION("Sustains are handled correctly")
    {
        NoteTrack<NoteColour> track {{{0, 96}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnNote<NoteColour>> expected_notes {
            {0.0, 0.5, NoteColour::Green, false}};

        REQUIRE(builder.notes() == expected_notes);
    }

    SECTION("SP notes are recorded")
    {
        NoteTrack<NoteColour> track {{{0}, {768}}, {{768, 100}}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnNote<NoteColour>> expected_notes {
            {0.0, 0.0, NoteColour::Green, false},
            {4.0, 0.0, NoteColour::Green, true}};

        REQUIRE(builder.notes() == expected_notes);
    }

    SECTION("6 fret notes are handled correctly")
    {
        NoteTrack<GHLNoteColour> track {
            {{0}, {768, 0, GHLNoteColour::BlackHigh}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnNote<GHLNoteColour>> expected_notes {
            {0.0, 0.0, GHLNoteColour::WhiteLow, false},
            {4.0, 0.0, GHLNoteColour::BlackHigh, false}};

        REQUIRE(builder.ghl_notes() == expected_notes);
    }

    SECTION("Drum notes are handled correctly")
    {
        NoteTrack<DrumNoteColour> track {
            {{0}, {768, 0, DrumNoteColour::YellowCymbal}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnNote<DrumNoteColour>> expected_notes {
            {0.0, 0.0, DrumNoteColour::Red, false},
            {4.0, 0.0, DrumNoteColour::YellowCymbal, false}};

        REQUIRE(builder.drum_notes() == expected_notes);
    }
}

TEST_CASE("Drawn rows are handled correctly")
{
    SECTION("Simple 4/4 is handled correctly")
    {
        NoteTrack<NoteColour> track {{{2880}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<DrawnRow> expected_rows {{0.0, 16.0}};

        REQUIRE(builder.rows() == expected_rows);
    }

    SECTION("3/4 and 3/8 are coped with")
    {
        NoteTrack<NoteColour> track {{{2450}}, {}, {}, 192};
        SyncTrack sync_track {
            {{0, 4, 4}, {768, 3, 4}, {1344, 3, 8}, {1632, 4, 4}}, {}};
        ImageBuilder builder {track, sync_track};
        std::vector<DrawnRow> expected_rows {{0.0, 12.5}, {12.5, 16.5}};

        REQUIRE(builder.rows() == expected_rows);
    }

    SECTION("Time signature changes off measure are coped with")
    {
        NoteTrack<NoteColour> track {{{768}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}, {767, 3, 4}, {1344, 3, 8}}, {}};
        ImageBuilder builder {track, sync_track};
        std::vector<DrawnRow> expected_rows {{0.0, 7.0}};

        REQUIRE(builder.rows() == expected_rows);
    }

    SECTION("x/4 for x > 16 is coped with")
    {
        NoteTrack<NoteColour> track {{{0}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 17, 4}}, {}};
        ImageBuilder builder {track, sync_track};
        std::vector<DrawnRow> expected_rows {{0.0, 16.0}, {16.0, 17.0}};

        REQUIRE(builder.rows() == expected_rows);
    }

    SECTION("Enough rows are drawn for end of song sustains")
    {
        NoteTrack<NoteColour> track {{{0, 3840}}, {}, {}, 192};
        ImageBuilder builder {track, {}};

        REQUIRE(builder.rows().size() == 2);
    }
}

TEST_CASE("Beat lines are correct")
{
    SECTION("4/4 works fine")
    {
        NoteTrack<NoteColour> track {{{767}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        std::vector<double> expected_half_beat_lines {0.5, 1.5, 2.5, 3.5};
        std::vector<double> expected_beat_lines {1.0, 2.0, 3.0};
        std::vector<double> expected_measure_lines {0.0, 4.0};

        REQUIRE(builder.half_beat_lines() == expected_half_beat_lines);
        REQUIRE(builder.beat_lines() == expected_beat_lines);
        REQUIRE(builder.measure_lines() == expected_measure_lines);
    }

    SECTION("4/8 works fine")
    {
        NoteTrack<NoteColour> track {{{767}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 8}}, {}};
        ImageBuilder builder {track, sync_track};
        std::vector<double> expected_half_beat_lines {0.25, 0.75, 1.25, 1.75,
                                                      2.25, 2.75, 3.25, 3.75};
        std::vector<double> expected_beat_lines {0.5, 1.0, 1.5, 2.5, 3.0, 3.5};
        std::vector<double> expected_measure_lines {0.0, 2.0, 4.0};

        REQUIRE(builder.half_beat_lines() == expected_half_beat_lines);
        REQUIRE(builder.beat_lines() == expected_beat_lines);
        REQUIRE(builder.measure_lines() == expected_measure_lines);
    }

    SECTION("Combination of 4/4 and 4/8 works fine")
    {
        NoteTrack<NoteColour> track {{{1151}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}, {768, 4, 8}}, {}};
        ImageBuilder builder {track, sync_track};
        std::vector<double> expected_half_beat_lines {0.5,  1.5,  2.5,  3.5,
                                                      4.25, 4.75, 5.25, 5.75};
        std::vector<double> expected_beat_lines {1.0, 2.0, 3.0, 4.5, 5.0, 5.5};
        std::vector<double> expected_measure_lines {0.0, 4.0, 6.0};

        REQUIRE(builder.half_beat_lines() == expected_half_beat_lines);
        REQUIRE(builder.beat_lines() == expected_beat_lines);
        REQUIRE(builder.measure_lines() == expected_measure_lines);
    }
}

TEST_CASE("Time signatures are handled correctly")
{
    SECTION("Normal time signatures are handled correctly")
    {
        NoteTrack<NoteColour> track {{{1920}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}, {768, 4, 8}}, {}};
        ImageBuilder builder {track, sync_track};
        builder.add_time_sigs(sync_track, 192);
        std::vector<std::tuple<double, int, int>> expected_time_sigs {
            {0.0, 4, 4}, {4.0, 4, 8}};

        REQUIRE(builder.time_sigs() == expected_time_sigs);
    }

    SECTION("Time signature changes past the end of the song are removed")
    {
        NoteTrack<NoteColour> track {{{768}}, {}, {}, 192};
        SyncTrack sync_track {{{0, 4, 4}, {1920, 3, 4}}, {}};
        ImageBuilder builder {track, sync_track};
        builder.add_time_sigs(sync_track, 192);

        REQUIRE(builder.time_sigs().size() == 1);
    }
}

TEST_CASE("Tempos are handled correctly")
{
    SECTION("Normal tempos are handled correctly")
    {
        NoteTrack<NoteColour> track {{{1920}}, {}, {}, 192};
        SyncTrack sync_track {{}, {{0, 150000}, {384, 120000}, {768, 200000}}};
        ImageBuilder builder {track, sync_track};
        builder.add_bpms(sync_track, 192);
        std::vector<std::tuple<double, double>> expected_bpms {
            {0.0, 150.0}, {2.0, 120.0}, {4.0, 200.0}};

        REQUIRE(builder.bpms() == expected_bpms);
    }

    SECTION("Tempo changes past the end of the song are removed")
    {
        NoteTrack<NoteColour> track {{{768}}, {}, {}, 192};
        SyncTrack sync_track {{}, {{0, 120000}, {1920, 200000}}};
        ImageBuilder builder {track, sync_track};
        builder.add_bpms(sync_track, 192);

        REQUIRE(builder.bpms().size() == 1);
    }
}

TEST_CASE("SongHeader information is added")
{
    SECTION("100% speed")
    {
        NoteTrack<NoteColour> track {{{0}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        builder.add_song_header("TestName", "GMS", "NotGMS", 100);

        REQUIRE(builder.song_name() == "TestName");
        REQUIRE(builder.artist() == "GMS");
        REQUIRE(builder.charter() == "NotGMS");
    }

    SECTION("200% speed")
    {
        NoteTrack<NoteColour> track {{{0}}, {}, {}, 192};
        ImageBuilder builder {track, {}};
        builder.add_song_header("TestName", "GMS", "NotGMS", 200);

        REQUIRE(builder.song_name() == "TestName (200%)");
        REQUIRE(builder.artist() == "GMS");
        REQUIRE(builder.charter() == "NotGMS");
    }
}

TEST_CASE("Green ranges for SP phrases are added correctly")
{
    NoteTrack<NoteColour> track {
        {{960}, {1344, 96}}, {{768, 384}, {1200, 150}}, {}, 192};
    ImageBuilder builder {track, {}};
    builder.add_sp_phrases(track);
    std::vector<std::tuple<double, double>> expected_green_ranges {{5.0, 5.1},
                                                                   {7.0, 7.5}};

    REQUIRE(builder.green_ranges() == expected_green_ranges);
}

TEST_CASE("Green ranges have a minimum size")
{
    NoteTrack<NoteColour> track {{{768}}, {{768, 384}}, {}, 192};
    ImageBuilder builder {track, {}};
    builder.add_sp_phrases(track);
    std::vector<std::tuple<double, double>> expected_green_ranges {{4.0, 4.1}};

    REQUIRE(builder.green_ranges() == expected_green_ranges);
}

TEST_CASE("Green ranges for six fret SP phrases are added correctly")
{
    NoteTrack<GHLNoteColour> track {
        {{960}, {1344, 96}}, {{768, 384}, {1200, 150}}, {}, 192};
    ImageBuilder builder {track, {}};
    builder.add_sp_phrases(track);
    std::vector<std::tuple<double, double>> expected_green_ranges {{5.0, 5.0},
                                                                   {7.0, 7.5}};

    REQUIRE(builder.green_ranges() == expected_green_ranges);
}

TEST_CASE("Green ranges for drums SP phrases are added correctly")
{
    NoteTrack<DrumNoteColour> track {
        {{960}, {1344}}, {{768, 384}, {1200, 150}}, {}, 192};
    ImageBuilder builder {track, {}};
    builder.add_sp_phrases(track);
    std::vector<std::tuple<double, double>> expected_green_ranges {{5.0, 5.0},
                                                                   {7.0, 7.0}};

    REQUIRE(builder.green_ranges() == expected_green_ranges);
}

TEST_CASE("add_sp_acts adds correct ranges")
{
    SECTION("Normal path is drawn correctly")
    {
        NoteTrack<NoteColour> track {{{0, 96}, {192}}, {{0, 50}}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        ImageBuilder builder {track, {}};
        Path path {{{points.cbegin(), points.cend() - 1, Beat {0.25},
                     Beat {0.1}, Beat {0.9}}},
                   0};
        builder.add_sp_phrases(track);
        builder.add_sp_acts(points, converter, path);
        std::vector<std::tuple<double, double>> expected_blue_ranges {
            {0.1, 0.9}};
        std::vector<std::tuple<double, double>> expected_red_ranges {
            {0.0, 0.1}, {0.9, 1.0}};
        std::vector<std::tuple<double, double>> expected_yellow_ranges {
            {0.25, 0.5}};

        REQUIRE(builder.blue_ranges() == expected_blue_ranges);
        REQUIRE(builder.red_ranges() == expected_red_ranges);
        REQUIRE(builder.yellow_ranges() == expected_yellow_ranges);
    }

    SECTION("Squeezes are only drawn when required")
    {
        NoteTrack<NoteColour> track {{{0}, {192}, {384}, {576}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        ImageBuilder builder {track, {}};
        Path path {{{points.cbegin(), points.cbegin() + 1, Beat {0.25},
                     Beat {0.1}, Beat {1.1}},
                    {points.cbegin() + 2, points.cbegin() + 3, Beat {0.25},
                     Beat {2.0}, Beat {2.9}}},
                   0};
        builder.add_sp_acts(points, converter, path);
        std::vector<std::tuple<double, double>> expected_red_ranges {
            {0.0, 0.1}, {2.9, 3.0}};

        REQUIRE(builder.red_ranges() == expected_red_ranges);
    }

    SECTION("Blue ranges are cropped for reverse squeezes")
    {
        NoteTrack<NoteColour> track {{{192}, {384}, {576}, {768}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        ImageBuilder builder {track, {}};
        Path path {{{points.cbegin() + 1, points.cbegin() + 2, Beat {5.0},
                     Beat {0.0}, Beat {5.0}}},
                   0};
        builder.add_sp_acts(points, converter, path);
        std::vector<std::tuple<double, double>> expected_blue_ranges {
            {1.0, 4.0}};

        REQUIRE(builder.blue_ranges() == expected_blue_ranges);
    }

    SECTION("Blue ranges are cropped by the end of the song")
    {
        NoteTrack<NoteColour> track {{{192}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.0)};
        ImageBuilder builder {track, {}};
        Path path {{{points.cbegin(), points.cbegin(), Beat {0.0}, Beat {0.0},
                     Beat {16.0}}},
                   0};
        builder.add_sp_acts(points, converter, path);
        std::vector<std::tuple<double, double>> expected_blue_ranges {
            {0.0, 4.0}};

        REQUIRE(builder.blue_ranges() == expected_blue_ranges);
    }

    SECTION("Blue and red ranges are shifted by video lag")
    {
        NoteTrack<NoteColour> track {
            {{0}, {192}, {384}, {576}, {768}, {1530}}, {}, {}, 192};
        TimeConverter converter {{}, 192};
        PointSet points {track, converter, 1.0, Second(0.05)};
        ImageBuilder builder {track, {}};
        Path path {{{points.cbegin(), points.cbegin() + 1, Beat {0.25},
                     Beat {0.1}, Beat {1.1}},
                    {points.cbegin() + 2, points.cbegin() + 3, Beat {0.25},
                     Beat {2.0}, Beat {2.9}},
                    {points.cbegin() + 5, points.cbegin() + 5, Beat {0.25},
                     Beat {7.0}, Beat {23.0}}},
                   0};
        std::vector<std::tuple<double, double>> expected_blue_ranges {
            {0.0, 1.0}, {1.9, 2.8}, {6.9, 8.0}};
        std::vector<std::tuple<double, double>> expected_red_ranges {
            {2.8, 3.0}};

        builder.add_sp_acts(points, converter, path);

        REQUIRE(builder.blue_ranges() == expected_blue_ranges);
        REQUIRE(builder.red_ranges() == expected_red_ranges);
    }
}

TEST_CASE("add_solo_sections add correct ranges")
{
    NoteTrack<NoteColour> track {{{0}}, {}, {{192, 384, 0}}, 192};
    ImageBuilder builder {track, {}};
    builder.add_solo_sections(track.solos(), 192);
    std::vector<std::tuple<double, double>> expected_solo_ranges {{1.0, 2.0}};

    REQUIRE(builder.solo_ranges() == expected_solo_ranges);
}

TEST_CASE("add_measure_values gives correct values")
{
    SECTION("Notes with no activations or solos")
    {
        NoteTrack<NoteColour> track {{{0}, {768}}, {}, {}, 192};
        PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
        Path path;
        ImageBuilder builder {track, {}};
        builder.add_measure_values(points, {{}, 192}, path);
        std::vector<int> expected_base_values {50, 50};
        std::vector<int> expected_score_values {50, 100};

        REQUIRE(builder.base_values() == expected_base_values);
        REQUIRE(builder.score_values() == expected_score_values);
    }

    SECTION("Solos are added")
    {
        NoteTrack<NoteColour> track {
            {{768}}, {}, {{0, 100, 100}, {200, 800, 100}}, 192};
        PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
        Path path;
        ImageBuilder builder {track, {}};
        builder.add_measure_values(points, {{}, 192}, path);
        std::vector<int> expected_score_values {100, 250};

        REQUIRE(builder.score_values() == expected_score_values);
    }

    // This bug caused a crash in a few songs, for example Satch Boogie (Live)
    // from Guitar Hero X.
    SECTION("Solos ending past last note are handled correctly")
    {
        NoteTrack<NoteColour> track {{{0}}, {}, {{0, 1600, 50}}, 192};
        PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
        Path path;
        ImageBuilder builder {track, {}};
        builder.add_measure_values(points, {{}, 192}, path);
        std::vector<int> expected_score_values {100};

        REQUIRE(builder.score_values() == expected_score_values);
    }

    SECTION("Activations are added")
    {
        NoteTrack<NoteColour> track {{{0}, {192}, {384}, {768}}, {}, {}, 192};
        PointSet points {track, {{}, 192}, 1.0, Second(0.0)};
        Path path {{{points.cbegin() + 2, points.cbegin() + 3, Beat {0.0},
                     Beat {0.0}}},
                   100};
        ImageBuilder builder {track, {}};
        builder.add_measure_values(points, {{}, 192}, path);
        std::vector<int> expected_score_values {200, 300};

        REQUIRE(builder.score_values() == expected_score_values);
    }

    SECTION("Video lag is accounted for")
    {
        NoteTrack<NoteColour> track {{{0}, {768}}, {}, {}, 192};
        PointSet points {track, {{}, 192}, 1.0, Second(-0.1)};
        Path path {{{points.cbegin() + 1, points.cbegin() + 1, Beat {0.0},
                     Beat {0.0}}},
                   50};
        ImageBuilder builder {track, {}};
        builder.add_measure_values(points, {{}, 192}, path);
        std::vector<int> expected_base_values {50, 50};
        std::vector<int> expected_score_values {50, 150};

        REQUIRE(builder.base_values() == expected_base_values);
        REQUIRE(builder.score_values() == expected_score_values);
    }
}

TEST_CASE("add_sp_values gives correct values")
{
    NoteTrack<NoteColour> track {{{0}, {192, 768}}, {{192, 50}}, {}, 192};
    SpData sp_data {track, {}, 1.0, Second(0.0), Second(0.0)};
    ImageBuilder builder {track, {}};
    builder.add_sp_values(sp_data);
    std::vector<double> expected_sp_values {3.14, 1.0};

    REQUIRE(builder.sp_values() == expected_sp_values);
}

TEST_CASE("set_total_score sets the correct value")
{
    NoteTrack<NoteColour> track {{{0}, {192}}, {{0, 50}}, {}, 192};
    TimeConverter converter {{}, 192};
    PointSet points {track, converter, 1.0, Second(0.0)};
    ImageBuilder builder {track, {}};
    Path path {{{points.cbegin(), points.cend() - 1, Beat {0.25}, Beat {0.1},
                 Beat {0.9}}},
               50};
    builder.set_total_score(points, {{0, 1, 100}}, path);

    REQUIRE(builder.total_score() == 250);
}
