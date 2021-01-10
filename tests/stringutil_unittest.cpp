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

#include "catch.hpp"

#include "stringutil.hpp"

TEST_CASE("break_off_newline works correctly")
{
    std::string_view data = "Hello\nThere\r\nWorld!";

    REQUIRE(break_off_newline(data) == "Hello");
    REQUIRE(data == "There\r\nWorld!");
    REQUIRE(break_off_newline(data) == "There");
    REQUIRE(data == "World!");
}

TEST_CASE("skip_whitespace works correctly")
{
    REQUIRE(skip_whitespace("Hello") == "Hello");
    REQUIRE(skip_whitespace("  Hello") == "Hello");
    REQUIRE(skip_whitespace("H  ello") == "H  ello");
}

TEST_CASE("to_utf8_string strips UTF-8 BOM")
{
    const std::string text {"\xEF\xBB\xBF\x6E"};

    REQUIRE(to_utf8_string(text) == "n");
}

TEST_CASE("to_utf8_string treats BOM-less string as UTF-8")
{
    const std::string text {"Hello"};

    REQUIRE(to_utf8_string(text) == "Hello");
}

TEST_CASE("to_utf8_string correctly converts a UTF16-le string to UTF-8")
{
    const std::string text {
        "\xFF\xFE\x6E\x00\x61\x00\x6D\x00\x65\x00\x3D\x00\x54\x00\x65\x00\x73"
        "\x00\x74\x00",
        20};

    REQUIRE(to_utf8_string(text) == "name=Test");
}

TEST_CASE("to_utf8_string throws on a string with odd length")
{
    const std::string text {"\xFF\xFE\x6E"};

    REQUIRE_THROWS([&] { return to_utf8_string(text); }());
}

TEST_CASE("to_ordinal works correctly")
{
    REQUIRE(to_ordinal(0) == "0th");
    REQUIRE(to_ordinal(1) == "1st");
    REQUIRE(to_ordinal(2) == "2nd");
    REQUIRE(to_ordinal(3) == "3rd");
    REQUIRE(to_ordinal(4) == "4th");
    REQUIRE(to_ordinal(11) == "11th");
    REQUIRE(to_ordinal(12) == "12th");
    REQUIRE(to_ordinal(13) == "13th");
    REQUIRE_THROWS([&] { return to_ordinal(-1); }());
}
