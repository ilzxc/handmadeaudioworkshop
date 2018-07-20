// Handmade Audio Workshop
// Microsoft 2018

#include "stdafx.h"
#include "SDL.h"
#undef main

#include <memory>
#include <atomic>
#include <iostream>
#include <string>
#include <stdlib.h>

#define TWO_PI 6.283185307179586476925286766559f

using namespace std;

struct OscillatorData
{
    float phase;
    float frequency;
    float amplitude;
    float samplerate;
};

void AudioCallback(void* userData, Uint8* buffer, int bufferSizeBytes)
{
    // this is where all the audio computation takes place:
    OscillatorData* oscData = reinterpret_cast<OscillatorData*>(userData); // see main() for setup
    float* sampleArray = reinterpret_cast<float*>(buffer);
    size_t sampleArraySize = bufferSizeBytes / sizeof(float); // assuming it's one channel!

    // grab the values that we need from our oscData:
    float phase = oscData->phase;
    const float amplitude = oscData->amplitude;
    const float phaseIncrement = TWO_PI * oscData->frequency / oscData->samplerate;
    
    // compute the samples:
    for (size_t i = 0; i < sampleArraySize; ++i)
    {
        sampleArray[i] = sinf(phase) * amplitude;
        phase += phaseIncrement;
        phase -= (phase > 1.f) * TWO_PI;
    }

    oscData->phase = phase;
}

float clamp(float x) { return (x > 1) ? 1 : ((x < 0) ? 0 : x); }

int main(int argc, const char** argv)
{
    // initilization
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    // our data structure that will be passed to the audio callback:
    unique_ptr<OscillatorData> oscData = make_unique<OscillatorData>();
    oscData->phase = 0.f;
    oscData->frequency = 440.f;
    oscData->amplitude = 1.f;

    // audio setup 
    SDL_AudioDeviceID deviceId;
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

        SDL_AudioSpec outputObtained;                       // these are the settings we actually ended up with
        memset(&outputObtained, 0, sizeof(outputDesired));
        deviceId = SDL_OpenAudioDevice(nullptr, 0, &outputDesired, &outputObtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
        oscData->samplerate = (float)outputObtained.freq;

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
        cout << "f + number to set frequency (e.g. f261.1 or f440 or f1001.111)\n";
        cout << "a + number to set amplitude (e.g. a0.155, clamped zero to one)\n";
        cout << "q to quit\n";
        cout << " > ";
        cin >> dummy; // blocks, but that's ok becuase audio runs in a separate thread!
        if (dummy == "q") break;
        if (dummy[0] == 'f')
        {
            float frequency = (float)atof(dummy.c_str() + 1);
            cout << " -  Setting frequency to " << frequency << '\n';
            oscData->frequency = frequency;
        }
        else if (dummy[0] == 'a')
        {
            float amplitude = clamp((float)atof(dummy.c_str() + 1));
            cout << " -  Setting amplitude to " << amplitude << '\n';
            oscData->amplitude = amplitude;
        }
    }

    SDL_Log("Finished playback, cleaning up & stopping everything.");
    SDL_PauseAudioDevice(deviceId, 1);
    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();

    return 0;
}

