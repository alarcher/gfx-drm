/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright (c) 2007-2008, 2013, Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 * Copyright © 2014 Intel Corporation
 *   Daniel Vetter <daniel.vetter@ffwll.ch>
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
#ifndef __DRM_CRTC_INTERNAL_H__
#define __DRM_CRTC_INTERNAL_H__

#include "drmP.h"

/*
 * This header file contains mode setting related functions and definitions
 * which are only used within the drm module as internal implementation details
 * and are not exported to drivers.
 */


/* drm_crtc */
int drm_mode_crtc_set_obj_prop(struct drm_mode_object *obj,
			       struct drm_property *property,
			       uint64_t value);
int drm_mode_getcrtc(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_setcrtc(DRM_IOCTL_ARGS); 						/* IOCTL */

/* drm_mode_config */
int drm_mode_getresources(DRM_IOCTL_ARGS); 					/* IOCTL */

/* drm_dumb_buffers */
int drm_mode_create_dumb_ioctl(DRM_IOCTL_ARGS); 			/* IOCTL */
int drm_mode_mmap_dumb_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */
int drm_mode_destroy_dumb_ioctl(DRM_IOCTL_ARGS); 			/* IOCTL */

/* drm_color_mgmt */
int drm_mode_gamma_get_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */
int drm_mode_gamma_set_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */

/* drm_property */
int drm_mode_getproperty_ioctl(DRM_IOCTL_ARGS); 			/* IOCTL */
int drm_mode_getblob_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */

/* drm_mode_object */
int drm_mode_obj_get_properties_ioctl(DRM_IOCTL_ARGS); 		/* IOCTL */
int drm_mode_obj_set_property_ioctl(DRM_IOCTL_ARGS); 		/* IOCTL */

/* drm_encoder */
int drm_mode_getencoder(DRM_IOCTL_ARGS); 					/* IOCTL */

/* drm_connector */
int drm_connector_create_standard_properties(struct drm_device *dev);
int drm_mode_connector_property_set_ioctl(DRM_IOCTL_ARGS);	/* IOCTL */
int drm_mode_getconnector(DRM_IOCTL_ARGS); 					/* IOCTL */

/* drm_framebuffer */
void drm_framebuffer_free(struct kref *kref);
void drm_fb_release(struct drm_file *file_priv);
int drm_mode_addfb(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_addfb2(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_rmfb(DRM_IOCTL_ARGS); 							/* IOCTL */
int drm_mode_getfb(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_dirtyfb_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */

/* drm_atomic */

/* drm_plane */
int drm_mode_getplane_res(DRM_IOCTL_ARGS); 					/* IOCTL */
int drm_mode_getplane(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_setplane(DRM_IOCTL_ARGS); 						/* IOCTL */
int drm_mode_cursor_ioctl(DRM_IOCTL_ARGS); 					/* IOCTL */
int drm_mode_cursor2_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */
int drm_mode_page_flip_ioctl(DRM_IOCTL_ARGS); 				/* IOCTL */

/* drm_edid */

#endif /* __DRM_CRTC_INTERNAL_H__ */
