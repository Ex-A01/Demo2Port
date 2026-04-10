//
//------------------------------------------------------------
// Copyright(c) 2009-2010 by Digital Media Professionals Inc.
// All rights reserved.
//------------------------------------------------------------
// This source code is the confidential and proprietary
// of Digital Media Professionals Inc.
//------------------------------------------------------------
//
//
// VShader.asm
// This shader emulates the OpenGL1.1 vertex processing fixed pipeline.
//  Performs calculations assuming there is one light and it is treated as a point light source.
//

// Input registers map
#define aPosition       v0
#define aNormal         v1

// Output registers map
#define vPosition       o0

// Constant registers
#define CONSTANT        c93

def     CONSTANT[0], 0.0, 1.0, 2.0, 3.0
def     CONSTANT[1], 0.125, 0.00390625, 0.5, 0.25

#define CONST_0                 CONSTANT[0].x
#define CONST_1                 CONSTANT[0].y
#define CONST_2                 CONSTANT[0].z
#define CONST_3                 CONSTANT[0].w
#define CONST_HALF              CONSTANT[1].z
#define CONST_QUARTER           CONSTANT[1].w
#define CONST_1_0               CONSTANT[0].yx
#define CONST_1__4              CONSTANT[1].w
#define CONST_1__8              CONSTANT[1].x
#define CONST_1__256            CONSTANT[1].y

#pragma bind_symbol( aPosition.xyz, v0, v0 )
#pragma bind_symbol( aNormal.xyz, v1, v1 )

#pragma output_map( position, o0 )
#pragma output_map( color, o1 )

#pragma bind_symbol( uProjection, c0, c3 )
#pragma bind_symbol( uModelView, c4, c7 )
#pragma bind_symbol( uLightPos, c8, c8 )
#pragma bind_symbol( uDiff, c9, c9 )
#pragma bind_symbol( uAmb, c10, c10 )
#pragma bind_symbol( uSpec, c11, c11 )
#pragma bind_symbol( uMatShiniess.x, c12, c12 )

main:
    // Converts vertex coordinates configured in object coordinates into perspective coordinates, and then converts them to clip coordinates.
    // 
    // The vertex coordinates in the perspective coordinate system are stored in `r15`, and output to `o0` in the clip coordinate system.
    // 
    m4x4    r15,        v0,         c4
	m4x4    o0,       r15,        c0
    // Converts a normal vector configured in object coordinates into perspective coordinates.
    // Here, the upper-left3x3 matrix of the model view matrix is used for conversion.
    // 
    // The vertex processing does not normalize the normal vector because the specified lines are normalized by the application, and the conversion matrix does not have a scaling component.
    // 
    // 
	m3x3    r14.xyz,    v1,       c4
    // Creates a normalized light vector.
    // `-r15` represents the normal vector, because `r15` is the vector in perspective coordinates.
    // 
    // The variable `c8` holds the light position in perspective coordinates. The application converts the light position into perspective coordinates, and stores that in `c8`.
    // 
    // From the above, c8 to r15 becomes the light vector. (not normalized at this time)
    add     r0,         c8,         -r15
    // The following 3 lines normalize r0 (light vector).
    dp3     r0.w,       r0,         r0
    rsq     r0.w,       r0.w
    mul     r0,         r0,         r0.w
    // Prepare a half-vector for the specular component.
    // First, prepare a normalized view vector.
    mov     r1,         -r15
    dp3     r1.w,       r1,         r1
    rsq     r1.w,       r1.w
    mul     r1,         r1,         r1.w
    // Adds the view vector and light vector (the light vector is normalized), and normalizes the result.
    add     r2,         r0,         r1
    //  `r2` is a normalized half-vector.
    dp3     r2.w,       r2,         r2
    rsq     r2.w,       r2.w
    mul     r2,         r2,         r2.w
    // Calculate NL and NH.
    // NL is the inner product of the normal and light vector and stored in r3.x.
    dp3     r3.x,       r14.xyz,    r0
    // NH is the inner product of the normal and half vector and stored in r3.y.
    dp3     r3.y,       r14.xyz,    r2
    // NL and NH are clamped at greater than 0.
    max     r3,         r3.xy,      CONST_0
    // If `NL` or `NH` is negative, they are deemed to not have specular contribution, so `NH` and `NL` are clamped to `0.0` when <tt>NL < 0.0 || NH < 0.0</tt>.
    // 
    cmp     2,          2,          r3,     CONST_0
    ifc     1,          1,          2
        mov r3.y,       CONST_0
    endif
    // Calculates the final vertex color.
    // First, the sum of the global ambient and `light0` ambient is stored in `r10`. 
    // The application sets that sum in c10.
    mov     r10,        c10
    // Add the diffuse contribution.
    mad     r10,        r3.x,       c9,     r10 
    // then add the specular contribution.
    // NH is raised to the power of shininess.
    // shininess is set in c12.x.
    // First, performs power calculations, then adds the specular contribution.
    pow     r6,         r3.y,       c12.x
    // 
    mad     r10,        r6.x,       c11,    r10
    // Sets the vertex color in o1.
    mov     o1,         r10
    end
endmain:
