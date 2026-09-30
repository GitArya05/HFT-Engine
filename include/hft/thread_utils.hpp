#pragma once

#include <cstdint>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace hft {

// Pins a given thread to a specific CPU core hardware ID
inline bool pin_thread_to_core(std::thread& t, int core_id) {
#ifdef _WIN32
    HANDLE native_handle = t.native_handle();
    DWORD_PTR mask = 1ULL << core_id;
    return SetThreadAffinityMask(native_handle, mask) != 0;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}

// Pins the currently executing thread to a specific CPU core
inline bool pin_current_thread_to_core(int core_id) {
#ifdef _WIN32
    DWORD_PTR mask = 1ULL << core_id;
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}

}  // namespace hft