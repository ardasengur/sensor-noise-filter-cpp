#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <string>

using namespace std;

struct SensorData {
    int timestamp;
    float FirstValue;
    float FilteredValue;
};

// İki değer birbirine eşik değerinden (threshold) daha mı yakın?
bool isClose(float a, float b, float threshold) {
    return abs(a - b) <= threshold;
}

void movingAverage(SensorData* data, int size, int windowSize) {
    if (size <= 0) return;

    float threshold = 20.0f; // 20 birimden fazla ani değişimler "şüpheli" (spike adayı)
    
    // Eski moving average'ı unutmak istediğimizde bu başlangıç sınırını ileri taşıyacağız
    int windowStartIndex = 0;

    // İlk veriyi başlangıç referansı olarak al
    data[0].FilteredValue = data[0].FirstValue;
    float lastValidAverage = data[0].FilteredValue;

    int i = 1;
    while (i < size) {
        float currentVal = data[i].FirstValue;

        // 1. DURUM: Veri eski ortalamaya yakınsa normal Moving Average devam eder
        if (isClose(currentVal, lastValidAverage, threshold)) {
            float sum = 0.0f;
            int count = 0;

            for (int j = 0; j < windowSize; ++j) {
                // windowStartIndex sayesinde unutulan eski geçmişe ASLA bakılmaz
                if (i - j >= windowStartIndex) {
                    sum += data[i - j].FirstValue;
                    count++;
                }
            }

            data[i].FilteredValue = count > 0 ? sum / count : 0.0f;
            lastValidAverage = data[i].FilteredValue;
            i++;
        } 
        // 2. DURUM: Spike yakalandı! Hemen aksiyon alma, bir sonraki verilerle doğrula
        else {
            int k = i + 1;
            bool isConfirmedNewLevel = false;
            bool decisionMade = false;

            // Kararsız durumlar (Örn: 500 -> 600 -> 550) bitene kadar ileriye bakan döngü
            while (k < size) {
                float nextVal = data[k].FirstValue;
                float prevSuspect = data[k - 1].FirstValue;

                // A) Sonraki veri ESKİ ortalamaya yakın (Örn: 500 -> 580 -> 502)
                // Demek ki bu gerçek bir spike (hatalı okuma)!
                if (isClose(nextVal, lastValidAverage, threshold)) {
                    isConfirmedNewLevel = false;
                    decisionMade = true;
                    break;
                }
                // B) Sonraki veri bir önceki şüpheli (spike) değerine yakın (Örn: 500 -> 580 -> 585)
                // Demek ki bu gerçek bir seviye değişimi!
                else if (isClose(nextVal, prevSuspect, threshold)) {
                    isConfirmedNewLevel = true;
                    decisionMade = true;
                    break;
                }

                // C) İkisine de yakın değilse (Örn: 500 -> 600 -> 550), bir sonrakini beklemeye devam et
                k++;
            }

            if (decisionMade && isConfirmedNewLevel) {
                // YENİ SEVİYE DOĞRULANDI! (Örn: 580 -> 585)
                int confirmedStartIdx = k - 1;

                // Eğer arada kararsız veriler (Örn: 500 -> 600 -> 550 -> 552'deki 600) varsa onları temizle
                for (int m = i; m < confirmedStartIdx; ++m) {
                    data[m].FirstValue = lastValidAverage;
                    data[m].FilteredValue = lastValidAverage;
                }

                // ESKİ MOVING AVERAGE'I UNUT ve confirmedStartIdx'ten baştan başla!
                windowStartIndex = confirmedStartIdx;
                data[confirmedStartIdx].FilteredValue = data[confirmedStartIdx].FirstValue;
                lastValidAverage = data[confirmedStartIdx].FilteredValue;

                // Döngüyü doğrulanan verinin hemen sonrasından (k'dan) devam ettir
                i = k;
            } 
            else {
                // YANLIŞ ALARM (SPIKE)! (Örn: 500 -> 580 -> 502)
                // Spike verisini sil (eski ortalama ile ez) ve eski Moving Average'a devam et
                for (int m = i; m < k; ++m) {
                    data[m].FirstValue = lastValidAverage;
                    data[m].FilteredValue = lastValidAverage;
                }
                i = k;
            }
        }
    }
}

void generateRealisticSensorData(SensorData* data, int size, float baseValue) {
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<float> backgroundNoise(0.0f, 2.5f);
    uniform_real_distribution<float> chance(0.0f, 1.0f);
    uniform_real_distribution<float> spikeMagnitude(35.0f, 80.0f);
    bernoulli_distribution isPositive(0.5);

    float currentBase = baseValue;

    for (int i = 0; i < size; i++) {
        data[i].timestamp = i * 10;

        // Algoritmanın "yeni seviyeyi doğrulama" özelliğini de test edebilmek için
        // 40. ve 75. adımlarda sensörün gerçek değerini kalıcı olarak değiştiriyoruz (Örn: 500 -> 580)
        if (i == 40 || i == 75) {
            float stepDirection = isPositive(gen) ? 1.0f : -1.0f;
            currentBase += stepDirection * 60.0f;
        }

        float currentReading = currentBase + backgroundNoise(gen);

        // %8 ihtimalle anlık tekil spike (gürültü) üret
        if (chance(gen) < 0.08f) {
            float direction = isPositive(gen) ? 1.0f : -1.0f;
            currentReading += direction * spikeMagnitude(gen);
        }

        data[i].FirstValue = currentReading;
        data[i].FilteredValue = 0.0f;
    }
}

int main() {
    int Samplecount = 101;
    int windowsize = 5;
    float baseSensorValue = 500.0f;

    SensorData* readings = new SensorData[Samplecount];
    float* originalRawValues = new float[Samplecount]; // Silinmeden önceki ham hallerini ekranda görmek için

    generateRealisticSensorData(readings, Samplecount, baseSensorValue);

    for (int i = 0; i < Samplecount; i++) {
        originalRawValues[i] = readings[i].FirstValue;
    }

    movingAverage(readings, Samplecount, windowsize);

    cout << fixed << setprecision(2);
    cout << "Zaman(ms)\tHam\t\tFiltreli\tDurum" << endl;
    cout << "-------------------------------------------------------------------------" << endl;
    
    for (int i = 0; i < Samplecount; i++) {
        string status = "";
        
        // Ekranda ne olduğunu net görmek için durum analizi:
        if (abs(originalRawValues[i] - readings[i].FirstValue) > 0.01f) {
            status = "<-- SPIKE SILINDI";
        } else if (i > 0 && abs(readings[i].FilteredValue - readings[i - 1].FilteredValue) > 20.0f) {
            status = "<-- YENI SEVIYE ONAYLANDI";
        }
        
        cout << readings[i].timestamp << "\t\t" 
             << originalRawValues[i] << "\t\t" 
             << readings[i].FilteredValue << "\t\t"
             << status << endl;
    }

    delete[] readings;
    delete[] originalRawValues;
    readings = nullptr;

    return 0;
}