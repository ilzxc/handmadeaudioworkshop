// Handmade Audio Workshop
// Microsoft 2018

#include "stdafx.h"
#include "SDL.h"
#undef main

#include <memory>
#include <chrono>
#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;
using namespace std::chrono;

struct OscillatorData
{
    float frequency;
    float amplitude;
};

void AudioCallback(void* userData, Uint8* buffer, int bufferSizeBytes)
{
    memset(buffer, 0, bufferSizeBytes); // create silence

    // this is where all the audio computation takes place:
    OscillatorData* oscData = reinterpret_cast<OscillatorData*>(userData); // see main() for setup
    float* sampleArray = reinterpret_cast<float*>(buffer);

    // how fast does this execute?
}

float clamp(float x) { return (x > 1) ? 1 : ((x < 0) ? 0 : x); }

int main(int argc, const char** argv)
{
    // initilization
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    // our data structure that will be passed to the audio callback:
    unique_ptr<OscillatorData> oscData = make_unique<OscillatorData>();
    oscData->frequency = 440.f;
    oscData->amplitude = 1.f;

    // audio setup 
    SDL_AudioDeviceID deviceId;
    SDL_AudioSpec outputObtained;
    {
        // I've taken the liberty of getting audio initialization taken care of:
        SDL_AudioSpec outputDesired;                        // structure to request settings from the system
        memset(&outputDesired, 0, sizeof(outputDesired));   // zero out the mem block
        outputDesired.freq = 44100;                         // samples per second
        outputDesired.format = AUDIO_F32SYS;                // want floating point values
        outputDesired.channels = 1;                         // mono signal (we can mess with stereo later)
        outputDesired.samples = 1024;                       // number of samples per audio block
        outputDesired.callback = AudioCallback;             // hand `AudioCallback` function to be called by audio subsystem
        outputDesired.userdata = oscData.get();             // hand the pointer to our data so AudioCallback can see it

        // these are the settings we actually ended up with
        memset(&outputObtained, 0, sizeof(outputDesired));
        deviceId = SDL_OpenAudioDevice(nullptr, 0, &outputDesired, &outputObtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

        if (deviceId == 0) 
        {
            SDL_Log("Failed to open audio output: %s", SDL_GetError());
        }
        else {
            if (outputObtained.format != outputDesired.format) 
            {
                SDL_Log("Floating point audio is not supported on this system."); // this doesn't really ever happen anymore :)
            }
            else 
            {
                SDL_Log("Opened audio output successfully. Using driver %s", SDL_GetCurrentAudioDriver());
                SDL_Log("Samples: %d", outputObtained.samples);
                SDL_PauseAudioDevice(deviceId, 0);
            }
        }
    }

    string dummy;
    bool keepAsking = true;
    while (keepAsking)
    {
        cout << "Amplitude (0 to 1) or `q` to stop: ";
        cin >> dummy; // blocks, but that's ok becuase audio runs in a separate thread!
        if (dummy == "q") break;
        float amplitude = clamp((float)atof(dummy.c_str()));
        cout << " -  Setting float to " << amplitude << '\n';
        oscData->amplitude = amplitude;
    }

    SDL_Log("Finished playback, cleaning up & stopping everything.");
    SDL_PauseAudioDevice(deviceId, 1);
    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();

    // time the function execution using the values we've had:
    const int numRuns = 100000;
    auto blockDurationNanoseconds = nanoseconds((size_t)(outputObtained.samples * 1e9 / 44100.f));

    size_t* timingData = new size_t[numRuns];
    size_t maxData = 0, sum = 0;
    float* buffer = new float[outputObtained.samples];
    int size = outputObtained.size;


    for (size_t i = 0; i < numRuns; ++i)
    {
        auto start = high_resolution_clock::now();
        AudioCallback(oscData.get(), (Uint8*)buffer, size);
        auto delta = duration_cast<nanoseconds>(high_resolution_clock::now() - start);
        timingData[i] = delta.count();
        if (timingData[i] > maxData)
        {
            maxData = timingData[i];
        }

        sum += timingData[i];
    }

    auto avg = sum / numRuns;

    cout << "# sinewave function avg          : " << avg << '\n';
    cout << "# sinewave worst case            : " << maxData << '\n';
    cout << "# total block duration           : " << blockDurationNanoseconds.count() << '\n';
    cout << "# percent of vector worst case   : " << 100.f * maxData / blockDurationNanoseconds.count() << "%\n";
    cout << "# can compute in vector worst    : " << blockDurationNanoseconds.count() / maxData << '\n';
    cout << "# percent of vector average case : " << 100.f * avg / blockDurationNanoseconds.count() << "%\n";
    cout << "# can compute in vector average  : " << blockDurationNanoseconds.count() / avg << '\n';

    delete[] timingData;
    delete[] buffer;

    return 0;
}

