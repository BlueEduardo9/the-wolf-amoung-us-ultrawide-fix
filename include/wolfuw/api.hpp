#pragma once

#include <cstdint>

#define WOLF_UW_CALL __cdecl

namespace wolfuw {
inline constexpr std::uint32_t api_version = 1;
enum class StartupResult : std::int32_t {
    observe_only_no_verified_profile = 1,
    invalid_config = -1,
    io_failure = -2,
    internal_failure = -3
};
using ApiVersionFn = std::uint32_t (WOLF_UW_CALL*)();
using InitializeFn = std::int32_t (WOLF_UW_CALL*)(const wchar_t*, const wchar_t*);
using ShutdownFn = void (WOLF_UW_CALL*)();
} // namespace wolfuw

// C ABI, exported with stable names by exports.def. Paths must be absolute if set.
// Call only after LoadLibrary has returned, outside DllMain/the loader lock.
// Null paths select WolfUltrawide.ini and WolfUltrawide.log beside this DLL.
// Explicit log names must end in .log. Serialize calls; do not unload while active.
extern "C" std::uint32_t WOLF_UW_CALL WolfUW_ApiVersion() noexcept;
extern "C" std::int32_t WOLF_UW_CALL WolfUW_Initialize(const wchar_t* config_path, const wchar_t* log_path) noexcept;
extern "C" void WOLF_UW_CALL WolfUW_Shutdown() noexcept;
