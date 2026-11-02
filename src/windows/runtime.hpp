#pragma once

#include "wolfuw/core.hpp"
#include <windows.h>
#include <cstdint>
#include <string_view>

namespace wolfuw {
extern HMODULE dll_module;

#if defined(_M_X64) || defined(__x86_64__)
inline constexpr auto runtime_machine = IMAGE_FILE_MACHINE_AMD64;
inline constexpr auto runtime_header_magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
inline constexpr const char* runtime_architecture = "x64";
#elif defined(_M_IX86) || defined(__i386__)
inline constexpr auto runtime_machine = IMAGE_FILE_MACHINE_I386;
inline constexpr auto runtime_header_magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
inline constexpr const char* runtime_architecture = "x86";
#else
#error The runtime supports Windows x86 and x64 only.
#endif

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
