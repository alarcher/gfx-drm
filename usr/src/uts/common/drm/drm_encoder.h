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
#ifndef __DRM_ENCODER_H__
#define __DRM_ENCODER_H__

#include <drm/drm_linux.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mode.h>
#include <drm/drm_mode_object.h>

struct drm_encoder;

/**
 * drm_encoder_funcs - encoder controls
 * @reset: reset state (e.g. at init or resume time)
 * @destroy: cleanup and free associated data
 *
 * Encoders sit between CRTCs and connectors.
 */
struct drm_encoder_funcs {
	void (*reset)(struct drm_encoder *encoder);
	void (*destroy)(struct drm_encoder *encoder);
};

#define DRM_CONNECTOR_MAX_UMODES 16
#define DRM_CONNECTOR_LEN 32

/**
 * drm_encoder - central DRM encoder structure
 * @dev: parent DRM device
 * @head: list management
 * @base: base KMS object
 * @encoder_type: one of the %DRM_MODE_ENCODER_<foo> types in drm_mode.h
 * @possible_crtcs: bitmask of potential CRTC bindings
 * @possible_clones: bitmask of potential sibling encoders for cloning
 * @crtc: currently bound CRTC
 * @funcs: control functions
 * @helper_private: mid-layer private data
 *
 * CRTCs drive pixels to encoders, which convert them into signals
 * appropriate for a given connector or set of connectors.
 */
struct drm_encoder {
	struct drm_device *dev;
	struct list_head head;

	struct drm_mode_object base;
	int encoder_type;
	uint32_t possible_crtcs;
	uint32_t possible_clones;

	struct drm_crtc *crtc;
	const struct drm_encoder_funcs *funcs;
	void *helper_private;
};

#define obj_to_encoder(x) container_of(x, struct drm_encoder, base)

int drm_encoder_init(struct drm_device *dev,
			     struct drm_encoder *encoder,
			     const struct drm_encoder_funcs *funcs,
			     int encoder_type);

void drm_encoder_cleanup(struct drm_encoder *encoder);

const char *drm_get_encoder_name(const struct drm_encoder *encoder);

#define /* __DRM_ENCODER_H__ */
