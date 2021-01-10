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
#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "points.hpp"

static bool phrase_contains_pos(const StarPower& phrase, int position)
{
    if (position < phrase.position) {
        return false;
    }
    return position < (phrase.position + phrase.length);
}

template <typename OutputIt>
static void append_sustain_points(OutputIt points, int position,
                                  int sust_length, int resolution,
                                  const TimeConverter& converter)
{
    constexpr double HALF_RES_OFFSET = 0.5;

    const double float_res = resolution;
    const auto tick_gap = std::max(resolution / 25, 1);
    auto sust_ticks = (sust_length + tick_gap - 1) / tick_gap;

    while (sust_length > (resolution / 4)) {
        position += tick_gap;
        sust_length -= tick_gap;
        const Beat beat {(position - HALF_RES_OFFSET) / float_res};
        const auto meas = converter.beats_to_measures(beat);
        --sust_ticks;
        *points++
            = {{beat, meas}, {beat, meas}, {beat, meas}, 1, 1, true, false};
    }
    if (sust_ticks > 0) {
        const Beat beat {(position + HALF_RES_OFFSET) / float_res};
        const auto meas = converter.beats_to_measures(beat);
        *points++ = {{beat, meas}, {beat, meas}, {beat, meas}, sust_ticks,
                     sust_ticks,   true,         false};
    }
}

template <typename InputIt, typename OutputIt>
static void append_note_points(InputIt first, InputIt last, OutputIt points,
                               int resolution, bool is_note_sp_ender,
                               const TimeConverter& converter, double squeeze)
{
    assert(first != last); // NOLINT

    constexpr int NOTE_VALUE = 50;
    const auto EARLY_WINDOW = Second(0.07 * squeeze);
    const auto LATE_WINDOW = Second(0.07 * squeeze);

    const auto chord_size = static_cast<int>(std::distance(first, last));
    auto pos = first->position;
    const Beat beat {pos / static_cast<double>(resolution)};
    const auto meas = converter.beats_to_measures(beat);
    const auto note_seconds = converter.beats_to_seconds(beat);
    const auto early_beat
        = converter.seconds_to_beats(note_seconds - EARLY_WINDOW);
    const auto early_meas = converter.beats_to_measures(early_beat);
    const auto late_beat
        = converter.seconds_to_beats(note_seconds + LATE_WINDOW);
    const auto late_meas = converter.beats_to_measures(late_beat);
    *points++ = {{beat, meas},
                 {early_beat, early_meas},
                 {late_beat, late_meas},
                 NOTE_VALUE * chord_size,
                 NOTE_VALUE * chord_size,
                 false,
                 is_note_sp_ender};

    auto [min_iter, max_iter]
        = std::minmax_element(first, last, [](const auto& x, const auto& y) {
              return x.length < y.length;
          });
    if (min_iter->length == max_iter->length) {
        append_sustain_points(points, pos, max_iter->length, resolution,
                              converter);
    } else {
        for (auto p = first; p < last; ++p) {
            append_sustain_points(points, pos, p->length, resolution,
                                  converter);
        }
    }
}

template <typename T>
static std::vector<Point> points_from_track(const NoteTrack<T>& track,
                                            const TimeConverter& converter,
                                            double squeeze, Second video_lag)
{
    const auto& notes = track.notes();
    std::vector<Point> points;

    auto current_phrase = track.sp_phrases().cbegin();
    for (auto p = notes.cbegin(); p != notes.cend();) {
        auto q = std::next(p);
        if constexpr (!std::is_same_v<T, DrumNoteColour>) {
            q = std::find_if_not(p, notes.cend(), [=](const auto& x) {
                return x.position == p->position;
            });
        }
        auto is_note_sp_ender = false;
        if (current_phrase != track.sp_phrases().cend()
            && phrase_contains_pos(*current_phrase, p->position)
            && ((q == notes.cend())
                || !phrase_contains_pos(*current_phrase, q->position))) {
            is_note_sp_ender = true;
            ++current_phrase;
        }
        append_note_points(p, q, std::back_inserter(points), track.resolution(),
                           is_note_sp_ender, converter, squeeze);
        p = q;
    }

    std::stable_sort(points.begin(), points.end(),
                     [](const auto& x, const auto& y) {
                         return x.position.beat < y.position.beat;
                     });

    auto combo = 0U;
    for (auto& point : points) {
        if (!point.is_hold_point) {
            ++combo;
        }
        const auto multiplier = 1 + std::min(combo / 10, 3U);
        point.value *= multiplier;
    }

    const auto add_video_lag = [&](auto& position) {
        auto seconds = converter.beats_to_seconds(position.beat);
        seconds += video_lag;
        position.beat = converter.seconds_to_beats(seconds);
        position.measure = converter.beats_to_measures(position.beat);
    };

    for (auto& point : points) {
        if (point.is_hold_point) {
            continue;
        }
        add_video_lag(point.position);
        add_video_lag(point.hit_window_start);
        add_video_lag(point.hit_window_end);
    }

    return points;
}

template <typename P>
static std::vector<PointPtr>
next_matching_vector(const std::vector<Point>& points, P predicate)
{
    if (points.empty()) {
        return {};
    }
    std::vector<PointPtr> next_matching_points;
    auto next_matching_point = points.cend();
    for (auto p = std::prev(points.cend());; --p) {
        if (predicate(*p)) {
            next_matching_point = p;
        }
        next_matching_points.push_back(next_matching_point);
        // We can't have the loop condition be p >= points.cbegin() because
        // decrementing past .begin() is undefined behaviour.
        if (p == points.cbegin()) {
            break;
        }
    }
    std::reverse(next_matching_points.begin(), next_matching_points.end());
    return next_matching_points;
}

static std::vector<PointPtr>
next_non_hold_vector(const std::vector<Point>& points)
{
    return next_matching_vector(points,
                                [](const auto& p) { return !p.is_hold_point; });
}

static std::vector<PointPtr>
next_sp_note_vector(const std::vector<Point>& points)
{
    return next_matching_vector(
        points, [](const auto& p) { return p.is_sp_granting_note; });
}

static std::vector<std::tuple<Position, int>>
solo_boosts_from_solos(const std::vector<Solo>& solos, int resolution,
                       const TimeConverter& converter)
{
    std::vector<std::tuple<Position, int>> solo_boosts;
    solo_boosts.reserve(solos.size());
    for (const auto& solo : solos) {
        Beat end_beat {solo.end / static_cast<double>(resolution)};
        Measure end_meas = converter.beats_to_measures(end_beat);
        Position end_pos {end_beat, end_meas};
        solo_boosts.emplace_back(end_pos, solo.value);
    }
    return solo_boosts;
}

static std::vector<int> score_totals(const std::vector<Point>& points)
{
    std::vector<int> scores;
    scores.reserve(points.size() + 1);
    scores.push_back(0);
    auto sum = 0;
    for (const auto& p : points) {
        sum += p.value;
        scores.push_back(sum);
    }
    return scores;
}

static std::string to_colour_string(const std::vector<NoteColour>& colours)
{
    const std::vector<std::tuple<NoteColour, std::string>> COLOUR_NAMES {
        {NoteColour::Green, "G"},  {NoteColour::Red, "R"},
        {NoteColour::Yellow, "Y"}, {NoteColour::Blue, "B"},
        {NoteColour::Orange, "O"}, {NoteColour::Open, "open"}};

    std::string colour_string;
    for (const auto& [colour, string] : COLOUR_NAMES) {
        if (std::find(colours.cbegin(), colours.cend(), colour)
            != colours.cend()) {
            colour_string += string;
        }
    }

    return colour_string;
}

static std::string to_colour_string(const std::vector<GHLNoteColour>& colours)
{
    const std::vector<std::tuple<GHLNoteColour, std::string>> COLOUR_NAMES {
        {GHLNoteColour::WhiteLow, "W1"},  {GHLNoteColour::WhiteMid, "W2"},
        {GHLNoteColour::WhiteHigh, "W3"}, {GHLNoteColour::BlackLow, "B1"},
        {GHLNoteColour::BlackMid, "B2"},  {GHLNoteColour::BlackHigh, "B3"},
        {GHLNoteColour::Open, "open"}};

    std::string colour_string;
    for (const auto& [colour, string] : COLOUR_NAMES) {
        if (std::find(colours.cbegin(), colours.cend(), colour)
            != colours.cend()) {
            colour_string += string;
        }
    }

    return colour_string;
}

template <typename T>
static std::vector<std::string> note_colours(const std::vector<Note<T>>& notes,
                                             const std::vector<Point>& points)
{
    std::vector<std::string> colours;
    colours.reserve(points.size());
    auto note_ptr = notes.cbegin();
    for (const auto& p : points) {
        if (p.is_hold_point) {
            colours.emplace_back("");
            continue;
        }
        std::vector<T> current_colours;
        const auto position = note_ptr->position;
        while ((note_ptr != notes.cend()) && (note_ptr->position == position)) {
            current_colours.push_back(note_ptr->colour);
            ++note_ptr;
        }
        colours.push_back(to_colour_string(current_colours));
    }
    return colours;
}

PointSet::PointSet(const NoteTrack<NoteColour>& track,
                   const TimeConverter& converter, double squeeze,
                   Second video_lag)
    : m_points {points_from_track(track, converter, squeeze, video_lag)}
    , m_next_non_hold_point {next_non_hold_vector(m_points)}
    , m_next_sp_granting_note {next_sp_note_vector(m_points)}
    , m_solo_boosts {solo_boosts_from_solos(track.solos(), track.resolution(),
                                            converter)}
    , m_cumulative_score_totals {score_totals(m_points)}
    , m_video_lag {video_lag}
    , m_colours {note_colours(track.notes(), m_points)}
{
}

PointSet::PointSet(const NoteTrack<GHLNoteColour>& track,
                   const TimeConverter& converter, double squeeze,
                   Second video_lag)
    : m_points {points_from_track(track, converter, squeeze, video_lag)}
    , m_next_non_hold_point {next_non_hold_vector(m_points)}
    , m_next_sp_granting_note {next_sp_note_vector(m_points)}
    , m_solo_boosts {solo_boosts_from_solos(track.solos(), track.resolution(),
                                            converter)}
    , m_cumulative_score_totals {score_totals(m_points)}
    , m_video_lag {video_lag}
    , m_colours {note_colours(track.notes(), m_points)}
{
}

PointSet::PointSet(const NoteTrack<DrumNoteColour>& track,
                   const TimeConverter& converter, double squeeze,
                   Second video_lag)
    : m_points {points_from_track(track, converter, squeeze, video_lag)}
    , m_next_non_hold_point {next_non_hold_vector(m_points)}
    , m_next_sp_granting_note {next_sp_note_vector(m_points)}
    , m_solo_boosts {solo_boosts_from_solos(track.solos(), track.resolution(),
                                            converter)}
    , m_cumulative_score_totals {score_totals(m_points)}
    , m_video_lag {video_lag}
{
}

PointPtr PointSet::next_non_hold_point(PointPtr point) const
{
    const auto index
        = static_cast<std::size_t>(std::distance(m_points.cbegin(), point));
    return m_next_non_hold_point[index];
}

PointPtr PointSet::next_sp_granting_note(PointPtr point) const
{
    const auto index
        = static_cast<std::size_t>(std::distance(m_points.cbegin(), point));
    return m_next_sp_granting_note[index];
}

int PointSet::range_score(PointPtr start, PointPtr end) const
{
    const auto start_index
        = static_cast<std::size_t>(std::distance(m_points.cbegin(), start));
    const auto end_index
        = static_cast<std::size_t>(std::distance(m_points.cbegin(), end));
    return m_cumulative_score_totals[end_index]
        - m_cumulative_score_totals[start_index];
}
