Measurement Machine

CPU: Intel(R) Xeon(R) 6985P-C (x86_64, 2 cores / 2 threads)

Caches:

L1d cache: 48 KiB

L1i cache: 64 KiB

L2 cache: 2 MiB

L3 cache: 504 MiB

OS: Linux 6.12.105+ x86_64 (Debian 12)

Compiler: g++ (Debian 12.2.0-14+deb12u1) 12.2.0

Compile flags: -std=c++17 -O3 -Wall -Wextra -Wpedantic

Block size setting: Default automatic b = ceil(sqrt(n)) (b = 317 for n = 100,000; b = 1,000 for n = 1,000,000)

Timing method: std::chrono::steady_clock around the operations loop only (build() and reset_counters() run before the timer starts; correctness validation runs before the repetition loop). Minimum 5 repetitions per width.
