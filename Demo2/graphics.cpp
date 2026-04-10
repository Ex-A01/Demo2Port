#include <nn.h>
#include <nn/math.h>
#include <nn/fs.h>
#include <nn/ulcd.h>
#include "graphics.h"
#include "snd.h"

#include "demo.h"

/* program id */
GLuint s_PgID;

/* shader id */
GLuint s_ShID;

/* OpenGLES1.1 vertex lighting specular shininess */
#define SPEC_SHININESS  32.f
#define STEP 6.f

/* object names */
enum {
    OBJECT_SPHERE,  /* sphere */
    OBJECT_PLANE,   /* plane */
    OBJECT_COUNT    /* object count */
};

/* buffer object ID */
static struct tagObject
{
    GLuint id[OBJECT_COUNT];
    GLuint idxId[OBJECT_COUNT];
} s_Object;

/* buffer object information */
static struct tagObjectInfo
{
    GLushort idxcnt[OBJECT_COUNT];
    GLushort vtxcnt[OBJECT_COUNT];
} s_ObjectInfo;


#define ROW_NUM     (50)    /* NUM in ROW */
#define COL_NUM     (50)    /* NUM in COLUMN */
#define deltaROW    (DMP_PI / (ROW_NUM - 1))
#define deltaCOL    (2 * DMP_PI / (COL_NUM - 1))

struct tagVertex{
    GLfloat pos[ROW_NUM * COL_NUM][3];
    GLfloat nor[ROW_NUM * COL_NUM][3];
} s_Vtx;
GLushort s_Idx[COL_NUM * (ROW_NUM - 1) * 2];

/* ExpHeap for app. */
extern nn::fnd::ExpHeap s_AppHeap;
uptr s_HeapForGx;
const u32 s_GxHeapSize = 0x800000;

/* Render system */
demo::RenderSystemExt       s_RenderSystem;
void* s_pShader;

/* 3D view */
nn::ulcd::CTR::StereoCamera s_StereoCamera;
const f32 DEMO2_3D_DEPTH_LEVEL = 10.0f;
const f32 DEMO2_3D_FACTOR = 1.0f;


/* load sphere object */
static void LoadSphere(void)
{
    /* vertex array */
    for(int row = 0; row < ROW_NUM; row++)
    {
        for(int col = 0; col < COL_NUM; col++)
        {
            /* position */
            s_Vtx.pos[row * COL_NUM + col][0] =0.5f * (GLfloat)nn::math::SinRad(deltaROW * row) * nn::math::CosRad(deltaCOL * col);
            s_Vtx.pos[row * COL_NUM + col][1] =0.5f * (GLfloat)nn::math::CosRad(deltaROW * row);
            s_Vtx.pos[row * COL_NUM + col][2] =0.5f * (GLfloat)nn::math::SinRad(deltaROW * row) * nn::math::SinRad(deltaCOL * col);
            /* normal */
            s_Vtx.nor[row * COL_NUM + col][0] =0.5f * (GLfloat)nn::math::SinRad(deltaROW * row) * nn::math::CosRad(deltaCOL * col);
            s_Vtx.nor[row * COL_NUM + col][1] =0.5f * (GLfloat)nn::math::CosRad(deltaROW * row);
            s_Vtx.nor[row * COL_NUM + col][2] =0.5f * (GLfloat)nn::math::SinRad(deltaROW * row) * nn::math::SinRad(deltaCOL * col);
        }
    }

    /* index array */
    for(int i = 0, row = 0; row < ROW_NUM - 1; row++)
    {
    #define __INDEX(ROW, COL)   ((ROW) * COL_NUM + (COL))
        for(int col = 0; col < COL_NUM; col++)
        {
            s_Idx[i++] = __INDEX(row + 1, col);
            s_Idx[i++] = __INDEX(row, col);
        }
    #undef __INDEX
    }
    /* count */
    s_ObjectInfo.idxcnt[OBJECT_SPHERE] = COL_NUM * (ROW_NUM - 1) * 2;
    s_ObjectInfo.vtxcnt[OBJECT_SPHERE] = ROW_NUM * COL_NUM;

    /* load vertex array and index array */
    glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[OBJECT_SPHERE]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_Vtx), &s_Vtx, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[OBJECT_SPHERE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_Idx), s_Idx, GL_STATIC_DRAW);


#undef LONG_NUM
#undef LATI_NUM
#undef deltaROW
#undef deltaCOL
}

/* load plane object */
static void LoadPlane(void)
{
    struct tagVertex{
        GLfloat pos[4][3];
        GLfloat nor[4][3];
    } vtx;
    GLushort idx[4] ={0, 1, 2, 3};

    /* vertex array */
    vtx.pos[0][0] = +4.0f;
    vtx.pos[0][1] = 0.f;
    vtx.pos[0][2] = +4.0f;

    vtx.pos[1][0] = +4.0f;
    vtx.pos[1][1] = 0.f;
    vtx.pos[1][2] = -4.0f;

    vtx.pos[2][0] = -4.0f;
    vtx.pos[2][1] = 0.f;
    vtx.pos[2][2] = +4.0f;

    vtx.pos[3][0] = -4.0f;
    vtx.pos[3][1] = 0.f;
    vtx.pos[3][2] = -4.0f;

    for(int i = 0; i < 4; i++)
    {
        vtx.nor[i][0] = 0.f;
        vtx.nor[i][1] = 1.f;
        vtx.nor[i][2] = 0.f;
    }

    /* count */
    s_ObjectInfo.idxcnt[OBJECT_PLANE] = 4;
    s_ObjectInfo.vtxcnt[OBJECT_PLANE] = 4;

    /* load vertex array and index array */
    glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[OBJECT_PLANE]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx.pos[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[OBJECT_PLANE]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

/* load objects */
static void LoadObjects(void)
{
    glGenBuffers(OBJECT_COUNT * 2, (GLuint*)&s_Object);

    LoadSphere();
    LoadPlane();
}

static void UpdateParams(
                 nn::math::VEC3* v_pos,         // Sphere position (output)
                 nn::math::VEC2* v_degSphere,   // Slope of plane when sphere is on a plane (output)
                 nn::math::VEC2* v_deg,         // Plane slope (output)
                 const nn::math::VEC3 v_cam,    // Camera position
                 const nn::math::VEC3 v_focus,  // Focus position
                 const nn::math::VEC2 v_acc,    // Accelerometer
                 const u8 loudness)             // Mic volume
{
    /* 
     * About Coordinates (initial position)
     * x: To the right direction on the horizontal plane
     * y: Up in the vertical direction
     * z: Down direction on the horizontal plane
     */
    const f32 SPEED = 0.05f;            // Parameters for moving speed of the sphere
    static nn::math::VEC3 v_v(0, 0, 0);           // Sphere speed
    nn::math::VEC3 v_g(v_acc.x, -1.f, v_acc.y);   // Gravitational acceleration

    // Calculate the plane's incline
    if(v_g.x > 1.f)         v_g.x =  1.f;
    else if(v_g.x < -1.f)   v_g.x = -1.f;
    if(v_g.z > 1.f)         v_g.z =  1.f;
    else if(v_g.z < -1.f)   v_g.z = -1.f;
    v_deg->x = nn::math::AsinDeg(v_g.x);          // Incline of x-axis
    v_deg->y = nn::math::AsinDeg(v_g.z);          // Incline of z-axis

    // Calculate acceleration with the microphone
    const f32 MIC_STRENGTH = 0.25f;     // Parameter for the effect from the microphone volume
    f32 micAcc = loudness * MIC_STRENGTH;    
    nn::math::VEC3 v_f(v_focus - v_cam);          // Focus vector from camera
    nn::math::VEC3 v_s(*v_pos - v_cam);           // Vector from camera to sphere
    if( nn::math::VEC3Dot(&v_f, &v_s) > 0 )
    {
        // When the direction from the camera to the focus and to the sphere are the same
        // Vector normalization
        nn::math::VEC3Normalize(&v_f, &v_f);
        nn::math::VEC3Normalize(&v_s, &v_s);
        // Calculate effect on sphere by microphone
        micAcc *= nn::math::VEC3Dot(&v_f, &v_s) / nn::math::VEC3Len(*v_pos - v_cam);
        nn::math::VEC3Scale(&v_s, &v_s, micAcc);
    }
    else
    {
        // When the direction from the camera to the focus and to the sphere are the opposite
        nn::math::VEC3Scale(&v_s, &v_s, 0);
    }

    if(v_pos->x > -4.f && v_pos->x < 4.f && v_pos->z > -4.f && v_pos->z < 4.f)
    {
        // When the sphere is on the plane, ignore speed in y-axis direction
        // 
        v_g.y = 0;
        v_s.y = 0;
        v_v += (v_g + v_s) * SPEED;
        *v_pos += v_v;
        v_degSphere->x = v_deg->x;
        v_degSphere->y = v_deg->y;
    }
    else if( v_pos->y > -50.f )
    {
        // When the sphere is falling
        v_v.y += v_g.y * SPEED;
        *v_pos += v_v;

        PlaySound();
    }
    else
    {
        // Reset sphere position
        v_pos->x = 0.f;
        v_pos->y = 0.f;
        v_pos->z = 0.f;
        v_v.x   = 0.f;
        v_v.y   = 0.f;
        v_v.z   = 0.f;

        StopSound();
    }
}

void DrawObjects(
    const nn::math::Matrix44& proj,
    const nn::math::Matrix34& view,
    const nn::math::VEC3 v_pos,
    const nn::math::VEC2 v_degSphere,
    const nn::math::VEC2 v_deg,
    f32   posYforSphere
)
{
    glUseProgram(s_PgID);
    glBindAttribLocation(s_PgID, 0, "aPosition");
    glBindAttribLocation(s_PgID, 1, "aNormal");

    glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uProjection"), 1, GL_TRUE, static_cast<const f32*>(proj));
    
    /* set light position */
    GLfloat lpos[] = {3.f, 3.f, 0.f, 1.f};      /* Light position */
    /* constant light position is transformed from object-space to eye-space */
    /* Lighting calculations in the vertex shader assume view coordinates system.
     * For this reason, the light position is converted to view coordinates system. The shader does not perform conversion.
     * This is because common values are used for lights in all vertex processing.
     * mv is the view matrix, and multiplication by this matrix converts to view coordinates system.*/
    nn::math::Vector4 p(lpos);
    nn::math::Vector4 mvv0(view.m[0]);
    nn::math::Vector4 mvv1(view.m[1]);
    nn::math::Vector4 mvv2(view.m[2]);
    nn::math::Vector4 lpos2( VEC4Dot(&mvv0, &p), VEC4Dot(&mvv1, &p), VEC4Dot(&mvv2, &p), 1.f);

    glUniform4fv(glGetUniformLocation(s_PgID, "uLightPos"), 1, static_cast<f32*>(lpos2));

    /* draw objects */
    for (int i = 0; i < OBJECT_COUNT; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, s_Object.id[i]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(GLfloat) * 3 * s_ObjectInfo.vtxcnt[i]));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Object.idxId[i]);

        if (i == OBJECT_SPHERE) /* for sphere */
        {
            nn::math::Matrix34 arr[3];
            nn::math::Matrix34 mv2;
            nn::math::Vector3 trans(v_pos.x, posYforSphere, v_pos.z);
            nn::math::Vector3 fall(0.f, v_pos.y, 0.f);
            nn::math::MTX34Translate(&arr[0], &trans);
            nn::math::MTX34RotXYZDeg(&arr[1], v_degSphere.y, 0.f, -v_degSphere.x);
            nn::math::MTX34Translate(&arr[2], &fall);
            nn::math::MTX34Mult(&mv2, &view, &arr[2]);
            nn::math::MTX34Mult(&mv2, &mv2, &arr[1]);
            nn::math::MTX34Mult(&mv2, &mv2, &arr[0]);
            nn::math::Matrix44 tmp2(mv2);
            glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uModelView"), 1, GL_TRUE, static_cast<f32*>(tmp2));
        }
        else if(i == OBJECT_PLANE) /* for plane */
        {
            nn::math::Matrix34 arr;
            nn::math::Matrix34 mv2;
            nn::math::MTX34RotXYZDeg(&arr, v_deg.y, 0.f, -v_deg.x);
            nn::math::MTX34Mult(&mv2, &view, &arr);
            nn::math::Matrix44 tmp2(mv2);
            glUniformMatrix4fv(glGetUniformLocation(s_PgID, "uModelView"), 1, GL_TRUE, static_cast<f32*>(tmp2));
        }

        glDrawElements(GL_TRIANGLE_STRIP, s_ObjectInfo.idxcnt[i], GL_UNSIGNED_SHORT,(GLvoid*)0);
    }
    glFinish();
}

int DrawFrame(nn::math::VEC3 v_cam, nn::math::VEC3 v_focus, nn::math::VEC2 v_acc, u8 loudness)
{
    /* 
     * About Coordinates (initial position)
     * x: To the right direction on the horizontal plane
     * y: Up in the vertical direction
     * z: Down direction on the horizontal plane
     */
    const f32 posYforSphere = 0.5f;               // Sphere y-axis position correction parameter
    static nn::math::VEC3 v_pos(0, 0, 0);         // Sphere position
    static nn::math::VEC2 v_degSphere(0, 0);      // Slope of plane when sphere is on a plane
    nn::math::VEC2 v_deg(0, 0);                   // Plane slope

    // Update render parameters
    UpdateParams(&v_pos, &v_degSphere, &v_deg, v_cam, v_focus, v_acc, loudness);


    // Lower screen display
    s_RenderSystem.SetRenderTarget(NN_GX_DISPLAY1);
    s_RenderSystem.Clear();

    // Display current time
    nn::fnd::DateTime now = nn::fnd::DateTime::GetNow();
    s_RenderSystem.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    s_RenderSystem.DrawText(0, 0, "CTR demo2");
    s_RenderSystem.DrawText(0, 16, "%04d/%02d/%02d %02d:%02d:%02d", now.GetYear(), now.GetMonth(), now.GetDay(), now.GetHour(), now.GetMinute(), now.GetSecond());

    s_RenderSystem.Transfer();
    s_RenderSystem.SwapBuffers();


    /* Projection settings */
    s_StereoCamera.SetBaseFrustum(-0.07f * HEIGHT / WIDTH, 0.07f * HEIGHT / WIDTH, -0.07f, 0.07f, 0.2f, 200.f);

    /* modelview setting */
    /* mv indicates the view matrix.*/
    nn::math::Matrix34 cam;
    nn::math::Vector3 camUp(0.f, 1.f, 0.f);
    nn::math::MTX34LookAt(&cam, &v_cam, &camUp, &v_focus);
    
    // Calculate the view matrix and projection matrix for rendering the image that corresponds to the left and right parallax
    nn::math::Matrix44 projL, projR;
    nn::math::Matrix34 viewL, viewR;

    s_StereoCamera.SetBaseCamera(&cam);
    s_StereoCamera.CalculateMatrices(
        &projL, &viewL, &projR, &viewR,
        DEMO2_3D_DEPTH_LEVEL,
        DEMO2_3D_FACTOR,
        nn::math::PIVOT_UPSIDE_TO_TOP
    );

    // Display upper screen (left)
    s_RenderSystem.SetRenderTarget(NN_GX_DISPLAY0);
    s_RenderSystem.Clear();

    DrawObjects(projL, viewL, v_pos, v_degSphere, v_deg, posYforSphere);
    s_RenderSystem.Transfer();

    // Display upper screen (right)
    s_RenderSystem.SetRenderTarget(NN_GX_DISPLAY0_EXT);
    s_RenderSystem.Clear();

    DrawObjects(projR, viewR, v_pos, v_degSphere, v_deg, posYforSphere);
    s_RenderSystem.Transfer();

    s_RenderSystem.SwapBuffers();

    /* it is possible to save the content of the buffer */
    /*
    char fname[256];
    sprintf(fname, "frame-%04d.tga", f);
    outputImage(WIDTH, HEIGHT, fname);
    */

    s_RenderSystem.WaitVsync(NN_GX_DISPLAY_BOTH);

    return !glGetError();
}

/* initialization */
int InitializeGraphics(void)
{
    s_HeapForGx = reinterpret_cast<uptr>(s_AppHeap.Allocate(s_GxHeapSize));

    /* Initialize display */
    s_RenderSystem.Initialize(s_HeapForGx, s_GxHeapSize);

    s_StereoCamera.Initialize();

    /* setup vertex shader */
    /* The shader setup process with DMPGL2.0 uses the same mechanism as OpenGLES2.0.
     * 
     * Because only the vertex shader can be created as user-defined with DMPGL2.0, only one shader object is created.
     * */
    s_PgID = glCreateProgram();
    s_ShID = glCreateShader(GL_VERTEX_SHADER);

    // Load shader binary with file system
    nn::fs::FileReader shaderReader(L"rom:/shader.shbin");
    size_t size = shaderReader.GetSize();
    s_pShader = s_AppHeap.Allocate(size);
    s32 read = shaderReader.Read(s_pShader, size);
    NN_ASSERT(read == size);
    glShaderBinary(1, &s_ShID, GL_PLATFORM_BINARY_DMP, s_pShader, read);

    glAttachShader(s_PgID, s_ShID);
    /* The GL_DMP_FRAGMENT_SHADER_DMP shader object is the shader object of the fragment shader reserved with DMPGL2.0.
     * */
    glAttachShader(s_PgID, GL_DMP_FRAGMENT_SHADER_DMP);

    /* Because the shader program performs coordinate conversions and lighting calculations, vertex coordinates and normals are required as vertex attributes, and the vertex coordinates are allocated to attribute0 and the normals are allocated to attribute1.
     * 
     * */
    glBindAttribLocation(s_PgID, 0, "aPosition");
    glBindAttribLocation(s_PgID, 1, "aNormal");

    /* Links the vertex shader and fragment shader.*/
    glLinkProgram(s_PgID);

    glValidateProgram(s_PgID);
    glUseProgram(s_PgID);

    s_RenderSystem.SetClearColor(NN_GX_DISPLAY0, 0.36f, 0.42f, 0.5f, 1.0f);
    s_RenderSystem.SetClearColor(NN_GX_DISPLAY1, 0.f, 0.f, 0.f, 1.0f);

    glClearDepthf(1.f);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    LoadObjects();

    /* The following parameters are the same as the OpenGLES1.1 vertex lighting parameters.
     * 
     * Note that product of multiplying the same components of lights and materials is set in the shader Uniform.
     * */
    GLfloat ldif[] = {1.f, 1.f, 1.f, 1.f};      /* light diffuse */
    GLfloat lspc[] = {1.f, 1.f, 1.f, 1.f};      /* light specular */
    GLfloat lamb[] = {0.f, 0.f, 0.f, 1.f};      /* light ambient */
    GLfloat lmamb[] = {0.2f, 0.2f, 0.f, 1.f};   /* light model ambient */

    GLfloat mspc[] = {1.f, 1.f, 1.f, 1.f};      /* material specular */
    GLfloat mdif[] = {0.8f, 0.8f, 0.f, 1.f};    /* material diffuse */
    GLfloat mamb[] = {0.8f, 0.f, 0.f, 1.f};     /* material ambient */

    /* Set the sum of the light ambient and global ambient as ambient.
     * */
    nn::math::Vector4 amb;
    nn::math::Vector4 lamb_mamb  = nn::math::VEC4(lamb[0]*mamb[0], lamb[1]*mamb[1], lamb[2]*mamb[2], lamb[3]*mamb[3]);
    nn::math::Vector4 lmamb_mamb = nn::math::VEC4(lmamb[0]*mamb[0], lmamb[1]*mamb[1], lmamb[2]*mamb[2], lmamb[3]*mamb[3]);
    nn::math::VEC4Add(&amb, &lamb_mamb, &lmamb_mamb);

    nn::math::Vector4 dif = nn::math::VEC4(ldif[0]*mdif[0], ldif[1]*mdif[1], ldif[2]*mdif[2], ldif[3]*mdif[3]);
    nn::math::Vector4 spc = nn::math::VEC4(lspc[0]*mspc[0], lspc[1]*mspc[1], lspc[2]*mspc[2], lspc[3]*mspc[3]);
    GLfloat a[] = {amb.x, amb.y, amb.z, amb.w};
    GLfloat d[] = {dif.x, dif.y, dif.z, dif.w};
    GLfloat s[] = {spc.x, spc.y, spc.z, spc.w};

    glUniform4fv(glGetUniformLocation(s_PgID, "uDiff"), 1, d);
    glUniform4fv(glGetUniformLocation(s_PgID, "uAmb"), 1, a);
    glUniform4fv(glGetUniformLocation(s_PgID, "uSpec"), 1, s);
    glUniform1f(glGetUniformLocation(s_PgID, "uMatShiniess"), SPEC_SHININESS);

    glUniform3i(glGetUniformLocation(s_PgID, "dmp_TexEnv[0].srcRgb"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);
    glUniform3i(glGetUniformLocation(s_PgID, "dmp_TexEnv[0].srcAlpha"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);

    s_RenderSystem.SetLcdMode(NN_GX_DISPLAYMODE_STEREO);

    return 0;
}

int FinalizeGraphics(void)
{
    /* shutdown_display */
    s_RenderSystem.Finalize();
    s_AppHeap.Free(s_pShader);
    s_AppHeap.Free(reinterpret_cast<void*>(s_HeapForGx));
    
    return 0;
}

/*---------------------------------------------------------------------------*
  End of file
 *---------------------------------------------------------------------------*/
