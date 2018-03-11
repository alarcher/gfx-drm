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

#ifndef __DRM_MODESET_H__
#define __DRM_MODESET_H__

#include <uapi/drm/drm_mode.h>

struct drm_object_properties;
struct drm_device;


struct drm_mode_object {
	uint32_t id;
	uint32_t type;
	struct drm_object_properties *properties;
};

#define DRM_OBJECT_MAX_PROPERTY 24
/**
 * struct drm_object_properties - property tracking for &drm_mode_object
 */
struct drm_object_properties {
	/**
	 * @count: number of valid properties, must be less than or equal to
	 * DRM_OBJECT_MAX_PROPERTY.
	 */

	int count;
	uint32_t ids[DRM_OBJECT_MAX_PROPERTY];
	uint64_t values[DRM_OBJECT_MAX_PROPERTY];
};

struct drm_mode_object *drm_mode_object_find(struct drm_device *dev,
		uint32_t id, uint32_t type);
int drm_mode_object_get(struct drm_device *dev,
			       struct drm_mode_object *obj, uint32_t obj_type)

int drm_object_property_set_value(struct drm_mode_object *obj,
					 struct drm_property *property,
					 uint64_t val);
int drm_object_property_get_value(struct drm_mode_object *obj,
					 struct drm_property *property,
					 uint64_t *value);
void drm_object_attach_property(struct drm_mode_object *obj,
				       struct drm_property *property,
				       uint64_t init_val);

#endif /* __DRM_MODESET_H__ */
