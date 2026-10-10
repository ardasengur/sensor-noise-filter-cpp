#include <iostream>
using namespace std;

struct SensorData{
    int timestamp;
    float FirstValue;
    float FilteredValue;
};

void applyMovingAverage(SensorData* data, int size, int windowSize) {
    for (int i = 0; i < size; ++i) {
        float sum = 0.0;
        int count = 0;
        
        for (int j = 0; j < windowSize; ++j) {
            if (i - j >= 0) { 
                sum += data[i - j].FirstValue;
                count++;
            }
        }
        
        data[i].FilteredValue = sum / count;
    }
}

int main(){
    int Samplecount = 100;
    int windowsize = 5;

    SensorData* readings = new SensorData[Samplecount];

    for (int i = 0; i < Samplecount; i++) {
        readings[i].timestamp = i * 10;  
        readings[i].FirstValue = 50.0 + (i % 5);
        readings[i].FilteredValue = 0.0; 
    }

    applyMovingAverage(readings, Samplecount, windowsize);

    cout << "Zaman(ms)\tHam\tFiltreli" << endl;
    cout << "---------------------------------" << endl;
    
    for (int i = 0; i < Samplecount; i++){
        cout << readings[i].timestamp << "\t\t" 
             << readings[i].FirstValue << "\t" 
             << readings[i].FilteredValue << endl;
    }

    delete[] readings;
    readings = nullptr;
}