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

#include "songparts.hpp"

SyncTrack::SyncTrack(std::vector<TimeSignature> time_sigs,
                     std::vector<BPM> bpms)
{
    constexpr auto DEFAULT_BPM = 120000;

    for (const auto& bpm : bpms) {
        if (bpm.bpm <= 0) {
            throw ParseError("BPMs must be positive");
        }
    }
    for (const auto& ts : time_sigs) {
        if (ts.numerator <= 0 || ts.denominator <= 0) {
            throw ParseError("Time signatures must be positive/positive");
        }
    }

    std::stable_sort(
        bpms.begin(), bpms.end(),
        [](const auto& x, const auto& y) { return x.position < y.position; });
    BPM prev_bpm {0, DEFAULT_BPM};
    for (auto p = bpms.cbegin(); p < bpms.cend(); ++p) {
        if (p->position != prev_bpm.position) {
            m_bpms.push_back(prev_bpm);
        }
        prev_bpm = *p;
    }
    m_bpms.push_back(prev_bpm);

    std::stable_sort(
        time_sigs.begin(), time_sigs.end(),
        [](const auto& x, const auto& y) { return x.position < y.position; });
    TimeSignature prev_ts {0, 4, 4};
    for (auto p = time_sigs.cbegin(); p < time_sigs.cend(); ++p) {
        if (p->position != prev_ts.position) {
            m_time_sigs.push_back(prev_ts);
        }
        prev_ts = *p;
    }
    m_time_sigs.push_back(prev_ts);
}

SyncTrack SyncTrack::speedup(int speed) const
{
    constexpr auto DEFAULT_SPEED = 100;

    SyncTrack speedup {m_time_sigs, m_bpms};
    for (auto& bpm : speedup.m_bpms) {
        bpm.bpm = (bpm.bpm * speed) / DEFAULT_SPEED;
    }
    return speedup;
}
