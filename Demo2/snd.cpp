#include <nn.h>
#include <nn/os.h>
#include <nn/fnd.h>
#include <nn/math.h>

#include <nn/dsp.h>
#include <nn/snd.h>

#include "snd.h"

// Heap for application
extern nn::fnd::ExpHeap s_AppHeap;

namespace
{
    nn::snd::Voice* pVoice;
    s16* pMemory;
    nn::snd::WaveBuffer waveBuffer;
    const int nBufferSize = 2048;
    bool isPlaying = false;
    f32 pitch = 1.0f;
    f32 volume = 0.0f;

    // Generate sine wave
    void MakeSineWave(s16* p, s32 n)
    {
        for (int i = 0; i < n; i++)
        {
            p[i] = (32767.0f * nn::math::SinFIdx(4.0f*i));
        }
    }

    // Sound thread related
    const int SOUND_THREAD_PRIORITY = 2;
    const int SOUND_THREAD_STACK_SIZE = 1024;
    nn::os::Thread threadSound;
    bool threadSoundFlag;
    void SoundThreadFunc(uptr arg)
    {
        (void)arg;
        threadSoundFlag = true;

        // Specify stereo output
        nn::snd::SetSoundOutputMode(nn::snd::OUTPUT_MODE_STEREO);

        // Set the master volume.
        nn::snd::SetMasterVolume(1.0f);

        // Get and confirm contiguous memory region
        pMemory = reinterpret_cast<s16*>(s_AppHeap.Allocate(nBufferSize * sizeof(s16), 32));
        NN_TASSERT_(pMemory);
        MakeSineWave(pMemory, nBufferSize);
        nn::snd::FlushDataCache(reinterpret_cast<uptr>(pMemory), nBufferSize * sizeof(s16));

        pVoice = nn::snd::AllocVoice(128, NULL, NULL);
        NN_TASSERT_(pVoice);

        pVoice->SetChannelCount(1);
        pVoice->SetSampleFormat(nn::snd::SAMPLE_FORMAT_PCM16);

        // Set volume.
        nn::snd::MixParam mix;
        mix.mainBus[nn::snd::CHANNEL_INDEX_FRONT_LEFT ] = 0.707f; // Main volume (L)
        mix.mainBus[nn::snd::CHANNEL_INDEX_FRONT_RIGHT] = 0.707f; // Main volume (R)
        pVoice->SetMixParam(mix);
        pVoice->SetVolume(volume);

        // Set pitch.
        pVoice->SetSampleRate(NN_SND_HW_I2S_CLOCK_32KHZ);
        pVoice->SetPitch(pitch);

        nn::snd::InitializeWaveBuffer(&waveBuffer);
        waveBuffer.bufferAddress = pMemory;
        waveBuffer.sampleLength  = nn::snd::GetSampleLength(nBufferSize * sizeof(s16), nn::snd::SAMPLE_FORMAT_PCM16, 1);
        waveBuffer.loopFlag = true;
        pVoice->AppendWaveBuffer(&waveBuffer);

        pVoice->SetState(nn::snd::Voice::STATE_PLAY);

        while (threadSoundFlag)
        {
            nn::snd::WaitForDspSync();  // Wait to receive data from DSP.
            nn::snd::SendParameterToDsp();  // Send parameters to the DSP.
        }

        // End playback.
        pVoice->SetState(nn::snd::Voice::STATE_STOP);
        FreeVoice(pVoice);
    }
}

void InitializeSnd()
{
    nn::Result result;

    // Initialize DSP and SND libraries.
    result = nn::dsp::Initialize();
    NN_UTIL_PANIC_IF_FAILED(result);
    result = nn::dsp::LoadDefaultComponent();
    NN_UTIL_PANIC_IF_FAILED(result);
    result = nn::snd::Initialize();
    NN_UTIL_PANIC_IF_FAILED(result);

    // Start the sound thread (wait for DSP interrupt event)
    threadSound.StartUsingAutoStack(
        SoundThreadFunc,
        NULL,
        SOUND_THREAD_STACK_SIZE,
        SOUND_THREAD_PRIORITY
    );
}

void FinalizeSnd(void)
{
    // Destroy the sound thread.
    threadSoundFlag = false;
    threadSound.Join();
    threadSound.Finalize();

    // Finalize SND.
    nn::Result result = nn::snd::Finalize();
    NN_UTIL_PANIC_IF_FAILED(result);

    // Finalize DSP.
    result = nn::dsp::UnloadComponent();
    NN_UTIL_PANIC_IF_FAILED(result);
    nn::dsp::Finalize();

    // Frees the heap.
    s_AppHeap.Free(pMemory);
}

void PlaySound(void)
{
    if (!isPlaying)
    {
        pitch = 1.0f;
        volume = 1.0f;
        isPlaying = true;
    }
    else
    {
        pitch -= 0.02f;
        if (pitch < 0.0f)
        {
            pitch = 0.0f;
        }
        volume -= 0.03f;
        if (volume < 0.0f)
        {
            volume = 0.0f;
        }
    }
    pVoice->SetPitch(pitch);
    pVoice->SetVolume(volume);
}

void StopSound(void)
{
    isPlaying = false;

    volume = 0.0f;
    pVoice->SetVolume(volume);
}

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
