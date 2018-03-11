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
#ifndef __DRM_CRTC_H__
#define __DRM_CRTC_H__

#include <sys/ksynch.h>
#include "drm.h"
#include "drmP.h"
#include "drm_sun_idr.h"
#include "drm_sun_i2c.h"
#include "drm_mode_object.h"
#include "drm_framebuffer.h"
#include "drm_modes.h"
#include "drm_connector.h"
#include "drm_property.h"
#include "drm_plane.h"

#include <uapi/drm/drm_fourcc.h>

struct drm_device;
struct drm_mode_set;

struct drm_crtc;
struct drm_connector;
struct drm_encoder;
struct drm_pending_vblank_event;
struct drm_plane;

/**
 * drm_crtc_funcs - control CRTCs for a given device
 * @save: save CRTC state
 * @restore: restore CRTC state
 * @reset: reset CRTC after state has been invalidate (e.g. resume)
 * @cursor_set: setup the cursor
 * @cursor_move: move the cursor
 * @gamma_set: specify color ramp for CRTC
 * @destroy: deinit and free object
 * @set_property: called when a property is changed
 * @set_config: apply a new CRTC configuration
 * @page_flip: initiate a page flip
 *
 * The drm_crtc_funcs structure is the central CRTC management structure
 * in the DRM.  Each CRTC controls one or more connectors (note that the name
 * CRTC is simply historical, a CRTC may control LVDS, VGA, DVI, TV out, etc.
 * connectors, not just CRTs).
 *
 * Each driver is responsible for filling out this structure at startup time,
 * in addition to providing other modesetting features, like i2c and DDC
 * bus accessors.
 */
struct drm_crtc_funcs {
	/* Save CRTC state */
	void (*save)(struct drm_crtc *crtc); /* suspend? */
	/* Restore CRTC state */
	void (*restore)(struct drm_crtc *crtc); /* resume? */
	/* Reset CRTC state */
	void (*reset)(struct drm_crtc *crtc);

	/* cursor controls */
	int (*cursor_set)(struct drm_crtc *crtc, struct drm_file *file_priv,
			  uint32_t handle, uint32_t width, uint32_t height);
	int (*cursor_set2)(struct drm_crtc *crtc, struct drm_file *file_priv,
			   uint32_t handle, uint32_t width, uint32_t height,
			   int32_t hot_x, int32_t hot_y);
	int (*cursor_move)(struct drm_crtc *crtc, int x, int y);

	/* Set gamma on the CRTC */
	void (*gamma_set)(struct drm_crtc *crtc, u16 *r, u16 *g, u16 *b,
			  uint32_t start, uint32_t size);
	/* Object destroy routine */
	void (*destroy)(struct drm_crtc *crtc);

	int (*set_config)(struct drm_mode_set *set);

	/*
	 * Flip to the given framebuffer.  This implements the page
	 * flip ioctl descibed in drm_mode.h, specifically, the
	 * implementation must return immediately and block all
	 * rendering to the current fb until the flip has completed.
	 * If userspace set the event flag in the ioctl, the event
	 * argument will point to an event to send back when the flip
	 * completes, otherwise it will be NULL.
	 */
	int (*page_flip)(struct drm_crtc *crtc,
			 struct drm_framebuffer *fb,
			 struct drm_pending_vblank_event *event);

	int (*set_property)(struct drm_crtc *crtc,
			    struct drm_property *property, uint64_t val);
};

/**
 * drm_crtc - central CRTC control structure
 * @dev: parent DRM device
 * @head: list management
 * @base: base KMS object for ID tracking etc.
 * @enabled: is this CRTC enabled?
 * @mode: current mode timings
 * @hwmode: mode timings as programmed to hw regs
 * @invert_dimensions: for purposes of error checking crtc vs fb sizes,
 *    invert the width/height of the crtc.  This is used if the driver
 *    is performing 90 or 270 degree rotated scanout
 * @x: x position on screen
 * @y: y position on screen
 * @funcs: CRTC control functions
 * @gamma_size: size of gamma ramp
 * @gamma_store: gamma ramp values
 * @framedur_ns: precise frame timing
 * @framedur_ns: precise line timing
 * @pixeldur_ns: precise pixel timing
 * @helper_private: mid-layer private data
 * @properties: property tracking for this CRTC
 *
 * Each CRTC may have one or more connectors associated with it.  This structure
 * allows the CRTC to be controlled.
 */
struct drm_crtc {
	struct drm_device *dev;
	struct list_head head;

	/**
	 * crtc mutex
	 *
	 * This provides a read lock for the overall crtc state (mode, dpms
	 * state, ...) and a write lock for everything which can be update
	 * without a full modeset (fb, cursor data, ...)
	 */
	struct mutex mutex;

	struct drm_mode_object base;

	/* framebuffer the connector is currently bound to */
	struct drm_framebuffer *fb;

	/* Temporary tracking of the old fb while a modeset is ongoing. Used
	 * by drm_mode_set_config_internal to implement correct refcounting. */
	struct drm_framebuffer *old_fb;

	bool enabled;

	/* Requested mode from modesetting. */
	struct drm_display_mode mode;

	/* Programmed mode in hw, after adjustments for encoders,
	 * crtc, panel scaling etc. Needed for timestamping etc.
	 */
	struct drm_display_mode hwmode;

	bool invert_dimensions;

	int x, y;
	const struct drm_crtc_funcs *funcs;

	/* CRTC gamma size for reporting to userspace */
	uint32_t gamma_size;
	uint16_t *gamma_store;

	/* Constants needed for precise vblank and swap timestamping. */
	s64 framedur_ns, linedur_ns, pixeldur_ns;

	/* if you are using the helper */
	void *helper_private;

	struct drm_object_properties properties;
};

/**
 * drm_mode_set - new values for a CRTC config change
 * @head: list management
 * @fb: framebuffer to use for new config
 * @crtc: CRTC whose configuration we're about to change
 * @mode: mode timings to use
 * @x: position of this CRTC relative to @fb
 * @y: position of this CRTC relative to @fb
 * @connectors: array of connectors to drive with this CRTC if possible
 * @num_connectors: size of @connectors array
 *
 * Represents a single crtc the connectors that it drives with what mode
 * and from which framebuffer it scans out from.
 *
 * This is used to set modes.
 */
struct drm_mode_set {
	struct drm_framebuffer *fb;
	struct drm_crtc *crtc;
	struct drm_display_mode *mode;

	uint32_t x;
	uint32_t y;

	struct drm_connector **connectors;
	size_t num_connectors;
};

/**
 * drm_mode_group - group of mode setting resources for potential sub-grouping
 * @num_crtcs: CRTC count
 * @num_encoders: encoder count
 * @num_connectors: connector count
 * @id_list: list of KMS object IDs in this group
 *
 * Currently this simply tracks the global mode setting state.  But in the
 * future it could allow groups of objects to be set aside into independent
 * control groups for use by different user level processes (e.g. two X servers
 * running simultaneously on different heads, each with their own mode
 * configuration and freedom of mode setting).
 */
struct drm_mode_group {
	uint32_t num_crtcs;
	uint32_t num_encoders;
	uint32_t num_connectors;

	/* list of object IDs for this group */
	uint32_t *id_list;
};

#define obj_to_crtc(x) container_of(x, struct drm_crtc, base)

void drm_crtc_cleanup(struct drm_crtc *crtc);
int drm_mode_set_config_internal(struct drm_mode_set *set);

#endif /* __DRM_CRTC_H__ */
