/******************************************************************************
    Copyright (C) 2024 by Jacob Secunda <secundaja@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "gl-haiku.h"
#include "gl-subsystem.h"

#include <Bitmap.h>
#include <Window.h>

static const int ctx_attribs[] = {
	EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
	EGL_CONTEXT_MAJOR_VERSION, 3,
	EGL_CONTEXT_MINOR_VERSION, 3,
	EGL_NONE,
};

static int ctx_pbuffer_attribs[] = {
		EGL_WIDTH, 2,
		EGL_HEIGHT, 2,
		EGL_NONE
};

static const EGLint ctx_config_attribs[] = {
		EGL_STENCIL_SIZE, 0,
		EGL_DEPTH_SIZE, 0,
		EGL_BUFFER_SIZE, 24,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
		EGL_NONE
};

//static const EGLint ctx_config_attribs[] = {
//		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
////		EGL_BLUE_SIZE, 8,
////		EGL_GREEN_SIZE, 8,
////		EGL_RED_SIZE, 8,
////		EGL_DEPTH_SIZE, 8,
//		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
//		EGL_NONE
//};

struct gl_platform {
	EGLDisplay edisplay;
	EGLConfig config;
	EGLContext context;
	EGLSurface pbuffer;
};

static const char *
get_egl_error_string(const EGLint error)
{
	switch (error) {
#define OBS_EGL_CASE_ERROR(e) \
	case e:               \
		return #e;
		OBS_EGL_CASE_ERROR(EGL_SUCCESS)
		OBS_EGL_CASE_ERROR(EGL_NOT_INITIALIZED)
		OBS_EGL_CASE_ERROR(EGL_BAD_ACCESS)
		OBS_EGL_CASE_ERROR(EGL_BAD_ALLOC)
		OBS_EGL_CASE_ERROR(EGL_BAD_ATTRIBUTE)
		OBS_EGL_CASE_ERROR(EGL_BAD_CONTEXT)
		OBS_EGL_CASE_ERROR(EGL_BAD_CONFIG)
		OBS_EGL_CASE_ERROR(EGL_BAD_CURRENT_SURFACE)
		OBS_EGL_CASE_ERROR(EGL_BAD_DISPLAY)
		OBS_EGL_CASE_ERROR(EGL_BAD_SURFACE)
		OBS_EGL_CASE_ERROR(EGL_BAD_MATCH)
		OBS_EGL_CASE_ERROR(EGL_BAD_PARAMETER)
		OBS_EGL_CASE_ERROR(EGL_BAD_NATIVE_PIXMAP)
		OBS_EGL_CASE_ERROR(EGL_BAD_NATIVE_WINDOW)
		OBS_EGL_CASE_ERROR(EGL_CONTEXT_LOST)
#undef OBS_EGL_CASE_ERROR
	default:
		return "Unknown";
	}
}

bool
gl_context_create(struct gl_platform *plat)
{
	int frame_buf_config_count = 0;
	EGLDisplay edisplay = EGL_NO_DISPLAY;
	EGLConfig config = NULL;
	EGLContext context = EGL_NO_CONTEXT;
	int egl_min = 0, egl_maj = 0;
	bool success = false;

	eglBindAPI(EGL_OPENGL_API);

	// Miraculously, this will get Haiku's EGL platform
	edisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (edisplay == EGL_NO_DISPLAY) {
		blog(LOG_ERROR, "Failed to get EGL display using eglGetDisplay");
		return false;
	}

	if (!eglInitialize(edisplay, &egl_maj, &egl_min)) {
		blog(LOG_ERROR, "Failed to initialize EGL: %s", get_egl_error_string(eglGetError()));
		return false;
	}

	if (!eglChooseConfig(edisplay, ctx_config_attribs, &config, 1, &frame_buf_config_count)) {
		blog(LOG_ERROR, "Unable to find suitable EGL config: %s", get_egl_error_string(eglGetError()));
		goto error;
	}

	context = eglCreateContext(edisplay, config, EGL_NO_CONTEXT, ctx_attribs);
	if (context == EGL_NO_CONTEXT) {
		blog(LOG_ERROR, "Unable to create EGL context: %s", get_egl_error_string(eglGetError()));
		goto error;
	}

	plat->pbuffer = eglCreatePbufferSurface(edisplay, config, ctx_pbuffer_attribs);
	if (EGL_NO_SURFACE == plat->pbuffer) {
		blog(LOG_ERROR, "Failed to create OpenGL pbuffer: %s", get_egl_error_string(eglGetError()));
		goto error;
	}

	plat->edisplay = edisplay;
	plat->config = config;
	plat->context = context;

	success = true;
	blog(LOG_DEBUG, "Created EGLDisplay %p", plat->edisplay);

error:
	if (!success) {
		if (EGL_NO_CONTEXT != context)
			eglDestroyContext(edisplay, context);
		eglTerminate(edisplay);
	}

	return success;
}

void
gl_context_destroy(struct gl_platform *plat)
{
	eglMakeCurrent(plat->edisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(plat->edisplay, plat->context);
}

struct gl_windowinfo *
gl_windowinfo_create(const struct gs_init_data *info)
{
	auto windowInfo = static_cast<gl_windowinfo *>(bzalloc(sizeof(struct gl_windowinfo)));

	auto parentWindow = static_cast<BWindow *>(info->window.window);
	windowInfo->surfaceView = parentWindow->ChildAt(0);

	return windowInfo;
}

void
gl_windowinfo_destroy(struct gl_windowinfo *info)
{
	bfree(info);
}

struct gl_platform *
gl_platform_create(gs_device_t *device, uint32_t adapter)
{
	UNUSED_PARAMETER(adapter);

	if (!gladLoadEGL()) {
		blog(LOG_ERROR, "Unable to load EGL entry functions.");
		return NULL;
	}

	auto plat = static_cast<gl_platform *>(bmalloc(sizeof(struct gl_platform)));

	/* We assume later that cur_swap is already set. */
	device->plat = plat;

	if (!gl_context_create(plat)) {
		blog(LOG_ERROR, "Failed to create context!");
		goto fail_context_create;
	}

	if (!eglMakeCurrent(plat->edisplay, plat->pbuffer, plat->pbuffer, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current: %s", get_egl_error_string(eglGetError()));
		goto fail_make_current;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		goto fail_load_gl;
	}

	goto success;

fail_make_current:
	gl_context_destroy(plat);
fail_context_create:
fail_load_gl:
	bfree(plat);
	plat = NULL;
success:
	return plat;
}

void
gl_platform_destroy(struct gl_platform *plat)
{
	if (!plat)
		return;

	gl_context_destroy(plat);
	eglTerminate(plat->edisplay);
	bfree(plat);
}

bool
gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	const struct gl_platform *plat = swap->device->plat;

	swap->wi->bitmapHook = new(std::nothrow) SwapChainBitmapHook(swap->wi);
	if (swap->wi->bitmapHook == NULL)
		return false;

	const EGLSurface surface = eglCreateWindowSurface(plat->edisplay, plat->config, reinterpret_cast<EGLNativeWindowType>(swap->wi->bitmapHook), 0);
	if (surface == EGL_NO_SURFACE) {
		blog(LOG_ERROR, "Cannot get window EGL surface: %s", get_egl_error_string(eglGetError()));
		delete swap->wi->bitmapHook;
		return false;
	}
	swap->wi->surface = surface;

	return true;
}

void
gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	eglDestroySurface(swap->device->plat->edisplay, swap->wi->surface);
	swap->wi->surface = EGL_NO_SURFACE;

	delete swap->wi->bitmapHook;
	swap->wi->bitmapHook = NULL;
}

void device_enter_context(gs_device_t *device)
{
	const EGLContext context = device->plat->context;
	const EGLDisplay display = device->plat->edisplay;
	const EGLSurface surface = (device->cur_swap)
					   ? device->cur_swap->wi->surface
					   : device->plat->pbuffer;

	if (!eglMakeCurrent(display, surface, surface, context))
		blog(LOG_ERROR, "Failed to make context current: %s", get_egl_error_string(eglGetError()));
}

void
device_leave_context(gs_device_t *device)
{
	device->cur_render_target = NULL;
	device->cur_zstencil_buffer = NULL;
	device->cur_vertex_buffer = NULL;
	device->cur_index_buffer = NULL;
	device->cur_swap = NULL;
	device->cur_fbo = NULL;

	const EGLDisplay display = device->plat->edisplay;

	if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,  EGL_NO_CONTEXT)) {
		blog(LOG_ERROR, "Failed to reset current context: %s", get_egl_error_string(eglGetError()));
	}
}

void *
device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

void
gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
	BRect frame = swap->wi->surfaceView->Frame();

	if (frame.IsValid()) {
		*width = frame.IntegerWidth() + 1;
		*height = frame.IntegerHeight() + 1;
	}
}

void
gl_update(gs_device_t *device)
{
//	Display *display = device->plat->xdisplay;
//	xcb_window_t window = device->cur_swap->wi->window;
//
//	uint32_t values[] = {device->cur_swap->info.cx,
//			     device->cur_swap->info.cy};
//
//	xcb_configure_window(XGetXCBConnection(display), window,
//			     XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
//			     values);

	UNUSED_PARAMETER(device);
	// Nothing to do?
}

void
gl_clear_context(gs_device_t *device)
{
	EGLDisplay display = device->plat->edisplay;

	if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,  EGL_NO_CONTEXT)) {
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

void
device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (device->cur_swap == swap)
		return;

	device->cur_swap = swap;

	device_enter_context(device);
}

bool
device_is_present_ready(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
    return true;
}

void
device_present(gs_device_t *device)
{
	BView *surfaceView = device->cur_swap->wi->surfaceView;
	BBitmap *surfaceBitmap = device->cur_swap->wi->surfaceBitmap;
	if (surfaceView != NULL && surfaceBitmap != NULL && surfaceView->LockLooperWithTimeout(10)) {
		surfaceView->DrawBitmap(surfaceBitmap, B_ORIGIN);
		surfaceView->UnlockLooper();
	}

	if (eglSwapInterval(device->plat->edisplay, 0) == EGL_FALSE)
		blog(LOG_ERROR, "eglSwapInterval failed");

	if (!eglSwapBuffers(device->plat->edisplay, device->cur_swap->wi->surface))
		blog(LOG_ERROR, "Cannot swap EGL buffers: %s", get_egl_error_string(eglGetError()));
}

bool
device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(monitor);
    return false;
}
