#include "runtime.hpp"

namespace wolfuw {
HMODULE dll_module = nullptr;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) wolfuw::dll_module = instance;
    // No I/O, threads, hooks, scanning, initialization or cleanup under loader lock.
    return TRUE;
}
