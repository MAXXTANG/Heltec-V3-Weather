#pragma once
#include <cstdint>

// All intervals use the monotonic microsecond clock, never NTP/wall time.
struct PrgGesture {
    enum Action : uint8_t { None=0, TogglePage=1, StartTimer=2, OpenSetup=4 };
    static constexpr int64_t DebounceUs=30000;
    static constexpr int64_t ClickGapUs=400000;
    static constexpr int64_t LongPressUs=3000000;
    bool down=false, longHandled=false;
    uint8_t clicks=0;
    int64_t pressedAt=0, settleAt=0;

    void clearClicks(){clicks=0;settleAt=0;}
    uint8_t settle(){
        // Two ordinary clicks return to the original page. Delay presentation
        // until the sequence is known so a triple click never flashes pages.
        const uint8_t action=(clicks&1)?TogglePage:None;
        clearClicks();return action;
    }
    uint8_t update(int64_t now,bool pressed){
        uint8_t action=None;
        if(!down && clicks && now>=settleAt)action=settle();
        if(pressed && !down){down=true;longHandled=false;pressedAt=now;}
        if(pressed && !longHandled && now-pressedAt>=LongPressUs){
            longHandled=true;clearClicks();return action|OpenSetup;
        }
        if(!pressed && down){
            down=false;
            if(!longHandled && now-pressedAt>=LongPressUs){
                longHandled=true;clearClicks();return action|OpenSetup;
            }
            if(!longHandled && now-pressedAt>=DebounceUs){
                if(++clicks==3){clearClicks();return action|StartTimer;}
                settleAt=now+ClickGapUs;
            }
        }
        return action;
    }
};

enum class TimerPhase : uint8_t { Normal, Counting, Zero, Fireworks };
struct TimerView {
    TimerPhase phase=TimerPhase::Normal;
    int hundredths=0;
    bool visible=true;
    int64_t effectUs=0;
};
struct Countdown {
    static constexpr int64_t DurationUs=30000000;
    static constexpr int64_t ZeroHoldUs=200000;
    static constexpr int64_t FireworksUs=4000000;
    static constexpr int64_t FrameUs=40000; // 25 fps target on the 400 kHz OLED.
    bool running=false;
    int64_t startedAt=0;

    void start(int64_t now){running=true;startedAt=now;}
    void stop(){running=false;}
    TimerView view(int64_t now) const {
        TimerView v;
        if(!running)return v;
        const int64_t elapsed=now>startedAt?now-startedAt:0;
        if(elapsed<DurationUs){
            const int64_t remaining=DurationUs-elapsed;
            v.phase=TimerPhase::Counting;
            // Ceil prevents showing 00:00 before the actual deadline.
            v.hundredths=(int)((remaining+9999)/10000);
            if(remaining<=5000000)
                v.visible=((5000000-remaining)/125000)%2==0; // 4 Hz.
            else if(remaining<=15000000)
                v.visible=((15000000-remaining)/500000)%2==0; // 1 Hz.
        }else if(elapsed<DurationUs+ZeroHoldUs){
            v.phase=TimerPhase::Zero;
        }else if(elapsed<DurationUs+ZeroHoldUs+FireworksUs){
            v.phase=TimerPhase::Fireworks;
            v.effectUs=elapsed-DurationUs-ZeroHoldUs;
        }
        return v;
    }
};
