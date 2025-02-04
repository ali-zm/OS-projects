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

void readWavFile(const std::string& inputFile, std::vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        std::cerr << "Error reading frames from file." << std::endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    std::cout << "Successfully read " << numFrames << " frames from " << inputFile << std::endl;
}

void writeWavFile(const std::string& outputFile, const std::vector<float>& data,  SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        std::cerr << "Error writing frames to file." << std::endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    std::cout << "Successfully wrote " << numFrames << " frames to " << outputFile << std::endl;
}

void applyBandPassFilter(vector<float> &audioData, double sampleRate, double lowerFreq, double upperFreq, double deltaFreq) {
    int numSamples = audioData.size();
    std::vector<float> filteredData(numSamples);
    auto filterResponse = [=](double f) {
        return (f * f) / (f * f + deltaFreq * deltaFreq);
    };
    for (int i = 0; i < numSamples; i++) 
    {
        double f = i * sampleRate / numSamples; 
        if (f >= lowerFreq && f <= upperFreq) 
            filteredData[i] = audioData[i] * filterResponse(f);
        else 
            filteredData[i] = 0; 
    }
    audioData = filteredData;
}

void applyNotchFilter(vector<float>& audioData, float sampleRate, float f0, int n) 
{
    int numSamples = audioData.size();

    std::vector<float> filteredData(numSamples);
    for (int i = 0; i < numSamples; ++i) 
    {
        float f = (sampleRate * i) / numSamples;
        float response = (1.0 / (pow((f / f0), 2 * n) + 1));
        filteredData[i] = audioData[i] * response;
    }
    audioData = filteredData;
}



void applyFIRFilter(vector<float>& inputSignal, const vector<float>& coefficients) {
    int numTaps = coefficients.size(); 
    int signalSize = inputSignal.size();
    vector<float> outputSignal(signalSize);

    for (int i = 0; i < signalSize; ++i) 
    {
        outputSignal[i] = 0.0; 
        for (int j = 0; j < numTaps; ++j) 
            if (i >= j) 
                outputSignal[i] += coefficients[j] * inputSignal[i - j];
    }

    for (int i = 0; i < signalSize; ++i)
        inputSignal[i] = outputSignal[i];
}


void applyIIRFilter(vector<float>& inputSignal,const vector<float>& feedforwardCoefficients, const vector<float>& feedbackCoefficients) 
{
    int numFeedforward = feedforwardCoefficients.size(); 
    int numFeedback = feedbackCoefficients.size();       
    int signalSize = inputSignal.size();

    vector<float> outputSignal;

    outputSignal.resize(signalSize, 0.0f);

    for (int n = 0; n < signalSize; ++n) 
    {
        for (int k = 0; k < numFeedforward; ++k)
            if (n >= k) 
                outputSignal[n] += feedforwardCoefficients[k] * inputSignal[n - k];

        for (int k = 1; k <= numFeedback; ++k) 
            if (n >= k)
            {
                outputSignal[n] -= feedbackCoefficients[k - 1] * outputSignal[n - k];
            }
    }

    for (int i = 0; i < signalSize; ++i)
        inputSignal[i] = outputSignal[i];
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
    string outputFile = "output-"+filter_name+"-sequential.wav";

    SF_INFO fileInfo;
    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));

    readWavFile(inputFile, audioData, fileInfo);    


    double lowerFreq = 0.0;   
    double upperFreq = 5000.0;  
    double deltaFreq = 100.0; 

    int n = 2;
    float f0 = 1000.0;


    vector<float> coefficients_xi(audioData.size(),0.1);
    vector<float> coefficients_yi(audioData.size(),0.5);
    auto timeStart = std::chrono::high_resolution_clock::now();
    if(filter_name=="BandPass")
        applyBandPassFilter(audioData, fileInfo.samplerate, lowerFreq, upperFreq, deltaFreq);
    else if(filter_name=="Notch")
        applyNotchFilter(audioData, fileInfo.samplerate, f0, n);
    else if(filter_name=="FIR")
        applyFIRFilter(audioData, coefficients_xi);
    else if(filter_name=="IIR")
        applyIIRFilter(audioData, coefficients_xi, coefficients_yi);
    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Read Time: " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(timeEnd - timeStart).count() << " ms\n";
    cout << "sum " << sum_all(audioData) << endl;


    writeWavFile(outputFile, audioData, fileInfo);
    return 0;
}





