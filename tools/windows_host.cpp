#include "wolfuw/api.hpp"
#include <windows.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string Utf8(const std::wstring& text) {
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) throw std::runtime_error("Cannot encode output path as UTF-8");
    std::vector<char> buffer(static_cast<std::size_t>(size));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.c_str(), -1, buffer.data(), size, nullptr, nullptr) != size) {
        throw std::runtime_error("Cannot encode output path as UTF-8");
    }
    return std::string(buffer.data(), static_cast<std::size_t>(size - 1));
}

template<class Function>
Function LoadFunction(HMODULE module, const char* name) {
    const FARPROC address = GetProcAddress(module, name);
    if (!address) throw std::runtime_error("Required DLL export is missing");
    static_assert(sizeof(Function) == sizeof(address));
    Function function{};
    std::memcpy(&function, &address, sizeof(function));
    return function;
}

struct Module {
    HMODULE value;
    ~Module() { if (value) FreeLibrary(value); }
};

std::filesystem::path ExeDirectory() {
    std::vector<wchar_t> buffer(32768);
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) throw std::runtime_error("Cannot determine host location");
    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    try {
        auto dll = ExeDirectory() / L"WolfUltrawide.dll";
        std::filesystem::path config;
        std::filesystem::path log;
        for (int index = 1; index < argc; ++index) {
            const std::wstring option = argv[index];
            if (option == L"--help") {
                std::cout << "WolfUWHost [--dll PATH] [--config PATH] [--log PATH]\n"
                    "Runs the observe-only DLL inside this test host. Does not launch or inject into the game.\n";
                return 0;
            }
            if ((option != L"--dll" && option != L"--config" && option != L"--log") || index + 1 >= argc) {
                throw std::invalid_argument("Unknown option or missing path; use --help");
            }
            const auto value = std::filesystem::absolute(argv[++index]);
            if (option == L"--dll") dll = value;
            if (option == L"--config") config = value;
            if (option == L"--log") log = value;
        }
        if (config.empty()) config = dll.parent_path() / L"WolfUltrawide.ini";
        if (log.empty()) log = dll.parent_path() / L"WolfUltrawide.log";
        // Restrict dependent-DLL lookup. All our paths are absolute.
        Module module{LoadLibraryExW(dll.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)};
        if (!module.value) {
            std::cerr << "LoadLibraryExW failed; Windows error " << GetLastError() << "\n";
            return 2;
        }
        const auto version = LoadFunction<wolfuw::ApiVersionFn>(module.value, "WolfUW_ApiVersion");
        const auto initialize = LoadFunction<wolfuw::InitializeFn>(module.value, "WolfUW_Initialize");
        const auto shutdown = LoadFunction<wolfuw::ShutdownFn>(module.value, "WolfUW_Shutdown");
        if (version() != wolfuw::api_version) throw std::runtime_error("Unsupported DLL API version");
        const auto result = initialize(config.c_str(), log.c_str());
        shutdown();
        std::cout << "Startup result: " << result << "\nLog: " << Utf8(log.wstring()) << "\n";
        if (result != static_cast<std::int32_t>(wolfuw::StartupResult::observe_only_no_verified_profile)) return 3;
        std::cout << "Diagnostic session completed in WolfUWHost. No verified game profile; no game fix applied.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
