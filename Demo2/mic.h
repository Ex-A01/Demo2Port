#ifndef NN_SAMPLE_DEMOS_DEMO2_MIC_H_
#define NN_SAMPLE_DEMOS_DEMO2_MIC_H_

const int MIC_MEMORY_SIZE = nn::mic::BUFFER_UNITSIZE;
const u8 MIC_GAIN = 43;

void StartMic(void);
void EndMic(void);
u8 GetLoudness(void);

/* NN_SAMPLE_DEMOS_DEMO2_MIC_H_ */
#endif

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
