#pragma once

// Coarse single-cell Li-ion voltage bands, not a fuel-gauge percentage.
inline bool batteryVoltageValid(int mv) { return mv>=2500 && mv<=4400; }
inline int batteryControlLevel(int lowMv,int highMv) {
    // V3/V3.1 use a directly driven P-MOSFET (active LOW); V3.2 adds
    // an inverting transistor (active HIGH). Both use a 390k/100k divider.
    if(batteryVoltageValid(lowMv))return 0;
    if(lowMv<1000 && batteryVoltageValid(highMv))return 1;
    return -1;
}
inline int batterySegments(int mv,int previous=-1) {
    if(!batteryVoltageValid(mv))return -1;
    if(mv<=3450)return 0;
    const int thresholds[]={3450,3650,3850,4000};
    int level=1;
    while(level<4 && mv>=thresholds[level])++level;
    if(previous>=0 && previous<=4) {
        if(level>previous && mv<thresholds[previous]+40)return previous;
        if(level<previous && mv>thresholds[previous-1]-40)return previous;
    }
    return level;
}
