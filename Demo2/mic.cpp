#include <nn/mic.h>
#include "mic.h"

namespace
{
    char s_MicBuffer[MIC_MEMORY_SIZE] NN_ATTRIBUTE_ALIGN(nn::mic::BUFFER_ALIGNMENT_ADDRESS);
    bool s_RetryStart;
    u8 s_MicGain;
    u32 s_SamplingSize;

    void StartSampling()
    {
        nn::Result result = nn::mic::StartSampling(nn::mic::SAMPLING_TYPE_SIGNED_8BIT, nn::mic::SAMPLING_RATE_8180, 0, s_SamplingSize, true);
        if ( result.IsFailure() )
        {
            if ( result != nn::mic::CTR::ResultShellClose() )
            {
                NN_UTIL_PANIC_IF_FAILED(result);
            }
            s_RetryStart = true;
        }
        else
        {
            s_RetryStart = false;
        }
    }
}

void StartMic(void)
{
    // Initialize mic
    nn::Result result = nn::mic::Initialize();
    NN_UTIL_PANIC_IF_FAILED(result);

    // Sets buffer that the mic library uses
    result = nn::mic::SetBuffer(s_MicBuffer, sizeof(s_MicBuffer));
    NN_UTIL_PANIC_IF_FAILED(result);

    // Gets sampling buffer size
    result = nn::mic::GetSamplingBufferSize(&s_SamplingSize);
    NN_UTIL_PANIC_IF_FAILED(result);

    // Sets MIC amp
    result = nn::mic::GetAmpGain(&s_MicGain);
    NN_UTIL_PANIC_IF_FAILED(result);
    
    result = nn::mic::SetAmpGain(MIC_GAIN);
    NN_UTIL_PANIC_IF_FAILED(result);
    
    // MIC power on
    result = nn::mic::SetAmp(true);
    NN_UTIL_PANIC_IF_FAILED(result);

    // Start mic sampling
    StartSampling();
}

void EndMic(void)
{
    // MIC power off
    nn::Result result = nn::mic::SetAmp(false);
    NN_UTIL_PANIC_IF_FAILED(result);

    // Restore MIC amp
    result = nn::mic::SetAmpGain(s_MicGain);
    NN_UTIL_PANIC_IF_FAILED(result);

    // Even if stopping the sample fails here, it stops automatically with mic::Finalize
    result = nn::mic::StopSampling();
    if ( result.IsFailure() )
    {
        if ( result != nn::mic::CTR::ResultShellClose() )
        {
            NN_UTIL_PANIC_IF_FAILED(result);
        }
    }

    // Does not use the buffer set in the mic library
    result = nn::mic::ResetBuffer();
    NN_UTIL_PANIC_IF_FAILED(result);

    result = nn::mic::Finalize();
    NN_UTIL_PANIC_IF_FAILED(result);
}

u8 GetLoudness(void)
{
    if ( s_RetryStart )
    {
        StartSampling();
    }

    const u32 MAX_LOUDNESS = s_SamplingSize * 128;
    s8* buffer = reinterpret_cast<s8*>(s_MicBuffer);

    u32 sum = 0;
    for(int i = 0; i < s_SamplingSize; ++i)
    {
        s8 val = *(buffer + i);
        if(val < 0) sum -= val;
        else        sum += val;
    }

    return static_cast<u8>(sum * 255 / MAX_LOUDNESS);
}

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
