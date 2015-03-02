// 5th order high pass butterworth filter at 1.5 Hz from mkfilter 60Hz samplerate
#define NZEROS 5
#define NPOLES 5
#define GAIN   1.289795686e+00
static float highPassFilter(float input) {
    static float xv[NZEROS+1], yv[NPOLES+1];
    static float runningLowAmp = 0;
    static float runningHighAmp = 0;
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5];
    xv[5] = input / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5];
    yv[5] =   (xv[5] - xv[0]) + 5 * (xv[1] - xv[4]) + 10 * (xv[3] - xv[2])
    + (  0.6011158229 * yv[0]) + ( -3.3110475620 * yv[1])
    + (  7.3120812802 * yv[2]) + ( -8.0940554178 * yv[3])
    + (  4.4918309651 * yv[4]);
    return yv[5];
}

// heart kernel based on observation at 60Hz samplerate
static float heartFilter(float input) {
    static const int heartKernelSize = 9;
    static float heartHistory [heartKernelSize];
    static float heartKernel [] = {
        -0.000312664, -0.00870198, -0.076416, -0.179242, 0,
        0.179242, 0.076416, 0.00870198, 0.000312664};
    
    for(int i = 0; i < heartKernelSize - 1; i++) {
        heartHistory[i] = heartHistory[i + 1];
    }
    heartHistory[heartKernelSize - 1] = input;
    float sumSquaredDiffs = 0;
    for(int i = 0; i < heartKernelSize; i++) {
        float diff = heartHistory[i] - heartKernel[i];
        sumSquaredDiffs += diff * diff;
    }
    return sqrt(sumSquaredDiffs);
}

// moving max filter to introduce some hysteresis
static float movingMaxFilter(float input) {
    static float hysteresis = .05;
    static float max = 0;
    if(input > max) {
        max = input;
    } else {
        max = (hysteresis * input) + ((1-hysteresis) * max);
    }
    return max;
}

// rising values filter checks for onsets
static bool risingFilter(float input) {
    static float previous = 0;
    bool rising = input > previous;
    previous = input;
    return rising;
}

// peak filter triggers immediately but is rate limited
static bool rateLimitedPeakFilter(bool input) {
    static long previousPeak = 0;
    static const long rateLimit = 20; // 180 bpm at 60 Hz (60 * 60 / 180)
    if(input && previousPeak > rateLimit) {
        previousPeak = 0;
        return true;
    } else {
        previousPeak++;
        return false;
    }
}

bool rangeFilter(float input) {
    static const int count = 3;
    static float recent[count];
    static const float range = 2.;
    bool inRange = true;
    for(int i = 0; i < count; i++) {
        if(recent[i] / range > input || recent[i] * range < input) {
            inRange = false;
        }
        recent[i] = recent[i + 1];
    }
    recent[count - 1] = input;
    return inRange;
}

bool heartbeatFilter(float x, unsigned long timeMs) {
    static unsigned long previousTimeMs = 0;
    if(x != x) {
        return false;
    }
    x = highPassFilter(x);
    x = heartFilter(x);
    x = movingMaxFilter(x);
    bool curBeat = rateLimitedPeakFilter(risingFilter(x));
    if(curBeat) {
        float curBpm = (60. * 1000.) / (timeMs - previousTimeMs);
        previousTimeMs = timeMs;
        return rangeFilter(curBpm);
    }
    return false;
}
