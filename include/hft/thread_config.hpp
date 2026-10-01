#pragma once

#if defined(_WIN32)
#include <windows.h>
#include <thread>

namespace hft {

inline void configure_current_thread(int core_id) {
    // Pin thread to a specific physical/logical core
    DWORD_PTR mask = (1ULL << core_id);
    SetThreadAffinityMask(GetCurrentThread(), mask);

    // Elevate thread priority to bypass OS scheduling jitter
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

inline void maximize_process_priority() {
    // Elevate the entire simulation process priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

}  // namespace hft

#else
// POSIX / Linux fallback stub
namespace hft {
inline void configure_current_thread(int) {}
inline void maximize_process_priority() {}
}  // namespace hft
#endif