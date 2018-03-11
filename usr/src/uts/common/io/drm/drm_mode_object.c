/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright (c) 2006-2008, 2013, Intel Corporation
 * Copyright (c) 2007 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2008 Red Hat Inc.
 *
 * DRM core CRTC related functions
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Authors:
 *      Keith Packard
 *	Eric Anholt <eric@anholt.net>
 *      Dave Airlie <airlied@linux.ie>
 *      Jesse Barnes <jesse.barnes@intel.com>
 */

#include <drm/drmP.h>
#include <drm/drm_mode_object.h>

/**
 * drm_mode_object_find - look up a drm object with static lifetime
 * @dev: drm device
 * @id: id of the mode object
 * @type: type of the mode object
 *
 * Note that framebuffers cannot be looked up with this functions - since those
 * are reference counted, they need special treatment.
 */
struct drm_mode_object *drm_mode_object_find(struct drm_device *dev,
		uint32_t id, uint32_t type)
{
	struct drm_mode_object *obj = NULL;

	/* Framebuffers are reference counted and need their own lookup
	 * function.*/
	WARN_ON(type == DRM_MODE_OBJECT_FB);

	mutex_lock(&dev->mode_config.idr_mutex);
	obj = idr_find(&dev->mode_config.crtc_idr, id);
	if (!obj || (obj->type != type) || (obj->id != id))
		obj = NULL;
	mutex_unlock(&dev->mode_config.idr_mutex);

	return obj;
}

/**
 * drm_mode_object_get - allocate a new modeset identifier
 * @dev: DRM device
 * @obj: object pointer, used to generate unique ID
 * @obj_type: object type
 *
 * Create a unique identifier based on @ptr in @dev's identifier space.  Used
 * for tracking modes, CRTCs and connectors.
 *
 * RETURNS:
 * New unique (relative to other objects in @dev) integer identifier for the
 * object.
 */
int drm_mode_object_get(struct drm_device *dev,
			       struct drm_mode_object *obj, uint32_t obj_type)
{
	int new_id = 0;
	int ret;

again:
	if (idr_pre_get(&dev->mode_config.crtc_idr, GFP_KERNEL) == 0) {
		DRM_ERROR("Ran out memory getting a mode number\n");
		return -ENOMEM;
	}

	mutex_lock(&dev->mode_config.idr_mutex);
	ret = idr_get_new_above(&dev->mode_config.crtc_idr, obj, 1, &new_id);

	if (!ret) {
		/*
		 * Set up the object linking under the protection of the idr
		 * lock so that other users can't see inconsistent state.
		 */
		obj->id = new_id;
		obj->type = obj_type;
	}
	mutex_unlock(&dev->mode_config.idr_mutex);

	if (ret == -EAGAIN)
		goto again;
	return ret;
}

/**
 * drm_mode_object_put - free an identifer
 * @dev: DRM device
 * @id: ID to free
 *
 *
 * Free @id from @dev's unique identifier pool.
 */
void drm_mode_object_put(struct drm_device *dev,
				struct drm_mode_object *object)
{
	mutex_lock(&dev->mode_config.idr_mutex);
	(void) idr_remove(&dev->mode_config.crtc_idr, object->id);
	mutex_unlock(&dev->mode_config.idr_mutex);
}


int drm_object_property_set_value(struct drm_mode_object *obj,
				  struct drm_property *property, uint64_t val)
{
	int i;

	for (i = 0; i < obj->properties->count; i++) {
		if (obj->properties->ids[i] == property->base.id) {
			obj->properties->values[i] = val;
			return 0;
		}
	}

	return -EINVAL;
}

int drm_object_property_get_value(struct drm_mode_object *obj,
				  struct drm_property *property, uint64_t *val)
{
	int i;

	for (i = 0; i < obj->properties->count; i++) {
		if (obj->properties->ids[i] == property->base.id) {
			*val = obj->properties->values[i];
			return 0;
		}
	}

	return -EINVAL;
}

void drm_object_attach_property(struct drm_mode_object *obj,
				struct drm_property *property,
				uint64_t init_val)
{
	int count = obj->properties->count;

	if (count == DRM_OBJECT_MAX_PROPERTY) {
		DRM_ERROR("Failed to attach object property (type: 0x%x). Please "
			"increase DRM_OBJECT_MAX_PROPERTY by 1 for each time "
			"you see this message on the same object type.\n",
			obj->type);
		return;
	}

	obj->properties->ids[count] = property->base.id;
	obj->properties->values[count] = init_val;
	obj->properties->count++;
}

/* LINTED */
int drm_mode_obj_get_properties_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_obj_get_properties *arg = data;
	struct drm_mode_object *obj;
	int ret = 0;
	int i;
	int copied = 0;
	int props_count = 0;
	uint32_t __user *props_ptr;
	uint64_t __user *prop_values_ptr;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);

	obj = drm_mode_object_find(dev, arg->obj_id, arg->obj_type);
	if (!obj) {
		ret = -EINVAL;
		goto out;
	}
	if (!obj->properties) {
		ret = -EINVAL;
		goto out;
	}

	props_count = obj->properties->count;

	/* This ioctl is called twice, once to determine how much space is
	 * needed, and the 2nd time to fill it. */
	if ((arg->count_props >= props_count) && props_count) {
		copied = 0;
		props_ptr = (uint32_t __user *)(unsigned long)(arg->props_ptr);
		prop_values_ptr = (uint64_t __user *)(unsigned long)
				  (arg->prop_values_ptr);
		for (i = 0; i < props_count; i++) {
			if (put_user(obj->properties->ids[i],
				     props_ptr + copied)) {
				ret = -EFAULT;
				goto out;
			}
			if (put_user(obj->properties->values[i],
				     prop_values_ptr + copied)) {
				ret = -EFAULT;
				goto out;
			}
			copied++;
		}
	}
	arg->count_props = props_count;
out:
	drm_modeset_unlock_all(dev);
	return ret;
}

static bool drm_property_change_is_valid(struct drm_property *property,
					 uint64_t value)
{
	if (property->flags & DRM_MODE_PROP_IMMUTABLE)
		return false;
	if (property->flags & DRM_MODE_PROP_RANGE) {
		if (value < property->values[0] || value > property->values[1])
			return false;
		return true;
	} else if (property->flags & DRM_MODE_PROP_BITMASK) {
		int i;
		uint64_t valid_mask = 0;
		for (i = 0; i < property->num_values; i++)
			valid_mask |= (1ULL << property->values[i]);
		return !(value & ~valid_mask);
	} else if (property->flags & DRM_MODE_PROP_BLOB) {
		/* Only the driver knows */
		return true;
	} else {
		int i;
		for (i = 0; i < property->num_values; i++)
			if (property->values[i] == value)
				return true;
		return false;
	}
}

/* LINTED */
int drm_mode_obj_set_property_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_obj_set_property *arg = data;
	struct drm_mode_object *arg_obj;
	struct drm_mode_object *prop_obj;
	struct drm_property *property;
	int ret = -EINVAL;
	int i;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);

	arg_obj = drm_mode_object_find(dev, arg->obj_id, arg->obj_type);
	if (!arg_obj)
		goto out;
	if (!arg_obj->properties)
		goto out;

	for (i = 0; i < arg_obj->properties->count; i++)
		if (arg_obj->properties->ids[i] == arg->prop_id)
			break;

	if (i == arg_obj->properties->count)
		goto out;

	prop_obj = drm_mode_object_find(dev, arg->prop_id,
					DRM_MODE_OBJECT_PROPERTY);
	if (!prop_obj)
		goto out;
	property = obj_to_property(prop_obj);

	if (!drm_property_change_is_valid(property, arg->value))
		goto out;

	switch (arg_obj->type) {
	case DRM_MODE_OBJECT_CONNECTOR:
		ret = drm_mode_connector_set_obj_prop(arg_obj, property,
						      arg->value);
		break;
	case DRM_MODE_OBJECT_CRTC:
		ret = drm_mode_crtc_set_obj_prop(arg_obj, property, arg->value);
		break;
	case DRM_MODE_OBJECT_PLANE:
		ret = drm_mode_plane_set_obj_prop(arg_obj, property, arg->value);
		break;
	}

out:
	drm_modeset_unlock_all(dev);
	return ret;
}
