#include "wolfuw/core.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int passed = 0;
int failed = 0;

void Check(bool condition, const char* name) {
    if (condition) {
        ++passed;
    } else {
        ++failed;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void Rejects(const std::function<void()>& function, const char* name) {
    try {
        function();
        Check(false, name);
    } catch (const std::invalid_argument&) {
        Check(true, name);
    }
}

bool Near(double a, double b) { return std::abs(a - b) < 0.000001; }

wolfuw::Settings Config(const std::string& text) {
    std::istringstream stream(text);
    return wolfuw::ParseSettings(stream);
}
} // namespace

int main() {
    using namespace wolfuw;
    const auto config = Config("; comment\n[Runtime]\nMode=Observe\n[Display]\nWidth=3440\nHeight=1440\n");
    Check(config.width == 3440 && config.height == 1440, "3440x1440 config");
    Check(Near(AspectRatio(config), 43.0 / 18.0), "actual aspect is 43:18");
    const auto reference = CenteredReferenceRect(config, 16.0 / 9.0);
    Check(Near(reference.width, 2560) && Near(reference.height, 1440), "reference rectangle dimensions");
    Check(Near(reference.x, 440) && Near(reference.y, 0), "reference rectangle centered");
    const auto narrow = CenteredReferenceRect({1280, 1024}, 16.0 / 9.0);
    Check(Near(narrow.x, 0) && Near(narrow.y, 152) && Near(narrow.height, 720), "narrow display reference rectangle");
    Check(Near(HorizontalFovDegrees(90, 1), 90), "square aspect FOV");
    const double fov = HorizontalFovDegrees(60, 43.0 / 18.0);
    const double vertical = 2 * std::atan(std::tan(fov * std::acos(-1.0) / 360) / (43.0 / 18.0)) * 180 / std::acos(-1.0);
    Check(Near(vertical, 60), "FOV round trip preserves vertical angle");
    Check(HorizontalFovDegrees(60, 43.0 / 18.0) > HorizontalFovDegrees(60, 16.0 / 9.0), "fixed vertical FOV widens horizontal FOV");
    Rejects([] { AspectRatio({3440, 0}); }, "reject zero height");
    Rejects([] { AspectRatio({0, 1440}); }, "reject zero width");
    Rejects([] { AspectRatio({65536, 1440}); }, "reject excessive dimension");
    Rejects([] { CenteredReferenceRect({}, 0); }, "reject zero reference aspect");
    Rejects([] { CenteredReferenceRect({}, std::numeric_limits<double>::quiet_NaN()); }, "reject NaN reference aspect");
    Rejects([] { HorizontalFovDegrees(180, 2); }, "reject 180 degree vertical FOV");
    Rejects([] { HorizontalFovDegrees(-1, 2); }, "reject negative FOV");
    Rejects([] { HorizontalFovDegrees(60, std::numeric_limits<double>::infinity()); }, "reject infinite aspect");

    const auto bom = Config("\xEF\xBB\xBF[Runtime]\r\nMode = Observe\r\n[Display]\r\nWidth = 3440\r\nHeight = 1440\r\n");
    Check(bom.width == 3440, "UTF-8 BOM and Windows line endings");
    const std::string prefix = "[Runtime]\nMode=Observe\n[Display]\n";
    Rejects([&] { Config(prefix + "Width=3440\nHeight=0\n"); }, "reject zero config dimension");
    Rejects([&] { Config(prefix + "Width=3440oops\nHeight=1440\n"); }, "reject numeric suffix");
    Rejects([&] { Config(prefix + "Width=-3440\nHeight=1440\n"); }, "reject negative config dimension");
    Rejects([&] { Config(prefix + "Width=3440\nWidth=1920\nHeight=1440\n"); }, "reject duplicate config key");
    Rejects([&] { Config(prefix + "Width=3440\nHeight=1440\nHeigth=1440\n"); }, "reject typo key");
    Rejects([&] { Config(prefix + "Width=3440\n"); }, "reject missing key");
    Rejects([] { Config("[Runtime]\nMode=Apply\n[Display]\nWidth=3440\nHeight=1440\n"); }, "refuse Apply mode");
    Rejects([] { Config("[Unknown]\nMode=Observe\n"); }, "reject unknown section");
    Rejects([] { Config("[Runtime\nMode=Observe\n"); }, "reject malformed section");
    Rejects([] { Config("[Runtime]\nMode Observe\n"); }, "reject malformed key");
    Rejects([] { Config(""); }, "reject empty config");

    // All bytes in this test are synthetic fixtures, never game signatures.
    const std::vector<std::uint8_t> bytes{0x00, 0xDE, 0xAD, 0x37, 0xEF};
    const auto pattern = ParsePattern(" de AD ?? ef ");
    const auto match = FindUnique(bytes.data(), bytes.size(), pattern);
    Check(match.state == MatchState::unique && match.offset == 1, "wildcard unique match at last valid start");
    Check(FindUnique(bytes.data(), bytes.size(), ParsePattern("DE AD 37 EF")).state == MatchState::unique, "exact match");
    Check(FindUnique(bytes.data(), bytes.size(), ParsePattern("DE AD 38 EF")).state == MatchState::missing, "no match");
    Check(FindUnique(bytes.data(), 2, pattern).state == MatchState::missing, "pattern longer than buffer");
    Check(FindUnique(nullptr, 0, pattern).state == MatchState::missing, "empty buffer is safe");
    const std::uint8_t repeated[]{0xAA, 0xAA, 0xAA};
    Check(FindUnique(repeated, 3, ParsePattern("AA AA")).state == MatchState::ambiguous, "overlapping matches rejected as ambiguous");
    Check(FindUnique(bytes.data(), bytes.size(), ParsePattern("00")).offset == 0, "match at offset zero");
    Rejects([] { ParsePattern(""); }, "reject empty pattern");
    Rejects([] { ParsePattern("?? ??"); }, "reject wildcard-only pattern");
    Rejects([] { ParsePattern("DE ? EF"); }, "reject single question mark");
    Rejects([] { ParsePattern("DE GG EF"); }, "reject invalid hex");
    Rejects([] { ParsePattern("0xDE"); }, "reject 0x syntax");
    Rejects([&] { FindUnique(nullptr, 5, pattern); }, "reject null nonempty buffer");
    Rejects([&] { FindUnique(bytes.data(), bytes.size(), {}); }, "reject empty parsed pattern");
    Rejects([&] { FindUnique(bytes.data(), bytes.size(), {{0, true}}); }, "reject constructed wildcard-only pattern");

    std::cout << passed << " checks passed; " << failed << " failed. Synthetic tests only; no game tested.\n";
    return failed == 0 ? 0 : 1;
}
