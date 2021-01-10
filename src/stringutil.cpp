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
#include <stdexcept>

// libc++ is special and doesn't support <cuchar> yet, so we need a different
// method for the UTF-16le -> UTF-8 conversion. The alternative method isn't
// general because it uses a feature deprecated in C++17 and Microsoft's STL
// will complain if we use it.
#ifndef _LIBCPP_VERSION
#include <array>
#include <climits>
#include <cstdint>
#include <cuchar>
#else
#include <codecvt>
#include <locale>
#endif

#include "songparts.hpp"
#include "stringutil.hpp"

std::string_view break_off_newline(std::string_view& input)
{
    if (input.empty()) {
        throw ParseError("No lines left");
    }

    const auto newline_location = input.find_first_of("\r\n");
    if (newline_location == std::string_view::npos) {
        const auto line = input;
        input.remove_prefix(input.size());
        return line;
    }

    const auto line = input.substr(0, newline_location);
    input.remove_prefix(newline_location);
    input = skip_whitespace(input);
    return line;
}

std::string_view skip_whitespace(std::string_view input)
{
    const auto first_non_ws_location = input.find_first_not_of(" \f\n\r\t\v");
    input.remove_prefix(std::min(first_non_ws_location, input.size()));
    return input;
}

static std::string utf16_to_utf8_string(std::string_view input)
{
    if (input.size() % 2 != 0) {
        throw std::invalid_argument("UTF-16 strings must have even length");
    }

    // I'm pretty sure I really do need the reinterpret_cast here.
    std::u16string_view utf16_string_view {
        reinterpret_cast<const char16_t*>(input.data()), // NOLINT
        input.size() / 2};

#ifndef _LIBCPP_VERSION
    std::string u8_string;

    // This conversion method is from the c16rtomb page on cppreference. This
    // does not handle surrogate pairs, but I've only come across two UTF-16
    // charts so a proper solution can wait until the C++ standard library gets
    // a fix or a non-artificial chart comes up that this is a problem for.
    std::mbstate_t state {};
    std::array<char, MB_LEN_MAX> out {};
    for (auto c : utf16_string_view) {
        auto rc = std::c16rtomb(out.data(), c, &state);
        if (rc != static_cast<std::size_t>(-1)) {
            for (auto i = 0U; i < rc; ++i) {
                u8_string.push_back(out.at(i));
            }
        }
    }

    return u8_string;
#else
    // std::codecvt_utf8_utf16 is deprecated in C++17 and Microsoft's STL
    // complains about it so we can't have this be a general solution.
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}
        .to_bytes(utf16_string_view.cbegin(), utf16_string_view.cend());
#endif
}

static bool string_starts_with(std::string_view input, std::string_view pattern)
{
    if (input.size() < pattern.size()) {
        return false;
    }

    return input.substr(0, pattern.size()) == pattern;
}

std::string to_ordinal(int ordinal)
{
    constexpr int TENS_MODULUS = 10;
    constexpr int HUNDREDS_MODULUS = 100;
    constexpr std::array<int, 3> EXCEPTIONAL_TEENS {11, 12, 13};

    if (ordinal < 0) {
        throw std::runtime_error("ordinal was negative");
    }
    if (std::find(EXCEPTIONAL_TEENS.cbegin(), EXCEPTIONAL_TEENS.cend(),
                  ordinal % HUNDREDS_MODULUS)
        != EXCEPTIONAL_TEENS.cend()) {
        return std::to_string(ordinal) + "th";
    }
    if (ordinal % TENS_MODULUS == 1) {
        return std::to_string(ordinal) + "st";
    }
    if (ordinal % TENS_MODULUS == 2) {
        return std::to_string(ordinal) + "nd";
    }
    if (ordinal % TENS_MODULUS == 3) {
        return std::to_string(ordinal) + "rd";
    }
    return std::to_string(ordinal) + "th";
}

std::string to_utf8_string(std::string_view input)
{
    if (string_starts_with(input, "\xEF\xBB\xBF")) {
        // Trim off UTF-8 BOM.
        input.remove_prefix(3);
        return std::string(input);
    }
    if (string_starts_with(input, "\xFF\xFE")) {
        // Trim off UTF-16le BOM.
        input.remove_prefix(2);
        return utf16_to_utf8_string(input);
    }
    return std::string(input);
}
