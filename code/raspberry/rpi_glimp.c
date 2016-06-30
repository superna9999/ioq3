/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <bcm_host.h>
#include <EGL/egl.h>

#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"

void myglMultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	glMultiTexCoord4f(texture, s, t, 0, 1);
}

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

static DISPMANX_DISPLAY_HANDLE_T s_dispman_display;
static DISPMANX_ELEMENT_HANDLE_T s_element;
static EGL_DISPMANX_WINDOW_T s_nativeWindow;
static EGLDisplay s_egl_display;
static EGLSurface s_egl_surface;
static EGLContext s_egl_context;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( char *comment )
{
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
}


/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes(void)
{
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriver()
{
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	EGLint num_config;
	EGLBoolean result;

	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 0,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE
	};

	bcm_host_init();

	EGLConfig config;
	s_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(s_egl_display == EGL_NO_DISPLAY)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not load EGL display");

	result = eglInitialize(s_egl_display, NULL, NULL);
	if(result == qfalse)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not initialize EGL display");

	result = eglChooseConfig(s_egl_display, attribute_list, &config, 1, &num_config);
	if(result == qfalse)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not choose EGL config");

	static const EGLint context_params[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};

	eglBindAPI(EGL_OPENGL_ES_API);
	s_egl_context = eglCreateContext(s_egl_display, config, EGL_NO_CONTEXT, context_params);
	if(s_egl_context == EGL_NO_CONTEXT)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not create EGL context");

	int windowWidth = ri.Cvar_Get("r_gleswidth", "853", CVAR_ARCHIVE | CVAR_LATCH )->integer;
	int windowHeight = ri.Cvar_Get("r_glesheight", "480", CVAR_ARCHIVE | CVAR_LATCH )->integer;
	int screenWidth;
	int screenHeight;

	graphics_get_display_size(0 /* LCD */, &screenWidth, &screenHeight);
	float scale = (float)screenWidth / windowWidth;
	if(windowHeight * scale > screenHeight)
		scale = (float)screenHeight / windowHeight;

	dst_rect.width = windowWidth * scale;
	dst_rect.height = windowHeight * scale;
	dst_rect.x = (screenWidth - dst_rect.width) / 2;
	dst_rect.y = (screenHeight - dst_rect.height) / 2;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = windowWidth << 16;
	src_rect.height = windowHeight << 16;

	s_dispman_display = vc_dispmanx_display_open(0 /* LCD */);
	dispman_update = vc_dispmanx_update_start(0);

	VC_DISPMANX_ALPHA_T alpha;
	alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha.opacity = 0xFF;
	alpha.mask = 0x00;
	s_element = vc_dispmanx_element_add(dispman_update, s_dispman_display, 0/*layer*/, &dst_rect, 0/*src*/, &src_rect, DISPMANX_PROTECTION_NONE, &alpha /*alpha*/, 0/*clamp*/, 0/*transform*/);

	s_nativeWindow.element = s_element;
	s_nativeWindow.width = windowWidth;
	s_nativeWindow.height = windowHeight;
	vc_dispmanx_update_submit_sync(dispman_update);

	s_egl_surface = eglCreateWindowSurface(s_egl_display, config, &s_nativeWindow, NULL);
	if(s_egl_surface == EGL_NO_SURFACE)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not create EGL window surface");

	result = eglMakeCurrent(s_egl_display, s_egl_surface, s_egl_surface, s_egl_context);
	if(result == qfalse)
		ri.Error(ERR_FATAL, "GLimp_StartDriver() - could not select EGL context");

	qglClearColor(0, 0, 0, 1);
	qglClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(s_egl_display, s_egl_surface);
	eglSwapInterval(s_egl_display, 1);

	glConfig.colorBits = 24;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 0;
	glConfig.vidWidth = windowWidth;
	glConfig.vidHeight = windowHeight;
	glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	r_swapInterval->integer = 1;

	ri.Printf(PRINT_ALL, "Initialized RPI Display\n");

	return qtrue;
}

static qboolean GLimp_HaveExtension(const char* ext)
{
	const char *ptr = Q_stristr(glConfig.extensions_string, ext);
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));
}


/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( void )
{
	if (!r_allowExtensions->integer)
	{
		ri.Printf(PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n");
		return;
	}

	// WARNING: This will be RPi specific
	glConfig.textureCompression = TC_NONE;
	glConfig.textureEnvAddAvailable = qtrue;
	ri.Printf(PRINT_ALL, "...using GL_EXT_texture_env_add\n");

	qglGetIntegerv(GL_MAX_TEXTURE_UNITS, &glConfig.numTextureUnits);
	qglMultiTexCoord2fARB = myglMultiTexCoord2f;
	qglActiveTextureARB = glActiveTexture;
	qglClientActiveTextureARB = glClientActiveTexture;
	if(glConfig.numTextureUnits > 1)
		ri.Printf(PRINT_ALL, "...using GL_ARB_multitexture (%i texture units)\n", glConfig.numTextureUnits);

	textureFilterAnisotropic = qfalse;
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( void )
{
	ri.Sys_GLimpInit();

	if(GLimp_StartDriver())
		goto success;

	ri.Error(ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem");

success:
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = qfalse;

	Q_strncpyz(glConfig.vendor_string, (char*)qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, (char*)qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz(glConfig.version_string, (char*)qglGetString(GL_VERSION), sizeof(glConfig.version_string));
	Q_strncpyz(glConfig.extensions_string, (char*)qglGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));

	GLimp_InitExtensions();
	ri.IN_Init(NULL);
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame(void)
{
	if(Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0)
		eglSwapBuffers(s_egl_display, s_egl_surface);
}
