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

const int TABLE_POW = 14;
const int TABLE_COMP = 32 - TABLE_POW;
const int TABLE_SIZE = 1 << 14;
const int LOGBASE2OFTABLEELEMENT = 2; // log2(4) for float = 2; lof2(8) for double = 3

typedef unsigned char byte;

using namespace std;

struct OscillatorData
{
    unsigned long phaseCurrent; // use overflow to loop around the wavetable
    long nextPhaseIncrement;    // we'll use this to "ease" between frequency values
    long phaseIncrement;        // since phase is non-floating point, phaseIncrement is also a long
    float nextAmplitude;        // we'll use this to "ease" between amplitude values
    float amplitude;            // amplitude is still a float (0..1)
    float* wavetable;           // we will store one iteration of our sine wave here
    // useful cached values:
    float oneOverSampleRate;    // 1.f / sampleRate
    float oneOverVectorSize;    // 1.f / blockSize (sampleArraySize)

    OscillatorData()
        : phaseCurrent(0), nextPhaseIncrement(0), phaseIncrement(0),
          nextAmplitude(0.f), amplitude(0.f), oneOverSampleRate(0.f), oneOverVectorSize(0.f)
    {
        wavetable = new float[TABLE_SIZE];
        float increment = TWO_PI / TABLE_SIZE;
        float phase = 0.f;

        //cout << "wavetable = np.array([";
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            wavetable[i] = sinf(phase);
            phase += increment;
            //cout << wavetable[i] << ",";
        }
        //cout << "0])\n\n"; // NOTE: NO MORE TABLE_SIZE + 1...
    }

    ~OscillatorData() { delete[] wavetable; cout << "cleaning up wavetable\n"; }

    void setFrequency(float frequency)
    {
        nextPhaseIncrement = (long)(frequency * ((TABLE_SIZE * oneOverSampleRate) * (1 << TABLE_COMP)));
    }
};

void AudioCallback(void* userData, Uint8* buffer, int bufferSizeBytes)
{
    // this is where all the audio computation takes place:
    OscillatorData* oscData = reinterpret_cast<OscillatorData*>(userData); // see main() for setup
    float* sampleArray = reinterpret_cast<float*>(buffer);
    size_t sampleArraySize = bufferSizeBytes / sizeof(float); // assuming it's one channel!

    // grab the values that we need from our oscData:
    const byte* const data = (byte*)oscData->wavetable;
    const float rate = oscData->oneOverVectorSize;
    const float astep = (float)((oscData->nextAmplitude - oscData->amplitude) * rate);
    const long  pstep = (long)((oscData->nextPhaseIncrement - oscData->phaseIncrement) * rate);

    float amplitude = oscData->amplitude;
    long  pc = oscData->phaseCurrent; 
    long  pi = oscData->phaseIncrement; 

    // compute the samples:
    for (size_t i = 0; i < sampleArraySize; ++i)
    {
        // compute the memory address using bit-shifting tricks; cast to float* and dereference.
        sampleArray[i] = amplitude *
            *((float*)(data + ((pc >> (TABLE_COMP - LOGBASE2OFTABLEELEMENT)) & ((TABLE_SIZE - 1) * sizeof(float)))));

        // increment values:
        pc += pi;
        pi += pstep;
        amplitude += astep;
    }

    // update state for enxt time:
    oscData->amplitude = oscData->nextAmplitude;
    oscData->phaseIncrement = oscData->nextPhaseIncrement;
    oscData->phaseCurrent = pc;
}

float clamp(float x) { return (x > 1) ? 1 : ((x < 0) ? 0 : x); }

int main(int argc, const char** argv)
{
    // initilization
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    // our data structure that will be passed to the audio callback:
    unique_ptr<OscillatorData> oscData = make_unique<OscillatorData>();

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
        oscData->oneOverSampleRate = 1.f / (float)outputObtained.freq;
        oscData->oneOverVectorSize = 1.f / (float)outputObtained.samples;

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
            oscData->setFrequency(frequency);
        }
        else if (dummy[0] == 'a')
        {
            float amplitude = clamp((float)atof(dummy.c_str() + 1));
            cout << " -  Setting amplitude to " << amplitude << '\n';
            oscData->nextAmplitude = amplitude;
        }
    }

    SDL_Log("Finished playback, cleaning up & stopping everything.");
    SDL_PauseAudioDevice(deviceId, 1);
    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();

    return 0;
}

