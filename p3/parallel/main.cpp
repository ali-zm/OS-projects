#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

struct BandPassFilterData {
    const vector<float> *audioData;
    vector<float> *filteredData;
    float sampleRate;
    float lowerFreq;
    float upperFreq;
    float deltaFreq;
    int start;
    int end;
};

struct NotchFilterData {
    vector<float>* audioData;
    vector<float>* filteredData;
    float sampleRate;
    float f0;
    int n;
    int startIdx;
    int endIdx;
};

struct FIRFilterData {
    const vector<float>* inputSignal;
    vector<float>* outputSignal;
    const vector<float>* coefficients;
    int startIdx;
    int endIdx;
};

struct IIRFilterData {
    const vector<float>* inputSignal;
    vector<float>* outputSignal;
    const vector<float>* feedforwardCoefficients;
    const vector<float>* feedbackCoefficients;
    size_t start;
    size_t end;
    size_t chunkIndex;
    vector<bool>* completedChunks;
    vector<mutex>* mutexes;
    vector<condition_variable>* cvs;
};

void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        cerr << "Error opening input file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        cerr << "Error reading frames from file." << endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    cout << "Successfully read " << numFrames << " frames from " << inputFile << endl;
}

void writeWavFile(const string& outputFile, const vector<float>& data,  SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        cerr << "Error opening output file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        cerr << "Error writing frames to file." << endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    cout << "Successfully wrote " << numFrames << " frames to " << outputFile << endl;
}

void *applyBandPassFilterChunk(void *arg) 
{
    BandPassFilterData *data = (BandPassFilterData *)arg;
    const vector<float> &audioData = *(data->audioData);
    vector<float> &filteredData = *(data->filteredData);
    float sampleRate = data->sampleRate;
    float lowerFreq = data->lowerFreq;
    float upperFreq = data->upperFreq;
    float deltaFreq = data->deltaFreq;

    auto filterResponse = [=](float f) {
        return (f * f) / (f * f + deltaFreq * deltaFreq);
    };

    for (int i = data->start; i < data->end; ++i) {
        float f = i * sampleRate / audioData.size();
        if (f >= lowerFreq && f <= upperFreq) 
            filteredData[i] = audioData[i] * filterResponse(f);
         else 
            filteredData[i] = 0;
    }

    return nullptr;
}

void applyBandPassFilterParallel(vector<float> &audioData, float sampleRate, float lowerFreq, float upperFreq, float deltaFreq, int numThreads) {
    int numSamples = audioData.size();
    vector<float> filteredData(numSamples);

    pthread_t threads[numThreads];
    BandPassFilterData threadData[numThreads];

    int chunkSize = numSamples / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        int start = t * chunkSize;
        int end = (t == numThreads - 1) ? numSamples : start + chunkSize;

        threadData[t] = {&audioData, &filteredData, sampleRate, lowerFreq, upperFreq, deltaFreq, start, end};
        pthread_create(&threads[t], nullptr, applyBandPassFilterChunk, &threadData[t]);
    }
    for (int t = 0; t < numThreads; ++t) 
        pthread_join(threads[t], nullptr);
    

    audioData = filteredData;
}

void* applyNotchFilterChunk(void* arg) {
    NotchFilterData* data = (NotchFilterData*)arg;

    vector<float>& audioData = *(data->audioData);
    vector<float>& filteredData = *(data->filteredData);
    float sampleRate = data->sampleRate;
    float f0 = data->f0;
    int n = data->n;

    for (int i = data->startIdx; i < data->endIdx; ++i) {
        float f = (sampleRate * i) / audioData.size();
        float response = (1.0 / (pow((f / f0), 2 * n) + 1));
        filteredData[i] = audioData[i] * response;
    }

    pthread_exit(nullptr);
}

void applyNotchFilter(vector<float>& audioData, float sampleRate, float f0, int n, int numThreads) {
    int numSamples = audioData.size();
    vector<float> filteredData(numSamples);
    pthread_t threads[numThreads];
    NotchFilterData threadData[numThreads];
    int chunkSize = numSamples / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        int start =  t * chunkSize;
        int end = (t == numThreads - 1) ? numSamples : (t + 1) * chunkSize;
        threadData[t] = {&audioData, &filteredData, sampleRate, f0, n, start, end};
        if (pthread_create(&threads[t], nullptr, applyNotchFilterChunk, &threadData[t]) != 0) {
            cerr << "Error creating thread " << t << endl;
            exit(1);
        }
    }
    for (int t = 0; t < numThreads; ++t) 
        pthread_join(threads[t], nullptr);
    
    audioData = filteredData;
}

void* applyFIRFilterChunk(void* arg) {
    FIRFilterData* data = (FIRFilterData*)arg;

    const vector<float>& inputSignal = *(data->inputSignal);
    vector<float>& outputSignal = *(data->outputSignal);
    const vector<float>& coefficients = *(data->coefficients);
    int numTaps = coefficients.size();


    for (int i = data->startIdx; i < data->endIdx; ++i) {
        outputSignal[i] = 0.0; 
        for (int j = 0; j < numTaps; ++j) {
            if (i >= j) {
                outputSignal[i] += coefficients[j] * inputSignal[i - j];
            }
        }
    }

    pthread_exit(nullptr);
}

void applyFIRFilter(vector<float>& inputSignal, const vector<float>& coefficients, int numThreads) {
    int signalSize = inputSignal.size();
    vector<float> outputSignal(signalSize);
    pthread_t threads[numThreads];
    FIRFilterData threadData[numThreads];
    int chunkSize = signalSize / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        int start = t * chunkSize;
        int end = (t == numThreads - 1) ? signalSize : (t + 1) * chunkSize;
        threadData[t] =  {&inputSignal, &outputSignal, &coefficients, start, end};

        if (pthread_create(&threads[t], nullptr, applyFIRFilterChunk, &threadData[t]) != 0) {
            cerr << "Error creating thread " << t << endl;
            exit(1);
        }
    }

    for (int t = 0; t < numThreads; ++t) 
        pthread_join(threads[t], nullptr);

    inputSignal = outputSignal;
}


void* applyIIRFilterChunk(void* arg) {
    auto* data = (IIRFilterData*)arg;

    size_t numFeedforward = data->feedforwardCoefficients->size();
    size_t numFeedback = data->feedbackCoefficients->size();

    if (data->chunkIndex > 0) 
    {
        unique_lock<mutex> lock((*data->mutexes)[data->chunkIndex - 1]);
        (*data->cvs)[data->chunkIndex - 1].wait(lock, [data] 
        {
            return (*data->completedChunks)[data->chunkIndex - 1];
        });
    }
    for (size_t n = data->start; n < data->end; ++n) 
        for (size_t k = 0; k <= n; ++k) 
                (*data->outputSignal)[n] += (*data->feedforwardCoefficients)[k] * (*data->inputSignal)[n - k];

    {
        unique_lock<mutex> lock((*data->mutexes)[data->chunkIndex]);
        (*data->completedChunks)[data->chunkIndex] = true;
        (*data->cvs)[data->chunkIndex].notify_one();
    }

    for (size_t n = data->start; n < data->end; ++n) 
        for (size_t k = 1; k <= n; ++k)
                (*data->outputSignal)[n] -= (*data->feedbackCoefficients)[k - 1] * (*data->outputSignal)[n - k];

    return nullptr;
}

void applyIIRFilterParallel(vector<float>& inputSignal, const vector<float>& feedforwardCoefficients, const vector<float>& feedbackCoefficients, int numThreads) {
    vector<float> outputSignal;
    size_t signalSize = inputSignal.size();
    outputSignal.resize(signalSize, 0.0f);

    size_t chunkSize = signalSize / numThreads;
    vector<pthread_t> threads(numThreads);
    vector<IIRFilterData> threadData(numThreads);

    vector<mutex> mutexes(numThreads);
    vector<condition_variable> cvs(numThreads);
    vector<bool> completedChunks(numThreads, false);

    for (int i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? signalSize : start + chunkSize;

        threadData[i] = {&inputSignal, &outputSignal, &feedforwardCoefficients, &feedbackCoefficients,
                         start, end, static_cast<size_t>(i), &completedChunks, &mutexes, &cvs};

        pthread_create(&threads[i], nullptr, applyIIRFilterChunk, &threadData[i]);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    inputSignal = outputSignal;
    cout << "IIR filter applied using " << numThreads << " threads." << endl;
}

float sum_all(vector<float>& audioData)
{
    float sum = 0;
    for(int i=0 ; i<audioData.size(); i++)
        sum+=audioData[i];
    return sum;
}

int main(int argc, char const *argv[])
{
    string filter_name = argv[1]; 
    string inputFile = argv[2]; 
    int numThreads = 5;
    string outputFile = "output-"+filter_name+"-parallel.wav";

    SF_INFO fileInfo;
    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));
    readWavFile(inputFile, audioData, fileInfo);    


    float lowerFreq = 0.0;   
    float upperFreq = 5000.0;  
    float deltaFreq = 100.0; 
    int n = 2;
    float f0 = 1000.0;
    vector<float> coefficients_xi(audioData.size(),0.1);
    vector<float> coefficients_yi(audioData.size(),0.5);

    auto timeStart = chrono::high_resolution_clock::now();
    if(filter_name=="BandPass")
        applyBandPassFilterParallel(audioData, fileInfo.samplerate, lowerFreq, upperFreq, deltaFreq, numThreads);
    else if(filter_name=="Notch")
        applyNotchFilter(audioData, fileInfo.samplerate, f0, n, numThreads);
    else if(filter_name=="FIR")
        applyFIRFilter(audioData, coefficients_xi, numThreads);
    else if(filter_name=="IIR")
        applyIIRFilterParallel(audioData, coefficients_xi, coefficients_yi, numThreads);
    auto timeEnd = chrono::high_resolution_clock::now();
    cout << "Read Time: " << chrono::duration_cast<chrono::duration<float, milli>>(timeEnd - timeStart).count() << " ms\n";
    cout << "sum " << sum_all(audioData) << endl;

    writeWavFile(outputFile, audioData, fileInfo);
    return 0;
}

