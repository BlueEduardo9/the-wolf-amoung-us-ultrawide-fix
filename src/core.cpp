#include "wolfuw/core.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>
#include <stdexcept>

namespace wolfuw {
namespace {

std::string_view Trim(std::string_view value) {
    constexpr std::string_view whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}

unsigned ParseDimension(std::string_view value) {
    unsigned parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        parsed == 0 || parsed > 32768) {
        throw std::invalid_argument("Display dimensions must be whole numbers from 1 to 32768");
    }
    return parsed;
}

int HexDigit(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

} // namespace

Settings ParseSettings(std::istream& input) {
    Settings settings;
    std::string section;
    std::set<std::string> seen;
    std::string line;
    bool first_line = true;
    while (std::getline(input, line)) {
        if (first_line && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        first_line = false;
        auto text = Trim(line);
        if (text.empty() || text.front() == ';' || text.front() == '#') continue;
        if (text.front() == '[') {
            if (text.size() < 3 || text.back() != ']') throw std::invalid_argument("Invalid INI section");
            section = std::string(Trim(text.substr(1, text.size() - 2)));
            if (section != "Runtime" && section != "Display") throw std::invalid_argument("Unknown INI section");
            continue;
        }
        const auto equals = text.find('=');
        if (equals == std::string_view::npos) throw std::invalid_argument("Expected INI key=value");
        const auto key = Trim(text.substr(0, equals));
        const auto value = Trim(text.substr(equals + 1));
        const std::string full_key = section + "." + std::string(key);
        if (!seen.insert(full_key).second) throw std::invalid_argument("Duplicate INI key");
        if (full_key == "Runtime.Mode") {
            if (value != "Observe") throw std::invalid_argument("Only Mode=Observe is implemented; no verified game profile exists");
        } else if (full_key == "Display.Width") {
            settings.width = ParseDimension(value);
        } else if (full_key == "Display.Height") {
            settings.height = ParseDimension(value);
        } else {
            throw std::invalid_argument("Unknown INI key");
        }
    }
    if (input.bad()) throw std::runtime_error("Failed while reading INI file");
    if (seen.size() != 3) throw std::invalid_argument("INI requires Runtime.Mode, Display.Width and Display.Height");
    return settings;
}

double AspectRatio(const Settings& settings) {
    if (settings.width == 0 || settings.height == 0 || settings.width > 32768 || settings.height > 32768) {
        throw std::invalid_argument("Invalid display dimensions");
    }
    return static_cast<double>(settings.width) / settings.height;
}

Rect CenteredReferenceRect(const Settings& settings, double reference_aspect) {
    const double aspect = AspectRatio(settings);
    if (!std::isfinite(reference_aspect) || reference_aspect <= 0) throw std::invalid_argument("Invalid reference aspect");
    const double width = aspect >= reference_aspect ? settings.height * reference_aspect : settings.width;
    const double height = aspect >= reference_aspect ? settings.height : settings.width / reference_aspect;
    return {(settings.width - width) / 2.0, (settings.height - height) / 2.0, width, height};
}

double HorizontalFovDegrees(double vertical_fov_degrees, double aspect) {
    if (!std::isfinite(vertical_fov_degrees) || vertical_fov_degrees <= 0 || vertical_fov_degrees >= 180 ||
        !std::isfinite(aspect) || aspect <= 0) throw std::invalid_argument("Invalid FOV or aspect");
    constexpr double pi = 3.14159265358979323846;
    return 2.0 * std::atan(std::tan(vertical_fov_degrees * pi / 360.0) * aspect) * 180.0 / pi;
}

Pattern ParsePattern(std::string_view text) {
    Pattern pattern;
    while (!(text = Trim(text)).empty()) {
        const auto end = text.find_first_of(" \t\r\n");
        const auto token = text.substr(0, end);
        if (token == "??") {
            pattern.push_back({0, true});
        } else {
            if (token.size() != 2 || HexDigit(token[0]) < 0 || HexDigit(token[1]) < 0) {
                throw std::invalid_argument("Pattern tokens must be two hex digits or ??");
            }
            pattern.push_back({static_cast<std::uint8_t>(HexDigit(token[0]) * 16 + HexDigit(token[1])), false});
        }
        if (end == std::string_view::npos) break;
        text.remove_prefix(end);
    }
    if (pattern.empty() || std::all_of(pattern.begin(), pattern.end(), [](PatternByte b) { return b.wildcard; })) {
        throw std::invalid_argument("Pattern must contain at least one exact byte");
    }
    return pattern;
}

MatchResult FindUnique(const std::uint8_t* data, std::size_t size, const Pattern& pattern) {
    if ((!data && size != 0) || pattern.empty() ||
        std::all_of(pattern.begin(), pattern.end(), [](PatternByte b) { return b.wildcard; })) {
        throw std::invalid_argument("Invalid scan input");
    }
    MatchResult result{MatchState::missing, 0};
    if (size < pattern.size()) return result;
    const auto last = size - pattern.size();
    for (std::size_t offset = 0;; ++offset) {
        bool matches = true;
        for (std::size_t index = 0; index < pattern.size(); ++index) {
            if (!pattern[index].wildcard && data[offset + index] != pattern[index].value) {
                matches = false;
                break;
            }
        }
        if (matches) {
            if (result.state == MatchState::unique) return {MatchState::ambiguous, 0};
            result = {MatchState::unique, offset};
        }
        if (offset == last) break;
    }
    return result;
}

} // namespace wolfuw
