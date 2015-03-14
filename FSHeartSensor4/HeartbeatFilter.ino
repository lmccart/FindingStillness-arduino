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
static float lerp(float x, float y, float t) {
    return x*(1-t) + y*(t);
}

struct agcDoubleFilter{
    float smoothing = .001;
    float min = 0;
    float max = 0;
    float run(float x) {
        if(x <= min) {
            min = x;
        } else {
            min = lerp(min, x, smoothing);
        }
        if(x >= max) {
            max = x;
        } else {
            max = lerp(max, x, smoothing);
        }
        return (x - min) / (max - min);
    }
};

struct agcSingleFilter{
    float smoothingRising = .9;
    float smoothingFalling = .002;
    float max = 0;
    float run(float x) {
        if(x >= max) {
            max = lerp(max, x, smoothingRising);
        } else {
            max = lerp(max, x, smoothingFalling);
        }
        return x / max;
    }
};

struct derivativeFilter {
    float px = 0;
    float run(float x) {
        float cur = x - px;
        px = x;
        return cur;
    }
};

float unsignedToSigned(float x) {
    return (x * 2) - 1;
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
    if(input < heartRateBpmMin || input > heartRateBpmMax) {
        return false;
    }
    int inRange = 0;
    for(int i = 0; i < count; i++) {
        if(recent[i] / range < input && recent[i] * range > input) {
            inRange++;
        }
        recent[i] = recent[i + 1];
    }
    recent[count - 1] = input;
    return (inRange + 1 >= count);
}

float bpmFilter(unsigned long timeMs) {
    static unsigned long previousTimeMs = 0;
    float curBpm = (60. * 1000.) / (timeMs - previousTimeMs);
    previousTimeMs = timeMs;
    return curBpm;
}

bool heartbeatFilter(float x, unsigned long timeMs, float& heartRateBpm) {
    static derivativeFilter derivativea, derivativeb;
    static agcDoubleFilter agca;
    static  agcSingleFilter agcb;
    static float smoothBpm = 60;
    static float prevBpm = 60;
    if(x != x) {
        return false;
    }
    x = highPassFilter(x);
    x = agca.run(x);
    x = unsignedToSigned(x);
    x = derivativea.run(x);
    x = derivativeb.run(x);
    x = abs(x);
    x = agcb.run(x);
    x = rateLimitedPeakFilter(x > .5);
    bool beat = false;
    if(x) {
        float filteredBpm = bpmFilter(timeMs);
        if(rangeFilter(filteredBpm)) {
            x = filteredBpm;
            prevBpm = filteredBpm;
            beat = true;
        } else {
            x = prevBpm;
        }
    } else {
        x = prevBpm;
    }
    smoothBpm = lerp(smoothBpm, x, .005);
    if(beat) {
      heartRateBpm = smoothBpm;
    }
    return beat;
}
