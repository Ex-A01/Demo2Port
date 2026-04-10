#include <nn.h>
#include <nn/math.h>
#include <nn/applet.h>
#include "graphics.h"
#include "mic.h"
#include "snd.h"
#include "applet.h"

// Heap used by the application
nn::fnd::ExpHeap s_AppHeap;

void StartDemo()
{
    // Description of Operations
    NN_LOG("BUTTON_UP,DOWN,LEFT,RIGHT,L,R : Change camera position\n");
    NN_LOG("AnalogStick, TouchPanel, BUTTON_A,B,X,Y : Change camera focus\n");
    NN_LOG("Accelerometer : Incline plane\n");
    NN_LOG("Mic : Blow ball\n");
    NN_LOG("BUTTON_START : Reset camera position and focus\n");

    nn::Result result;

    // Initializes the heap.
    s_AppHeap.Initialize(nn::os::GetDeviceMemoryAddress(), nn::os::GetDeviceMemorySize() );

    // fs initialization
    nn::fs::Initialize();

    const size_t ROMFS_BUFFER_SIZE = 1024 * 64;
    static char buffer[ROMFS_BUFFER_SIZE];
    NN_UTIL_PANIC_IF_FAILED(
        nn::fs::MountRom(16, 16, buffer, ROMFS_BUFFER_SIZE));

    // Initialize hid
    result = nn::hid::Initialize();
    NN_UTIL_PANIC_IF_FAILED(result);

    // Initialize sound
    InitializeSnd();

    // Prepare pad
    nn::hid::PadReader padReader;
    nn::hid::PadStatus padStatus;
    // Prepare touch panel
    nn::hid::TouchPanelReader tpReader;
    nn::hid::TouchPanelStatus tpStatus;
    // Prepare accelerometer
    nn::hid::AccelerometerReader accReader;
    nn::hid::AccelerometerStatus accStatus;
    accReader.SetSensitivity(0, nn::hid::MAX_OF_ACCELEROMETER_SENSITIVITY / 4);  // Set tracking

    // Initialize mic and start sampling
    StartMic();

    // Initialize graphics
    InitializeGraphics();

    // Parameters for rendering
    f32 SPEED = 0.05f;
    f32 FOCUS_DISTANCE = 10.0f + SPEED / 2;    // Shift a little so that the camera position and focus do not match
    // Camera position
    f32 cam_x = 0;
    f32 cam_y = 7.f;
    f32 cam_z = FOCUS_DISTANCE;
    // Camera viewpoint
    f32 focus_x = 0;
    f32 focus_y = 0;
    f32 focus_z = 0;
    // Accelerometer
    f32 gx = 0;
    f32 gz = 0;

    // Wait for HID input to stabilize
    nn::os::Thread::Sleep(nn::fnd::TimeSpan::FromSeconds(1));

    // After this, the application itself handles sleep
    TransitionHandler::EnableSleep();

    while(true)
    {
        // Get input
        padReader.ReadLatest(&padStatus);
        tpReader.ReadLatest(&tpStatus);
        accReader.ReadLatest(&accStatus);

        nn::hid::AccelerationFloat acceleration;
        accReader.ConvertToAcceleration(&acceleration, 1, &accStatus);
        if(acceleration.y > 0)
        {
            NN_LOG("Upside-down!\n");
        }
        else
        {
            gx = acceleration.x;
            gz = acceleration.z;
        }

        // Calculate parameters for rendering
        if(tpStatus.touch == 1)
        {
            focus_x += ((static_cast<f32>(tpStatus.x) / 160) - 1) * SPEED;
            focus_y -= ((static_cast<f32>(tpStatus.y) / 120) - 1) * SPEED;
        }
        else
        {
            focus_x += padReader.NormalizeStick(padStatus.stick.x) * SPEED;
            focus_y += padReader.NormalizeStick(padStatus.stick.y) * SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_X)
        {
            focus_y += SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_B)
        {
            focus_y -= SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_A)
        {
            focus_x += SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_Y)
        {
            focus_x -= SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_UP)
        {
            cam_z -= SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_DOWN)
        {
            cam_z += SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_LEFT)
        {
            cam_x -= SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_RIGHT)
        {
            cam_x += SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_L)
        {
            cam_y += SPEED;
        }

        if(padStatus.hold & nn::hid::BUTTON_R)
        {
            cam_y -= SPEED;
        }

        if(padStatus.trigger & nn::hid::BUTTON_START)
        {
            cam_x = 0;
            cam_y = 7.f;
            cam_z = FOCUS_DISTANCE;
            focus_x = 0;
            focus_y = 0;
            focus_z = 0;
        }

        // Microphone
        u8 loudness = GetLoudness();

        // Rendering
        nn::math::VEC3 cam(cam_x, cam_y, cam_z);
        nn::math::VEC3 focus(focus_x, focus_y, focus_z);
        nn::math::VEC2 g(-gx, -gz);
        DrawFrame(cam, focus, g, loudness);

        // Run sleep, HOME Menu, and POWER Menu transitions.
        TransitionHandler::Process();
        // Determine whether there has been an exit notification.
        if (TransitionHandler::IsExitRequired())
        {
            break;
        }
    }

    // Graphics termination processing
    FinalizeGraphics();

    // Mic termination processing
    EndMic();

    // Sound termination processing
    FinalizeSnd();

    // Destroy heap
    s_AppHeap.Finalize();
}

void nnMain()
{
    NN_LOG("Demo Start\n");

    // Initialize via the applet.
    // Sleep Mode is rejected automatically until TransitionHandler::EnableSleep is called.
    TransitionHandler::Initialize();
    // Here we need to check for exit notifications.
    if (!TransitionHandler::IsExitRequired())
    {
        StartDemo();
    }
    // Finalize via the applet.
    TransitionHandler::Finalize();

    NN_LOG("Demo2 End\n");

    // Finalize the application. The call to nn::applet::CloseApplication does not return.
    nn::applet::CloseApplication();
}

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
