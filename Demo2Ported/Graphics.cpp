#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <math.h>
#include <stdlib.h>

#include "Display.h"
#include "Vecalg.h"
#include "File.h"
#include "graphics.h"

inline float Vec3Dot(const DemoVec3& a, const DemoVec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float Vec3Len(const DemoVec3& a) { return sqrtf(Vec3Dot(a, a)); }
inline void Vec3Normalize(DemoVec3* out, const DemoVec3* in) {
    float l = Vec3Len(*in);
    *out = l > 0.0f ? (*in) * (1.0f / l) : *in;
}

#define APP_NAME "Demo2Port"
#define WIDTH 240
#define HEIGHT 400
#define DMP_PI (3.1415926f)

/* program id & shader id */
GLuint s_PgID;
GLuint s_ShID;

/* OpenGLES1.1 vertex lighting specular shininess */
#define SPEC_SHININESS  32.f
#define STEP 6.f

enum {
    OBJECT_SPHERE,
    OBJECT_PLANE,
    OBJECT_COUNT
};

static struct tagObject {
    GLuint id[OBJECT_COUNT];
    GLuint idxId[OBJECT_COUNT];
} s_Object;

static struct tagObjectInfo {
    GLushort idxcnt[OBJECT_COUNT];
    GLushort vtxcnt[OBJECT_COUNT];
} s_ObjectInfo;

#define ROW_NUM     (50)
#define COL_NUM     (50)
#define deltaROW    (DMP_PI / (ROW_NUM - 1))
#define deltaCOL    (2 * DMP_PI / (COL_NUM - 1))

struct tagVertex {
    GLfloat pos[ROW_NUM * COL_NUM][3];
    GLfloat nor[ROW_NUM * COL_NUM][3];
} s_Vtx;
GLushort s_Idx[COL_NUM * (ROW_NUM - 1) * 2];

// Helper sound stubs (puisque snd.cpp n'est pas encore porté)
void PlaySoundPC() {}
void StopSoundPC() {}

/* Load sphere object */
static void LoadSphere(void)
{
    for (int row = 0; row < ROW_NUM; row++) {
        for (int col = 0; col < COL_NUM; col++) {
            s_Vtx.pos[row * COL_NUM + col][0] = 0.5f * (GLfloat)sinf(deltaROW * row) * cosf(deltaCOL * col);
            s_Vtx.pos[row * COL_NUM + col][1] = 0.5f * (GLfloat)cosf(deltaROW * row);
            s_Vtx.pos[row * COL_NUM + col][2] = 0.5f * (GLfloat)sinf(deltaROW * row) * sinf(deltaCOL * col);

            s_Vtx.nor[row * COL_NUM + col][0] = 0.5f * (GLfloat)sinf(deltaROW * row) * cosf(deltaCOL * col);
            s_Vtx.nor[row * COL_NUM + col][1] = 0.5f * (GLfloat)cosf(deltaROW * row);
            s_Vtx.nor[row * COL_NUM + col][2] = 0.5f * (GLfloat)sinf(deltaROW * row) * sinf(deltaCOL * col);
        }
    }

    for (int i = 0, row = 0; row < ROW_NUM - 1; row++) {
#define __INDEX(ROW, COL)   ((ROW) * COL_NUM + (COL))
        for (int col = 0; col < COL_NUM; col++) {
            s_Idx[i++] = __INDEX(row + 1, col);
            s_Idx[i++] = __INDEX(row, col);
        }
#undef __INDEX
    }

    s_ObjectInfo.idxcnt[OBJECT_SPHERE] = COL_NUM * (ROW_NUM - 1) * 2;
    s_ObjectInfo.vtxcnt[OBJECT_SPHERE] = ROW_NUM * COL_NUM;

    glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[OBJECT_SPHERE]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_Vtx), &s_Vtx, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[OBJECT_SPHERE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_Idx), s_Idx, GL_STATIC_DRAW);
}

/* Load plane object */
static void LoadPlane(void)
{
    struct {
        GLfloat pos[4][3];
        GLfloat nor[4][3];
    } vtx;
    GLushort idx[4] = { 0, 1, 2, 3 };

    vtx.pos[0][0] = +4.0f; vtx.pos[0][1] = 0.f; vtx.pos[0][2] = +4.0f;
    vtx.pos[1][0] = +4.0f; vtx.pos[1][1] = 0.f; vtx.pos[1][2] = -4.0f;
    vtx.pos[2][0] = -4.0f; vtx.pos[2][1] = 0.f; vtx.pos[2][2] = +4.0f;
    vtx.pos[3][0] = -4.0f; vtx.pos[3][1] = 0.f; vtx.pos[3][2] = -4.0f;

    for (int i = 0; i < 4; i++) {
        vtx.nor[i][0] = 0.f;
        vtx.nor[i][1] = 1.f;
        vtx.nor[i][2] = 0.f;
    }

    s_ObjectInfo.idxcnt[OBJECT_PLANE] = 4;
    s_ObjectInfo.vtxcnt[OBJECT_PLANE] = 4;

    glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[OBJECT_PLANE]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx.pos[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[OBJECT_PLANE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

static void LoadObjects(void)
{
    glGenBuffers(OBJECT_COUNT * 2, (GLuint*)&s_Object);
    LoadSphere();
    LoadPlane();
}

static void UpdateParams(DemoVec3* v_pos, DemoVec2* v_degSphere, DemoVec2* v_deg,
    const DemoVec3& v_cam, const DemoVec3& v_focus,
    const DemoVec2& v_acc, unsigned char loudness)
{
    const float SPEED = 0.05f;
    static DemoVec3 v_v(0, 0, 0);
    DemoVec3 v_g(v_acc.x, -1.f, v_acc.y);

    if (v_g.x > 1.f) v_g.x = 1.f; else if (v_g.x < -1.f) v_g.x = -1.f;
    if (v_g.z > 1.f) v_g.z = 1.f; else if (v_g.z < -1.f) v_g.z = -1.f;

    // asin in radians to degrees
    v_deg->x = asinf(v_g.x) * 180.0f / DMP_PI;
    v_deg->y = asinf(v_g.z) * 180.0f / DMP_PI;

    const float MIC_STRENGTH = 0.25f;
    float micAcc = loudness * MIC_STRENGTH;
    DemoVec3 v_f = v_focus - v_cam;
    DemoVec3 v_s = *v_pos - v_cam;

    if (Vec3Dot(v_f, v_s) > 0) {
        Vec3Normalize(&v_f, &v_f);
        Vec3Normalize(&v_s, &v_s);
        micAcc *= Vec3Dot(v_f, v_s) / Vec3Len(*v_pos - v_cam);
        v_s = v_s * micAcc;
    }
    else {
        v_s = v_s * 0;
    }

    if (v_pos->x > -4.f && v_pos->x < 4.f && v_pos->z > -4.f && v_pos->z < 4.f) {
        v_g.y = 0;
        v_s.y = 0;
        v_v += (v_g + v_s) * SPEED;
        *v_pos += v_v;
        v_degSphere->x = v_deg->x;
        v_degSphere->y = v_deg->y;
    }
    else if (v_pos->y > -50.f) {
        v_v.y += v_g.y * SPEED;
        *v_pos += v_v;
        PlaySoundPC();
    }
    else {
        v_pos->x = 0.f; v_pos->y = 0.f; v_pos->z = 0.f;
        v_v.x = 0.f; v_v.y = 0.f; v_v.z = 0.f;
        StopSoundPC();
    }
}

void DrawObjects(mat4_t proj, mat4_t view, const DemoVec3& v_pos,
    const DemoVec2& v_degSphere, const DemoVec2& v_deg, float posYforSphere)
{
    glUseProgram(s_PgID);

    float m[16];
    proj.toFloatArr(m);
    glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uProjection"), 1, GL_FALSE, m);

    // Light Position
    GLfloat lpos[] = { 3.f, 3.f, 0.f, 1.f };
    vec4_t p(lpos);
    p = view * p;
    float lpos2[] = { p[0], p[1], p[2], p[3] };
    glUniform4fv(glGetUniformLocation(s_PgID, "uLightPos"), 1, lpos2);

    for (int i = 0; i < OBJECT_COUNT; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[i]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(GLfloat) * 3 * s_ObjectInfo.vtxcnt[i]));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[i]);

        if (i == OBJECT_SPHERE) {
            mat4_t mv2 = view * mat4_t::translate(0.f, v_pos.y, 0.f)
                * mat4_t::rotate(v_degSphere.y, 1.f, 0.f, 0.f)
                * mat4_t::rotate(-v_degSphere.x, 0.f, 0.f, 1.f)
                * mat4_t::translate(v_pos.x, posYforSphere, v_pos.z);
            mv2.toFloatArr(m);
            glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uModelView"), 1, GL_FALSE, m);
        }
        else if (i == OBJECT_PLANE) {
            mat4_t mv2 = view * mat4_t::rotate(v_deg.y, 1.f, 0.f, 0.f)
                * mat4_t::rotate(-v_deg.x, 0.f, 0.f, 1.f);
            mv2.toFloatArr(m);
            glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uModelView"), 1, GL_FALSE, m);
        }

        glDrawElements(GL_TRIANGLE_STRIP, s_ObjectInfo.idxcnt[i], GL_UNSIGNED_SHORT, (GLvoid*)0);
    }
    glFinish();
}

int DrawFrame(DemoVec3 v_cam, DemoVec3 v_focus, DemoVec2 v_acc, unsigned char loudness)
{
    const float posYforSphere = 0.5f;
    static DemoVec3 v_pos(0, 0, 0);
    static DemoVec2 v_degSphere = { 0, 0 };
    DemoVec2 v_deg = { 0, 0 };

    UpdateParams(&v_pos, &v_degSphere, &v_deg, v_cam, v_focus, v_acc, loudness);

    // ==========================================
    // 0. FOND GLOBAL (Le contour violet façon Citra)
    // ==========================================
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.3f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ==========================================
    // 1. GESTION DU FOND (ÉCRAN DU BAS)
    // ==========================================
    // Cible PC : X=40, Y=0, Largeur=320, Hauteur=240
    // Coordonnées inversées pour DMPGL : X=0, Y=40, Largeur=240, Hauteur=320
    glViewport(0, 40, 240, 320);
    
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 40, 240, 320); 
    
    glClearColor(0.2f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* * Ici viendra l'affichage 2D tactile plus tard * */

    // ==========================================
    // 2. GESTION DE L'ÉCRAN DU HAUT (SCÈNE 3D)
    // ==========================================
    // Cible PC : X=0, Y=240, Largeur=400, Hauteur=240
    // Coordonnées inversées pour DMPGL : X=240, Y=0, Largeur=240, Hauteur=400
    glViewport(240, 0, 240, 400);
    glScissor(240, 0, 240, 400);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_SCISSOR_TEST);

    // ==========================================
    // 3. MATRICES (RETOUR AU COMPORTEMENT 3DS)
    // ==========================================
    // Comme DMPGL travaille en portrait, on doit remettre le ratio sur l'axe Y
    float aspect = 400.0f / 240.0f;
    mat4_t proj = mat4_t::frustum(-0.07f, 0.07f, -0.07f * aspect, 0.07f * aspect, 0.2f, 200.f);

    // On remet la rotation de -90° pour tourner la scène 3D sur le côté,
    // ce qui compensera la rotation imposée par l'émulateur !
    mat4_t view = mat4_t::rotate(-90.f, 0.f, 0.f, 1.f) * mat4_t::lookAt(v_cam.x, v_cam.y, v_cam.z, v_focus.x, v_focus.y, v_focus.z, 0.f, 1.f, 0.f);

    DrawObjects(proj, view, v_pos, v_degSphere, v_deg, posYforSphere);

    swap_buffer();

    return !glGetError();
}

int InitializeGraphics(void)
{
    s_PgID = glCreateProgram();
    s_ShID = glCreateShader(GL_VERTEX_SHADER);

    int fsize;
    unsigned char* binary = ReadFile(FILE_APP_ROOT "shader.bin", &fsize);
    if (!binary) return -1;

    glShaderBinary(1, &s_ShID, GL_PLATFORM_BINARY_DMP, binary, fsize);
    free(binary);

    glAttachShader(s_PgID, s_ShID);
    glAttachShader(s_PgID, GL_DMP_FRAGMENT_SHADER_DMP);

    glBindAttribLocation(s_PgID, 0, "aPosition");
    glBindAttribLocation(s_PgID, 1, "aNormal");

    glLinkProgram(s_PgID);
    glValidateProgram(s_PgID);
    glUseProgram(s_PgID);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    LoadObjects();

    GLfloat ldif[] = { 1.f, 1.f, 1.f, 1.f };
    GLfloat lspc[] = { 1.f, 1.f, 1.f, 1.f };
    GLfloat lamb[] = { 0.f, 0.f, 0.f, 1.f };
    GLfloat lmamb[] = { 0.2f, 0.2f, 0.f, 1.f };

    GLfloat mspc[] = { 1.f, 1.f, 1.f, 1.f };
    GLfloat mdif[] = { 0.8f, 0.8f, 0.f, 1.f };
    GLfloat mamb[] = { 0.8f, 0.f, 0.f, 1.f };

    vec4_t amb = vec4_t(lamb).vmul(vec4_t(mamb)) + vec4_t(lmamb).vmul(vec4_t(mamb));
    vec4_t dif = vec4_t(ldif).vmul(vec4_t(mdif));
    vec4_t spc = vec4_t(lspc).vmul(vec4_t(mspc));
    GLfloat a[] = { amb[0], amb[1], amb[2], amb[3] };
    GLfloat d[] = { dif[0], dif[1], dif[2], dif[3] };
    GLfloat s[] = { spc[0], spc[1], spc[2], spc[3] };

    glUniform4fv(glGetUniformLocation(s_PgID, "uDiff"), 1, d);
    glUniform4fv(glGetUniformLocation(s_PgID, "uAmb"), 1, a);
    glUniform4fv(glGetUniformLocation(s_PgID, "uSpec"), 1, s);
    glUniform1f(glGetUniformLocation(s_PgID, "uMatShiniess"), SPEC_SHININESS);

    glUniform3i(glGetUniformLocation(s_PgID, "dmp_TexEnv[0].srcRgb"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);
    glUniform3i(glGetUniformLocation(s_PgID, "dmp_TexEnv[0].srcAlpha"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);

    return 0;
}

int FinalizeGraphics(void)
{
    return 0;
}