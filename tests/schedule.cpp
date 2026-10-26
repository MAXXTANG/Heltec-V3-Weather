#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../src/network_schedule.h"

static void check(int64_t now, bool success, int64_t expected) {
    const int64_t due = nextNetworkMinute(now, success);
    const int64_t interval = success ? 900 : 300;
    assert(due == expected);
    assert(due % 60 == 0);
    assert(due > now);
    assert(due - now <= interval);
    assert(due - now > interval - 60);
}

int main() {
    const int seconds[] = {0, 3, 59};
    for (int second : seconds) {
        // Before NTP has supplied a real date, RTC time starts near epoch zero.
        check(second, true, 900);
        check(second, false, 300);
        check(60 + second, true, 960);
        check(60 + second, false, 360);

        // Normal daytime requests end at :00, :03, or :59 in the same minute.
        constexpr int64_t day = INT64_C(20600) * 86400;
        constexpr int64_t daytime = day + 14 * 3600 + 6 * 60;
        check(daytime + second, true, day + 14 * 3600 + 21 * 60);
        check(daytime + second, false, day + 14 * 3600 + 11 * 60);

        // 23:59 rolls over to 00:14 / 00:04 on the following day.
        check(day + 23 * 3600 + 59 * 60 + second, true, day + 86400 + 14 * 60);
        check(day + 23 * 3600 + 59 * 60 + second, false, day + 86400 + 4 * 60);

        // Keep arithmetic wide enough for timestamps beyond signed 32-bit time.
        constexpr int64_t later = INT64_C(3000000000);
        check(later + second, true, later + 900);
        check(later + second, false, later + 300);
    }
    std::puts("network schedule: 30 boundary cases passed");
    constexpr int64_t now = INT64_C(9000000000);
    assert(nextIdleWakeUs(now, 60000000, 0) == now + 60000000);
    assert(nextIdleWakeUs(now, 10000000, now + 5000000) == now + 5000000);
    assert(nextIdleWakeUs(now, 10000000, now + 20000000) == now + 10000000);
    assert(nextIdleWakeUs(now, 10000000, now + 10000000) == now + 10000000);
    assert(nextIdleWakeUs(now, 10000000, now) == now + 10000000);
    assert(nextIdleWakeUs(now, 10000000, now - 1) == now + 10000000);
    assert(nextIdleWakeUs(now, 1, now + 5000000) == now + 1);
    std::puts("idle schedule: minute/battery deadlines passed; no page timer");
}
