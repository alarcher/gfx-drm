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

#include "drmP.h"
#include "drm_linux_list.h"
#include "drm_encoder.h"
#include "drm_mode_config.h"

#include "drm_crtc_internal.h"

/**
 * drm_mode_getresources - get graphics configuration
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @cmd: cmd from ioctl
 * @arg: arg from ioctl
 *
 *
 * Construct a set of configuration description structures and return
 * them to the user, including CRTC, connector and framebuffer configuration.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_getresources(DRM_IOCTL_ARGS)
{
	struct drm_mode_card_res *card_res = data;
	struct list_head *lh;
	struct drm_framebuffer *fb;
	struct drm_connector *connector;
	struct drm_crtc *crtc;
	struct drm_encoder *encoder;
	int ret = 0;
	int connector_count = 0;
	int crtc_count = 0;
	int fb_count = 0;
	int encoder_count = 0;
	int copied = 0, i;
	uint32_t __user *fb_id;
	uint32_t __user *crtc_id;
	uint32_t __user *connector_id;
	uint32_t __user *encoder_id;
	struct drm_mode_group *mode_group;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	mutex_lock(&file->fbs_lock);

	/*
	 * For the non-control nodes we need to limit the list of resources
	 * by IDs in the group list for this node
	 */
	list_for_each(lh, &file->fbs)
		fb_count++;

	/* handle this in 4 parts */
	/* FBs */
	if (card_res->count_fbs >= fb_count) {
		copied = 0;
		fb_id = (uint32_t __user *)(unsigned long)card_res->fb_id_ptr;
		list_for_each_entry(fb, struct drm_framebuffer, &file->fbs, filp_head) {
			if (put_user(fb->base.id, fb_id + copied)) {
				mutex_unlock(&file->fbs_lock);
				return -EFAULT;
			}
			copied++;
		}
	}
	card_res->count_fbs = fb_count;
	mutex_unlock(&file->fbs_lock);

	drm_modeset_lock_all(dev);

	mode_group = &file->master->minor->mode_group;
	if (file->master->minor->type == DRM_MINOR_CONTROL) {

		list_for_each(lh, &dev->mode_config.crtc_list)
			crtc_count++;

		list_for_each(lh, &dev->mode_config.connector_list)
			connector_count++;

		list_for_each(lh, &dev->mode_config.encoder_list)
			encoder_count++;
	} else {

		crtc_count = mode_group->num_crtcs;
		connector_count = mode_group->num_connectors;
		encoder_count = mode_group->num_encoders;
	}

	card_res->max_height = dev->mode_config.max_height;
	card_res->min_height = dev->mode_config.min_height;
	card_res->max_width = dev->mode_config.max_width;
	card_res->min_width = dev->mode_config.min_width;

	/* CRTCs */
	if (card_res->count_crtcs >= crtc_count) {
		copied = 0;
		crtc_id = (uint32_t __user *)(unsigned long)card_res->crtc_id_ptr;
		if (file->master->minor->type == DRM_MINOR_CONTROL) {
			list_for_each_entry(crtc, struct drm_crtc, &dev->mode_config.crtc_list,
					    head) {
				DRM_DEBUG_KMS("[CRTC:%d]\n", crtc->base.id);
				if (put_user(crtc->base.id, crtc_id + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		} else {
			for (i = 0; i < mode_group->num_crtcs; i++) {
				if (put_user(mode_group->id_list[i],
					     crtc_id + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		}
	}
	card_res->count_crtcs = crtc_count;

	/* Encoders */
	if (card_res->count_encoders >= encoder_count) {
		copied = 0;
		encoder_id = (uint32_t __user *)(unsigned long)card_res->encoder_id_ptr;
		if (file->master->minor->type == DRM_MINOR_CONTROL) {
			list_for_each_entry(encoder, struct drm_encoder,
					    &dev->mode_config.encoder_list,
					    head) {
				DRM_DEBUG_KMS("[ENCODER:%d:%s]\n", encoder->base.id,
						drm_get_encoder_name(encoder));
				if (put_user(encoder->base.id, encoder_id +
					     copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		} else {
			for (i = mode_group->num_crtcs; i < mode_group->num_crtcs + mode_group->num_encoders; i++) {
				if (put_user(mode_group->id_list[i],
					     encoder_id + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}

		}
	}
	card_res->count_encoders = encoder_count;

	/* Connectors */
	if (card_res->count_connectors >= connector_count) {
		copied = 0;
		connector_id = (uint32_t __user *)(unsigned long)card_res->connector_id_ptr;
		if (file->master->minor->type == DRM_MINOR_CONTROL) {
			list_for_each_entry(connector, struct drm_connector,
					    &dev->mode_config.connector_list,
					    head) {
				DRM_DEBUG_KMS("[CONNECTOR:%d:%s]\n",
					connector->base.id,
					drm_get_connector_name(connector));
				if (put_user(connector->base.id,
					     connector_id + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		} else {
			int start = mode_group->num_crtcs +
				mode_group->num_encoders;
			for (i = start; i < start + mode_group->num_connectors; i++) {
				if (put_user(mode_group->id_list[i],
					     connector_id + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		}
	}
	card_res->count_connectors = connector_count;

	DRM_DEBUG_KMS("CRTC[%d] CONNECTORS[%d] ENCODERS[%d]\n", card_res->count_crtcs,
		  card_res->count_connectors, card_res->count_encoders);

out:
	drm_modeset_unlock_all(dev);
	return ret;
}

void drm_mode_config_reset(struct drm_device *dev)
{
	struct drm_crtc *crtc;
	struct drm_encoder *encoder;
	struct drm_connector *connector;

	list_for_each_entry(crtc, struct drm_crtc, &dev->mode_config.crtc_list, head)
		if (crtc->funcs->reset)
			crtc->funcs->reset(crtc);

	list_for_each_entry(encoder, struct drm_encoder, &dev->mode_config.encoder_list, head)
		if (encoder->funcs->reset)
			encoder->funcs->reset(encoder);

	list_for_each_entry(connector, struct drm_connector, &dev->mode_config.connector_list, head)
		if (connector->funcs->reset)
			connector->funcs->reset(connector);
}

/**
 * drm_mode_config_init - initialize DRM mode_configuration structure
 * @dev: DRM device
 *
 * Initialize @dev's mode_config structure, used for tracking the graphics
 * configuration of @dev.
 *
 * Since this initializes the modeset locks, no locking is possible. Which is no
 * problem, since this should happen single threaded at init time. It is the
 * driver's problem to ensure this guarantee.
 *
 */
void drm_mode_config_init(struct drm_device *dev)
{
	mutex_init(&dev->mode_config.mutex, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&dev->mode_config.idr_mutex, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&dev->mode_config.fb_lock, NULL, MUTEX_DRIVER, NULL);
	INIT_LIST_HEAD(&dev->mode_config.fb_list);
	INIT_LIST_HEAD(&dev->mode_config.crtc_list);
	INIT_LIST_HEAD(&dev->mode_config.connector_list);
	INIT_LIST_HEAD(&dev->mode_config.encoder_list);
	INIT_LIST_HEAD(&dev->mode_config.property_list);
	INIT_LIST_HEAD(&dev->mode_config.property_blob_list);
	INIT_LIST_HEAD(&dev->mode_config.plane_list);
	idr_init(&dev->mode_config.crtc_idr);

	drm_modeset_lock_all(dev);
	(void) drm_connector_create_standard_properties(dev);
	drm_modeset_unlock_all(dev);

	/* Just to be sure */
	dev->mode_config.num_fb = 0;
	dev->mode_config.num_connector = 0;
	dev->mode_config.num_crtc = 0;
	dev->mode_config.num_encoder = 0;
}

/**
 * drm_mode_config_cleanup - free up DRM mode_config info
 * @dev: DRM device
 *
 * Free up all the connectors and CRTCs associated with this DRM device, then
 * free up the framebuffers and associated buffer objects.
 *
 * Note that since this /should/ happen single-threaded at driver/device
 * teardown time, no locking is required. It's the driver's job to ensure that
 * this guarantee actually holds true.
 *
 * FIXME: cleanup any dangling user buffer objects too
 */
void drm_mode_config_cleanup(struct drm_device *dev)
{
	struct drm_connector *connector, *ot;
	struct drm_crtc *crtc, *ct;
	struct drm_encoder *encoder, *enct;
	struct drm_framebuffer *fb, *fbt;
	struct drm_property *property, *pt;
	struct drm_property_blob *blob, *bt;
	struct drm_plane *plane, *plt;

	list_for_each_entry_safe(encoder, enct, struct drm_encoder, &dev->mode_config.encoder_list,
				 head) {
		encoder->funcs->destroy(encoder);
	}

	list_for_each_entry_safe(connector, ot, struct drm_connector,
				 &dev->mode_config.connector_list, head) {
		connector->funcs->destroy(connector);
	}

	list_for_each_entry_safe(property, pt, struct drm_property, &dev->mode_config.property_list,
				 head) {
		drm_property_destroy(dev, property);
	}

	list_for_each_entry_safe(blob, bt, struct drm_property_blob, &dev->mode_config.property_blob_list,
				 head) {
		drm_property_destroy_blob(dev, blob);
	}

	/*
	 * Single-threaded teardown context, so it's not required to grab the
	 * fb_lock to protect against concurrent fb_list access. Contrary, it
	 * would actually deadlock with the drm_framebuffer_cleanup function.
	 *
	 * Also, if there are any framebuffers left, that's a driver leak now,
	 * so politely WARN about this.
	 */
	WARN_ON(!list_empty(&dev->mode_config.fb_list));
	list_for_each_entry_safe(fb, fbt, struct drm_framebuffer, &dev->mode_config.fb_list, head) {
		drm_framebuffer_remove(fb);
	}

	list_for_each_entry_safe(plane, plt, struct drm_plane, &dev->mode_config.plane_list,
				 head) {
		plane->funcs->destroy(plane);
	}

	list_for_each_entry_safe(crtc, ct, struct drm_crtc, &dev->mode_config.crtc_list, head) {
		crtc->funcs->destroy(crtc);
	}
	idr_remove_all(&dev->mode_config.crtc_idr);
	idr_destroy(&dev->mode_config.crtc_idr);
}
