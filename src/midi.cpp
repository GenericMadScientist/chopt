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

#include <climits>
#include <cstddef>
#include <utility>

#include "midi.hpp"
#include "songparts.hpp"

// This is a version of string_view for uint8_t. Once C++20 is ratified we can
// replace this with span<uint8_t>.
class ByteSpan {
private:
    using Iter = std::vector<std::uint8_t>::const_iterator;
    static constexpr std::size_t MAX_SIZE = static_cast<std::size_t>(-1);
    Iter m_begin;
    Iter m_end;
    std::size_t m_size;
    ByteSpan(Iter begin, Iter end)
        : m_begin {begin}
        , m_end {end}
        , m_size {static_cast<std::size_t>(m_end - m_begin)}
    {
    }

public:
    explicit ByteSpan(const std::vector<std::uint8_t>& data)
        : m_begin {data.cbegin()}
        , m_end {data.cend()}
        , m_size {data.size()}
    {
    }

    [[nodiscard]] std::vector<std::uint8_t>::const_iterator begin() const
    {
        return m_begin;
    }
    [[nodiscard]] std::vector<std::uint8_t>::const_iterator end() const
    {
        return m_end;
    }
    [[nodiscard]] std::size_t size() const { return m_size; }

    std::uint8_t operator[](std::size_t index) const
    {
        if (index >= m_size) {
            throw ParseError("index too large");
        }
        return *(m_begin + static_cast<std::ptrdiff_t>(index));
    }

    // Returns the subspan starting at offset of size count. Throws an exception
    // if offset is OOB. If count is -1, then returns the rest of the span, else
    // if count would go OOB then an exception is thrown.
    [[nodiscard]] ByteSpan subspan(std::size_t offset,
                                   std::size_t count = MAX_SIZE) const
    {
        if (offset > m_size) {
            throw ParseError("offset would be OOB");
        }
        if (count == MAX_SIZE) {
            return ByteSpan {m_begin + static_cast<std::ptrdiff_t>(offset),
                             m_end};
        }
        if (count > m_size - offset) {
            throw ParseError("count is too large");
        }
        return ByteSpan {m_begin + static_cast<std::ptrdiff_t>(offset),
                         m_begin + static_cast<std::ptrdiff_t>(offset + count)};
    }
};

// Read a two byte big endian number from the specified offset.
static int read_two_byte_be(ByteSpan span, std::size_t offset)
{
    return span[offset] << CHAR_BIT | span[offset + 1];
}

// Read a four byte big endian number from the specified offset.
static int read_four_byte_be(ByteSpan span, std::size_t offset)
{
    return span[offset] << (3 * CHAR_BIT) | span[offset + 1] << (2 * CHAR_BIT)
        | span[offset + 2] << CHAR_BIT | span[offset + 3];
}

struct MidiHeader {
    int ticks_per_quarter_note;
    int num_of_tracks;
};

static ByteSpan read_midi_header(ByteSpan span, MidiHeader& header)
{
    constexpr std::array<std::uint8_t, 10> MAGIC_NUMBER {
        0x4D, 0x54, 0x68, 0x64, 0, 0, 0, 6, 0, 1};
    constexpr int DIVISION_NEGATIVE_SMPTE_MASK = 0x8000;
    constexpr int FIRST_TRACK_OFFSET = 14;
    constexpr int TICKS_OFFSET = 12;
    constexpr int TRACK_COUNT_OFFSET = 10;

    const auto first_ten_bytes = span.subspan(0, MAGIC_NUMBER.size());
    if (!std::equal(first_ten_bytes.begin(), first_ten_bytes.end(),
                    MAGIC_NUMBER.cbegin())) {
        throw ParseError("Invalid MIDI file");
    }
    header.num_of_tracks = read_two_byte_be(span, TRACK_COUNT_OFFSET);
    auto division = read_two_byte_be(span, TICKS_OFFSET);
    if ((division & DIVISION_NEGATIVE_SMPTE_MASK) != 0) {
        throw ParseError("Only ticks per quarter-note is supported");
    }
    header.ticks_per_quarter_note = division;
    return span.subspan(FIRST_TRACK_OFFSET);
}

static ByteSpan read_variable_length_num(ByteSpan span, int& number)
{
    constexpr int VARIABLE_LENGTH_DATA_MASK = 0x7F;
    constexpr int VARIABLE_LENGTH_DATA_SIZE = 7;
    constexpr int VARIABLE_LENGTH_HIGH_MASK = 0x80;

    number = 0;
    int bytes_read = 0;
    while ((span[0] & VARIABLE_LENGTH_HIGH_MASK) != 0) {
        ++bytes_read;
        if (bytes_read >= 4) {
            throw ParseError("Too long variable length number");
        }
        number <<= VARIABLE_LENGTH_DATA_SIZE;
        number |= span[0] & VARIABLE_LENGTH_DATA_MASK;
        span = span.subspan(1);
    }
    number <<= VARIABLE_LENGTH_DATA_SIZE;
    number |= span[0] & VARIABLE_LENGTH_DATA_MASK;
    return span.subspan(1);
}

static ByteSpan read_meta_event(ByteSpan span, MetaEvent& event)
{
    event.type = span[0];
    span = span.subspan(1);
    int data_length = 0;
    span = read_variable_length_num(span, data_length);
    if (static_cast<std::size_t>(data_length) > span.size()) {
        throw ParseError("Meta Event too long");
    }
    event.data
        = std::vector<std::uint8_t> {span.begin(), span.begin() + data_length};
    return span.subspan(static_cast<std::size_t>(data_length));
}

static ByteSpan read_midi_event(ByteSpan span, MidiEvent& event,
                                int prev_status_byte)
{
    constexpr int CHANNEL_PRESSURE_ID = 0xD0;
    constexpr int IS_STATUS_BYTE_MASK = 0x80;
    constexpr int PROGRAM_CHANGE_ID = 0xC0;
    constexpr int SYSTEM_COMMON_MSG_ID = 0xF0;
    constexpr int UPPER_NIBBLE_MASK = 0xF0;

    auto event_type = span[0];
    if ((event_type & IS_STATUS_BYTE_MASK) != 0) {
        span = span.subspan(1);
    } else if (prev_status_byte != -1) {
        event_type = static_cast<std::uint8_t>(prev_status_byte);
    } else {
        throw ParseError(
            "MIDI Event has no status byte and there is no running status");
    }

    if ((event_type & UPPER_NIBBLE_MASK) == SYSTEM_COMMON_MSG_ID) {
        throw ParseError("MIDI Events with high nibble 0xF are not supported");
    }
    if ((event_type & UPPER_NIBBLE_MASK) == PROGRAM_CHANGE_ID
        || (event_type & UPPER_NIBBLE_MASK) == CHANNEL_PRESSURE_ID) {
        event.data = {span[0], 0};
        span = span.subspan(1);
    } else {
        event.data = {span[0], span[1]};
        span = span.subspan(2);
    }
    event.status = event_type;

    return span;
}

static ByteSpan read_sysex_event(ByteSpan span, SysexEvent& event)
{
    int data_length = 0;
    span = read_variable_length_num(span, data_length);
    if (static_cast<std::size_t>(data_length) > span.size()) {
        throw ParseError("Sysex Event too long");
    }
    event.data
        = std::vector<std::uint8_t> {span.begin(), span.begin() + data_length};
    return span.subspan(static_cast<std::size_t>(data_length));
}

static ByteSpan read_midi_track(ByteSpan span, MidiTrack& track)
{
    constexpr int META_EVENT_ID = 0xFF;
    constexpr int SYSEX_EVENT_ID = 0xF0;
    constexpr int TRACK_HEADER_MAGIC_NUMBER = 0x4D54726B;
    constexpr int TRACK_HEADER_SIZE = 8;

    if (read_four_byte_be(span, 0) != TRACK_HEADER_MAGIC_NUMBER) {
        throw ParseError("Invalid MIDI file");
    }
    auto track_size = read_four_byte_be(span, 4);
    span = span.subspan(TRACK_HEADER_SIZE);
    auto absolute_time = 0;
    const auto final_span_size
        = span.size() - static_cast<std::size_t>(track_size);
    auto prev_status_byte = -1;
    while (span.size() != final_span_size) {
        int delta_time = 0;
        span = read_variable_length_num(span, delta_time);
        absolute_time += delta_time;
        TimedEvent event {absolute_time, {}};
        const auto event_type = span[0];
        if (event_type == META_EVENT_ID) {
            span = span.subspan(1);
            MetaEvent meta_event {};
            span = read_meta_event(span, meta_event);
            event.event = meta_event;
        } else if (event_type == SYSEX_EVENT_ID) {
            span = span.subspan(1);
            SysexEvent sysex_event {};
            span = read_sysex_event(span, sysex_event);
            event.event = sysex_event;
        } else {
            MidiEvent midi_event {};
            span = read_midi_event(span, midi_event, prev_status_byte);
            prev_status_byte = midi_event.status;
            event.event = midi_event;
        }
        track.events.push_back(event);
    }
    return span;
}

Midi parse_midi(const std::vector<std::uint8_t>& data)
{
    ByteSpan span {data};
    MidiHeader header {};
    span = read_midi_header(span, header);
    std::vector<MidiTrack> tracks;
    for (auto i = 0; i < header.num_of_tracks; ++i) {
        MidiTrack track {};
        span = read_midi_track(span, track);
        tracks.push_back(track);
    }
    return Midi {header.ticks_per_quarter_note, std::move(tracks)};
}
