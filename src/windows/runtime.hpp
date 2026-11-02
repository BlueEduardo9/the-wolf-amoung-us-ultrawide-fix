#pragma once

#include "wolfuw/core.hpp"
#include <windows.h>
#include <cstdint>
#include <string_view>

namespace wolfuw {
extern HMODULE dll_module;

struct HostIdentity {
    std::uint16_t machine;
    std::uint32_t pe_timestamp;
    std::uint32_t image_size;
};

struct IntegrationDecision {
    bool verified_profile_available;
    std::string_view explanation;
};

IntegrationDecision EvaluateGameIntegration(const HostIdentity& identity, const Settings& settings) noexcept;
} // namespace wolfuw
