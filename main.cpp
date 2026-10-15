#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <string>

using namespace std;

enum FilterStatus {
    STATUS_NORMAL,
    STATUS_HOLDING_SUSPECT,
    STATUS_SPIKE_REJECTED,  
    STATUS_NEW_LEVEL        
};

class AdaptiveMovingAverage {
private:
    enum State {
        STATE_NORMAL,
        STATE_SUSPECT
    };

    int windowSize;
    float threshold;

    float* ringBuffer;
    int head;         
    int count;        
    float runningSum;  

    State currentState;
    bool isInitialized;
    float lastValidAverage;
    float lastSuspectValue;
    FilterStatus lastActionStatus;

    bool isClose(float a, float b) const {
        return abs(a - b) <= threshold;
    }

    void pushToBuffer(float val) {
        if (count == windowSize) {
            runningSum -= ringBuffer[head];
        } else {
            count++;
        }
        ringBuffer[head] = val;
        runningSum += val;
        head = (head + 1) % windowSize;
        
        lastValidAverage = runningSum / count;
    }

    void resetBuffer() {
        head = 0;
        count = 0;
        runningSum = 0.0f;
    }

public:
    AdaptiveMovingAverage(int winSize, float thresh) 
        : windowSize(winSize), threshold(thresh), head(0), count(0), 
          runningSum(0.0f), currentState(STATE_NORMAL), isInitialized(false),
          lastValidAverage(0.0f), lastSuspectValue(0.0f), lastActionStatus(STATUS_NORMAL) 
    {
        ringBuffer = new float[windowSize];
        for (int i = 0; i < windowSize; ++i) ringBuffer[i] = 0.0f;
    }

    ~AdaptiveMovingAverage() {
        delete[] ringBuffer;
    }

    float update(float newReading) {
        if (!isInitialized) {
            pushToBuffer(newReading);
            isInitialized = true;
            lastActionStatus = STATUS_NORMAL;
            return lastValidAverage;
        }

        if (currentState == STATE_NORMAL) {
            if (isClose(newReading, lastValidAverage)) {
                pushToBuffer(newReading);
                lastActionStatus = STATUS_NORMAL;
                return lastValidAverage;
            } else {
                currentState = STATE_SUSPECT;
                lastSuspectValue = newReading;
                lastActionStatus = STATUS_HOLDING_SUSPECT;
                return lastValidAverage;
            }
        }
        else {
            if (isClose(newReading, lastValidAverage)) {
                pushToBuffer(newReading);
                currentState = STATE_NORMAL;
                lastActionStatus = STATUS_SPIKE_REJECTED;
                return lastValidAverage;
            }
            else if (isClose(newReading, lastSuspectValue)) {
                resetBuffer();
                pushToBuffer(lastSuspectValue);
                pushToBuffer(newReading);     
                
                currentState = STATE_NORMAL;
                lastActionStatus = STATUS_NEW_LEVEL;
                return lastValidAverage;
            }
            else {
                lastSuspectValue = newReading;
                lastActionStatus = STATUS_HOLDING_SUSPECT;
                return lastValidAverage;
            }
        }
    }

    FilterStatus getStatus() const {
        return lastActionStatus;
    }
};

float getSimulatedSensorReading(int step, float& currentBase, mt19937& gen) {
    normal_distribution<float> backgroundNoise(0.0f, 2.5f);
    uniform_real_distribution<float> chance(0.0f, 1.0f);
    uniform_real_distribution<float> spikeMagnitude(35.0f, 80.0f);
    bernoulli_distribution isPositive(0.5);

    if (step == 40 || step == 75) {
        float stepDirection = isPositive(gen) ? 1.0f : -1.0f;
        currentBase += stepDirection * 60.0f;
    }

    float reading = currentBase + backgroundNoise(gen);

    if (chance(gen) < 0.08f) {
        float direction = isPositive(gen) ? 1.0f : -1.0f;
        reading += direction * spikeMagnitude(gen);
    }

    return reading;
}

int main() {
    int totalSteps = 101;
    int windowSize = 5;
    float threshold = 20.0f;
    float currentBase = 500.0f;

    random_device rd;
    mt19937 gen(rd());

    AdaptiveMovingAverage filter(windowSize, threshold);

    cout << fixed << setprecision(2);
    cout << "Zaman(ms)\tHam\t\tFiltreli\tDurum (Real-Time Karar)" << endl;
    cout << "----------------------------------------------------------------------------------" << endl;

    for (int i = 0; i < totalSteps; i++) {
        int timestamp = i * 10;
        
        float rawReading = getSimulatedSensorReading(i, currentBase, gen);

        float filteredOutput = filter.update(rawReading);

        string statusText = "";
        switch (filter.getStatus()) {
            case STATUS_HOLDING_SUSPECT:
                statusText = "<-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...";
                break;
            case STATUS_SPIKE_REJECTED:
                statusText = "<-- [SPIKE SILINDI] Onceki supheli veri reddedildi!";
                break;
            case STATUS_NEW_LEVEL:
                statusText = "<-- [YENI SEVIYE] Dogrulandi! Eski MA unutuldu!";
                break;
            default:
                statusText = "";
                break;
        }

        cout << timestamp << "\t\t" 
             << rawReading << "\t\t" 
             << filteredOutput << "\t\t" 
             << statusText << endl;
    }

    return 0;
}