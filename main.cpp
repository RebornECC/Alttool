#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Buildable starting point only: actual editor UI and object placement
// are intentionally not claimed to exist yet.
$on_mod(Loaded) {
    log::info("Auto Builder prototype loaded.");
}
