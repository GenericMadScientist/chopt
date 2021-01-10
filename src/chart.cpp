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

#include <charconv>
#include <optional>

#include <iostream>

#include "chart.hpp"
#include "songparts.hpp"
#include "stringutil.hpp"

static std::string_view strip_square_brackets(std::string_view input)
{
    if (input.empty()) {
        throw ParseError("Header string empty");
    }
    return input.substr(1, input.size() - 2);
}

// Convert a string_view to an int. If there are any problems with the input,
// this function returns std::nullopt.
static std::optional<int> string_view_to_int(std::string_view input)
{
    int result = 0;
    const char* last = input.data() + input.size();
    auto [p, ec] = std::from_chars(input.data(), last, result);
    if ((ec != std::errc()) || (p != last)) {
        return std::nullopt;
    }
    return result;
}

// Split input by space characters, similar to .Split(' ') in C#. Note that
// the lifetime of the string_views in the output is the same as that of the
// input.
static std::vector<std::string_view> split_by_space(std::string_view input)
{
    std::vector<std::string_view> substrings;

    while (true) {
        const auto space_location = input.find(' ');
        if (space_location == std::string_view::npos) {
            break;
        }
        substrings.push_back(input.substr(0, space_location));
        input.remove_prefix(space_location + 1);
    }

    substrings.push_back(input);
    return substrings;
}

static ChartSection read_section(std::string_view& input)
{
    constexpr int MAX_NORMAL_EVENT_SIZE = 5;

    ChartSection section;
    section.name = strip_square_brackets(break_off_newline(input));

    if (break_off_newline(input) != "{") {
        throw ParseError("Section does not open with {");
    }

    while (true) {
        const auto next_line = break_off_newline(input);
        if (next_line == "}") {
            break;
        }
        const auto separated_line = split_by_space(next_line);
        if (separated_line.size() < 3) {
            throw ParseError("Line incomplete");
        }
        const auto key = separated_line[0];
        const auto key_val = string_view_to_int(key);
        if (key_val.has_value()) {
            const auto pos = *key_val;
            if (separated_line[2] == "N") {
                if (separated_line.size() < MAX_NORMAL_EVENT_SIZE) {
                    throw ParseError("Line incomplete");
                }
                const auto fret = string_view_to_int(separated_line[3]);
                const auto length = string_view_to_int(separated_line[4]);
                if (!fret.has_value() || !length.has_value()) {
                    throw ParseError("Bad note event");
                }
                section.note_events.push_back(NoteEvent {pos, *fret, *length});
            } else if (separated_line[2] == "S") {
                if (separated_line.size() < MAX_NORMAL_EVENT_SIZE) {
                    throw ParseError("Line incomplete");
                }
                const auto sp_key = string_view_to_int(separated_line[3]);
                const auto length = string_view_to_int(separated_line[4]);
                if (!sp_key.has_value() || !length.has_value()) {
                    throw ParseError("Bad SP event");
                }
                section.sp_events.push_back(SPEvent {pos, *sp_key, *length});
            } else if (separated_line[2] == "B") {
                if (separated_line.size() < 4) {
                    throw ParseError("Line incomplete");
                }
                const auto bpm = string_view_to_int(separated_line[3]);
                if (!bpm.has_value()) {
                    throw ParseError("Bad BPM event");
                }
                section.bpm_events.push_back(BpmEvent {pos, *bpm});
            } else if (separated_line[2] == "TS") {
                if (separated_line.size() < 4) {
                    throw ParseError("Line incomplete");
                }
                const auto numer = string_view_to_int(separated_line[3]);
                std::optional<int> denom = 2;
                if (separated_line.size() >= MAX_NORMAL_EVENT_SIZE) {
                    denom = string_view_to_int(separated_line[4]);
                }
                if (!numer.has_value() || !denom.has_value()) {
                    throw ParseError("Bad TS event");
                }
                section.ts_events.push_back(TimeSigEvent {pos, *numer, *denom});
            } else if (separated_line[2] == "E") {
                if (separated_line.size() < 4) {
                    throw ParseError("Line incomplete");
                }
                section.events.push_back(
                    Event {pos, std::string {separated_line[3]}});
            }
        } else {
            std::string value {separated_line[2]};
            for (auto i = 3U; i < separated_line.size(); ++i) {
                value.append(separated_line[i]);
            }
            section.key_value_pairs[std::string(key)] = value;
        }
    }

    return section;
}

Chart parse_chart(std::string_view data)
{
    Chart chart;
    std::string u8_string;

    try {
        u8_string = to_utf8_string(data);
    } catch (const std::invalid_argument& e) {
        throw ParseError(e.what());
    }

    data = u8_string;

    while (!data.empty()) {
        chart.sections.push_back(read_section(data));
    }

    return chart;
}
