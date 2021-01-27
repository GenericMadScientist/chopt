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

#ifndef CHOPT_PROCESSED_HPP
#define CHOPT_PROCESSED_HPP

#include <limits>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

#include "points.hpp"
#include "songparts.hpp"
#include "sp.hpp"
#include "time.hpp"

struct ActivationCandidate {
    PointPtr act_start;
    PointPtr act_end;
    Position earliest_activation_point {Beat(0.0), Measure(0.0)};
    SpBar sp_bar {0.0, 0.0};
};

struct ProtoActivation {
    PointPtr act_start;
    PointPtr act_end;
};

struct Activation {
    PointPtr act_start;
    PointPtr act_end;
    Beat whammy_end {0.0};
    Beat sp_start {0.0};
    Beat sp_end {0.0};
};

// Part of the return value of ProcessedSong::is_candidate_valid. Says if an
// activation is valid, and if not whether the problem is too little or too much
// Star Power.
enum class ActValidity { success, insufficient_sp, surplus_sp };

// Return value of ProcessedSong::is_candidate_valid, providing information on
// whether an activation is valid, and if so the earliest position it can end.
struct ActResult {
    Position ending_position;
    ActValidity validity;
};

struct Path {
    std::vector<Activation> activations;
    int score_boost;
};

// Represents a song processed for Star Power optimisation. The constructor
// should only fail due to OOM; invariants on the song are supposed to be
// upheld by the constructors of the arguments.
class ProcessedSong {
private:
    static constexpr double NEG_INF = -std::numeric_limits<double>::infinity();

    // The order of these members is important. We must have m_converter before
    // m_points.
    TimeConverter m_converter;
    PointSet m_points;
    SpData m_sp_data;
    int m_total_solo_boost;
    int m_base_score;

public:
    template <typename T>
    ProcessedSong(const NoteTrack<T>& track, const SyncTrack& sync_track,
                  double early_whammy, double squeeze, Second lazy_whammy,
                  Second video_lag)
        : m_converter {sync_track, track.resolution()}
        , m_points {track, m_converter, squeeze, video_lag}
        , m_sp_data {track, sync_track, early_whammy, lazy_whammy, video_lag}
        , m_base_score {track.base_score()}
    {
        m_total_solo_boost = std::accumulate(
            track.solos().cbegin(), track.solos().cend(), 0,
            [](const auto x, const auto& y) { return x + y.value; });
    }

    // Return the minimum and maximum amount of SP can be acquired between two
    // points. Does not include SP from the point act_start. first_point is
    // given for the purposes of counting SP grantings notes, e.g. if start is
    // after the middle of first_point's timing window. All whammy up to
    // required_whammy_end is mandatory.
    [[nodiscard]] SpBar
    total_available_sp(Beat start, PointPtr first_point, PointPtr act_start,
                       Beat required_whammy_end = Beat {NEG_INF}) const;
    // Similar to total_available_sp, but no whammy is required and if it is
    // possible to get a half bar then the earliest position >=
    // earliest_potential_pos that grants a half bar is returned along with the
    // SP only up to that position.
    [[nodiscard]] std::tuple<SpBar, Position>
    total_available_sp_with_earliest_pos(Beat start, PointPtr first_point,
                                         PointPtr act_start,
                                         Position earliest_potential_pos) const;
    // Returns an ActResult which says if an activation is valid, and if so the
    // earliest position it can end. Checks squeezes against the given amount
    // only.
    [[nodiscard]] ActResult
    is_candidate_valid(const ActivationCandidate& activation,
                       double squeeze = 1.0,
                       Position required_whammy_end
                       = {Beat {NEG_INF}, Measure {NEG_INF}}) const;
    // Return the summary of a path.
    [[nodiscard]] std::string path_summary(const Path& path) const;

    // Return the position that is (100 - squeeze)% along the start of point's
    // timing window.
    [[nodiscard]] Position adjusted_hit_window_start(PointPtr point,
                                                     double squeeze) const;
    // Return the position that is squeeze% along the end of point's timing
    // window.
    [[nodiscard]] Position adjusted_hit_window_end(PointPtr point,
                                                   double squeeze) const;

    [[nodiscard]] const PointSet& points() const { return m_points; }
    [[nodiscard]] const SpData& sp_data() const { return m_sp_data; }
    [[nodiscard]] const TimeConverter& converter() const { return m_converter; }
};

#endif
