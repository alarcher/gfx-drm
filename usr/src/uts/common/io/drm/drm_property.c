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
#include <drm/drm_property.h>

struct drm_property *drm_property_create(struct drm_device *dev, int flags,
					 const char *name, int num_values)
{
	struct drm_property *property = NULL;
	int ret;

	property = kzalloc(sizeof(struct drm_property), GFP_KERNEL);
	if (!property)
		return NULL;

	if (num_values) {
		property->values = kzalloc(sizeof(uint64_t)*num_values, GFP_KERNEL);
		if (!property->values)
			goto fail;
	}

	ret = drm_mode_object_get(dev, &property->base, DRM_MODE_OBJECT_PROPERTY);
	if (ret)
		goto fail;

	property->flags = flags;
	property->num_values = num_values;
	INIT_LIST_HEAD(&property->enum_blob_list);

	if (name) {
		(void) strncpy(property->name, name, DRM_PROP_NAME_LEN);
		property->name[DRM_PROP_NAME_LEN-1] = '\0';
	}

	list_add_tail(&property->head, &dev->mode_config.property_list, (caddr_t)property);
	return property;
fail:
	kfree(property->values, sizeof(uint64_t)*num_values);
	kfree(property, sizeof(struct drm_property));
	return NULL;
}

struct drm_property *drm_property_create_enum(struct drm_device *dev, int flags,
					 const char *name,
					 const struct drm_prop_enum_list *props,
					 int num_values)
{
	struct drm_property *property;
	int i, ret;

	flags |= DRM_MODE_PROP_ENUM;

	property = drm_property_create(dev, flags, name, num_values);
	if (!property)
		return NULL;

	for (i = 0; i < num_values; i++) {
		ret = drm_property_add_enum(property, i,
				      props[i].type,
				      props[i].name);
		if (ret) {
			drm_property_destroy(dev, property);
			return NULL;
		}
	}

	return property;
}

struct drm_property *drm_property_create_bitmask(struct drm_device *dev,
					 int flags, const char *name,
					 const struct drm_prop_enum_list *props,
					 int num_values)
{
	struct drm_property *property;
	int i, ret;

	flags |= DRM_MODE_PROP_BITMASK;

	property = drm_property_create(dev, flags, name, num_values);
	if (!property)
		return NULL;

	for (i = 0; i < num_values; i++) {
		ret = drm_property_add_enum(property, i,
				      props[i].type,
				      props[i].name);
		if (ret) {
			drm_property_destroy(dev, property);
			return NULL;
		}
	}

	return property;
}

struct drm_property *drm_property_create_range(struct drm_device *dev, int flags,
					 const char *name,
					 uint64_t min, uint64_t max)
{
	struct drm_property *property;

	flags |= DRM_MODE_PROP_RANGE;

	property = drm_property_create(dev, flags, name, 2);
	if (!property)
		return NULL;

	property->values[0] = min;
	property->values[1] = max;

	return property;
}

int drm_property_add_enum(struct drm_property *property, int index,
			  uint64_t value, const char *name)
{
	struct drm_property_enum *prop_enum;

	if (!(property->flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)))
		return -EINVAL;

	/*
	 * Bitmask enum properties have the additional constraint of values
	 * from 0 to 63
	 */
	if ((property->flags & DRM_MODE_PROP_BITMASK) && (value > 63))
		return -EINVAL;

	if (!list_empty(&property->enum_blob_list)) {
		list_for_each_entry(prop_enum, struct drm_property_enum, &property->enum_blob_list, head) {
			if (prop_enum->value == value) {
				(void) strncpy(prop_enum->name, name, DRM_PROP_NAME_LEN);
				prop_enum->name[DRM_PROP_NAME_LEN-1] = '\0';
				return 0;
			}
		}
	}

	prop_enum = kzalloc(sizeof(struct drm_property_enum), GFP_KERNEL);
	if (!prop_enum)
		return -ENOMEM;

	(void) strncpy(prop_enum->name, name, DRM_PROP_NAME_LEN);
	prop_enum->name[DRM_PROP_NAME_LEN-1] = '\0';
	prop_enum->value = value;

	property->values[index] = value;
	list_add_tail(&prop_enum->head, &property->enum_blob_list, (caddr_t)prop_enum);
	return 0;
}

void drm_property_destroy(struct drm_device *dev, struct drm_property *property)
{
	struct drm_property_enum *prop_enum, *pt;

	list_for_each_entry_safe(prop_enum, pt, struct drm_property_enum, &property->enum_blob_list, head) {
		list_del(&prop_enum->head);
		kfree(prop_enum, sizeof(struct drm_property_enum));
	}

	if (property->num_values)
		kfree(property->values, sizeof(uint64_t) * property->num_values);
	drm_mode_object_put(dev, &property->base);
	list_del(&property->head);
	kfree(property, sizeof(struct drm_property));
}

/* LINTED */
int drm_mode_getproperty_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_object *obj;
	struct drm_mode_get_property *out_resp = data;
	struct drm_property *property;
	int enum_count = 0;
	int blob_count = 0;
	int value_count = 0;
	int ret = 0, i;
	int copied;
	struct drm_property_enum *prop_enum;
	struct drm_mode_property_enum __user *enum_ptr;
	struct drm_property_blob *prop_blob;
	uint32_t *blob_id_ptr;
	uint64_t *values_ptr;
	uint32_t *blob_length_ptr;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);
	obj = drm_mode_object_find(dev, out_resp->prop_id, DRM_MODE_OBJECT_PROPERTY);
	if (!obj) {
		ret = -EINVAL;
		goto done;
	}
	property = obj_to_property(obj);

	if (property->flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)) {
		list_for_each_entry(prop_enum, struct drm_property_enum, &property->enum_blob_list, head)
			enum_count++;
	} else if (property->flags & DRM_MODE_PROP_BLOB) {
		list_for_each_entry(prop_blob, struct drm_property_blob, &property->enum_blob_list, head)
			blob_count++;
	}

	value_count = property->num_values;

	(void) strncpy(out_resp->name, property->name, DRM_PROP_NAME_LEN);
	out_resp->name[DRM_PROP_NAME_LEN-1] = 0;
	out_resp->flags = property->flags;

	if ((out_resp->count_values >= value_count) && value_count) {
		values_ptr = (uint64_t *)(unsigned long)out_resp->values_ptr;
		for (i = 0; i < value_count; i++) {
			if (DRM_COPY_TO_USER(values_ptr + i, &property->values[i], sizeof(uint64_t))) {
				ret = -EFAULT;
				goto done;
			}
		}
	}
	out_resp->count_values = value_count;

	if (property->flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)) {
		if ((out_resp->count_enum_blobs >= enum_count) && enum_count) {
			copied = 0;
			enum_ptr = (struct drm_mode_property_enum *)(unsigned long)out_resp->enum_blob_ptr;
			list_for_each_entry(prop_enum, struct drm_property_enum, &property->enum_blob_list, head) {

				if (DRM_COPY_TO_USER(&enum_ptr[copied].value, &prop_enum->value, sizeof(uint64_t))) {
					ret = -EFAULT;
					goto done;
				}

				if (DRM_COPY_TO_USER(&enum_ptr[copied].name,
						 &prop_enum->name, DRM_PROP_NAME_LEN)) {
					ret = -EFAULT;
					goto done;
				}
				copied++;
			}
		}
		out_resp->count_enum_blobs = enum_count;
	}

	if (property->flags & DRM_MODE_PROP_BLOB) {
		if ((out_resp->count_enum_blobs >= blob_count) && blob_count) {
			copied = 0;
			blob_id_ptr = (uint32_t *)(unsigned long)out_resp->enum_blob_ptr;
			blob_length_ptr = (uint32_t *)(unsigned long)out_resp->values_ptr;

			list_for_each_entry(prop_blob, struct drm_property_blob, &property->enum_blob_list, head) {
				if (put_user(prop_blob->base.id, blob_id_ptr + copied)) {
					ret = -EFAULT;
					goto done;
				}

				if (put_user(prop_blob->length, blob_length_ptr + copied)) {
					ret = -EFAULT;
					goto done;
				}

				copied++;
			}
		}
		out_resp->count_enum_blobs = blob_count;
	}
done:
	drm_modeset_unlock_all(dev);
	return ret;
}

static struct drm_property_blob *drm_property_create_blob(struct drm_device *dev, int length,
							  void *data)
{
	struct drm_property_blob *blob;
	int ret;

	if (!length || !data)
		return NULL;

	blob = kzalloc(sizeof(struct drm_property_blob)+length, GFP_KERNEL);
	if (!blob)
		return NULL;

	ret = drm_mode_object_get(dev, &blob->base, DRM_MODE_OBJECT_BLOB);
	if (ret) {
		kfree(blob, sizeof(struct drm_property_blob)+length);
		return NULL;
	}

	blob->length = length;

	(void) memcpy(blob->data, data, length);

	list_add_tail(&blob->head, &dev->mode_config.property_blob_list, (caddr_t)blob);
	return blob;
}

static void drm_property_destroy_blob(struct drm_device *dev,
			       struct drm_property_blob *blob)
{
	drm_mode_object_put(dev, &blob->base);
	list_del(&blob->head);
	kfree(blob, sizeof(struct drm_property_blob) + blob->length);
}

/* LINTED */
int drm_mode_getblob_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_object *obj;
	struct drm_mode_get_blob *out_resp = data;
	struct drm_property_blob *blob;
	int ret = 0;
	void *blob_ptr;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);
	obj = drm_mode_object_find(dev, out_resp->blob_id, DRM_MODE_OBJECT_BLOB);
	if (!obj) {
		ret = -EINVAL;
		goto done;
	}
	blob = obj_to_blob(obj);

	if (out_resp->length == blob->length) {
		blob_ptr = (void *)(unsigned long)out_resp->data;
		if (DRM_COPY_TO_USER(blob_ptr, blob->data, blob->length)){
			ret = -EFAULT;
			goto done;
		}
	}
	out_resp->length = blob->length;

done:
	drm_modeset_unlock_all(dev);
	return ret;
}

