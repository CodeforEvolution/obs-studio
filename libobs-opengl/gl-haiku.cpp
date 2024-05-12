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

#include <GLView.h>

struct gl_windowinfo {
    BWindow *window;
    BGLView *view;
    gs_texture_t *texture;
    GLuint fbo;
};

struct gl_platform {
    BGLView *view;
};

static BGLView *
gl_context_create(bool share)
{
    uint32 mode = BGL_DOUBLE;

    if (share)
        mode |= BGL_SHARE_CONTEXT;

    const char* name = share ? "libobs-opengl-share" : "libobs-opengl";

    BGLView *view = new(std::nothrow) BGLView(BRect(), name, B_FOLLOW_ALL, 0,
        mode);
    if (!view) {
        blog(LOG_ERROR, "Failed to create BGLView");
        return NULL;
    }

    return view;
}

struct gl_platform *
gl_platform_create(gs_device_t *device, uint32_t adapter)
{
    UNUSED_PARAMETER(device);
    UNUSED_PARAMETER(adapter);

    BGLView *view = gl_context_create(false);
    if (!view) {
        blog(LOG_ERROR, "gl_context_create failed");
        return NULL;
    }

    view->LockGL();
    const bool success = gladLoadGL() != 0;

    if (!success) {
        blog(LOG_ERROR, "gladLoadGL failed");
        delete view;
        return NULL;
    }

    struct gl_platform *plat = static_cast<gl_platform*>(bzalloc(sizeof(struct gl_platform)));
    plat->view = view;
    return plat;
}

void
gl_platform_destroy(struct gl_platform *platform)
{
    if (!platform)
        return;

    platform->view->UnlockGL();
    platform->view = NULL;

    bfree(platform);
}

bool
gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
    BGLView *parent = swap->device->plat->view;
    BGLView *context = gl_context_create(true);
    bool success = context != NULL;
    if (success) {
        parent->LockGL();

        struct gs_init_data *init_data = &swap->info;
        swap->wi->texture = device_texture_create(swap->device, init_data->cx, init_data->cy, init_data->format, 1,
                                                  NULL, GS_RENDER_TARGET);
        glFlush();

        parent->UnlockGL();

        context->LockGL();

        gl_gen_framebuffers(1, &swap->wi->fbo);
        gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swap->wi->texture->texture, 0);
        gl_success("glFrameBufferTexture2D");
        glFlush();

        context->UnlockGL();

        swap->wi->view = context;
    }

    return success;
}

void
gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
    BGLView *parent = swap->device->plat->view;
    // CGLContextObj parent_obj = [parent CGLContextObj];
    // CGLLockContext(parent_obj);

    BGLView *context = swap->wi->view;
    // CGLContextObj context_obj = [context CGLContextObj];
    // CGLLockContext(context_obj);

    // [context makeCurrentContext];
    context->LockGL();

    gl_delete_framebuffers(1, &swap->wi->fbo);
    glFlush();

    // [NSOpenGLContext clearCurrentContext];
    context->UnlockGL();

    // CGLUnlockContext(context_obj);

    // [parent makeCurrentContext];
    parent->LockGL();

    gs_texture_destroy(swap->wi->texture);
    glFlush();

    // [NSOpenGLContext clearCurrentContext];
    parent->UnlockGL();

    delete swap->wi->view;
    swap->wi->view = NULL;
}

struct gl_windowinfo *
gl_windowinfo_create(const struct gs_init_data *info)
{
    if (!info)
        return NULL;

    struct gl_windowinfo *wi = static_cast<gl_windowinfo*>(bzalloc(sizeof(struct gl_windowinfo)));

    return wi;
}

void
gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
    if (!wi)
        return;

    wi->view = NULL;
    bfree(wi);
}

void
gl_update(gs_device_t *device)
{
    gs_swapchain_t *swap = device->cur_swap;
//    BGLView *parent = device->plat->view;
    BGLView *context = swap->wi->view;

    if (!swap || !swap->wi) {
        return;
    }

    // CGLContextObj parent_obj = [parent CGLContextObj];
    // CGLLockContext(parent_obj);

    // CGLContextObj context_obj = [context CGLContextObj];
    // CGLLockContext(context_obj);

    // [context makeCurrentContext];
    // [context update];
    context->LockGL();

    struct gs_init_data *info = &swap->info;
    gs_texture_t *previous = swap->wi->texture;
    swap->wi->texture = device_texture_create(device, info->cx, info->cy, info->format, 1, NULL, GS_RENDER_TARGET);
    gl_bind_framebuffer(GL_FRAMEBUFFER, swap->wi->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swap->wi->texture->texture, 0);
    gl_success("glFrameBufferTexture2D");
    gs_texture_destroy(previous);
    glFlush();

    // [NSOpenGLContext clearCurrentContext];
    context->UnlockGL();

    // CGLUnlockContext(context_obj);
    // CGLUnlockContext(parent_obj);
}

void
gl_clear_context(gs_device_t *device)
{
    // UNUSED_PARAMETER(device);
    // [NSOpenGLContext clearCurrentContext];
    device->plat->view->UnlockGL();
}

void
device_enter_context(gs_device_t *device)
{
    // CGLLockContext([device->plat->context CGLContextObj]);
    // [device->plat->context makeCurrentContext];
    device->plat->view->LockGL();
}

void
device_leave_context(gs_device_t *device)
{
    glFlush();

    // [NSOpenGLContext clearCurrentContext];
    device->plat->view->UnlockGL();

    device->cur_vertex_buffer = NULL;
    device->cur_index_buffer = NULL;
    device->cur_render_target = NULL;
    device->cur_zstencil_buffer = NULL;
    device->cur_swap = NULL;
    device->cur_fbo = NULL;

    // CGLUnlockContext([device->plat->context CGLContextObj]);
}

void *
device_get_device_obj(gs_device_t *device)
{
    return device->plat->view;
}

void
device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
    if (device->cur_swap == swap)
        return;

    device->cur_swap = swap;
    if (swap) {
        device_set_render_target(device, swap->wi->texture, NULL);
    }
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
    glFlush();
    // [NSOpenGLContext clearCurrentContext];
    device->plat->view->UnlockGL();

    // CGLLockContext([device->cur_swap->wi->context CGLContextObj]);

    // [device->cur_swap->wi->context makeCurrentContext];
    device->cur_swap->wi->view->LockGL();

    gl_bind_framebuffer(GL_READ_FRAMEBUFFER, device->cur_swap->wi->fbo);
    gl_bind_framebuffer(GL_DRAW_FRAMEBUFFER, 0);
    const uint32_t width = device->cur_swap->info.cx;
    const uint32_t height = device->cur_swap->info.cy;
    glBlitFramebuffer(0, 0, width, height, 0, height, width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    device->cur_swap->wi->view->SwapBuffers();
    glFlush();

    // [NSOpenGLContext clearCurrentContext];
    device->cur_swap->wi->view->UnlockGL();

    // CGLUnlockContext([device->cur_swap->wi->context CGLContextObj]);

    // [device->plat->context makeCurrentContext];
    device->plat->view->LockGL();
}

bool
device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
    UNUSED_PARAMETER(device);
    UNUSED_PARAMETER(monitor);

    return false;
}

void
gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width, uint32_t *height)
{
    if (width)
        *width = swap->info.cx;
    if (height)
        *height = swap->info.cy;
}
