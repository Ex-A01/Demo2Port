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
// This shader emulates the fixed pipeline for vertex processing in OpenGL 1.1.
//  Performs calculations assuming there is one light and it is treated as a point light source.
//

// Input registers map
#define aPosition		v0
#define aNormal			v1

// Output registers map
#define vPosition		o0

// Constant registers
#define CONSTANT		c93

def		CONSTANT[0], 0.0, 1.0, 2.0, 3.0
def		CONSTANT[1], 0.125, 0.00390625, 0.5, 0.25

#define CONST_0					CONSTANT[0].x
#define CONST_1					CONSTANT[0].y
#define CONST_2					CONSTANT[0].z
#define CONST_3					CONSTANT[0].w
#define CONST_HALF				CONSTANT[1].z
#define CONST_QUARTER			CONSTANT[1].w
#define CONST_1_0				CONSTANT[0].yx
#define CONST_1__4				CONSTANT[1].w
#define CONST_1__8				CONSTANT[1].x
#define CONST_1__256			CONSTANT[1].y

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
	// Convert the vertex coordinates set in the object coordinate system to the view coordinate system and then to the clip coordinate system.
	// 
	// However, DMPGL 2.0 handles conversions to the clip coordinate system slightly differently than OpenGL.
	// For details, see the DMPGL 2.0 Programming Guide.
	// r15 stores the vertex position in view coordinates. The vertex position in clip coordinates is output to o0.
	// 
	m4x4	r15,		v0,			c4
	mov		r12,		c0[2]
	add		r12,		r12,		c0[3]
	mul		r12,		r12,		-CONST_HALF
	dp4		o0.x,		r15,		c0[0]
	dp4		o0.y,		r15,		c0[1]
	dp4		o0.z,		r15,		r12
	dp4		o0.w,		r15,		c0[3]
    // Convert the normal vector(s) from object coordinates to view coordinates.
    // Here, the upper-left3x3 matrix of the model view matrix is used for conversion.
	// 
	// The normal vector(s) is not normalized during vertex processing because a normalized vector(s) is specified by the application and the transformation matrix does not have a scaling component.
	// 
	// 
	dp3		r14.x,		v1,			c4
	dp3		r14.y,		v1,			c5    
	dp3		r14.z,		v1,			c6
    // Creates a normalized light vector.
    // -r15 denotes the view vector because r15 is a vertex coordinate in the view coordinate system.
    // 
    // The application converts the light position into view coordinates and sets it in c8.
    // 
    // Since the above, c8-r15 is the light vector. (no normalized at this time)
    add		r0,			c8,			-r15
    // The following 3 lines normalize r0 (light vector).
    dp3		r0.w,		r0,			r0
    rsq		r0.w,		r0.w
    mul		r0,			r0,			r0.w
	// Prepare a half-vector for the specular component.
	// First, prepare a normalized view vector.
    mov		r1,			-r15
    dp3		r1.w,		r1,			r1
    rsq		r1.w,		r1.w
    mul		r1,			r1,			r1.w
    // Add the view vector and light vector (the light vector is normalized)
    add		r2,			r0,			r1
    // Then normalize the result. r2 is a normalized half-vector.
    dp3		r2.w,		r2,			r2
    rsq		r2.w,		r2.w
    mul		r2,			r2,			r2.w
    // Calculate NL and NH.
    // NL is the inner product of the normal and light vector and stored in r3.x.
	dp3		r3.x,		r14,		r0
	// NH is the inner product of the normal and half vector and stored in r3.y.
	dp3		r3.y,		r14,		r2
	// NL and NH are clamped at greater than 0.
	max		r3,			r3,			CONST_0
	// If NL or NH is negative, there is not considered to be any specular contribution and NH and NL are clamped to 0.0 if they are less than that.
	// 
	cmp		2,			2,			r3,		CONST_0
	ifc		1,			1,			2
		mov	r3.y,		CONST_0
	endif
	// Calculates the final vertex color.
	// First, the sum of the global ambient and light0 ambient is stored in r10.
	//  The application sets that sum in c10.
    mov		r10,		c10
    // Add the diffuse contribution.
    mad		r10,		r3.x,		c9,		r10	
    // then add the specular contribution.
    // NH is raised to the power of shininess.
    // shininess is set in c12.x.
    // First, perform power calculations,
    pow		r6,			r3.y,		c12.x
    // then add the specular contribution.
    mad		r10,		r6.x,		c11,	r10
	// Sets the vertex color in o1.
	mov		o1,			r10
	end
endmain:
