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
#include <drm/drm_plane.h>

/**
 * drm_plane_init - Initialise a new plane object
 * @dev: DRM device
 * @plane: plane object to init
 * @possible_crtcs: bitmask of possible CRTCs
 * @funcs: callbacks for the new plane
 * @formats: array of supported formats (%DRM_FORMAT_*)
 * @format_count: number of elements in @formats
 * @priv: plane is private (hidden from userspace)?
 *
 * Inits a new object created as base part of a driver plane object.
 *
 * RETURNS:
 * Zero on success, error code on failure.
 */
int drm_plane_init(struct drm_device *dev, struct drm_plane *plane,
		   unsigned long possible_crtcs,
		   const struct drm_plane_funcs *funcs,
		   const uint32_t *formats, uint32_t format_count,
		   bool priv)
{
	int ret;

	drm_modeset_lock_all(dev);

	ret = drm_mode_object_get(dev, &plane->base, DRM_MODE_OBJECT_PLANE);
	if (ret)
		goto out;

	plane->base.properties = &plane->properties;
	plane->dev = dev;
	plane->funcs = funcs;
	plane->format_types = kmalloc(sizeof(uint32_t) * format_count,
				      GFP_KERNEL);
	if (!plane->format_types) {
		DRM_DEBUG_KMS("out of memory when allocating plane\n");
		drm_mode_object_put(dev, &plane->base);
		ret = -ENOMEM;
		goto out;
	}

	(void) memcpy(plane->format_types, formats, format_count * sizeof(uint32_t));
	plane->format_count = format_count;
	plane->possible_crtcs = (uint32_t) possible_crtcs;

	/* private planes are not exposed to userspace, but depending on
	 * display hardware, might be convenient to allow sharing programming
	 * for the scanout engine with the crtc implementation.
	 */
	if (!priv) {
		list_add_tail(&plane->head, &dev->mode_config.plane_list, (caddr_t)plane);
		dev->mode_config.num_plane++;
	} else {
		INIT_LIST_HEAD(&plane->head);
	}

 out:
	drm_modeset_unlock_all(dev);

	return ret;
}

/**
 * drm_plane_cleanup - Clean up the core plane usage
 * @plane: plane to cleanup
 *
 * This function cleans up @plane and removes it from the DRM mode setting
 * core. Note that the function does *not* free the plane structure itself,
 * this is the responsibility of the caller.
 */
void drm_plane_cleanup(struct drm_plane *plane)
{
	struct drm_device *dev = plane->dev;

	drm_modeset_lock_all(dev);
	kfree(plane->format_types, sizeof(uint32_t) * plane->format_count);
	drm_mode_object_put(dev, &plane->base);
	/* if not added to a list, it must be a private plane */
	if (!list_empty(&plane->head)) {
		list_del(&plane->head);
		dev->mode_config.num_plane--;
	}
	drm_modeset_unlock_all(dev);
}

/**
 * drm_plane_force_disable - Forcibly disable a plane
 * @plane: plane to disable
 *
 * Forces the plane to be disabled.
 *
 * Used when the plane's current framebuffer is destroyed,
 * and when restoring fbdev mode.
 */
void drm_plane_force_disable(struct drm_plane *plane)
{
	int ret;

	if (!plane->fb)
		return;

	ret = plane->funcs->disable_plane(plane);
	if (ret)
		DRM_ERROR("failed to disable plane with busy fb\n");
	/* disconnect the plane from the fb and crtc: */
	__drm_framebuffer_unreference(plane->fb);
	plane->fb = NULL;
	plane->crtc = NULL;
}

int drm_mode_plane_set_obj_prop(struct drm_mode_object *obj,
				      struct drm_property *property,
				      uint64_t value)
{
	int ret = -EINVAL;
	struct drm_plane *plane = obj_to_plane(obj);

	if (plane->funcs->set_property)
		ret = plane->funcs->set_property(plane, property, value);
	if (!ret)
		ret = drm_object_property_set_value(obj, property, value);

	return ret;
}

/**
 * drm_mode_getplane_res - get plane info
 * @dev: DRM device
 * @data: ioctl data
 * @file_priv: DRM file info
 *
 *
 * Return an plane count and set of IDs.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_getplane_res(DRM_IOCTL_ARGS)
{
	struct drm_mode_get_plane_res *plane_resp = data;
	struct drm_mode_config *config;
	struct drm_plane *plane;
	uint32_t __user *plane_ptr;
	int copied = 0, ret = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);
	config = &dev->mode_config;

	/*
	 * This ioctl is called twice, once to determine how much space is
	 * needed, and the 2nd time to fill it.
	 */
	if (config->num_plane &&
	    (plane_resp->count_planes >= config->num_plane)) {
		plane_ptr = (uint32_t *)(unsigned long)plane_resp->plane_id_ptr;

		list_for_each_entry(plane, struct drm_plane, &config->plane_list, head) {
			if (put_user(plane->base.id, plane_ptr + copied)) {
				ret = -EFAULT;
				goto out;
			}
			copied++;
		}
	}
	plane_resp->count_planes = config->num_plane;

out:
	drm_modeset_unlock_all(dev);
	return ret;
}

/**
 * drm_mode_getplane - get plane info
 * @dev: DRM device
 * @data: ioctl data
 * @file_priv: DRM file info
 *
 *
 * Return plane info, including formats supported, gamma size, any
 * current fb, etc.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_getplane(DRM_IOCTL_ARGS)
{
	struct drm_mode_get_plane *plane_resp = data;
	struct drm_mode_object *obj;
	struct drm_plane *plane;
	uint32_t *format_ptr;
	int ret = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	drm_modeset_lock_all(dev);
	obj = drm_mode_object_find(dev, plane_resp->plane_id,
				   DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		ret = -ENOENT;
		goto out;
	}
	plane = obj_to_plane(obj);

	if (plane->crtc)
		plane_resp->crtc_id = plane->crtc->base.id;
	else
		plane_resp->crtc_id = 0;

	if (plane->fb)
		plane_resp->fb_id = plane->fb->base.id;
	else
		plane_resp->fb_id = 0;

	plane_resp->plane_id = plane->base.id;
	plane_resp->possible_crtcs = plane->possible_crtcs;
	plane_resp->gamma_size = 0;

	/*
	 * This ioctl is called twice, once to determine how much space is
	 * needed, and the 2nd time to fill it.
	 */
	if (plane->format_count &&
	    (plane_resp->count_format_types >= plane->format_count)) {
		format_ptr = (uint32_t __user *)(unsigned long)plane_resp->format_type_ptr;
		if (DRM_COPY_TO_USER(format_ptr,
				 plane->format_types,
				 sizeof(uint32_t) * plane->format_count)) {
			ret = -EFAULT;
			goto out;
		}
	}
	plane_resp->count_format_types = plane->format_count;

out:
	drm_modeset_unlock_all(dev);
	return ret;
}

/**
 * drm_mode_setplane - set up or tear down an plane
 * @dev: DRM device
 * @data: ioctl data*
 * @file_prive: DRM file info
 *
 *
 * Set plane info, including placement, fb, scaling, and other factors.
 * Or pass a NULL fb to disable.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_setplane(DRM_IOCTL_ARGS)
{
	struct drm_mode_set_plane *plane_req = data;
	struct drm_mode_object *obj;
	struct drm_plane *plane;
	struct drm_crtc *crtc;
	struct drm_framebuffer *fb = NULL, *old_fb = NULL;
	int ret = 0;
	unsigned int fb_width, fb_height;
	int i;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	/*
	 * First, find the plane, crtc, and fb objects.  If not available,
	 * we don't bother to call the driver.
	 */
	obj = drm_mode_object_find(dev, plane_req->plane_id,
				   DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		DRM_DEBUG_KMS("Unknown plane ID %d\n",
			      plane_req->plane_id);
		return -ENOENT;
	}
	plane = obj_to_plane(obj);

	/* No fb means shut it down */
	if (!plane_req->fb_id) {
		drm_modeset_lock_all(dev);
		old_fb = plane->fb;
		plane->funcs->disable_plane(plane);
		plane->crtc = NULL;
		plane->fb = NULL;
		drm_modeset_unlock_all(dev);
		goto out;
	}

	obj = drm_mode_object_find(dev, plane_req->crtc_id,
				   DRM_MODE_OBJECT_CRTC);
	if (!obj) {
		DRM_DEBUG_KMS("Unknown crtc ID %d\n",
			      plane_req->crtc_id);
		ret = -ENOENT;
		goto out;
	}
	crtc = obj_to_crtc(obj);

	fb = drm_framebuffer_lookup(dev, plane_req->fb_id);
	if (!fb) {
		DRM_DEBUG_KMS("Unknown framebuffer ID %d\n",
			      plane_req->fb_id);
		ret = -ENOENT;
		goto out;
	}

	/* Check whether this plane supports the fb pixel format. */
	for (i = 0; i < plane->format_count; i++)
		if (fb->pixel_format == plane->format_types[i])
			break;
	if (i == plane->format_count) {
		DRM_DEBUG_KMS("Invalid pixel format 0x%08x\n", fb->pixel_format);
		ret = -EINVAL;
		goto out;
	}

	fb_width = fb->width << 16;
	fb_height = fb->height << 16;

	/* Make sure source coordinates are inside the fb. */
	if (plane_req->src_w > fb_width ||
	    plane_req->src_x > fb_width - plane_req->src_w ||
	    plane_req->src_h > fb_height ||
	    plane_req->src_y > fb_height - plane_req->src_h) {
		DRM_DEBUG_KMS("Invalid source coordinates "
			      "%u.%06ux%u.%06u+%u.%06u+%u.%06u\n",
			      plane_req->src_w >> 16,
			      ((plane_req->src_w & 0xffff) * 15625) >> 10,
			      plane_req->src_h >> 16,
			      ((plane_req->src_h & 0xffff) * 15625) >> 10,
			      plane_req->src_x >> 16,
			      ((plane_req->src_x & 0xffff) * 15625) >> 10,
			      plane_req->src_y >> 16,
			      ((plane_req->src_y & 0xffff) * 15625) >> 10);
		ret = -ENOSPC;
		goto out;
	}

	/* Give drivers some help against integer overflows */
	if (plane_req->crtc_w > INT_MAX ||
	    plane_req->crtc_x > INT_MAX - (int32_t) plane_req->crtc_w ||
	    plane_req->crtc_h > INT_MAX ||
	    plane_req->crtc_y > INT_MAX - (int32_t) plane_req->crtc_h) {
		DRM_DEBUG_KMS("Invalid CRTC coordinates %ux%u+%d+%d\n",
			      plane_req->crtc_w, plane_req->crtc_h,
			      plane_req->crtc_x, plane_req->crtc_y);
		ret = -ERANGE;
		goto out;
	}

	drm_modeset_lock_all(dev);
	ret = plane->funcs->update_plane(plane, crtc, fb,
					 plane_req->crtc_x, plane_req->crtc_y,
					 plane_req->crtc_w, plane_req->crtc_h,
					 plane_req->src_x, plane_req->src_y,
					 plane_req->src_w, plane_req->src_h);
	if (!ret) {
		old_fb = plane->fb;
		plane->crtc = crtc;
		plane->fb = fb;
		fb = NULL;
	}
	drm_modeset_unlock_all(dev);

out:
	if (fb)
		drm_framebuffer_unreference(fb);
	if (old_fb)
		drm_framebuffer_unreference(old_fb);
	return ret;
}

static int drm_mode_cursor_common(struct drm_device *dev,
				  struct drm_mode_cursor2 *req,
				  struct drm_file *file_priv)
{
	struct drm_mode_object *obj;
	struct drm_crtc *crtc;
	int ret = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	if (!req->flags || (~DRM_MODE_CURSOR_FLAGS & req->flags))
		return -EINVAL;

	obj = drm_mode_object_find(dev, req->crtc_id, DRM_MODE_OBJECT_CRTC);
	if (!obj) {
		DRM_DEBUG_KMS("Unknown CRTC ID %d\n", req->crtc_id);
		return -EINVAL;
	}
	crtc = obj_to_crtc(obj);

	mutex_lock(&crtc->mutex);
	if (req->flags & DRM_MODE_CURSOR_BO) {
		if (!crtc->funcs->cursor_set && !crtc->funcs->cursor_set2) {
			ret = -ENXIO;
			goto out;
		}
		/* Turns off the cursor if handle is 0 */
		if (crtc->funcs->cursor_set2)
			ret = crtc->funcs->cursor_set2(crtc, file_priv, req->handle,
						      req->width, req->height, req->hot_x, req->hot_y);
		else
			ret = crtc->funcs->cursor_set(crtc, file_priv, req->handle,
					      req->width, req->height);
	}

	if (req->flags & DRM_MODE_CURSOR_MOVE) {
		if (crtc->funcs->cursor_move) {
			ret = crtc->funcs->cursor_move(crtc, req->x, req->y);
		} else {
			ret = -EFAULT;
			goto out;
		}
	}
out:
	mutex_unlock(&crtc->mutex);

	return ret;

}

/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_cursor_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_cursor *req = data;
	struct drm_mode_cursor2 new_req;

	(void) memcpy(&new_req, req, sizeof(struct drm_mode_cursor));
	new_req.hot_x = new_req.hot_y = 0;

	return drm_mode_cursor_common(dev, &new_req, file);
}

/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_cursor2_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_cursor2 *req = data;
	return drm_mode_cursor_common(dev, req, file);
}

/* LINTED */
int drm_mode_page_flip_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_crtc_page_flip *page_flip = data;
	struct drm_mode_object *obj;
	struct drm_crtc *crtc;
	struct drm_framebuffer *fb = NULL, *old_fb = NULL;
	struct drm_pending_vblank_event *e = NULL;
	unsigned long flags;
	int hdisplay, vdisplay;
	int ret = -EINVAL;

	if (page_flip->flags & ~DRM_MODE_PAGE_FLIP_FLAGS ||
	    page_flip->reserved != 0)
		return -EINVAL;

	obj = drm_mode_object_find(dev, page_flip->crtc_id, DRM_MODE_OBJECT_CRTC);
	if (!obj)
		return -EINVAL;
	crtc = obj_to_crtc(obj);

	mutex_lock(&crtc->mutex);
	if (crtc->fb == NULL) {
		/* The framebuffer is currently unbound, presumably
		 * due to a hotplug event, that userspace has not
		 * yet discovered.
		 */
		ret = -EBUSY;
		goto out;
	}

	if (crtc->funcs->page_flip == NULL)
		goto out;

	fb = drm_framebuffer_lookup(dev, page_flip->fb_id);
	if (!fb)
		goto out;

	hdisplay = crtc->mode.hdisplay;
	vdisplay = crtc->mode.vdisplay;

	if (crtc->invert_dimensions)
		swap(hdisplay, vdisplay);

	if (hdisplay > fb->width ||
	    vdisplay > fb->height ||
	    crtc->x > fb->width - hdisplay ||
	    crtc->y > fb->height - vdisplay) {
		DRM_DEBUG_KMS("Invalid fb size %ux%u for CRTC viewport %ux%u+%d+%d%s.\n",
			      fb->width, fb->height, hdisplay, vdisplay, crtc->x, crtc->y,
			      crtc->invert_dimensions ? " (inverted)" : "");
		ret = -ENOSPC;
		goto out;
	}

	if (crtc->fb->pixel_format != fb->pixel_format) {
		DRM_DEBUG_KMS("Page flip is not allowed to change frame buffer format.\n");
		ret = -EINVAL;
		goto out;
	}

	if (page_flip->flags & DRM_MODE_PAGE_FLIP_EVENT) {
		ret = -ENOMEM;
		spin_lock_irqsave(&dev->event_lock, flags);
		if (file->event_space < sizeof e->event) {
			spin_unlock_irqrestore(&dev->event_lock, flags);
			goto out;
		}
		file->event_space -= sizeof e->event;
		spin_unlock_irqrestore(&dev->event_lock, flags);

		e = kzalloc(sizeof *e, GFP_KERNEL);
		if (e == NULL) {
			spin_lock_irqsave(&dev->event_lock, flags);
			file->event_space += sizeof e->event;
			spin_unlock_irqrestore(&dev->event_lock, flags);
			goto out;
		}

		e->event.base.type = DRM_EVENT_FLIP_COMPLETE;
		e->event.base.length = sizeof e->event;
		e->event.user_data = page_flip->user_data;
		e->base.event = &e->event.base;
		e->base.file_priv = file;
		e->base.destroy =
			(void (*) (void *, size_t)) kfree;
	}

	old_fb = crtc->fb;
	ret = crtc->funcs->page_flip(crtc, fb, e);
	if (ret) {
		if (page_flip->flags & DRM_MODE_PAGE_FLIP_EVENT) {
		spin_lock_irqsave(&dev->event_lock, flags);
		file->event_space += sizeof e->event;
		spin_unlock_irqrestore(&dev->event_lock, flags);
		kfree(e, sizeof(*e));
		}
		/* Keep the old fb, don't unref it. */
		old_fb = NULL;
	} else {
		/*
		 * Warn if the driver hasn't properly updated the crtc->fb
		 * field to reflect that the new framebuffer is now used.
		 * Failing to do so will screw with the reference counting
		 * on framebuffers.
		 */
		WARN_ON(crtc->fb != fb);
		/* Unref only the old framebuffer. */
		fb = NULL;
	}

out:
	if (fb)
		drm_framebuffer_unreference(fb);
	if (old_fb)
		drm_framebuffer_unreference(old_fb);
	mutex_unlock(&crtc->mutex);

	return ret;
}
