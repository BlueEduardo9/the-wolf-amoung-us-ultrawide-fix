#include "runtime.hpp"
#include "wolfuw/api.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::filesystem::path ModulePath(HMODULE module) {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) throw std::runtime_error("Cannot determine module path");
    return std::filesystem::path(std::wstring(buffer.data(), length));
}

std::string Utf8(const std::wstring& text) {
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) throw std::runtime_error("Cannot encode module path as UTF-8");
    std::vector<char> buffer(static_cast<std::size_t>(size));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, buffer.data(), size, nullptr, nullptr) != size) {
        throw std::runtime_error("Cannot encode module path as UTF-8");
    }
    return std::string(buffer.data(), static_cast<std::size_t>(size - 1));
}

std::filesystem::path SelectPath(const wchar_t* supplied, const std::filesystem::path& fallback) {
    const std::filesystem::path path = supplied ? std::filesystem::path(supplied) : fallback;
    if (path.empty() || !path.is_absolute()) throw std::invalid_argument("Configuration and log paths must be absolute");
    return path.lexically_normal();
}

wolfuw::HostIdentity ReadHostIdentity() {
    // PE header fields from the currently loaded host. These are standard format
    // fields obtained via the Windows SDK, not invented game patch locations.
    const auto* base = reinterpret_cast<const std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!base) throw std::runtime_error("Cannot locate host module");
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 1024 * 1024) {
        throw std::runtime_error("Invalid host DOS header");
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        throw std::runtime_error("Expected a PE32 host");
    }
    return {nt->FileHeader.Machine, nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage};
}

void WriteTime(std::ostream& log) {
    SYSTEMTIME now{};
    GetSystemTime(&now);
    log << std::setfill('0') << std::setw(4) << now.wYear << '-' << std::setw(2) << now.wMonth
        << '-' << std::setw(2) << now.wDay << 'T' << std::setw(2) << now.wHour << ':'
        << std::setw(2) << now.wMinute << ':' << std::setw(2) << now.wSecond << "Z\n";
}

} // namespace

extern "C" std::uint32_t WOLF_UW_CALL WolfUW_ApiVersion() noexcept {
    return wolfuw::api_version;
}

extern "C" std::int32_t WOLF_UW_CALL WolfUW_Initialize(const wchar_t* config_path, const wchar_t* log_path) noexcept {
    using wolfuw::StartupResult;
    try {
        const auto directory = ModulePath(wolfuw::dll_module).parent_path();
        const auto ini = SelectPath(config_path, directory / L"WolfUltrawide.ini");
        const auto destination = SelectPath(log_path, directory / L"WolfUltrawide.log");
        if (CompareStringOrdinal(destination.extension().c_str(), -1, L".log", -1, TRUE) != CSTR_EQUAL) {
            return static_cast<std::int32_t>(StartupResult::invalid_config);
        }
        // Prevent a path typo from overwriting or appending diagnostics to config.
        if (ini == destination || (std::filesystem::exists(ini) && std::filesystem::exists(destination) && std::filesystem::equivalent(ini, destination))) {
            return static_cast<std::int32_t>(StartupResult::invalid_config);
        }
        // Log parents must exist. Never create directories or alter game files.
        std::ofstream log(destination, std::ios::app | std::ios::binary);
        if (!log) return static_cast<std::int32_t>(StartupResult::io_failure);
        log << "WolfUltrawideStarter 0.1.0 API=1 OBSERVE_ONLY\n";
        WriteTime(log);
        std::ifstream input(ini, std::ios::binary);
        if (!input) {
            log << "ERROR=config_open_failed\n";
            return static_cast<std::int32_t>(StartupResult::io_failure);
        }
        wolfuw::Settings settings;
        try {
            settings = wolfuw::ParseSettings(input);
        } catch (const std::exception& error) {
            log << "ERROR=invalid_config " << error.what() << '\n';
            return static_cast<std::int32_t>(StartupResult::invalid_config);
        }
        const auto host = ReadHostIdentity();
        const auto decision = wolfuw::EvaluateGameIntegration(host, settings);
        const auto reference = wolfuw::CenteredReferenceRect(settings, 16.0 / 9.0);
        log << "HOST_EXE=" << Utf8(ModulePath(nullptr).wstring()) << '\n'
            << "HOST_MACHINE=0x" << std::hex << host.machine << std::dec << '\n'
            << "HOST_PE_TIMESTAMP=" << host.pe_timestamp << '\n'
            << "HOST_IMAGE_SIZE=" << host.image_size << '\n'
            << "HOST_IDENTITY_NOTE=PE metadata is diagnostic only; no executable SHA-256 verified\n"
            << "TARGET_WIDTH=" << settings.width << '\n'
            << "TARGET_HEIGHT=" << settings.height << '\n'
            << std::fixed << std::setprecision(9)
            << "TARGET_ASPECT=" << wolfuw::AspectRatio(settings) << '\n'
            << "REFERENCE_16_9_RECT=" << reference.x << ',' << reference.y << ','
            << reference.width << ',' << reference.height << '\n'
            << "REFERENCE_NOTE=geometry calculation only; no game HUD change\n"
            << "PATCH_STATUS=no_verified_profile\n"
            << "DETAIL=" << decision.explanation << '\n'
            << "MEMORY_WRITES=0\nHOOKS_INSTALLED=0\n";
        // Even a future profile must not implicitly turn Observe into patch mode.
        log.flush();
        if (!log) return static_cast<std::int32_t>(StartupResult::io_failure);
        return static_cast<std::int32_t>(StartupResult::observe_only_no_verified_profile);
    } catch (const std::invalid_argument&) {
        return static_cast<std::int32_t>(StartupResult::invalid_config);
    } catch (const std::filesystem::filesystem_error&) {
        return static_cast<std::int32_t>(StartupResult::io_failure);
    } catch (...) {
        return static_cast<std::int32_t>(StartupResult::internal_failure);
    }
}

extern "C" void WOLF_UW_CALL WolfUW_Shutdown() noexcept {
    // No persistent resources or patches. Initialization closes the log before returning.
}
