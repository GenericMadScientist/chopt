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

#include <cassert>
#include <iomanip>
#include <iterator>
#include <sstream>

#include "processed.hpp"
#include "stringutil.hpp"

SpBar ProcessedSong::total_available_sp(Beat start, PointPtr first_point,
                                        PointPtr act_start,
                                        Beat required_whammy_end) const
{
    SpBar sp_bar {0.0, 0.0};
    for (auto p = m_points.next_sp_granting_note(first_point); p < act_start;
         p = m_points.next_sp_granting_note(std::next(p))) {
        sp_bar.add_phrase();
    }

    if (start >= required_whammy_end) {
        sp_bar.max()
            += m_sp_data.available_whammy(start, act_start->position.beat);
        sp_bar.max() = std::min(sp_bar.max(), 1.0);
    } else if (required_whammy_end >= act_start->position.beat) {
        sp_bar.min()
            += m_sp_data.available_whammy(start, act_start->position.beat);
        sp_bar.min() = std::min(sp_bar.min(), 1.0);
        sp_bar.max() = sp_bar.min();
    } else {
        sp_bar.min() += m_sp_data.available_whammy(start, required_whammy_end);
        sp_bar.min() = std::min(sp_bar.min(), 1.0);
        sp_bar.max() = sp_bar.min();
        sp_bar.max() += m_sp_data.available_whammy(required_whammy_end,
                                                   act_start->position.beat);
        sp_bar.max() = std::min(sp_bar.max(), 1.0);
    }

    return sp_bar;
}

std::tuple<SpBar, Position> ProcessedSong::total_available_sp_with_earliest_pos(
    Beat start, PointPtr first_point, PointPtr act_start,
    Position earliest_potential_pos) const
{
    const Beat BEAT_EPSILON {0.0001};

    SpBar sp_bar {0.0, 0.0};
    for (auto p = m_points.next_sp_granting_note(first_point); p < act_start;
         p = m_points.next_sp_granting_note(std::next(p))) {
        sp_bar.add_phrase();
    }

    sp_bar.max() += m_sp_data.available_whammy(
        start, earliest_potential_pos.beat, act_start->position.beat);
    sp_bar.max() = std::min(sp_bar.max(), 1.0);

    if (sp_bar.full_enough_to_activate()) {
        return {sp_bar, earliest_potential_pos};
    }

    const auto extra_sp_required = 0.5 - sp_bar.max();
    auto first_beat = earliest_potential_pos.beat;
    auto last_beat = act_start->position.beat;
    if (m_sp_data.available_whammy(first_beat, last_beat,
                                   act_start->position.beat)
        < extra_sp_required) {
        return {sp_bar, earliest_potential_pos};
    }

    while (last_beat - first_beat > BEAT_EPSILON) {
        const auto mid_beat = (first_beat + last_beat) * 0.5;
        if (m_sp_data.available_whammy(earliest_potential_pos.beat, mid_beat,
                                       act_start->position.beat)
            < extra_sp_required) {
            first_beat = mid_beat;
        } else {
            last_beat = mid_beat;
        }
    }

    sp_bar.max() += m_sp_data.available_whammy(
        earliest_potential_pos.beat, last_beat, act_start->position.beat);
    sp_bar.max() = std::min(sp_bar.max(), 1.0);

    return {sp_bar,
            Position {last_beat, m_converter.beats_to_measures(last_beat)}};
}

Position ProcessedSong::adjusted_hit_window_start(PointPtr point,
                                                  double squeeze) const
{
    assert((0.0 <= squeeze) && (squeeze <= 1.0)); // NOLINT

    if (squeeze == 1.0) {
        return point->hit_window_start;
    }

    auto start = m_converter.beats_to_seconds(point->hit_window_start.beat);
    auto mid = m_converter.beats_to_seconds(point->position.beat);
    auto adj_start_s = start + (mid - start) * (1.0 - squeeze);
    auto adj_start_b = m_converter.seconds_to_beats(adj_start_s);
    auto adj_start_m = m_converter.beats_to_measures(adj_start_b);

    return {adj_start_b, adj_start_m};
}

Position ProcessedSong::adjusted_hit_window_end(PointPtr point,
                                                double squeeze) const
{
    assert((0.0 <= squeeze) && (squeeze <= 1.0)); // NOLINT

    if (squeeze == 1.0) {
        return point->hit_window_end;
    }

    auto mid = m_converter.beats_to_seconds(point->position.beat);
    auto end = m_converter.beats_to_seconds(point->hit_window_end.beat);
    auto adj_end_s = mid + (end - mid) * squeeze;
    auto adj_end_b = m_converter.seconds_to_beats(adj_end_s);
    auto adj_end_m = m_converter.beats_to_measures(adj_end_b);

    return {adj_end_b, adj_end_m};
}

class SpStatus {
private:
    Position m_position;
    double m_sp;

public:
    SpStatus(Position position, double sp)
        : m_position {position}
        , m_sp {sp}
    {
    }

    [[nodiscard]] Position position() const { return m_position; }
    [[nodiscard]] double sp() const { return m_sp; }

    void add_phrase()
    {
        constexpr double SP_PHRASE_AMOUNT = 0.25;

        m_sp += SP_PHRASE_AMOUNT;
        m_sp = std::min(m_sp, 1.0);
    }

    void advance_whammy_max(Position end_position, const SpData& sp_data)
    {
        m_sp = sp_data.propagate_sp_over_whammy_max(m_position, end_position,
                                                    m_sp);
        m_position = end_position;
    }

    void update_early_end(Position sp_note_start, const SpData& sp_data,
                          Position required_whammy_end)
    {
        m_sp = sp_data.propagate_sp_over_whammy_min(m_position, sp_note_start,
                                                    m_sp, required_whammy_end);
        if (sp_note_start.beat > m_position.beat) {
            m_position = sp_note_start;
        }
    }

    void update_late_end(Position sp_note_start, Position sp_note_end,
                         const SpData& sp_data)
    {
        if (sp_note_start.beat < m_position.beat) {
            sp_note_start = m_position;
        }

        advance_whammy_max(sp_note_start, sp_data);
        if (m_sp < 0.0) {
            return;
        }
        // We might run out of SP between sp_note_start and sp_note_end. In this
        // case we just hit the note as early as possible.
        const auto new_sp = sp_data.propagate_sp_over_whammy_max(
            sp_note_start, sp_note_end, m_sp);
        if (new_sp >= 0.0) {
            m_sp = new_sp;
            m_position = sp_note_end;
        }
    }
};

ActResult
ProcessedSong::is_candidate_valid(const ActivationCandidate& activation,
                                  double squeeze,
                                  Position required_whammy_end) const
{
    static constexpr double MEASURES_PER_BAR = 8.0;
    static constexpr double MINIMUM_SP_AMOUNT = 0.5;
    const Position null_position {Beat(0.0), Measure(0.0)};

    if (!activation.sp_bar.full_enough_to_activate()) {
        return {null_position, ActValidity::insufficient_sp};
    }

    auto ending_pos = adjusted_hit_window_start(activation.act_end, squeeze);
    if (ending_pos.beat < activation.earliest_activation_point.beat) {
        ending_pos = activation.earliest_activation_point;
    }

    auto late_end_position
        = adjusted_hit_window_end(activation.act_start, squeeze);
    // This conditional can be taken if, for example, the first and last point
    // are the same.
    if (late_end_position.beat > ending_pos.beat) {
        late_end_position = ending_pos;
    }
    auto late_end_sp = activation.sp_bar.max();
    late_end_sp
        += m_sp_data.available_whammy(activation.earliest_activation_point.beat,
                                      activation.act_start->position.beat);
    late_end_sp = std::min(late_end_sp, 1.0);

    SpStatus status_for_early_end {
        activation.earliest_activation_point,
        std::max(activation.sp_bar.min(), MINIMUM_SP_AMOUNT)};
    SpStatus status_for_late_end {late_end_position, late_end_sp};

    for (auto p = m_points.next_sp_granting_note(activation.act_start);
         p < activation.act_end;
         p = m_points.next_sp_granting_note(std::next(p))) {
        auto p_start = adjusted_hit_window_start(p, squeeze);
        if (p_start.beat < activation.earliest_activation_point.beat) {
            p_start = activation.earliest_activation_point;
        }
        auto p_end = adjusted_hit_window_end(p, squeeze);
        if (p_end.beat > ending_pos.beat) {
            p_end = ending_pos;
        }
        status_for_late_end.update_late_end(p_start, p_end, m_sp_data);
        if (status_for_late_end.sp() < 0.0) {
            return {null_position, ActValidity::insufficient_sp};
        }
        status_for_early_end.update_early_end(p_start, m_sp_data,
                                              required_whammy_end);
        status_for_early_end.add_phrase();
        status_for_late_end.add_phrase();
    }

    status_for_late_end.advance_whammy_max(ending_pos, m_sp_data);
    if (status_for_late_end.sp() < 0.0) {
        return {null_position, ActValidity::insufficient_sp};
    }

    status_for_early_end.update_early_end(ending_pos, m_sp_data,
                                          required_whammy_end);
    if (activation.act_end->is_sp_granting_note) {
        status_for_early_end.add_phrase();
    }
    const auto end_meas = status_for_early_end.position().measure
        + Measure(status_for_early_end.sp() * MEASURES_PER_BAR);

    const auto next_point = std::next(activation.act_end);
    if (next_point != m_points.cend()
        && end_meas >= adjusted_hit_window_end(next_point, squeeze).measure) {
        return {null_position, ActValidity::surplus_sp};
    }

    const auto end_beat = m_converter.measures_to_beats(end_meas);
    return {{end_beat, end_meas}, ActValidity::success};
}

std::string ProcessedSong::path_summary(const Path& path) const
{
    // We use std::stringstream instead of std::string for better formating of
    // floats (average multiplier and mid-sustain activation positions).
    std::stringstream stream;

    auto no_sp_score = std::accumulate(
        m_points.cbegin(), m_points.cend(), 0,
        [](const auto x, const auto& y) { return x + y.value; });
    no_sp_score += m_total_solo_boost;
    stream << "No SP score: " << no_sp_score;

    const auto total_score = no_sp_score + path.score_boost;
    stream << "\nTotal score: " << total_score;

    const auto avg_mult
        = static_cast<double>(total_score - m_total_solo_boost) / m_base_score;
    stream.setf(std::ios_base::fixed, std::ios_base::floatfield);
    stream << std::setprecision(3);
    stream << "\nAverage multiplier: " << avg_mult << 'x';

    return stream.str();
}
