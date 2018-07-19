// Handmade Audio Workshop
// Microsoft 2018

#include "stdafx.h"
#include "SDL.h"
#undef main

#include <memory>
#include <atomic>
#include <iostream> // for cin :)
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

    // create a quiet click by setting the first sample to be non-zero:
    sampleArray[0] = oscData->amplitude;
}

float clamp(float x) { return (x > 1) ? 1 : ((x < 0) ? 0 : x); }

int main(int argc, const char** argv)
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    unique_ptr<OscillatorData> oscData = make_unique<OscillatorData>();
    oscData->frequency = 440.f;
    oscData->amplitude = 0.5f;

    SDL_AudioDeviceID deviceId;
    {
        // I've taken the liberty of getting audio initialization taken care of:
        SDL_AudioSpec outputWant; // structure to request settings from the system
        memset(&outputWant, 0, sizeof(outputWant)); // zero out the mem block
        outputWant.freq = 44100;             // samples per second
        outputWant.format = AUDIO_F32SYS;    // want floating point values
        outputWant.channels = 1;             // mono signal (we can mess with stereo later)
        outputWant.samples = 4096;            // number of samples per audio block
        outputWant.callback = AudioCallback; // hand `AudioCallback` function to be called by audio subsystem
        outputWant.userdata = oscData.get(); // hand the pointer to our data so AudioCallback can see it

        SDL_AudioSpec outputGranted; // this structure is for settings we actually end up with
        memset(&outputGranted, 0, sizeof(outputWant));
        deviceId = SDL_OpenAudioDevice(nullptr, 0, &outputWant, &outputGranted, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

        if (deviceId == 0) {
            SDL_Log("Failed to open audio output: %s", SDL_GetError());
        }
        else {
            if (outputGranted.format != outputWant.format) {
                SDL_Log("We didn't get Float32 audio format...");
            }
            else {
                SDL_Log("Opened audio output successfully. Using driver %s", SDL_GetCurrentAudioDriver());
                SDL_Log("Samples: %d", outputGranted.samples);
                SDL_PauseAudioDevice(deviceId, 0);
            }
        }
    }

    cout << "Amplitude (0 to 1) or `q` to stop: ";
    string dummy;
    bool keepAsking = true;
    while (keepAsking)
    {
        cin >> dummy; // blocking!
        if (dummy == "q") break;
        float amplitude = clamp((float)atof(dummy.c_str()));
        cout << " -  Setting float to " << amplitude << '\n';
        oscData->amplitude = amplitude;

        cout << "Amplitude (0 to 1) or `q` to stop: ";
    }

    SDL_Log("Finished playback, cleaning up & stopping everything.");
    SDL_PauseAudioDevice(deviceId, 1);
    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();

    return 0;
}

