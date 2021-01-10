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
#include <vector>

#include "catch.hpp"

#include "midi.hpp"
#include "songparts.hpp"

static std::vector<std::uint8_t>
midi_from_tracks(const std::vector<std::vector<std::uint8_t>>& track_sections)
{
    std::vector<std::uint8_t> data {0x4D, 0x54, 0x68, 0x64, 0, 0, 0, 6, 0, 1};
    auto count = track_sections.size();
    data.push_back((count >> 8) & 0xFF);
    data.push_back(count & 0xFF);
    data.push_back(1);
    data.push_back(0xE0);
    for (const auto& track : track_sections) {
        for (auto byte : track) {
            data.push_back(byte);
        }
    }
    return data;
}

static bool operator==(const MetaEvent& lhs, const MetaEvent& rhs)
{
    return std::tie(lhs.type, lhs.data) == std::tie(rhs.type, rhs.data);
}

static bool operator==(const MidiEvent& lhs, const MidiEvent& rhs)
{
    return std::tie(lhs.status, lhs.data) == std::tie(rhs.status, rhs.data);
}

static bool operator==(const SysexEvent& lhs, const SysexEvent& rhs)
{
    return lhs.data == rhs.data;
}

static bool operator==(const TimedEvent& lhs, const TimedEvent& rhs)
{
    return std::tie(lhs.time, lhs.event) == std::tie(rhs.time, rhs.event);
}

TEST_CASE("parse_midi reads header correctly")
{
    std::vector<std::uint8_t> data {0x4D, 0x54, 0x68, 0x64, 0, 0, 0,
                                    6,    0,    1,    0,    0, 1, 0xE0};
    std::vector<std::uint8_t> bad_data {0x4D, 0x53, 0x68, 0x64, 0, 0, 0,
                                        6,    0,    1,    0,    0, 1, 0xE0};

    const auto midi = parse_midi(data);

    REQUIRE(midi.ticks_per_quarter_note == 0x1E0);
    REQUIRE(midi.tracks.empty());
    REQUIRE_THROWS_AS([&] { return parse_midi(bad_data); }(), ParseError);
}

TEST_CASE("Division must not be in SMPTE format")
{
    std::vector<std::uint8_t> bad_data {0x4D, 0x54, 0x68, 0x64, 0, 0,    0,
                                        6,    0,    1,    0,    0, 0x80, 0};

    REQUIRE_THROWS_AS([&] { return parse_midi(bad_data); }(), ParseError);
}

TEST_CASE("Track lengths are read correctly")
{
    std::vector<std::uint8_t> track_one {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0, 0};
    std::vector<std::uint8_t> track_two {0x4D, 0x54, 0x72, 0x6B, 0,    0,
                                         0,    4,    0,    0x85, 0x60, 0};
    auto data = midi_from_tracks({track_one, track_two});

    const auto midi = parse_midi(data);

    REQUIRE(midi.tracks.size() == 2);
    REQUIRE(midi.tracks[0].events.empty());
    REQUIRE(midi.tracks[1].events.size() == 1);
}

TEST_CASE("Track magic number is checked")
{
    std::vector<std::uint8_t> bad_track {0x40, 0x54, 0x72, 0x6B, 0, 0, 0, 0};
    auto data = midi_from_tracks({bad_track});

    REQUIRE_THROWS_AS([&] { return parse_midi(data); }(), ParseError);
}

TEST_CASE("Event times are handled correctly")
{
    SECTION("Multi-byte delta times are parsed correctly")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0,
                                         5,    0x8F, 0x10, 0xFF, 2, 0};
        auto data = midi_from_tracks({track});

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events[0].time == 0x790);
    }

    SECTION("Times are absolute, not delta times")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0,    0, 8,
                                         0x60, 0xFF, 2,    0,    0, 0xFF, 2, 0};
        auto data = midi_from_tracks({track});

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events[1].time == 0x60);
    }

    SECTION("Five byte multi-byte delta times throw")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0,
                                         0,    8,    0x8F, 0x8F, 0x8F, 0x8F,
                                         0x10, 0xFF, 2,    0};
        const auto data = midi_from_tracks({track});

        REQUIRE_THROWS_AS([&] { return parse_midi(data); }(), ParseError);
    }
}

TEST_CASE("Meta events are read")
{
    SECTION("Simple meta event is read")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,
                                         0,    0,    7,    0x60, 0xFF,
                                         0x51, 3,    8,    0x6B, 0xC3};
        auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {
            {0x60, MetaEvent {0x51, {8, 0x6B, 0xC3}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Meta event with multi-byte length is read")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0,
                                         0,    8,    0x60, 0xFF, 0x51, 0x80,
                                         3,    8,    0x6B, 0xC3};
        const auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {
            {0x60, MetaEvent {0x51, {8, 0x6B, 0xC3}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Too long meta events throw")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0,
                                         0,    8,    0x60, 0xFF, 0x51, 0x80,
                                         100,  8,    0x6B, 0xC3};
        const auto data = midi_from_tracks({track});

        REQUIRE_THROWS_AS([&] { return parse_midi(data); }(), ParseError);
    }
}

TEST_CASE("Midi events are read")
{
    SECTION("A single event is read")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0,
                                         0,    4,    0,    0x94, 0x7F, 0x64};
        auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {{0, MidiEvent {0x94, {0x7F, 0x64}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Running status is parsed")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,
                                         0,    0,    7,    0,    0x94,
                                         0x7F, 0x64, 0x10, 0x7F, 0x64};
        auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {{0, MidiEvent {0x94, {0x7F, 0x64}}},
                                        {0x10, MidiEvent {0x94, {0x7F, 0x64}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Running status is not stopped by Meta Events")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0, 0,
                                         11,   0,    0x94, 0x7F, 0x64, 0, 0xFF,
                                         2,    0,    0x10, 0x7F, 0x64};
        auto data = midi_from_tracks({track});

        REQUIRE_NOTHROW([&] { return parse_midi(data); }());
    }

    SECTION("Running status is not stopped by Syex Events")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0,    0, 0,
                                         11,   0,    0x94, 0x7F, 0x64, 0, 0xF0,
                                         1,    0,    0x10, 0x7F, 0x64};
        auto data = midi_from_tracks({track});

        REQUIRE_NOTHROW([&] { return parse_midi(data); }());
    }

    SECTION("Not all MIDI events take two data bytes")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0,    0,
                                         6,    0,    0xC0, 0,    0, 0xD0, 0};
        auto data = midi_from_tracks({track});

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events.size() == 2);
    }

    SECTION("MIDI events with status byte high nibble F throw")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0,
                                         0,    4,    0,    0xF0, 0, 0};
        auto data = midi_from_tracks({track});

        REQUIRE_THROWS_AS([&] { return parse_midi(data); }(), ParseError);
    }
}

TEST_CASE("Sysex events are read")
{
    SECTION("Simple sysex event is read")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0,
                                         6,    0x0,  0xF0, 3,    1, 2, 3};
        const auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {{0, SysexEvent {{1, 2, 3}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Sysex event with multi-byte length is read")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0, 7,
                                         0x0,  0xF0, 0x80, 3,    1, 2, 3};
        const auto data = midi_from_tracks({track});
        std::vector<TimedEvent> events {{0, SysexEvent {{1, 2, 3}}}};

        const auto midi = parse_midi(data);

        REQUIRE(midi.tracks[0].events == events);
    }

    SECTION("Sysex event with too high length throws")
    {
        std::vector<std::uint8_t> track {0x4D, 0x54, 0x72, 0x6B, 0, 0, 0,
                                         6,    0x0,  0xF0, 100,  1, 2, 3};
        const auto data = midi_from_tracks({track});

        REQUIRE_THROWS_AS([&] { return parse_midi(data); }(), ParseError);
    }
}
