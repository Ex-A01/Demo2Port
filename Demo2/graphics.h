#include <nn/math.h>

#define APP_NAME "Demo2"
#define WIDTH 240
#define HEIGHT 400

#define DMP_PI  (3.1415926f)

int DrawFrame(nn::math::VEC3 v_cam, nn::math::VEC3 v_focus, nn::math::VEC2 v_acc, u8 loudness);

/* initialization */
int InitializeGraphics(void);
/* finalization */
int FinalizeGraphics(void);

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
