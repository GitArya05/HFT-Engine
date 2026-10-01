#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

namespace hft {

class LatencyTracker {
public:
    explicit LatencyTracker(size_t capacity) {
        samples_.reserve(capacity);
    }

    // Record a latency sample in nanoseconds
    inline void record(uint64_t latency_ns) {
        samples_.push_back(latency_ns);
    }

    void print_report() const {
        if (samples_.empty()) {
            std::cout << "[LATENCY] No samples recorded.\n";
            return;
        }

        // Create a mutable copy for sorting
        std::vector<uint64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        size_t total = sorted.size();
        uint64_t min_val = sorted.front();
        uint64_t max_val = sorted.back();

        uint64_t p50 = sorted[static_cast<size_t>(total * 0.50)];
        uint64_t p90 = sorted[static_cast<size_t>(total * 0.90)];
        uint64_t p99 = sorted[static_cast<size_t>(total * 0.99)];
        uint64_t p999 = sorted[static_cast<size_t>(total * 0.999)];

        // Compute average
        uint64_t sum = 0;
        for (auto val : sorted) sum += val;
        double mean = static_cast<double>(sum) / total;

        std::cout << "========================================================\n";
        std::cout << "             TAIL LATENCY PERCENTILES (NS)\n";
        std::cout << "========================================================\n";
        std::cout << " Sample Count: " << total << "\n";
        std::cout << " Min Latency:  " << min_val << " ns\n";
        std::cout << " P50 (Median): " << p50 << " ns\n";
        std::cout << " P90 Latency:  " << p90 << " ns\n";
        std::cout << " P99 Latency:  " << p99 << " ns\n";
        std::cout << " P99.9 Latency:" << p999 << " ns\n";
        std::cout << " Max Latency:  " << max_val << " ns\n";
        std::cout << " Mean Latency: " << mean << " ns\n";
        std::cout << "========================================================\n";
    }

private:
    std::vector<uint64_t> samples_;
};

}  // namespace hft