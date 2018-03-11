/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright (c) 2007-2008, 2013 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DRM_FRAMEBUFFER_H__
#define __DRM_FRAMEBUFFER_H__

#include "drm_linux_list.h"
#include "drm_mode_object.h"

struct drm_framebuffer;
struct drm_file;
struct drm_device;

/**
 * struct drm_framebuffer_funcs - framebuffer hooks
 */
struct drm_framebuffer_funcs {
	/* note: use drm_framebuffer_remove() */
	void (*destroy)(struct drm_framebuffer *framebuffer);
	int (*create_handle)(struct drm_framebuffer *fb,
			     struct drm_file *file_priv,
			     unsigned int *handle);
	/**
	 * Optinal callback for the dirty fb ioctl.
	 *
	 * Userspace can notify the driver via this callback
	 * that a area of the framebuffer has changed and should
	 * be flushed to the display hardware.
	 *
	 * See documentation in drm_mode.h for the struct
	 * drm_mode_fb_dirty_cmd for more information as all
	 * the semantics and arguments have a one to one mapping
	 * on this function.
	 */
	int (*dirty)(struct drm_framebuffer *framebuffer,
		     struct drm_file *file_priv, unsigned flags,
		     unsigned color, struct drm_clip_rect *clips,
		     unsigned num_clips);
};

/**
 * struct drm_framebuffer - frame buffer object
 */
struct drm_framebuffer {
	struct drm_device *dev;
	/*
	 * Note that the fb is refcounted for the benefit of driver internals,
	 * for example some hw, disabling a CRTC/plane is asynchronous, and
	 * scanout does not actually complete until the next vblank.  So some
	 * cleanup (like releasing the reference(s) on the backing GEM bo(s))
	 * should be deferred.  In cases like this, the driver would like to
	 * hold a ref to the fb even though it has already been removed from
	 * userspace perspective.
	 */
	struct kref refcount;
	/*
	 * Place on the dev->mode_config.fb_list, access protected by
	 * dev->mode_config.fb_lock.
	 */
	struct list_head head;
	struct drm_mode_object base;
	const struct drm_framebuffer_funcs *funcs;
	unsigned int pitches[4];
	unsigned int offsets[4];
	unsigned int width;
	unsigned int height;
	/* depth can be 15 or 16 */
	unsigned int depth;
	int bits_per_pixel;
	int flags;
	uint32_t pixel_format; /* fourcc format */
	struct list_head filp_head;
	/* if you are using the helper */
	void *helper_private;
};

#define obj_to_fb(x) container_of(x, struct drm_framebuffer, base)

int drm_framebuffer_init(struct drm_device *dev,
				struct drm_framebuffer *fb,
				const struct drm_framebuffer_funcs *funcs);
struct drm_framebuffer *drm_framebuffer_lookup(struct drm_device *dev,
						      uint32_t id);
void drm_framebuffer_remove(struct drm_framebuffer *fb);
void drm_framebuffer_cleanup(struct drm_framebuffer *fb);
void drm_framebuffer_unregister_private(struct drm_framebuffer *fb);

void drm_framebuffer_unreference(struct drm_framebuffer *fb);
void drm_framebuffer_reference(struct drm_framebuffer *fb);

#endif /* __DRM_FRAMEBUFFER_H__ */
