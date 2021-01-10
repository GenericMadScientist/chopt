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

#ifndef CHOPT_POINTS_HPP
#define CHOPT_POINTS_HPP

#include <string>
#include <tuple>
#include <vector>

#include "songparts.hpp"
#include "time.hpp"

struct Point {
    Position position;
    Position hit_window_start;
    Position hit_window_end;
    int value;
    int base_value;
    bool is_hold_point;
    bool is_sp_granting_note;
};

using PointPtr = std::vector<Point>::const_iterator;

class PointSet {
private:
    const std::vector<Point> m_points;
    const std::vector<PointPtr> m_next_non_hold_point;
    const std::vector<PointPtr> m_next_sp_granting_note;
    const std::vector<std::tuple<Position, int>> m_solo_boosts;
    const std::vector<int> m_cumulative_score_totals;
    const Second m_video_lag;
    const std::vector<std::string> m_colours;

public:
    PointSet(const NoteTrack<NoteColour>& track, const TimeConverter& converter,
             double squeeze, Second video_lag);
    PointSet(const NoteTrack<GHLNoteColour>& track,
             const TimeConverter& converter, double squeeze, Second video_lag);
    PointSet(const NoteTrack<DrumNoteColour>& track,
             const TimeConverter& converter, double squeeze, Second video_lag);
    [[nodiscard]] PointPtr cbegin() const { return m_points.cbegin(); }
    [[nodiscard]] PointPtr cend() const { return m_points.cend(); }
    [[nodiscard]] PointPtr next_non_hold_point(PointPtr point) const;
    [[nodiscard]] PointPtr next_sp_granting_note(PointPtr point) const;
    [[nodiscard]] std::string colour_set(PointPtr point) const
    {
        return m_colours[static_cast<std::size_t>(
            std::distance(m_points.cbegin(), point))];
    }
    // Get the combined score of all points that are >= start and < end.
    [[nodiscard]] int range_score(PointPtr start, PointPtr end) const;
    [[nodiscard]] const std::vector<std::tuple<Position, int>>&
    solo_boosts() const
    {
        return m_solo_boosts;
    }
    [[nodiscard]] Second video_lag() const { return m_video_lag; }
};

#endif
