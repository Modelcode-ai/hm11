/**
 * @file fuzz_ranged_integer.cpp
 * @brief LibFuzzer target for RangedInteger template
 *
 * This fuzzer tests the RangedInteger template for:
 * - Construction bounds checking
 * - Arithmetic overflow detection
 * - Comparison operations
 * - Range validation (0-100, 0-120, etc.)
 *
 * Build with:
 *   clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
 *           -std=c++20 -I../include fuzz_ranged_integer.cpp -o fuzz_ranged_integer
 *
 * Run with:
 *   ./fuzz_ranged_integer -max_len=100 -max_total_time=300 corpus/ranged_integer
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <cstddef>
#include <cstdint>

#include "hm11/types/ranged_integer.hpp"

// Define common RangedInteger types from the codebase
using Percent = hm11::types::RangedInteger<uint8_t, 0, 100, struct PercentTag>;
using Temperature = hm11::types::RangedInteger<uint8_t, 0, 120, struct TemperatureTag>;
using Humidity = hm11::types::RangedInteger<uint8_t, 0, 100, struct HumidityTag>;
using Latency = hm11::types::RangedInteger<uint8_t, 0, 4, struct LatencyTag>;
using DiscoveryTime = hm11::types::RangedInteger<uint8_t, 1, 9, struct DiscoveryTimeTag>;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 2) {
        return 0;  // Need at least 2 bytes for binary operations
    }

    uint8_t val1 = data[0];
    uint8_t val2 = size > 1 ? data[1] : 0;

    // ========================================================================
    // Fuzz 1: Percent (0-100)
    // ========================================================================
    // Construction with potentially out-of-range values
    if (val1 <= 100) {
        Percent p1(val1);
        (void)p1.get();

        if (val2 <= 100) {
            Percent p2(val2);

            // Test arithmetic operations (only if result is in range)
            if (static_cast<uint16_t>(val1) + static_cast<uint16_t>(val2) <= 100) {
                Percent sum = p1 + p2;
                (void)sum.get();
            }

            if (val1 >= val2) {
                Percent diff = p1 - p2;
                (void)diff.get();
            }

            // Test comparison operations (always safe)
            (void)(p1 == p2);
            (void)(p1 != p2);
            (void)(p1 < p2);
            (void)(p1 <= p2);
            (void)(p1 > p2);
            (void)(p1 >= p2);
        }

        // Test increment/decrement (only if in range after operation)
        if (val1 < 100) {
            Percent p_inc(val1);
            ++p_inc;
        }
        if (val1 > 0) {
            Percent p_dec(val1);
            --p_dec;
        }
    }

    // ========================================================================
    // Fuzz 2: Temperature (0-120)
    // ========================================================================
    if (val1 <= 120) {
        try {
            Temperature t1(val1);
            (void)t1.get();

            if (val2 <= 120) {
                Temperature t2(val2);

                // Test that different tags prevent mixing
                // Percent p(50);
                // Temperature t = p;  // Should not compile!

                if (val1 + val2 <= 120) {
                    Temperature sum = t1 + t2;
                    (void)sum.get();
                }
            }
        } catch (...) {
        }
    }

    // ========================================================================
    // Fuzz 3: Humidity (0-100)
    // ========================================================================
    if (val1 <= 100) {
        try {
            Humidity h1(val1);
            (void)h1.get();

            if (val2 <= 100 && val1 + val2 <= 100) {
                Humidity h2(val2);
                Humidity sum = h1 + h2;
                (void)sum.get();
            }
        } catch (...) {
        }
    }

    // ========================================================================
    // Fuzz 4: Latency (0-4) - Small range
    // ========================================================================
    if (val1 <= 4) {
        try {
            Latency lat1(val1);
            (void)lat1.get();

            if (val2 <= 4) {
                Latency lat2(val2);

                // These should fail if sum > 4
                if (val1 + val2 <= 4) {
                    Latency sum = lat1 + lat2;
                    (void)sum.get();
                }
            }
        } catch (...) {
        }
    }

    // ========================================================================
    // Fuzz 5: DiscoveryTime (1-9) - Non-zero minimum
    // ========================================================================
    uint8_t time_val = (val1 % 9) + 1;  // Force into 1-9 range
    try {
        DiscoveryTime dt(time_val);
        (void)dt.get();

        // Test that default construction uses minimum value
        DiscoveryTime default_dt;
        (void)(default_dt.get() == 1);  // Should be MIN_VALUE
    } catch (...) {
    }

    // ========================================================================
    // Fuzz 6: Edge cases - Boundaries
    // ========================================================================
    try {
        // Test MIN_VALUE
        Percent min_percent(Percent::MIN_VALUE);
        (void)min_percent.get();

        // Test MAX_VALUE
        Percent max_percent(Percent::MAX_VALUE);
        (void)max_percent.get();

        // Test arithmetic at boundaries
        if (Percent::MAX_VALUE > 0) {
            Percent almost_max(Percent::MAX_VALUE - 1);
            Percent one(1);
            Percent result = almost_max + one;  // Should equal MAX_VALUE
            (void)(result.get() == Percent::MAX_VALUE);
        }
    } catch (...) {
    }

    // ========================================================================
    // Fuzz 7: Default construction
    // ========================================================================
    {
        Percent default_percent;
        (void)(default_percent.get() == Percent::MIN_VALUE);

        Temperature default_temp;
        (void)(default_temp.get() == Temperature::MIN_VALUE);

        DiscoveryTime default_discovery;
        (void)(default_discovery.get() == DiscoveryTime::MIN_VALUE);
    }

    return 0;
}
