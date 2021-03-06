/*
 * CHOpt - Star Power optimiser for Clone Hero
 * Copyright (C) 2020 Raymond Wright
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

#include <cstdint>
#include <stdexcept>

#include "argparse_wrapper.hpp"

#include "settings.hpp"

static int str_to_int(const std::string& value)
{
    std::size_t pos = 0;
    const auto converted_value = std::stoi(value, &pos);
    if (pos != value.size()) {
        throw std::invalid_argument("Could not convert string to int");
    }
    return converted_value;
}

static float str_to_float(const std::string& value)
{
    std::size_t pos = 0;
    const auto converted_value = std::stof(value, &pos);
    if (pos != value.size()) {
        throw std::invalid_argument("Could not convert string to float");
    }
    return converted_value;
}

static bool is_valid_image_path(const std::string& path)
{
    if (path.size() < 4) {
        return false;
    }
    const auto file_type = path.substr(path.size() - 4, 4);
    if (file_type == ".bmp") {
        return true;
    }
    if (file_type == ".png") {
        return true;
    }
    return false;
}

Settings from_args(int argc, char** argv)
{
    constexpr float DEFAULT_OPACITY = 0.33F;
    constexpr int DEFAULT_SPEED = 100;
    constexpr int MAX_PERCENT = 100;
    constexpr int MAX_SPEED = 5000;
    constexpr int MAX_VIDEO_LAG = 200;
    constexpr int MIN_SPEED = 5;
    constexpr double MS_PER_SECOND = 1000.0;

    argparse::ArgumentParser program {"CHOpt"};

    program.add_argument("-f", "--file")
        .default_value(std::string {"-"})
        .help("chart filename");
    program.add_argument("-o", "--output")
        .default_value(std::string {"path.png"})
        .help("location to save output image (must be a .bmp or .png), "
              "defaults to path.png");
    program.add_argument("-d", "--diff")
        .default_value(std::string {"expert"})
        .help("difficulty, options are easy, medium, hard, expert, defaults to "
              "expert");
    program.add_argument("-i", "--instrument")
        .default_value(std::string {"guitar"})
        .help("instrument, options are guitar, coop, bass, rhythm, keys, ghl, "
              "ghlbass, drums, defaults to guitar");
    program.add_argument("--sqz", "--squeeze")
        .default_value(MAX_PERCENT)
        .help("squeeze% (0 to 100), defaults to 100")
        .action([](const std::string& value) { return str_to_int(value); });
    program.add_argument("--ew", "--early-whammy")
        .default_value(std::string {"match"})
        .help("early whammy% (0 to 100), <= squeeze, defaults to squeeze");
    program.add_argument("--lazy", "--lazy-whammy")
        .default_value(0)
        .help("time before whammying starts on sustains in milliseconds, "
              "defaults to 0")
        .action([](const std::string& value) { return str_to_int(value); });
    program.add_argument("--lag", "--video-lag")
        .default_value(0)
        .help("video lag calibration setting in milliseconds, defaults to 0")
        .action([](const std::string& value) { return str_to_int(value); });
    program.add_argument("-s", "--speed")
        .default_value(DEFAULT_SPEED)
        .help("speed in %, defaults to 100")
        .action([](const std::string& value) { return str_to_int(value); });
    program.add_argument("-b", "--blank")
        .help("give a blank chart image")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--no-bpms")
        .help("do not draw BPMs")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--no-solos")
        .help("do not draw solo sections")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--no-time-sigs")
        .help("do not draw time signatures")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--act-opacity")
        .help("opacity of drawn activations (0.0 to 1.0), defaults to 0.33")
        .default_value(DEFAULT_OPACITY)
        .action([](const std::string& value) { return str_to_float(value); });

    program.parse_args(argc, argv);

    Settings settings;

    settings.blank = program.get<bool>("--blank");
    settings.filename = program.get<std::string>("--file");
    if (settings.filename == "-") {
        throw std::invalid_argument("No file was specified");
    }

    const auto diff_string = program.get<std::string>("--diff");
    if (diff_string == "expert") {
        settings.difficulty = Difficulty::Expert;
    } else if (diff_string == "hard") {
        settings.difficulty = Difficulty::Hard;
    } else if (diff_string == "medium") {
        settings.difficulty = Difficulty::Medium;
    } else if (diff_string == "easy") {
        settings.difficulty = Difficulty::Easy;
    } else {
        throw std::invalid_argument("Unrecognised difficulty");
    }

    const auto inst_string = program.get<std::string>("--instrument");
    if (inst_string == "guitar") {
        settings.instrument = Instrument::Guitar;
    } else if (inst_string == "coop") {
        settings.instrument = Instrument::GuitarCoop;
    } else if (inst_string == "bass") {
        settings.instrument = Instrument::Bass;
    } else if (inst_string == "rhythm") {
        settings.instrument = Instrument::Rhythm;
    } else if (inst_string == "keys") {
        settings.instrument = Instrument::Keys;
    } else if (inst_string == "ghl") {
        settings.instrument = Instrument::GHLGuitar;
    } else if (inst_string == "ghlbass") {
        settings.instrument = Instrument::GHLBass;
    } else if (inst_string == "drums") {
        settings.instrument = Instrument::Drums;
    } else {
        throw std::invalid_argument("Unrecognised instrument");
    }

    const auto image_path = program.get<std::string>("--output");
    if (!is_valid_image_path(image_path)) {
        throw std::invalid_argument(
            "Image output must be a bitmap or png (.bmp / .png)");
    }
    settings.image_path = image_path;

    settings.draw_bpms = !program.get<bool>("--no-bpms");
    settings.draw_solos = !program.get<bool>("--no-solos");
    settings.draw_time_sigs = !program.get<bool>("--no-time-sigs");

    const auto squeeze = program.get<int>("--squeeze");
    auto early_whammy = squeeze;
    if (program.get<std::string>("--early-whammy") != "match") {
        early_whammy = str_to_int(program.get<std::string>("--early-whammy"));
    }
    const auto lazy_whammy = program.get<int>("--lazy-whammy");

    if (squeeze < 0 || squeeze > MAX_PERCENT) {
        throw std::invalid_argument("Squeeze must lie between 0 and 100");
    }
    if (early_whammy < 0 || early_whammy > MAX_PERCENT) {
        throw std::invalid_argument("Early whammy must lie between 0 and 100");
    }
    if (lazy_whammy < 0) {
        throw std::invalid_argument(
            "Lazy whammy must be greater than or equal to 0");
    }

    settings.squeeze = squeeze / 100.0;
    settings.early_whammy = early_whammy / 100.0;
    settings.lazy_whammy = lazy_whammy / MS_PER_SECOND;

    const auto video_lag = program.get<int>("--video-lag");
    if (video_lag < -MAX_VIDEO_LAG || video_lag > MAX_VIDEO_LAG) {
        throw std::invalid_argument(
            "Video lag setting unsupported by Clone Hero");
    }

    settings.video_lag = video_lag / MS_PER_SECOND;

    const auto speed = program.get<int>("--speed");
    if (speed < MIN_SPEED || speed > MAX_SPEED || speed % MIN_SPEED != 0) {
        throw std::invalid_argument("Speed unsupported by Clone Hero");
    }

    settings.speed = speed;

    const auto opacity = program.get<float>("--act-opacity");
    if (opacity < 0.0F || opacity > 1.0F) {
        throw std::invalid_argument(
            "Activation opacity should lie between 0.0 and 1.0");
    }

    settings.opacity = opacity;

    return settings;
}
