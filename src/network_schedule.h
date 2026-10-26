#pragma once
#include <cstdint>

// The device wakes on minute boundaries. Align the deadline so a request that
// finishes a few seconds after the minute does not add an extra minute of sleep.
constexpr int64_t nextNetworkMinute(int64_t now, bool success) {
    return (now / 60 + (success ? 15 : 5)) * 60;
}

// OLED pages stay selected; only the clock and battery deadlines wake the timer.
// A zero battery deadline means the ADC is unavailable.
constexpr int64_t nextIdleWakeUs(int64_t now, int64_t toMinute, int64_t batteryDue) {
    return batteryDue > now && batteryDue < now + toMinute ? batteryDue : now + toMinute;
}
