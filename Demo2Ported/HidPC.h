#pragma once
#ifndef HID_PC_H
#define HID_PC_H

#include <windows.h> // Pour lire les inputs clavier/souris sur PC

namespace nn {
    namespace hid {

        const unsigned int BUTTON_A = 1 << 0;
        const unsigned int BUTTON_B = 1 << 1;
        const unsigned int BUTTON_X = 1 << 10;
        const unsigned int BUTTON_Y = 1 << 11;
        const unsigned int BUTTON_LEFT = 1 << 5;
        const unsigned int BUTTON_RIGHT = 1 << 4;
        const unsigned int BUTTON_UP = 1 << 6;
        const unsigned int BUTTON_DOWN = 1 << 7;
        const unsigned int BUTTON_L = 1 << 9;
        const unsigned int BUTTON_R = 1 << 8;
        const unsigned int BUTTON_START = 1 << 3;
        const int MAX_OF_ACCELEROMETER_SENSITIVITY = 100;

        struct PadStatus {
            unsigned int hold;
            unsigned int trigger;
            struct { float x, y; } stick;
        };

        class PadReader {
            unsigned int last_hold = 0;
        public:
            void ReadLatest(PadStatus* status) {
                status->hold = 0;

                // D-PAD = Flèches directionnelles
                if (GetAsyncKeyState(VK_RIGHT) & 0x8000) status->hold |= BUTTON_RIGHT;
                if (GetAsyncKeyState(VK_LEFT) & 0x8000) status->hold |= BUTTON_LEFT;
                if (GetAsyncKeyState(VK_UP) & 0x8000) status->hold |= BUTTON_UP;
                if (GetAsyncKeyState(VK_DOWN) & 0x8000) status->hold |= BUTTON_DOWN;

                // ABXY = Touches D, S, W, A
                if (GetAsyncKeyState('D') & 0x8000) status->hold |= BUTTON_A;
                if (GetAsyncKeyState('S') & 0x8000) status->hold |= BUTTON_B;
                if (GetAsyncKeyState('W') & 0x8000) status->hold |= BUTTON_X;
                if (GetAsyncKeyState('A') & 0x8000) status->hold |= BUTTON_Y;

                // L/R = Q et E
                if (GetAsyncKeyState('Q') & 0x8000) status->hold |= BUTTON_L;
                if (GetAsyncKeyState('E') & 0x8000) status->hold |= BUTTON_R;

                // START = Entrée
                if (GetAsyncKeyState(VK_RETURN) & 0x8000) status->hold |= BUTTON_START;

                status->trigger = (status->hold ^ last_hold) & status->hold;
                last_hold = status->hold;

                // On laisse le stick analogique à 0 pour l'instant
                status->stick.x = 0.0f;
                status->stick.y = 0.0f;
            }
            float NormalizeStick(float v) { return v; }
        };

        struct TouchPanelStatus {
            int touch;
            int x, y;
        };

        class TouchPanelReader {
        public:
            void ReadLatest(TouchPanelStatus* status) {
                // Le clic gauche simule le toucher sur l'écran
                status->touch = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
                if (status->touch) {
                    POINT p;
                    GetCursorPos(&p); // Note: ce sont les coords de l'écran, pas de la fenêtre, mais ça suffira pour tester le focus
                    status->x = p.x % 320;
                    status->y = p.y % 240;
                }
            }
        };

        struct AccelerationFloat { float x, y, z; };
        struct AccelerometerStatus {};

        class AccelerometerReader {
        public:
            void SetSensitivity(int, int) {}
            void ReadLatest(AccelerometerStatus*) {}
            void ConvertToAcceleration(AccelerationFloat* acc, int, AccelerometerStatus*) {
                // On simule l'inclinaison (accéléromètre) avec le pavé numérique (8, 4, 6, 2)
                acc->x = 0.0f; acc->y = -1.0f; acc->z = 0.0f;
                if (GetAsyncKeyState(VK_NUMPAD6) & 0x8000) acc->x = 0.5f;
                if (GetAsyncKeyState(VK_NUMPAD4) & 0x8000) acc->x = -0.5f;
                if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) acc->z = 0.5f;
                if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) acc->z = -0.5f;
            }
        };

        inline int Initialize() { return 0; }

    } // namespace hid
} // namespace nn

#endif // HID_PC_H