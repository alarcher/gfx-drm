/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright (c) 2007-2008, 2013, Intel Corporation
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
#ifndef __DRM_PLANE_H__
#define __DRM_PLANE_H__

#include "drm_mode_object.h"

/**
 * drm_plane_funcs - driver plane control functions
 * @update_plane: update the plane configuration
 * @disable_plane: shut down the plane
 * @destroy: clean up plane resources
 * @set_property: called when a property is changed
 */
struct drm_plane_funcs {
	int (*update_plane)(struct drm_plane *plane,
			    struct drm_crtc *crtc, struct drm_framebuffer *fb,
			    int crtc_x, int crtc_y,
			    unsigned int crtc_w, unsigned int crtc_h,
			    uint32_t src_x, uint32_t src_y,
			    uint32_t src_w, uint32_t src_h);
	int (*disable_plane)(struct drm_plane *plane);
	void (*destroy)(struct drm_plane *plane);

	int (*set_property)(struct drm_plane *plane,
			    struct drm_property *property, uint64_t val);
};

/**
 * drm_plane - central DRM plane control structure
 * @dev: DRM device this plane belongs to
 * @head: for list management
 * @base: base mode object
 * @possible_crtcs: pipes this plane can be bound to
 * @format_types: array of formats supported by this plane
 * @format_count: number of formats supported
 * @crtc: currently bound CRTC
 * @fb: currently bound fb
 * @funcs: helper functions
 * @properties: property tracking for this plane
 */
struct drm_plane {
	struct drm_device *dev;
	struct list_head head;

	struct drm_mode_object base;

	uint32_t possible_crtcs;
	uint32_t *format_types;
	uint32_t format_count;

	struct drm_crtc *crtc;
	struct drm_framebuffer *fb;

	const struct drm_plane_funcs *funcs;

	struct drm_object_properties properties;
};

#define obj_to_plane(x) container_of(x, struct drm_plane, base)

int drm_plane_init(struct drm_device *dev,
			  struct drm_plane *plane,
			  unsigned long possible_crtcs,
			  const struct drm_plane_funcs *funcs,
			  const uint32_t *formats, uint32_t format_count,
			  bool priv);
void drm_plane_cleanup(struct drm_plane *plane);
void drm_plane_force_disable(struct drm_plane *plane);
int drm_mode_plane_set_obj_prop(struct drm_mode_object *obj,
				      struct drm_property *property,
				      uint64_t value);

#endif /* __DRM_PLANE_H__ */
