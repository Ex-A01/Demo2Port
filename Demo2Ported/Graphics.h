#pragma once
#ifndef GRAPHICS_H
#define GRAPHICS_H

// Nos types de remplacement
struct DemoVec2 {
    float x, y;
    DemoVec2() : x(0), y(0) {}
    DemoVec2(float x, float y) : x(x), y(y) {}
};
struct DemoVec3 {
    float x, y, z;
    DemoVec3() : x(0), y(0), z(0) {}
    DemoVec3(float x, float y, float z) : x(x), y(y), z(z) {}
    DemoVec3 operator+(const DemoVec3& o) const { return DemoVec3(x + o.x, y + o.y, z + o.z); }
    DemoVec3 operator-(const DemoVec3& o) const { return DemoVec3(x - o.x, y - o.y, z - o.z); }
    DemoVec3 operator*(float s) const { return DemoVec3(x * s, y * s, z * s); }
    DemoVec3& operator+=(const DemoVec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};

int InitializeGraphics(void);
int FinalizeGraphics(void);
int DrawFrame(DemoVec3 v_cam, DemoVec3 v_focus, DemoVec2 v_acc, unsigned char loudness);

#endif