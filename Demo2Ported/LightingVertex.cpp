#include <stdio.h>
//#include "Display.h" // Fournit init_display, draw_loop, shutdown_display
#include "OurDisplay.h"
#include "graphics.h"
#include "HidPC.h"

// Variables d'état (sorties de la boucle locale)
const float SPEED = 0.05f;
const float FOCUS_DISTANCE = 10.0f + SPEED / 2.0f;

float cam_x = 0.0f, cam_y = 7.0f, cam_z = FOCUS_DISTANCE;
float focus_x = 0.0f, focus_y = 0.0f, focus_z = 0.0f;
float gx = 0.0f, gz = 0.0f;
unsigned char loudness = 0;

nn::hid::PadReader padReader;
nn::hid::TouchPanelReader tpReader;
nn::hid::AccelerometerReader accReader;

// Cette fonction sera appelée à chaque frame par la boucle PC
int demo_drawframe(void)
{
    nn::hid::PadStatus padStatus;
    nn::hid::TouchPanelStatus tpStatus;
    nn::hid::AccelerometerStatus accStatus;

    // 1. Lecture de notre faux HID
    padReader.ReadLatest(&padStatus);
    tpReader.ReadLatest(&tpStatus);
    accReader.ReadLatest(&accStatus);

    nn::hid::AccelerationFloat acceleration;
    accReader.ConvertToAcceleration(&acceleration, 1, &accStatus);

    if (acceleration.y <= 0) {
        gx = acceleration.x;
        gz = acceleration.z;
    }

    // 2. Mise à jour de la logique
    if (tpStatus.touch == 1) {
        focus_x += ((static_cast<float>(tpStatus.x) / 160.0f) - 1.0f) * SPEED;
        focus_y -= ((static_cast<float>(tpStatus.y) / 120.0f) - 1.0f) * SPEED;
    }
    else {
        focus_x += padReader.NormalizeStick(padStatus.stick.x) * SPEED;
        focus_y += padReader.NormalizeStick(padStatus.stick.y) * SPEED;
    }

    if (padStatus.hold & nn::hid::BUTTON_X) focus_y += SPEED;
    if (padStatus.hold & nn::hid::BUTTON_B) focus_y -= SPEED;
    if (padStatus.hold & nn::hid::BUTTON_A) focus_x += SPEED;
    if (padStatus.hold & nn::hid::BUTTON_Y) focus_x -= SPEED;
    if (padStatus.hold & nn::hid::BUTTON_UP) cam_z -= SPEED;
    if (padStatus.hold & nn::hid::BUTTON_DOWN) cam_z += SPEED;
    if (padStatus.hold & nn::hid::BUTTON_LEFT) cam_x -= SPEED;
    if (padStatus.hold & nn::hid::BUTTON_RIGHT) cam_x += SPEED;
    if (padStatus.hold & nn::hid::BUTTON_L) cam_y += SPEED;
    if (padStatus.hold & nn::hid::BUTTON_R) cam_y -= SPEED;

    if (padStatus.trigger & nn::hid::BUTTON_START) {
        cam_x = 0; cam_y = 7.f; cam_z = FOCUS_DISTANCE;
        focus_x = 0; focus_y = 0; focus_z = 0;
    }

    // Micro simulé : Appuie sur Espace pour faire souffler le vent sur la sphère
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        loudness = 255;
    }
    else {
        loudness = 0;
    }

    // 3. Appel au rendu
    DemoVec3 cam(cam_x, cam_y, cam_z);
    DemoVec3 focus(focus_x, focus_y, focus_z);
    DemoVec2 g(-gx, -gz);

    return DrawFrame(cam, focus, g, loudness);
}

int main(int argc, char* argv[])
{
    printf("Demo PC Port Start\n");

    nn::hid::Initialize();

    // Initialisation du contexte PC (Fenêtre + OpenGL)
    //init_display(240, 400, "Demo2 PC Port", demo_drawframe);

    // Fenêtre de 440x540 (permet 20px de marge autour de nos écrans)
    init_display(480, 400, "Demo2 PC Port - Dual Screen", demo_drawframe);

    // Initialisation de nos objets 3D et Shaders
    if (InitializeGraphics() >= 0)
    {
        // Lancement de la boucle événementielle PicaOnDesktop
        draw_loop();
    }

    FinalizeGraphics();
    shutdown_display();

    printf("Demo PC Port End\n");
    return 0;
}