#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

namespace wolfuw {

struct Settings {
    unsigned width = 3440;
    unsigned height = 1440;
};

// Strict, observe-only INI parser. Throws on unknown, duplicate or missing keys.
Settings ParseSettings(std::istream& input);
double AspectRatio(const Settings& settings);

struct Rect {
    double x;
    double y;
    double width;
    double height;
};

// Design helpers only: no discovered game FOV convention or HUD behavior implied.
Rect CenteredReferenceRect(const Settings& settings, double reference_aspect);
double HorizontalFovDegrees(double vertical_fov_degrees, double aspect);

struct PatternByte {
    std::uint8_t value;
    bool wildcard;
};
using Pattern = std::vector<PatternByte>;
Pattern ParsePattern(std::string_view text);

enum class MatchState { missing, unique, ambiguous };
struct MatchResult {
    MatchState state;
    std::size_t offset; // Meaningful only when state == unique; buffer-relative.
};

// Scans a caller-owned readable buffer. Does not inspect processes or write memory.
// Empty/all-wildcard patterns and null non-empty buffers are rejected.
MatchResult FindUnique(const std::uint8_t* data, std::size_t size, const Pattern& pattern);

} // namespace wolfuw
