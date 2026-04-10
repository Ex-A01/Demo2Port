/*
 * Comments
 * --------
 *
 * This sample emulates the fixed vertex pipeline of OpenGL ES 1.1.
 * It is implemented using the vertex shader to convert vertex coordinates to clip space and calculate vertex colors from one light and point light source.
 * 
 */


#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "Display.h"
#include "Util.h"
#include "Vecalg.h"
#include "File.h"

#include <string.h>
#include <math.h>

#include "Memory.h"

#define APP_NAME "LightingVertex"
#define WIDTH 240
#define HEIGHT 400

#define DMP_PI	(3.1415926f)

/* program id */
GLuint pgid;

/* shader id */
GLuint shid;

/* OpenGLES1.1 vertex lighting specular shininess */
#define SPEC_SHININESS	32.f
#define STEP 6.f

/* object names */
enum {
	OBJECT_SPHERE,	/* sphere */
	OBJECT_PLANE,	/* plane */
	OBJECT_COUNT	/* object count */
};

/* buffer object ID */
static struct tagObject
{
	GLuint id[OBJECT_COUNT];
	GLuint idxId[OBJECT_COUNT];
} object;

/* buffer object information */
static struct tagObjectInfo
{
	GLushort idxcnt[OBJECT_COUNT];
	GLushort vtxcnt[OBJECT_COUNT];
} objectinfo;


#define ROW_NUM		(50)	/* NUM in ROW */
#define COL_NUM		(50)	/* NUM in COLUMN */
#define deltaROW	(DMP_PI / (ROW_NUM - 1))
#define deltaCOL	(2 * DMP_PI / (COL_NUM - 1))

struct tagVertex{
	GLfloat	pos[ROW_NUM * COL_NUM][3];
	GLfloat	nor[ROW_NUM * COL_NUM][3];
} vtx;
GLushort idx[COL_NUM * (ROW_NUM - 1) * 2];

/* load sphere object */
static void load_sphere(void)
{
	/* vertex array */
	for(int row = 0; row < ROW_NUM; row++)
	{
		for(int col = 0; col < COL_NUM; col++)
		{
			/* position */
			vtx.pos[row * COL_NUM + col][0] =(GLfloat)sin(deltaROW * row) * cos(deltaCOL * col);
			vtx.pos[row * COL_NUM + col][1] =(GLfloat)cos(deltaROW * row);
			vtx.pos[row * COL_NUM + col][2] =(GLfloat)sin(deltaROW * row) * sin(deltaCOL * col);
			/* normal */
			vtx.nor[row * COL_NUM + col][0] =(GLfloat)sin(deltaROW * row) * cos(deltaCOL * col);
			vtx.nor[row * COL_NUM + col][1] =(GLfloat)cos(deltaROW * row);
			vtx.nor[row * COL_NUM + col][2] =(GLfloat)sin(deltaROW * row) * sin(deltaCOL * col);
		}
	}

	/* index array */
	for(int i = 0, row = 0; row < ROW_NUM - 1; row++)
	{
	#define __INDEX(ROW, COL)	((ROW) * COL_NUM + (COL))
		for(int col = 0; col < COL_NUM; col++)
		{
			idx[i++] = __INDEX(row + 1, col);
			idx[i++] = __INDEX(row, col);
		}
	#undef __INDEX
	}
	/* count */
	objectinfo.idxcnt[OBJECT_SPHERE] = COL_NUM * (ROW_NUM - 1) * 2;
	objectinfo.vtxcnt[OBJECT_SPHERE] = ROW_NUM * COL_NUM;

	/* load vertex array and index array */
	glBindBuffer(GL_ARRAY_BUFFER, object.id[OBJECT_SPHERE]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), &vtx, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.idxId[OBJECT_SPHERE]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);


#undef LONG_NUM
#undef LATI_NUM
#undef deltaROW
#undef deltaCOL
}

/* load plane object */
static void load_plane(void)
{
	struct tagVertex{
		GLfloat	pos[4][3];
		GLfloat	nor[4][3];
	} vtx;
	GLushort idx[4] ={0, 1, 2, 3};

	/* vertex array */
	vtx.pos[0][0] = +3.0f;
	vtx.pos[0][1] = -1.0f;
	vtx.pos[0][2] = +3.0f;

	vtx.pos[1][0] = +3.0f;
	vtx.pos[1][1] = -1.0f;
	vtx.pos[1][2] = -3.0f;

	vtx.pos[2][0] = -3.0f;
	vtx.pos[2][1] = -1.0f;
	vtx.pos[2][2] = +3.0f;

	vtx.pos[3][0] = -3.0f;
	vtx.pos[3][1] = -1.0f;
	vtx.pos[3][2] = -3.0f;

	for(int i = 0; i < 4; i++)
	{
		vtx.nor[i][0] = 0.f;
		vtx.nor[i][1] = 1.f;
		vtx.nor[i][2] = 0.f;
	}

	/* count */
	objectinfo.idxcnt[OBJECT_PLANE] = 4;
	objectinfo.vtxcnt[OBJECT_PLANE] = 4;

	/* load vertex array and index array */
	glBindBuffer(GL_ARRAY_BUFFER, object.id[OBJECT_PLANE]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx.pos[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.idxId[OBJECT_PLANE]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

/* load objects */
static void load_objects(void)
{
	glGenBuffers(OBJECT_COUNT * 2, (GLuint*)&object);

	load_sphere();
	load_plane();
}

int drawframe(void)
{
	static int f = 0;
	mat4_t mv, proj;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* In this application light positions are specified in object coordinates.
	 * */
	GLfloat lpos[] = {3.f, 3.f, 0.f, 1.f};		/* Light position */
	GLfloat m[16];

	/* Projection settings */
	proj = mat4_t::frustum(-0.07f, 0.07f, -0.07f * HEIGHT / WIDTH, 0.07f * HEIGHT / WIDTH, 0.2f, 200.f);
	proj.toFloatArr(m);
	glUniformMatrix4fv(glGetUniformLocation(pgid, "uProjection"), 1, GL_FALSE, m);

	/* modelview setting */
	/* mv indicates the view matrix.*/
	mv = mat4_t::rotate(-90.f, 0.f, 0.f, 1.f);
	mv = mv * mat4_t::lookAt(
				0.f, 5.f, 10.f,		/* eye */
				0.f, 0.f, 0.f,		/* center */
				0.f, 1.f, 0.f);		/* up vector */
	mv.toFloatArr(m);
	glUniformMatrix4fv(glGetUniformLocation(pgid, "uModelView"), 1, GL_FALSE, m);

	/* set light position */
	/* constant light position is transformed from object-space to eye-space */
	/* The vertex shader assumes that the view coordinate system is used for lighting calculations.
	 * The light's position is therefore converted to the view coordinate system. The shader does not perform conversion.
	 * This is because lights use common values in all vertex processing.
	 * mv is the view matrix. Multiplication by this matrix converts coordinates to the view coordinate system.*/
	vec4_t p(lpos);
	p = mv * p;
	float lpos2[] = {p[0], p[1], p[2], p[3]};
	glUniform4fv(glGetUniformLocation(pgid, "uLightPos"), 1, lpos2);

	/* draw objects */
	for (int i = 0; i < OBJECT_COUNT; i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, object.id[i]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(GLfloat) * 3 * objectinfo.vtxcnt[i]));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.idxId[i]);

		if (i == OBJECT_SPHERE)	/* for sphere */
		{
			/* Rotation and translation are applied to the current modelview matrix (mv) only when a sphere model is used, resulting in mv2.
			 * This is reverted when rendering is complete.
			 * This is equivalent to the glPushMatrix and glPopMatrix operations in OpenGL ES 1.1.*/
			mat4_t r = mat4_t::rotate(STEP * f / 2, 0.f, 1.f, 0.f);
			mat4_t t = mat4_t::translate(2.0f, 0.f, 0.f);
			mat4_t mv2 = mv * r * t;
			mv2.toFloatArr(m);
			glUniformMatrix4fv(glGetUniformLocation(pgid, "uModelView"), 1, GL_FALSE, m);
		}

		glDrawElements(GL_TRIANGLE_STRIP, objectinfo.idxcnt[i], GL_UNSIGNED_SHORT,(GLvoid*)0);

		if (i == OBJECT_SPHERE)	/* for sphere */
		{
			/* Returns the model view matrix to its original state. (mv) */
			mv.toFloatArr(m);
			glUniformMatrix4fv(glGetUniformLocation(pgid, "uModelView"), 1, GL_FALSE, m);
		}
	}

	glFinish();

	/* it is possible to save the content of the buffer */
	/*
	char fname[256];
	sprintf(fname, "frame-%04d.tga", f);
	outputImage(WIDTH, HEIGHT, fname);
	*/

	swap_buffer();

	f++;

	return !glGetError();
}

/* initialization */
static int initialize(void)
{
	/* Initialize display */
	init_display(WIDTH, HEIGHT, APP_NAME, drawframe);

	/* setup vertex shader */
	/* The shader setup process with DMPGL2.0 uses the same mechanism as OpenGL ES 2.0.
	 * Because DMPGL 2.0 only allows the vertex shader to be user-defined, only a single shader object is created.
	 * 
	 * */
	pgid = glCreateProgram();
	shid = glCreateShader(GL_VERTEX_SHADER);

	int fsize;
	unsigned char* binary = ReadFile(FILE_APP_ROOT "shader.bin", &fsize);
	if (!binary)
		return -1;
	
	glShaderBinary(1, &shid, GL_PLATFORM_BINARY_DMP, binary, fsize);
	free(binary);

	glAttachShader(pgid, shid);
	/* The GL_DMP_FRAGMENT_SHADER_DMP shader object is for the fragment shader object reserved with DMPGL 2.0.
	 * */
	glAttachShader(pgid, GL_DMP_FRAGMENT_SHADER_DMP);

	/* Because the shader program handles coordinate conversions and lighting calculations, vertex coordinates and normals are required as vertex attributes and are assigned to attribute0 and attribute1 respectively.
	 * 
	 * */
	glBindAttribLocation(pgid, 0, "aPosition");
	glBindAttribLocation(pgid, 1, "aNormal");

	/* Links the vertex shader and fragment shader.*/
	glLinkProgram(pgid);

	glValidateProgram(pgid);
	glUseProgram(pgid);

	glClearColor(0.36f, 0.42f, 0.5f, 1.0f);
	glClearDepthf(1.f);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	load_objects();

	/* The following parameters are the same as the OpenGL ES 1.1 vertex lighting parameters.
	 * 
	 * Note that the result of multiplying the same light and material components together is set in the shader Uniform.
	 * */
	GLfloat ldif[] = {1.f, 1.f, 1.f, 1.f};		/* light diffuse */
	GLfloat lspc[] = {1.f, 1.f, 1.f, 1.f};		/* light specular */
	GLfloat lamb[] = {0.f, 0.f, 0.f, 1.f};		/* light ambient */
	GLfloat lmamb[] = {0.2f, 0.2f, 0.f, 1.f};	/* light model ambient */

	GLfloat mspc[] = {1.f, 1.f, 1.f, 1.f};		/* material specular */
	GLfloat mdif[] = {0.8f, 0.8f, 0.f, 1.f};	/* material diffuse */
	GLfloat mamb[] = {0.8f, 0.f, 0.f, 1.f};		/* material ambient */

	/* Set the sum of the light ambient and global ambient as ambient.
	 * */
	vec4_t amb = vec4_t(lamb).vmul(vec4_t(mamb)) + vec4_t(lmamb).vmul(vec4_t(mamb));

	vec4_t dif = vec4_t(ldif).vmul(vec4_t(mdif));
	vec4_t spc = vec4_t(lspc).vmul(vec4_t(mspc));
	GLfloat a[] = {amb[0], amb[1], amb[2], amb[3]};
	GLfloat d[] = {dif[0], dif[1], dif[2], dif[3]};
	GLfloat s[] = {spc[0], spc[1], spc[2], spc[3]};

	glUniform4fv(glGetUniformLocation(pgid, "uDiff"), 1, d);
	glUniform4fv(glGetUniformLocation(pgid, "uAmb"), 1, a);
	glUniform4fv(glGetUniformLocation(pgid, "uSpec"), 1, s);
	glUniform1f(glGetUniformLocation(pgid, "uMatShininess"), SPEC_SHININESS);

	glUniform3i(glGetUniformLocation(pgid, "dmp_TexEnv[0].srcRgb"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);
	glUniform3i(glGetUniformLocation(pgid, "dmp_TexEnv[0].srcAlpha"), GL_PRIMARY_COLOR, GL_PRIMARY_COLOR, GL_PRIMARY_COLOR);

	return 0;
}

#ifdef _NO_OS
int main(int argc, char* argv[])
#else
int sample_main(void)
#endif
{
	/* initialization */
	if (initialize() >= 0)
	{
		/* Enter loop */
		draw_loop();
	}
	
	/* shutdown_display */
	shutdown_display();

	return 0;
}
