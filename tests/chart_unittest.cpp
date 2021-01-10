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

#include <tuple>

#include "catch.hpp"

#include "chart.hpp"
#include "songparts.hpp"

static bool operator==(const BpmEvent& lhs, const BpmEvent& rhs)
{
    return std::tie(lhs.position, lhs.bpm) == std::tie(rhs.position, rhs.bpm);
}

static bool operator==(const Event& lhs, const Event& rhs)
{
    return std::tie(lhs.position, lhs.data) == std::tie(rhs.position, rhs.data);
}

static bool operator==(const NoteEvent& lhs, const NoteEvent& rhs)
{
    return std::tie(lhs.position, lhs.fret, lhs.length)
        == std::tie(rhs.position, rhs.fret, rhs.length);
}

static bool operator==(const SPEvent& lhs, const SPEvent& rhs)
{
    return std::tie(lhs.position, lhs.key, lhs.length)
        == std::tie(rhs.position, rhs.key, rhs.length);
}

static bool operator==(const TimeSigEvent& lhs, const TimeSigEvent& rhs)
{
    return std::tie(lhs.position, lhs.numerator, lhs.denominator)
        == std::tie(rhs.position, rhs.numerator, rhs.denominator);
}

TEST_CASE("Section names are read")
{
    const char* text = "[SectionA]\n{\n}\n[SectionB]\n{\n}\n";

    const auto chart = parse_chart(text);

    REQUIRE(chart.sections.size() == 2);
    REQUIRE(chart.sections[0].name == "SectionA");
    REQUIRE(chart.sections[1].name == "SectionB");
}

TEST_CASE("Parser skips UTF-8 BOM")
{
    const char* text = "\xEF\xBB\xBF[Song]\n{\n}\n";

    const auto chart = parse_chart(text);

    REQUIRE(chart.sections.size() == 1);
    REQUIRE(chart.sections[0].name == "Song");
}

TEST_CASE("Chart can end without a newline")
{
    const char* text = "[Song]\n{\n}";

    REQUIRE_NOTHROW([&] { return parse_chart(text); }());
}

TEST_CASE("Parser does not infinite loop on an unfinished section")
{
    const char* text = "[UnrecognisedSection]\n{\n";

    REQUIRE_THROWS_AS([&] { return parse_chart(text); }(), ParseError);
}

TEST_CASE("Key value pairs are read")
{
    const char* text = "[Section]\n{\nKey = Value\nKey2 = Value2\n}";
    const std::map<std::string, std::string> pairs {{"Key", "Value"},
                                                    {"Key2", "Value2"}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.key_value_pairs == pairs);
}

TEST_CASE("Note events are read")
{
    const char* text = "[Section]\n{\n1000 = N 1 0\n}";
    const std::vector<NoteEvent> events {{1000, 1, 0}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.note_events == events);
}

TEST_CASE("Note events with extra spaces cause an exception")
{
    const char* text = "[Section]\n{\n768 = N  0 0\n}";

    REQUIRE_THROWS_AS([&] { return parse_chart(text); }(), ParseError);
}

TEST_CASE("BPM events are read")
{
    const char* text = "[Section]\n{\n1000 = B 150000\n}";
    const std::vector<BpmEvent> events {{1000, 150000}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.bpm_events == events);
}

TEST_CASE("TS events are read")
{
    const char* text = "[Section]\n{\n1000 = TS 4\n2000 = TS 3 3\n}";
    const std::vector<TimeSigEvent> events {{1000, 4, 2}, {2000, 3, 3}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.ts_events == events);
}

TEST_CASE("S events are read")
{
    const char* text = "[Section]\n{\n1000 = S 2 700\n}";
    const std::vector<SPEvent> events {{1000, 2, 700}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.sp_events == events);
}

TEST_CASE("E events are read")
{
    const char* text = "[Section]\n{\n1000 = E soloing\n}";
    const std::vector<Event> events {{1000, "soloing"}};

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.events == events);
}

TEST_CASE("Other events are ignored")
{
    const char* text = "[Section]\n{\n1105 = A 133\n}";

    const auto section = parse_chart(text).sections[0];

    REQUIRE(section.note_events.empty());
    REQUIRE(section.note_events.empty());
    REQUIRE(section.bpm_events.empty());
    REQUIRE(section.ts_events.empty());
    REQUIRE(section.events.empty());
}

// Yes, these are actually a thing. Clone Hero accepts them, so I have to.
TEST_CASE("Charts in UTF-16le are read correctly")
{
    const std::string text {
        "\xFF\xFE\x5B\x00\x53\x00\x6F\x00\x6E\x00\x67\x00\x5D\x00\x0D\x00\x0A"
        "\x00\x7B\x00\x0D\x00\x0A\x00\x7D\x00",
        26};

    const auto chart = parse_chart(text);

    REQUIRE(chart.sections.size() == 1);
    REQUIRE(chart.sections[0].name == "Song");
}

TEST_CASE("Charts in UTF-16le must be an even number of bytes large")
{
    const std::string text {
        "\xFF\xFE\x5B\x00\x53\x00\x6F\x00\x6E\x00\x67\x00\x5D\x00\x0D\x00\x0A"
        "\x00\x7B\x00\x0D\x00\x0A\x00\x7D\x00\x00",
        27};

    REQUIRE_THROWS_AS([&] { return parse_chart(text); }(), ParseError);
}

TEST_CASE("Single character headers should throw an error")
{
    REQUIRE_THROWS_AS([] { return parse_chart("\n"); }(), ParseError);
}

TEST_CASE("Short mid-section lines throw an error")
{
    REQUIRE_THROWS_AS(
        [&] { return parse_chart("[ExpertGuitar]\n{\n1 1\n}"); }(), ParseError);
    REQUIRE_THROWS_AS(
        [&] { return parse_chart("[ExpertGuitar]\n{\n1 = N 1\n}"); }(),
        ParseError);
}
