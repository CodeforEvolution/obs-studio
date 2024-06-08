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

#include "gl-subsystem.h"
#define GLAPIENTRY APIENTRY
#include <GL/osmesa.h>

#include <Bitmap.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>


struct gl_windowinfo {
	BView *view;
	BBitmap *bitmap;
	OSMesaContext context;
	gs_texture_t *texture;
    GLuint fbo;
};

struct gl_platform {
	OSMesaContext context;
	BBitmap *bitmap;
};

static OSMesaContext
gl_context_create(OSMesaContext shareContext)
{
	int attributes[] = {
		OSMESA_FORMAT, OSMESA_BGRA,
		OSMESA_DEPTH_BITS, 0,
		OSMESA_STENCIL_BITS, 0,
		OSMESA_ACCUM_BITS, 0,
		OSMESA_PROFILE, OSMESA_CORE_PROFILE,
		OSMESA_CONTEXT_MAJOR_VERSION, 3,
		OSMESA_CONTEXT_MINOR_VERSION, 3,
		0, 0
	};

	OSMesaContext context = OSMesaCreateContextAttribs(attributes, shareContext);
	if (context == nullptr) {
		blog(LOG_ERROR, "Failed to create OSMesaContext");
		return nullptr;
	}

	return context;
}

void
gl_context_destroy(struct gl_platform *plat)
{
	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);
	OSMesaDestroyContext(plat->context);
	plat->context = nullptr;
}

struct gl_windowinfo *
gl_windowinfo_create(const struct gs_init_data *info)
{
	if (!info || !info->window.window)
		return nullptr;

	auto windowInfo = static_cast<gl_windowinfo *>(bzalloc(sizeof(struct gl_windowinfo)));
	if (windowInfo == nullptr)
		return nullptr;

	auto parentWindow = static_cast<BWindow *>(info->window.window);
	windowInfo->view = parentWindow->ChildAt(0);

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

	auto plat = static_cast<gl_platform *>(bmalloc(sizeof(struct gl_platform)));
	if (plat == nullptr)
		return nullptr;

	OSMesaContext context = gl_context_create(nullptr);
	if (context == nullptr) {
		blog(LOG_ERROR, "gl_context_create failed");
		bfree(plat);
		return nullptr;
	}

	BScreen screen;
	BBitmap *renderBitmap = new(std::nothrow) BBitmap(BRect(0, 0, screen.Frame().IntegerWidth(), screen.Frame().IntegerHeight()), B_RGBA32);
	if (renderBitmap == nullptr || renderBitmap->InitCheck() != B_OK) {
		blog(LOG_ERROR, "Failed to allocate the renderBitmap");
		OSMesaDestroyContext(context);
		bfree(plat);
		return nullptr;
	}

	if (!OSMesaMakeCurrent(context, renderBitmap->Bits(), GL_UNSIGNED_BYTE, renderBitmap->Bounds().IntegerWidth() + 1, renderBitmap->Bounds().IntegerHeight() + 1)) {
		delete renderBitmap;
		OSMesaDestroyContext(context);
		bfree(plat);
		return nullptr;
	}

	if (!gladLoadGL()) {
		blog(LOG_ERROR, "Failed to load OpenGL entry functions.");
		delete renderBitmap;
		OSMesaDestroyContext(context);
		bfree(plat);
		return nullptr;
	}

	device->plat = plat;
	plat->context = context;
	plat->bitmap = renderBitmap;

	return plat;
}

void
gl_platform_destroy(struct gl_platform *platform)
{
	if (platform == nullptr)
		return;

	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);
	OSMesaDestroyContext(platform->context);
	platform->context = nullptr;

	delete platform->bitmap;
	platform->bitmap = nullptr;

	bfree(platform);
}

bool
gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	const struct gl_platform *plat = swap->device->plat;

	OSMesaContext parentContext = plat->context;
	BBitmap *renderBitmap = plat->bitmap;

	OSMesaContext swapContext = gl_context_create(parentContext);
	if (swapContext == nullptr)
		return false;

    struct gs_init_data *init_data = &swap->info;

	BBitmap *swapBitmap = new(std::nothrow) BBitmap(BRect(B_ORIGIN, BSize(init_data->cx, init_data->cy)), B_RGBA32);
	if (swapBitmap == nullptr) {
		OSMesaDestroyContext(swapContext);
		return false;
	}

	OSMesaMakeCurrent(parentContext, renderBitmap->Bits(), GL_UNSIGNED_BYTE, renderBitmap->Bounds().IntegerWidth() + 1, renderBitmap->Bounds().IntegerHeight() + 1);

	swap->wi->texture = device_texture_create(swap->device, init_data->cx, init_data->cy, init_data->format, 1,
                                                  nullptr, GS_RENDER_TARGET);
	glFlush();

	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);

	OSMesaMakeCurrent(swapContext, swapBitmap->Bits(), GL_UNSIGNED_BYTE, swapBitmap->Bounds().IntegerWidth() + 1, swapBitmap->Bounds().IntegerHeight() + 1);

	gl_gen_framebuffers(1, &swap->wi->fbo);
	gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swap->wi->texture->texture, 0);
	gl_success("glFrameBufferTexture2D");
	glFlush();

	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);

	swap->wi->context = swapContext;
	swap->wi->bitmap = swapBitmap;

	return true;
}

void
gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
    OSMesaContext parentContext = swap->device->plat->context;
	BBitmap *parentBitmap = swap->device->plat->bitmap;

    OSMesaContext swapContext = swap->wi->context;
	BBitmap* swapBitmap = swap->wi->bitmap;

	OSMesaMakeCurrent(swapContext, swapBitmap->Bits(), GL_UNSIGNED_BYTE, swapBitmap->Bounds().IntegerWidth() + 1, swapBitmap->Bounds().IntegerHeight() + 1);

    gl_delete_framebuffers(1, &swap->wi->fbo);
    glFlush();

    OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);

	OSMesaMakeCurrent(parentContext, parentBitmap->Bits(), GL_UNSIGNED_BYTE, parentBitmap->Bounds().IntegerWidth() + 1, parentBitmap->Bounds().IntegerHeight() + 1);

    gs_texture_destroy(swap->wi->texture);
    glFlush();

    OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);

	OSMesaDestroyContext(swapContext);
	swap->wi->context = nullptr;

	delete swap->wi->bitmap;
	swap->wi->bitmap = nullptr;
}

void device_enter_context(gs_device_t *device)
{
	struct gl_platform *plat = device->plat;
	OSMesaContext context = plat->context;
	BBitmap *bitmap = plat->bitmap;

	OSMesaMakeCurrent(context, bitmap->Bits(), GL_UNSIGNED_BYTE, bitmap->Bounds().IntegerWidth() + 1, bitmap->Bounds().IntegerHeight() + 1);
}

void
device_leave_context(gs_device_t *device)
{
    glFlush();
	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);

    device->cur_vertex_buffer = nullptr;
    device->cur_index_buffer = nullptr;
    device->cur_render_target = nullptr;
    device->cur_zstencil_buffer = nullptr;
    device->cur_swap = nullptr;
    device->cur_fbo = nullptr;
}

void *
device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

void
gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
//	BRect frame = swap->wi->view->Frame();
//
//	if (frame.IsValid()) {
//		*width = frame.IntegerWidth() + 1;
//		*height = frame.IntegerHeight() + 1;
//	}
    if (width)
        *width = swap->info.cx;
    if (height)
        *height = swap->info.cy;
}

void
gl_update(gs_device_t *device)
{

	UNUSED_PARAMETER(device);
	// Nothing to do?
}

void
gl_clear_context(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	OSMesaMakeCurrent(nullptr, nullptr, GL_UNSIGNED_BYTE, 0, 0);
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
	BView *view = device->cur_swap->wi->view;
	BBitmap *bitmap = device->cur_swap->wi->bitmap;
	if (view != nullptr && bitmap != nullptr && view->LockLooperWithTimeout(100) == B_OK) {
		view->DrawBitmap(bitmap, B_ORIGIN);
		view->UnlockLooper();
	}
}

bool
device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(monitor);
    return false;
}
