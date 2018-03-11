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
#include <drm/drm_framebuffer.h>


/**
 * drm_mode_addfb - add an FB to the graphics configuration
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @cmd: cmd from ioctl
 * @arg: arg from ioctl
 *
 *
 * Add a new FB to the specified CRTC, given a user request.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_addfb(DRM_IOCTL_ARGS)
{
	struct drm_mode_fb_cmd *or = data;
	struct drm_mode_fb_cmd2 r;
	struct drm_mode_config *config = &dev->mode_config;
	struct drm_framebuffer *fb;
	int ret = 0;

	(void) memset(&r, 0, sizeof(struct drm_mode_fb_cmd2));

	/* Use new struct with format internally */
	r.fb_id = or->fb_id;
	r.width = or->width;
	r.height = or->height;
	r.pitches[0] = or->pitch;
	r.pixel_format = drm_mode_legacy_fb_format(or->bpp, or->depth);
	r.handles[0] = or->handle;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	if ((config->min_width > r.width) || (r.width > config->max_width))
		return -EINVAL;

	if ((config->min_height > r.height) || (r.height > config->max_height))
		return -EINVAL;

	fb = dev->mode_config.funcs->fb_create(dev, file, &r);
	if (fb == NULL) {
		DRM_DEBUG_KMS("could not create framebuffer\n");
		return -ENOMEM;
	}

	mutex_lock(&file->fbs_lock);
	or->fb_id = fb->base.id;
	list_add(&fb->filp_head, &file->fbs, (caddr_t)fb);
	DRM_DEBUG_KMS("[FB:%d]\n", fb->base.id);
	mutex_unlock(&file->fbs_lock);

	return ret;
}

static int framebuffer_check(const struct drm_mode_fb_cmd2 *r)
{
	int ret, hsub, vsub, num_planes, i;

	ret = format_check(r);
	if (ret) {
		DRM_DEBUG_KMS("bad framebuffer format %s\n",
			      drm_get_format_name(r->pixel_format));
		return ret;
	}

	hsub = drm_format_horz_chroma_subsampling(r->pixel_format);
	vsub = drm_format_vert_chroma_subsampling(r->pixel_format);
	num_planes = drm_format_num_planes(r->pixel_format);

	if (r->width == 0 || r->width % hsub) {
		DRM_DEBUG_KMS("bad framebuffer width %u\n", r->height);
		return -EINVAL;
	}

	if (r->height == 0 || r->height % vsub) {
		DRM_DEBUG_KMS("bad framebuffer height %u\n", r->height);
		return -EINVAL;
	}

	for (i = 0; i < num_planes; i++) {
		unsigned int width = r->width / (i != 0 ? hsub : 1);
		unsigned int height = r->height / (i != 0 ? vsub : 1);
		unsigned int cpp = drm_format_plane_cpp(r->pixel_format, i);

		if (!r->handles[i]) {
			DRM_DEBUG_KMS("no buffer object handle for plane %d\n", i);
			return -EINVAL;
		}

		if ((uint64_t) width * cpp > UINT_MAX)
			return -ERANGE;

		if ((uint64_t) height * r->pitches[i] + r->offsets[i] > UINT_MAX)
			return -ERANGE;

		if (r->pitches[i] < width * cpp) {
			DRM_DEBUG_KMS("bad pitch %u for plane %d\n", r->pitches[i], i);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * drm_mode_addfb2 - add an FB to the graphics configuration
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @arg: arg from ioctl
 *
 * Add a new FB to the specified CRTC, given a user request with format.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_addfb2(DRM_IOCTL_ARGS)
{
	struct drm_mode_fb_cmd2 *r = data;
	struct drm_mode_config *config = &dev->mode_config;
	struct drm_framebuffer *fb;
	int ret;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	if (r->flags & ~DRM_MODE_FB_INTERLACED) {
		DRM_DEBUG_KMS("bad framebuffer flags 0x%08x\n", r->flags);
		return -EINVAL;
	}

	if ((config->min_width > r->width) || (r->width > config->max_width)) {
		DRM_DEBUG_KMS("bad framebuffer width %d, should be >= %d && <= %d\n",
			  r->width, config->min_width, config->max_width);
		return -EINVAL;
	}
	if ((config->min_height > r->height) || (r->height > config->max_height)) {
		DRM_DEBUG_KMS("bad framebuffer height %d, should be >= %d && <= %d\n",
			  r->height, config->min_height, config->max_height);
		return -EINVAL;
	}

	ret = framebuffer_check(r);
	if (ret)
		return ret;

	fb = dev->mode_config.funcs->fb_create(dev, file, r);
	if (fb == NULL) {
		DRM_DEBUG_KMS("could not create framebuffer\n");
		return -ENOMEM;
	}

	mutex_lock(&file->fbs_lock);
	r->fb_id = fb->base.id;
	list_add(&fb->filp_head, &file->fbs, (caddr_t)fb);
	DRM_DEBUG_KMS("[FB:%d]\n", fb->base.id);
	mutex_unlock(&file->fbs_lock);

	return ret;
}

/**
 * drm_mode_rmfb - remove an FB from the configuration
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @arg: arg from ioctl
 *
 * Remove the FB specified by the user.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED */
int drm_mode_rmfb(DRM_IOCTL_ARGS)
{
	struct drm_framebuffer *fb = NULL;
	struct drm_framebuffer *fbl = NULL;
	uint32_t *id = data;
	int found = 0;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	mutex_lock(&file->fbs_lock);
	mutex_lock(&dev->mode_config.fb_lock);
	fb = __drm_framebuffer_lookup(dev, *id);
	if (!fb)
		goto fail_lookup;

	list_for_each_entry(fbl, struct drm_framebuffer, &file->fbs, filp_head)
		if (fb == fbl)
			found = 1;

	if (!found)
		goto fail_lookup;

	/* Mark fb as reaped, we still have a ref from fpriv->fbs. */
	__drm_framebuffer_unregister(dev, fb);

	list_del_init(&fb->filp_head);
	mutex_unlock(&dev->mode_config.fb_lock);
	mutex_unlock(&file->fbs_lock);

	drm_framebuffer_remove(fb);
	return 0;

fail_lookup:
	mutex_unlock(&dev->mode_config.fb_lock);
	mutex_unlock(&file->fbs_lock);

	return -EINVAL;
}

/**
 * drm_mode_getfb - get FB info
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @arg: arg from ioctl
 *
 * Lookup the FB given its ID and return info about it.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED */
int drm_mode_getfb(DRM_IOCTL_ARGS)
{
	struct drm_mode_fb_cmd *r = data;
	struct drm_framebuffer *fb;
	int ret;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	fb = drm_framebuffer_lookup(dev, r->fb_id);
	if (!fb)
		return -EINVAL;

	r->height = fb->height;
	r->width = fb->width;
	r->depth = fb->depth;
	r->bpp = fb->bits_per_pixel;
	r->pitch = fb->pitches[0];
	if (fb->funcs->create_handle)
		ret = fb->funcs->create_handle(fb, file, &r->handle);
	else
		ret = -ENODEV;

	drm_framebuffer_unreference(fb);

	return ret;
}

/* LINTED */
int drm_mode_dirtyfb_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_clip_rect __user *clips_ptr;
	struct drm_clip_rect *clips = NULL;
	struct drm_mode_fb_dirty_cmd *r = data;
	struct drm_framebuffer *fb;
	unsigned flags;
	int num_clips;
	int ret;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	fb = drm_framebuffer_lookup(dev, r->fb_id);
	if (!fb)
		return -EINVAL;

	num_clips = r->num_clips;
	clips_ptr = (struct drm_clip_rect *)(unsigned long)r->clips_ptr;

	if (!num_clips != !clips_ptr) {
		ret = -EINVAL;
		goto out_err1;
	}

	flags = DRM_MODE_FB_DIRTY_FLAGS & r->flags;

	/* If userspace annotates copy, clips must come in pairs */
	if (flags & DRM_MODE_FB_DIRTY_ANNOTATE_COPY && (num_clips % 2)) {
		ret = -EINVAL;
		goto out_err1;
	}

	if (num_clips && clips_ptr) {
		if (num_clips < 0 || num_clips > DRM_MODE_FB_DIRTY_MAX_CLIPS) {
			ret = -EINVAL;
			goto out_err1;
		}
		clips = kzalloc(num_clips * sizeof(*clips), GFP_KERNEL);
		if (!clips) {
			ret = -ENOMEM;
			goto out_err1;
		}

		ret = copy_from_user(clips, clips_ptr,
				     num_clips * sizeof(*clips));
		if (ret) {
			ret = -EFAULT;
			goto out_err2;
		}
	}

	if (fb->funcs->dirty) {
		drm_modeset_lock_all(dev);
		ret = fb->funcs->dirty(fb, file, flags, r->color,
				       clips, num_clips);
		drm_modeset_unlock_all(dev);
	} else {
		ret = -ENOSYS;
	}

out_err2:
	if (clips)
		kfree(clips, num_clips * sizeof(*clips));
out_err1:
	drm_framebuffer_unreference(fb);
	return ret;
}

/**
 * drm_fb_release - remove and free the FBs on this file
 * @filp: file * from the ioctl
 *
 *
 * Destroy all the FBs associated with @filp.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
void drm_fb_release(struct drm_file *priv)
{
	struct drm_device *dev = priv->minor->dev;
	struct drm_framebuffer *fb, *tfb;

	mutex_lock(&priv->fbs_lock);
	list_for_each_entry_safe(fb, tfb, struct drm_framebuffer, &priv->fbs, filp_head) {

		mutex_lock(&dev->mode_config.fb_lock);
		/* Mark fb as reaped, we still have a ref from fpriv->fbs. */
		__drm_framebuffer_unregister(dev, fb);
		mutex_unlock(&dev->mode_config.fb_lock);

		list_del_init(&fb->filp_head);

		/* This will also drop the fpriv->fbs reference. */
		drm_framebuffer_remove(fb);
	}
	mutex_unlock(&priv->fbs_lock);
}

void drm_framebuffer_free(struct kref *kref)
{
	struct drm_framebuffer *fb =
			container_of(kref, struct drm_framebuffer, refcount);
	fb->funcs->destroy(fb);
}

/**
 * drm_framebuffer_init - initialize a framebuffer
 * @dev: DRM device
 *
 *
 * Allocates an ID for the framebuffer's parent mode object, sets its mode
 * functions & device file and adds it to the master fd list.
 *
 * IMPORTANT:
 * This functions publishes the fb and makes it available for concurrent access
 * by other users. Which means by this point the fb _must_ be fully set up -
 * since all the fb attributes are invariant over its lifetime, no further
 * locking but only correct reference counting is required.
 *
 * RETURNS:
 * Zero on success, error code on failure.
 */
int drm_framebuffer_init(struct drm_device *dev, struct drm_framebuffer *fb,
			 const struct drm_framebuffer_funcs *funcs)
{
	int ret;

	mutex_lock(&dev->mode_config.fb_lock);
	kref_init(&fb->refcount);
	INIT_LIST_HEAD(&fb->filp_head);
	fb->dev = dev;
	fb->funcs = funcs;

	ret = drm_mode_object_get(dev, &fb->base, DRM_MODE_OBJECT_FB);
	if (ret)
		goto out;

	/* Grab the idr reference. */
	drm_framebuffer_reference(fb);

	dev->mode_config.num_fb++;
	list_add(&fb->head, &dev->mode_config.fb_list, (caddr_t)fb);
out:
	mutex_unlock(&dev->mode_config.fb_lock);

	return 0;
}

/**
 * drm_framebuffer_lookup - look up a drm framebuffer and grab a reference
 * @dev: drm device
 * @id: id of the fb object
 *
 * If successful, this grabs an additional reference to the framebuffer -
 * callers need to make sure to eventually unreference the returned framebuffer
 * again.
 */
struct drm_framebuffer *drm_framebuffer_lookup(struct drm_device *dev,
					       uint32_t id)
{
	struct drm_framebuffer *fb;

	mutex_lock(&dev->mode_config.fb_lock);

	fb = __drm_framebuffer_lookup(dev, id);
	if (fb)
		kref_get(&fb->refcount);

	mutex_unlock(&dev->mode_config.fb_lock);

	return fb;
}

static struct drm_framebuffer *__drm_framebuffer_lookup(struct drm_device *dev,
							uint32_t id)
{
	struct drm_mode_object *obj = NULL;
	struct drm_framebuffer *fb;

	mutex_lock(&dev->mode_config.idr_mutex);
	obj = idr_find(&dev->mode_config.crtc_idr, id);
	if (!obj || (obj->type != DRM_MODE_OBJECT_FB) || (obj->id != id))
		fb = NULL;
	else
		fb = obj_to_fb(obj);
	mutex_unlock(&dev->mode_config.idr_mutex);

	return fb;
}

/**
 * drm_framebuffer_unregister_private - unregister a private fb from the lookup idr
 * @fb: fb to unregister
 *
 * Drivers need to call this when cleaning up driver-private framebuffers, e.g.
 * those used for fbdev. Note that the caller must hold a reference of it's own,
 * i.e. the object may not be destroyed through this call (since it'll lead to a
 * locking inversion).
 */
void drm_framebuffer_unregister_private(struct drm_framebuffer *fb)
{
	struct drm_device *dev = fb->dev;

	mutex_lock(&dev->mode_config.fb_lock);
	/* Mark fb as reaped and drop idr ref. */
	__drm_framebuffer_unregister(dev, fb);
	mutex_unlock(&dev->mode_config.fb_lock);
}

/* dev->mode_config.fb_lock must be held! */
static void __drm_framebuffer_unregister(struct drm_device *dev,
					 struct drm_framebuffer *fb)
{
	mutex_lock(&dev->mode_config.idr_mutex);
	(void) idr_remove(&dev->mode_config.crtc_idr, fb->base.id);
	mutex_unlock(&dev->mode_config.idr_mutex);

	fb->base.id = 0;

	__drm_framebuffer_unreference(fb);
}

/**
 * drm_framebuffer_cleanup - remove a framebuffer object
 * @fb: framebuffer to remove
 *
 *
 * Cleanup references to a user-created framebuffer. This function is intended
 * to be used from the drivers ->destroy callback.
 *
 * Note that this function does not remove the fb from active usuage - if it is
 * still used anywhere, hilarity can ensue since userspace could call getfb on
 * the id and get back -EINVAL. Obviously no concern at driver unload time.
 *
 * Also, the framebuffer will not be removed from the lookup idr - for
 * user-created framebuffers this will happen in in the rmfb ioctl. For
 * driver-private objects (e.g. for fbdev) drivers need to explicitly call
 * drm_framebuffer_unregister_private.
 */
void drm_framebuffer_cleanup(struct drm_framebuffer *fb)
{
	struct drm_device *dev = fb->dev;
	mutex_lock(&dev->mode_config.fb_lock);
	list_del(&fb->head);
	dev->mode_config.num_fb--;
	mutex_unlock(&dev->mode_config.fb_lock);
}

/**
 * drm_framebuffer_remove - remove and unreference a framebuffer object
 * @fb: framebuffer to remove
 *
 * using @fb, removes it, setting it to NULL. Then drops the reference to the
 * passed-in framebuffer. Might take the modeset locks.
 *
 * Note that this function optimizes the cleanup away if the caller holds the
 * last reference to the framebuffer. It is also guaranteed to not take the
 * modeset locks in this case.
 */
void drm_framebuffer_remove(struct drm_framebuffer *fb)
{
	struct drm_device *dev = fb->dev;
	struct drm_crtc *crtc;
	struct drm_plane *plane;
	struct drm_mode_set set;
	int ret;

	WARN_ON(!list_empty(&fb->filp_head));

	/*
	 * drm ABI mandates that we remove any deleted framebuffers from active
	 * useage. But since most sane clients only remove framebuffers they no
	 * longer need, try to optimize this away.
	 *
	 * Since we're holding a reference ourselves, observing a refcount of 1
	 * means that we're the last holder and can skip it. Also, the refcount
	 * can never increase from 1 again, so we don't need any barriers or
	 * locks.
	 *
	 * Note that userspace could try to race with use and instate a new
	 * usage _after_ we've cleared all current ones. End result will be an
	 * in-use fb with fb-id == 0. Userspace is allowed to shoot its own foot
	 * in this manner.
	 */
	if (atomic_read(&fb->refcount.refcount) > 1) {
		drm_modeset_lock_all(dev);
		/* remove from any CRTC */
		list_for_each_entry(crtc, struct drm_crtc, &dev->mode_config.crtc_list, head) {
			if (crtc->fb == fb) {
				/* should turn off the crtc */
				(void) memset(&set, 0, sizeof(struct drm_mode_set));
				set.crtc = crtc;
				set.fb = NULL;
				ret = drm_mode_set_config_internal(&set);
				if (ret)
					DRM_ERROR("failed to reset crtc %p when fb was deleted\n", (void *)crtc);
			}
 		}
		list_for_each_entry(plane, struct drm_plane, &dev->mode_config.plane_list, head) {
			if (plane->fb == fb)
				drm_plane_force_disable(plane);
		}
		drm_modeset_unlock_all(dev);
	}

	drm_framebuffer_unreference(fb);
}

/**
 * drm_framebuffer_unreference - unref a framebuffer
 *
 */
void drm_framebuffer_unreference(struct drm_framebuffer *fb)
{
	DRM_DEBUG("FB ID: %d\n", fb->base.id);
	kref_put(&fb->refcount, drm_framebuffer_free);
}

/* LINTED E_FUNC_ARG_UNUSED */
static void drm_framebuffer_free_bug(struct kref *kref)
{
	BUG();
}

static void __drm_framebuffer_unreference(struct drm_framebuffer *fb)
{
	DRM_DEBUG("FB ID: %d\n", fb->base.id);
	kref_put(&fb->refcount, drm_framebuffer_free_bug);
}

/**
 * drm_framebuffer_reference - incr the fb refcnt
 */
void drm_framebuffer_reference(struct drm_framebuffer *fb)
{
	DRM_DEBUG("FB ID: %d\n", fb->base.id);
	kref_get(&fb->refcount);
}
