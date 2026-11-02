#include "runtime.hpp"

namespace wolfuw {

IntegrationDecision EvaluateGameIntegration(const HostIdentity& identity, const Settings& settings) noexcept {
    static_cast<void>(identity);
    static_cast<void>(settings);

    // Verified game profiles: EMPTY.
    // No game memory addresses, offsets, signatures or expected instruction bytes
    // have been discovered or tested. In particular, PE metadata is not a hash.
    //
    // Extension point after local research:
    // 1. Identify the exact EXE with SHA-256, architecture and Steam build record.
    // 2. Validate bounded section signatures, unique matches and expected bytes.
    // 3. Establish projection/FOV/HUD semantics and a safe patch lifetime.
    // 4. Implement guarded writes/hooks and restoration only after those checks.
    // 5. Test the actual game, then document only the observed compatibility.
    //
    // FindUnique is available in core.hpp for synthetic/bounded readable buffers.
    // The current DLL does not call it against the game and has no memory writer.
    return {false, "No verified Steam executable profile; no hooks or memory writes installed"};
}

} // namespace wolfuw
