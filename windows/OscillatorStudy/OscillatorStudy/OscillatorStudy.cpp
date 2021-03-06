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

using namespace std;

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

    return 0;
}

