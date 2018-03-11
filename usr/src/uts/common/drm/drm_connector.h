/*
 * Copyright (c) 2006, 2013, Oracle and/or its affiliates. All rights reserved.
 */

/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright (c) 2007-2008, 2013, 2016 Intel Corporation
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
#ifndef __DRM_CONNECTOR_H__
#define __DRM_CONNECTOR_H__

#include "drm_mode_object.h"

enum drm_connector_force {
	DRM_FORCE_UNSPECIFIED,
	DRM_FORCE_OFF,
	DRM_FORCE_ON,         /* force on analog part normally */
	DRM_FORCE_ON_DIGITAL, /* for DVI-I use digital connector */
};

/**
 * enum drm_connector_status - status for a &drm_connector
 *
 * This enum is used to track the connector status. There are no separate
 * #defines for the uapi!
 */
enum drm_connector_status {
	/**
	 * @connector_status_connected: The connector is definitely connected to
	 * a sink device, and can be enabled.
	 */
	connector_status_connected = 1,
	/**
	 * @connector_status_disconnected: The connector isn't connected to a
	 * sink device which can be autodetect. For digital outputs like DP or
	 * HDMI (which can be realiable probed) this means there's really
	 * nothing there. It is driver-dependent whether a connector with this
	 * status can be lit up or not.
	 */
	connector_status_disconnected = 2,
	/**
	 * @connector_status_unknown: The connector's status could not be
	 * reliably detected. This happens when probing would either cause
	 * flicker (like load-detection when the connector is in use), or when a
	 * hardware resource isn't available (like when load-detection needs a
	 * free CRTC). It should be possible to light up the connector with one
	 * of the listed fallback modes. For default configuration userspace
	 * should only try to light up connectors with unknown status when
	 * there's not connector with @connector_status_connected.
	 */
	connector_status_unknown = 3,
};

enum subpixel_order {
	SubPixelUnknown = 0,
	SubPixelHorizontalRGB,
	SubPixelHorizontalBGR,
	SubPixelVerticalRGB,
	SubPixelVerticalBGR,
	SubPixelNone,
};

/**
 * struct drm_display_info - runtime data about the connected sink
 *
 * Describes a given display (e.g. CRT or flat panel) and its limitations. For
 * fixed display sinks like built-in panels there's not much difference between
 * this and &struct drm_connector. But for sinks with a real cable this
 * structure is meant to describe all the things at the other end of the cable.
 *
 * For sinks which provide an EDID this can be filled out by calling
 * drm_add_edid_modes().
 */
struct drm_display_info {
	/**
	 * @name: Name of the display.
	 */
	char name[DRM_DISPLAY_INFO_LEN];

	/**
	 * @width_mm: Physical width in mm.
	 */
	unsigned int width_mm;
	/**
	 * @height_mm: Physical height in mm.
	 */
	unsigned int height_mm;

	/* Clock limits FIXME: storage format */
	unsigned int min_vfreq, max_vfreq;
	unsigned int min_hfreq, max_hfreq;

	/**
	 * @pixel_clock: Maximum pixel clock supported by the sink, in units of
	 * 100Hz. This mismatches the clock in &drm_display_mode (which is in
	 * kHZ), because that's what the EDID uses as base unit.
	 */
	unsigned int pixel_clock;

	/**
	 * @bpc: Maximum bits per color channel. Used by HDMI and DP outputs.
	 */
	unsigned int bpc;

	/**
	 * @subpixel_order: Subpixel order of LCD panels.
	 */
	enum subpixel_order subpixel_order;

#define DRM_COLOR_FORMAT_RGB444		(1<<0)
#define DRM_COLOR_FORMAT_YCRCB444	(1<<1)
#define DRM_COLOR_FORMAT_YCRCB422	(1<<2)
#define DRM_COLOR_FORMAT_YCRCB420	(1<<3)

	/**
	 * @color_formats: HDMI Color formats, selects between RGB and YCrCb
	 * modes. Used DRM_COLOR_FORMAT\_ defines, which are _not_ the same ones
	 * as used to describe the pixel format in framebuffers, and also don't
	 * match the formats in @bus_formats which are shared with v4l.
	 */
	u32 color_formats;

	/**
	 * @cea_rev: CEA revision of the HDMI sink.
	 */
	u8 cea_rev;
};

/**
 * drm_connector_funcs - control connectors on a given device
 * @dpms: set power state (see drm_crtc_funcs above)
 * @save: save connector state
 * @restore: restore connector state
 * @reset: reset connector after state has been invalidate (e.g. resume)
 * @detect: is this connector active?
 * @get_modes: get mode list for this connector
 * @set_property: property for this connector may need update
 * @destroy: make object go away
 * @force: notify the driver the connector is forced on
 *
 * Each CRTC may have one or more connectors attached to it.  The functions
 * below allow the core DRM code to control connectors, enumerate available modes,
 * etc.
 */
struct drm_connector_funcs {
	void (*dpms)(struct drm_connector *connector, int mode);
	void (*save)(struct drm_connector *connector);
	void (*restore)(struct drm_connector *connector);
	void (*reset)(struct drm_connector *connector);

	/* Check to see if anything is attached to the connector.
	 * @force is set to false whilst polling, true when checking the
	 * connector due to user request. @force can be used by the driver
	 * to avoid expensive, destructive operations during automated
	 * probing.
	 */
	enum drm_connector_status (*detect)(struct drm_connector *connector,
					    bool force);
	int (*fill_modes)(struct drm_connector *connector, uint32_t max_width, uint32_t max_height);
	int (*set_property)(struct drm_connector *connector, struct drm_property *property,
			     uint64_t val);
	void (*destroy)(struct drm_connector *connector);
	void (*force)(struct drm_connector *connector);
};

/**
 * drm_connector - central DRM connector control structure
 * @dev: parent DRM device
 * @kdev: kernel device for sysfs attributes
 * @attr: sysfs attributes
 * @head: list management
 * @base: base KMS object
 * @connector_type: one of the %DRM_MODE_CONNECTOR_<foo> types from drm_mode.h
 * @connector_type_id: index into connector type enum
 * @interlace_allowed: can this connector handle interlaced modes?
 * @doublescan_allowed: can this connector handle doublescan?
 * @modes: modes available on this connector (from fill_modes() + user)
 * @status: one of the drm_connector_status enums (connected, not, or unknown)
 * @probed_modes: list of modes derived directly from the display
 * @display_info: information about attached display (e.g. from EDID)
 * @funcs: connector control functions
 * @edid_blob_ptr: DRM property containing EDID if present
 * @property_ids: property tracking for this connector
 * @polled: a %DRM_CONNECTOR_POLL_<foo> value for core driven polling
 * @dpms: current dpms state
 * @helper_private: mid-layer private data
 * @force: a %DRM_FORCE_<foo> state for forced mode sets
 * @encoder_ids: valid encoders for this connector
 * @encoder: encoder driving this connector, if any
 * @eld: EDID-like data, if present
 * @dvi_dual: dual link DVI, if found
 * @max_tmds_clock: max clock rate, if found
 * @latency_present: AV delay info from ELD, if found
 * @video_latency: video latency info from ELD, if found
 * @audio_latency: audio latency info from ELD, if found
 * @null_edid_counter: track sinks that give us all zeros for the EDID
 *
 * Each connector may be connected to one or more CRTCs, or may be clonable by
 * another connector if they can share a CRTC.  Each connector also has a specific
 * position in the broader display (referred to as a 'screen' though it could
 * span multiple monitors).
 */
struct drm_connector {
	struct drm_device *dev;
	//struct device kdev;
	struct device_attribute *attr;
	struct list_head head;

	struct drm_mode_object base;

	int connector_type;
	int connector_type_id;
	bool interlace_allowed;
	bool doublescan_allowed;
	struct list_head modes; /* list of modes on this connector */

	enum drm_connector_status status;

	/* these are modes added by probing with DDC or the BIOS */
	struct list_head probed_modes;

	struct drm_display_info display_info;
	const struct drm_connector_funcs *funcs;

	struct drm_property_blob *edid_blob_ptr;
	struct drm_object_properties properties;

/* should we poll this connector for connects and disconnects */
/* hot plug detectable */
#define DRM_CONNECTOR_POLL_HPD (1 << 0)
/* poll for connections */
#define DRM_CONNECTOR_POLL_CONNECT (1 << 1)
/* can cleanly poll for disconnections without flickering the screen */
/* DACs should rarely do this without a lot of testing */
#define DRM_CONNECTOR_POLL_DISCONNECT (1 << 2)

	uint8_t polled; /* DRM_CONNECTOR_POLL_* */

	/* requested DPMS state */
	int dpms;

	void *helper_private;

	/* forced on connector */
	enum drm_connector_force force;

#define DRM_CONNECTOR_MAX_ENCODER 3
	uint32_t encoder_ids[DRM_CONNECTOR_MAX_ENCODER];
	struct drm_encoder *encoder; /* currently active encoder */

#define MAX_ELD_BYTES	128
	/* EDID bits */
	uint8_t eld[MAX_ELD_BYTES];
	bool dvi_dual;
	int max_tmds_clock;	/* in MHz */
	bool latency_present[2];
	int video_latency[2];	/* [0]: progressive, [1]: interlaced */
	int audio_latency[2];
	int null_edid_counter; /* needed to workaround some HW bugs where we get all 0s */
	unsigned bad_edid_counter;
};

#define obj_to_connector(x) container_of(x, struct drm_connector, base)

int drm_connector_init(struct drm_device *dev,
			    struct drm_connector *connector,
			    const struct drm_connector_funcs *funcs,
			    int connector_type);
void drm_connector_cleanup(struct drm_connector *connector);
/* helper to unplug all connectors from sysfs for device */
void drm_connector_unplug_all(struct drm_device *dev);

int drm_mode_connector_attach_encoder(struct drm_connector *connector,
					     struct drm_encoder *encoder);
void drm_mode_connector_detach_encoder(struct drm_connector *connector,
					   struct drm_encoder *encoder);

const char *drm_get_connector_name(const struct drm_connector *connector);
const char *drm_get_connector_status_name(enum drm_connector_status status);
const char *drm_get_dpms_name(int val);
const char *drm_get_dvi_i_subconnector_name(int val);
const char *drm_get_dvi_i_select_name(int val);
const char *drm_get_tv_subconnector_name(int val);
const char *drm_get_tv_select_name(int val);

int drm_mode_create_dvi_i_properties(struct drm_device *dev);
int drm_mode_create_tv_properties(struct drm_device *dev, int num_formats,
				     char *formats[]);
int drm_mode_create_scaling_mode_property(struct drm_device *dev);
int drm_mode_create_dithering_property(struct drm_device *dev);
int drm_mode_create_dirty_info_property(struct drm_device *dev);

int drm_mode_connector_update_edid_property(struct drm_connector *connector,
						struct edid *edid);

#endif /* __DRM_CONNECTOR_H__ */
