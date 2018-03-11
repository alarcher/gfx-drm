/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright (c) 2007-2008, 2013, 2016, Intel Corporation
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
#ifndef __DRM_PROPERTY_H__
#define __DRM_PROPERTY_H__

#include "drm_mode_object.h"

/**
 * struct drm_property_enum - symbolic values for enumerations
 * @value: numeric property value for this enum entry
 * @head: list of enum values, linked to &drm_property.enum_list
 * @name: symbolic name for the enum
 *
 * For enumeration and bitmask properties this structure stores the symbolic
 * decoding for each value. This is used for example for the rotation property.
 */
struct drm_property_enum {
	uint64_t value;
	struct list_head head;
	char name[DRM_PROP_NAME_LEN];
};

struct drm_property {
	/**
	 * @head: per-device list of properties, for cleanup.
	 */
	struct list_head head;

	/**
	 * @base: base KMS object
	 */
	struct drm_mode_object base;

	/**
	 * @flags:
	 *
	 * Property flags and type. A property needs to be one of the following
	 * types:
	 *
	 * DRM_MODE_PROP_RANGE
	 *     Range properties report their minimum and maximum admissible unsigned values.
	 *     The KMS core verifies that values set by application fit in that
	 *     range. The range is unsigned. Range properties are created using
	 *     drm_property_create_range().
	 *
	 * DRM_MODE_PROP_SIGNED_RANGE
	 *     Range properties report their minimum and maximum admissible unsigned values.
	 *     The KMS core verifies that values set by application fit in that
	 *     range. The range is signed. Range properties are created using
	 *     drm_property_create_signed_range().
	 *
	 * DRM_MODE_PROP_ENUM
	 *     Enumerated properties take a numerical value that ranges from 0 to
	 *     the number of enumerated values defined by the property minus one,
	 *     and associate a free-formed string name to each value. Applications
	 *     can retrieve the list of defined value-name pairs and use the
	 *     numerical value to get and set property instance values. Enum
	 *     properties are created using drm_property_create_enum().
	 *
	 * DRM_MODE_PROP_BITMASK
	 *     Bitmask properties are enumeration properties that additionally
	 *     restrict all enumerated values to the 0..63 range. Bitmask property
	 *     instance values combine one or more of the enumerated bits defined
	 *     by the property. Bitmask properties are created using
	 *     drm_property_create_bitmask().
	 *
	 * DRM_MODE_PROB_OBJECT
	 *     Object properties are used to link modeset objects. This is used
	 *     extensively in the atomic support to create the display pipeline,
	 *     by linking &drm_framebuffer to &drm_plane, &drm_plane to
	 *     &drm_crtc and &drm_connector to &drm_crtc. An object property can
	 *     only link to a specific type of &drm_mode_object, this limit is
	 *     enforced by the core. Object properties are created using
	 *     drm_property_create_object().
	 *
	 *     Object properties work like blob properties, but in a more
	 *     general fashion. They are limited to atomic drivers and must have
	 *     the DRM_MODE_PROP_ATOMIC flag set.
	 *
	 * DRM_MODE_PROP_BLOB
	 *     Blob properties store a binary blob without any format restriction.
	 *     The binary blobs are created as KMS standalone objects, and blob
	 *     property instance values store the ID of their associated blob
	 *     object. Blob properties are created by calling
	 *     drm_property_create() with DRM_MODE_PROP_BLOB as the type.
	 *
	 *     Actual blob objects to contain blob data are created using
	 *     drm_property_create_blob(), or through the corresponding IOCTL.
	 *
	 *     Besides the built-in limit to only accept blob objects blob
	 *     properties work exactly like object properties. The only reasons
	 *     blob properties exist is backwards compatibility with existing
	 *     userspace.
	 *
	 * In addition a property can have any combination of the below flags:
	 *
	 * DRM_MODE_PROP_ATOMIC
	 *     Set for properties which encode atomic modeset state. Such
	 *     properties are not exposed to legacy userspace.
	 *
	 * DRM_MODE_PROP_IMMUTABLE
	 *     Set for properties where userspace cannot be changed by
	 *     userspace. The kernel is allowed to update the value of these
	 *     properties. This is generally used to expose probe state to
	 *     usersapce, e.g. the EDID, or the connector path property on DP
	 *     MST sinks.
	 */
	uint32_t flags;

	/**
	 * @name: symbolic name of the properties
	 */
	char name[DRM_PROP_NAME_LEN];

	/**
	 * @num_values: size of the @values array.
	 */
	uint32_t num_values;

	/**
	 * @values:
	 *
	 * Array with limits and values for the property. The
	 * interpretation of these limits is dependent upon the type per @flags.
	 */
	uint64_t *values;


	/**
	 * @enum_blob_list:
	 *
	 * List of &drm_prop_enum_list structures with the symbolic names for
	 * enum and bitmask values.
	 */
	struct list_head enum_blob_list;
};

/**
 * struct drm_property_blob - Blob data for &drm_property
 * @base: base KMS object
 * @head: entry on the global blob list in &drm_mode_config.property_blob_list.
 * @length: size of the blob in bytes, invariant over the lifetime of the object
 * @data: actual data, embedded at the end of this structure
 *
 * Blobs are used to store bigger values than what fits directly into the 64
 * bits available for a &drm_property..
 */
struct drm_property_blob {
	struct drm_mode_object base;
	struct list_head head;
	unsigned int length;
	unsigned char data[];
};

struct drm_prop_enum_list {
	int type;
	char *name;
};

#define obj_to_property(x) container_of(x, struct drm_property, base)
#define obj_to_blob(x) container_of(x, struct drm_property_blob, base)

struct drm_property *drm_property_create(struct drm_device *dev, int flags,
						const char *name, int num_values);
struct drm_property *drm_property_create_enum(struct drm_device *dev, int flags,
					 const char *name,
					 const struct drm_prop_enum_list *props,
					 int num_values);
struct drm_property *drm_property_create_bitmask(struct drm_device *dev,
					 int flags, const char *name,
					 const struct drm_prop_enum_list *props,
					 int num_values);
struct drm_property *drm_property_create_range(struct drm_device *dev, int flags,
					 const char *name,
					 uint64_t min, uint64_t max);
int drm_property_add_enum(struct drm_property *property, int index,
				 uint64_t value, const char *name);
void drm_property_destroy(struct drm_device *dev, struct drm_property *property);

#endif /* __DRM_PROPERTY_H__ */
