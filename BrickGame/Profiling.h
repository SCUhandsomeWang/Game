#pragma once

// Lightweight profiling shim. Define TRACY_ENABLE to enable Tracy integration.
#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
// No-op fallbacks so code compiles without Tracy present.
#define ZoneScoped
#define FrameMark
#define ZoneNamed(name, active)
#define TracyAlloc(ptr, size)
#define TracyFree(ptr)
#endif
