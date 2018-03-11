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
#include <drm/drm_connector.h>
#include <drm/drm_edid.h>
#include <drm/drm_encoder.h>

struct drm_conn_prop_enum_list {
	int type;
	const char *name;
	int count;
};

/*
 * Connector and encoder types.
 */
static struct drm_conn_prop_enum_list drm_connector_enum_list[] =
{	{ DRM_MODE_CONNECTOR_Unknown, "Unknown", 0 },
	{ DRM_MODE_CONNECTOR_VGA, "VGA", 0 },
	{ DRM_MODE_CONNECTOR_DVII, "DVI-I", 0 },
	{ DRM_MODE_CONNECTOR_DVID, "DVI-D", 0 },
	{ DRM_MODE_CONNECTOR_DVIA, "DVI-A", 0 },
	{ DRM_MODE_CONNECTOR_Composite, "Composite", 0 },
	{ DRM_MODE_CONNECTOR_SVIDEO, "SVIDEO", 0 },
	{ DRM_MODE_CONNECTOR_LVDS, "LVDS", 0 },
	{ DRM_MODE_CONNECTOR_Component, "Component", 0 },
	{ DRM_MODE_CONNECTOR_9PinDIN, "DIN", 0 },
	{ DRM_MODE_CONNECTOR_DisplayPort, "DP", 0 },
	{ DRM_MODE_CONNECTOR_HDMIA, "HDMI-A", 0 },
	{ DRM_MODE_CONNECTOR_HDMIB, "HDMI-B", 0 },
	{ DRM_MODE_CONNECTOR_TV, "TV", 0 },
	{ DRM_MODE_CONNECTOR_eDP, "eDP", 0 },
	{ DRM_MODE_CONNECTOR_VIRTUAL, "Virtual", 0},
};



/**
 * drm_connector_init - Init a preallocated connector
 * @dev: DRM device
 * @connector: the connector to init
 * @funcs: callbacks for this connector
 * @name: user visible name of the connector
 *
 *
 * Initialises a preallocated connector. Connectors should be
 * subclassed as part of driver connector objects.
 *
 * RETURNS:
 * Zero on success, error code on failure.
 */
int drm_connector_init(struct drm_device *dev,
		     struct drm_connector *connector,
		     const struct drm_connector_funcs *funcs,
		     int connector_type)
{
	int ret;

	drm_modeset_lock_all(dev);

	ret = drm_mode_object_get(dev, &connector->base, DRM_MODE_OBJECT_CONNECTOR);
	if (ret)
		goto out;

	connector->base.properties = &connector->properties;
	connector->dev = dev;
	connector->funcs = funcs;
	connector->connector_type = connector_type;
	connector->connector_type_id =
		++drm_connector_enum_list[connector_type].count; /* TODO */
	INIT_LIST_HEAD(&connector->probed_modes);
	INIT_LIST_HEAD(&connector->modes);
	connector->edid_blob_ptr = NULL;
	connector->status = connector_status_unknown;

	list_add_tail(&connector->head, &dev->mode_config.connector_list, (caddr_t)connector);
	dev->mode_config.num_connector++;

	if (connector_type != DRM_MODE_CONNECTOR_VIRTUAL)
		drm_object_attach_property(&connector->base,
					      dev->mode_config.edid_property,
					      0);

	drm_object_attach_property(&connector->base,
				      dev->mode_config.dpms_property, 0);

 out:
	drm_modeset_unlock_all(dev);

	return ret;
}

/**
 * drm_mode_remove - remove and free a mode
 * @connector: connector list to modify
 * @mode: mode to remove
 *
 *
 * Remove @mode from @connector's mode list, then free it.
 */
static void drm_mode_remove(struct drm_connector *connector,
		     struct drm_display_mode *mode)
{
	list_del(&mode->head);
	drm_mode_destroy(connector->dev, mode);
}

/**
 * drm_connector_cleanup - cleans up an initialised connector
 * @connector: connector to cleanup
 *
 *
 * Cleans up the connector but doesn't free the object.
 */
void drm_connector_cleanup(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *mode, *t;

	list_for_each_entry_safe(mode, t, struct drm_display_mode, &connector->probed_modes, head)
		drm_mode_remove(connector, mode);

	list_for_each_entry_safe(mode, t, struct drm_display_mode, &connector->modes, head)
		drm_mode_remove(connector, mode);

	drm_mode_object_put(dev, &connector->base);
	list_del(&connector->head);
	dev->mode_config.num_connector--;
}

/* LINTED E_FUNC_ARG_UNUSED */
void drm_connector_unplug_all(struct drm_device *dev)
{
	/*
	struct drm_connector *connector;
	*/
	/* taking the mode config mutex ends up in a clash with sysfs */
	/*
	list_for_each_entry(connector, struct drm_connector, &dev->mode_config.connector_list, head)
		drm_sysfs_connector_remove(connector);
	*/
}

int drm_mode_connector_attach_encoder(struct drm_connector *connector,
				      struct drm_encoder *encoder)
{
	int i;

	for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
		if (connector->encoder_ids[i] == 0) {
			connector->encoder_ids[i] = encoder->base.id;
			return 0;
		}
	}
	return -ENOMEM;
}

void drm_mode_connector_detach_encoder(struct drm_connector *connector,
				    struct drm_encoder *encoder)
{
	int i;
	for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
		if (connector->encoder_ids[i] == encoder->base.id) {
			connector->encoder_ids[i] = 0;
			if (connector->encoder == encoder)
				connector->encoder = NULL;
			break;
		}
	}
}

const char *drm_get_connector_name(const struct drm_connector *connector)
{
	static char buf[32];

	(void) snprintf(buf, 32, "%s-%d",
		 drm_connector_enum_list[connector->connector_type].name,
		 connector->connector_type_id);
	return buf;
}

const char *drm_get_connector_status_name(enum drm_connector_status status)
{
	if (status == connector_status_connected)
		return "connected";
	else if (status == connector_status_disconnected)
		return "disconnected";
	else
		return "unknown";
}

/* Avoid boilerplate. */
#define DRM_ENUM_NAME_FN(fnname, list)				\
	const char *fnname(int val)					\
	{							\
		int i;						\
		for (i = 0; i < ARRAY_SIZE(list); i++) {	\
			if (list[i].type == val)		\
				return list[i].name;		\
		}						\
		return "(unknown)";				\
	}

/*
 * Global properties
 */
static const struct drm_prop_enum_list drm_dpms_enum_list[] =
{	{ DRM_MODE_DPMS_ON, "On" },
	{ DRM_MODE_DPMS_STANDBY, "Standby" },
	{ DRM_MODE_DPMS_SUSPEND, "Suspend" },
	{ DRM_MODE_DPMS_OFF, "Off" }
};

DRM_ENUM_NAME_FN(drm_get_dpms_name, drm_dpms_enum_list)

/*
 * Optional properties
 */
static const struct drm_prop_enum_list drm_scaling_mode_enum_list[] =
{
	{ DRM_MODE_SCALE_NONE, "None" },
	{ DRM_MODE_SCALE_FULLSCREEN, "Full" },
	{ DRM_MODE_SCALE_CENTER, "Center" },
	{ DRM_MODE_SCALE_ASPECT, "Full aspect" },
};

static const struct drm_prop_enum_list drm_dithering_mode_enum_list[] =
{
	{ DRM_MODE_DITHERING_OFF, "Off" },
	{ DRM_MODE_DITHERING_ON, "On" },
	{ DRM_MODE_DITHERING_AUTO, "Automatic" },
};

/*
 * Non-global properties, but "required" for certain connectors.
 */
static const struct drm_prop_enum_list drm_dvi_i_select_enum_list[] =
{
	{ DRM_MODE_SUBCONNECTOR_Automatic, "Automatic" }, /* DVI-I and TV-out */
	{ DRM_MODE_SUBCONNECTOR_DVID,      "DVI-D"     }, /* DVI-I  */
	{ DRM_MODE_SUBCONNECTOR_DVIA,      "DVI-A"     }, /* DVI-I  */
};

DRM_ENUM_NAME_FN(drm_get_dvi_i_select_name, drm_dvi_i_select_enum_list)

static const struct drm_prop_enum_list drm_dvi_i_subconnector_enum_list[] =
{
	{ DRM_MODE_SUBCONNECTOR_Unknown,   "Unknown"   }, /* DVI-I and TV-out */
	{ DRM_MODE_SUBCONNECTOR_DVID,      "DVI-D"     }, /* DVI-I  */
	{ DRM_MODE_SUBCONNECTOR_DVIA,      "DVI-A"     }, /* DVI-I  */
};

DRM_ENUM_NAME_FN(drm_get_dvi_i_subconnector_name,
		 drm_dvi_i_subconnector_enum_list)

static const struct drm_prop_enum_list drm_tv_select_enum_list[] =
{
	{ DRM_MODE_SUBCONNECTOR_Automatic, "Automatic" }, /* DVI-I and TV-out */
	{ DRM_MODE_SUBCONNECTOR_Composite, "Composite" }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_SVIDEO,    "SVIDEO"    }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_Component, "Component" }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_SCART,     "SCART"     }, /* TV-out */
};

DRM_ENUM_NAME_FN(drm_get_tv_select_name, drm_tv_select_enum_list)

static const struct drm_prop_enum_list drm_tv_subconnector_enum_list[] =
{
	{ DRM_MODE_SUBCONNECTOR_Unknown,   "Unknown"   }, /* DVI-I and TV-out */
	{ DRM_MODE_SUBCONNECTOR_Composite, "Composite" }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_SVIDEO,    "SVIDEO"    }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_Component, "Component" }, /* TV-out */
	{ DRM_MODE_SUBCONNECTOR_SCART,     "SCART"     }, /* TV-out */
};

DRM_ENUM_NAME_FN(drm_get_tv_subconnector_name,
		 drm_tv_subconnector_enum_list)

static const struct drm_prop_enum_list drm_dirty_info_enum_list[] = {
	{ DRM_MODE_DIRTY_OFF,      "Off"      },
	{ DRM_MODE_DIRTY_ON,       "On"       },
	{ DRM_MODE_DIRTY_ANNOTATE, "Annotate" },
};

int drm_connector_create_standard_properties(struct drm_device *dev)
{
	struct drm_property *edid;
	struct drm_property *dpms;

	/*
	 * Standard properties (apply to all connectors)
	 */
	edid = drm_property_create(dev, DRM_MODE_PROP_BLOB |
				   DRM_MODE_PROP_IMMUTABLE,
				   "EDID", 0);
	dev->mode_config.edid_property = edid;

	dpms = drm_property_create_enum(dev, 0,
				   "DPMS", drm_dpms_enum_list,
				   ARRAY_SIZE(drm_dpms_enum_list));
	dev->mode_config.dpms_property = dpms;

	return 0;
}

/**
 * drm_mode_create_dvi_i_properties - create DVI-I specific connector properties
 * @dev: DRM device
 *
 * Called by a driver the first time a DVI-I connector is made.
 */
int drm_mode_create_dvi_i_properties(struct drm_device *dev)
{
	struct drm_property *dvi_i_selector;
	struct drm_property *dvi_i_subconnector;

	if (dev->mode_config.dvi_i_select_subconnector_property)
		return 0;

	dvi_i_selector =
		drm_property_create_enum(dev, 0,
				    "select subconnector",
				    drm_dvi_i_select_enum_list,
				    ARRAY_SIZE(drm_dvi_i_select_enum_list));
	dev->mode_config.dvi_i_select_subconnector_property = dvi_i_selector;

	dvi_i_subconnector = drm_property_create_enum(dev, DRM_MODE_PROP_IMMUTABLE,
				    "subconnector",
				    drm_dvi_i_subconnector_enum_list,
				    ARRAY_SIZE(drm_dvi_i_subconnector_enum_list));
	dev->mode_config.dvi_i_subconnector_property = dvi_i_subconnector;

	return 0;
}

/**
 * drm_create_tv_properties - create TV specific connector properties
 * @dev: DRM device
 * @num_modes: number of different TV formats (modes) supported
 * @modes: array of pointers to strings containing name of each format
 *
 * Called by a driver's TV initialization routine, this function creates
 * the TV specific connector properties for a given device.  Caller is
 * responsible for allocating a list of format names and passing them to
 * this routine.
 */
int drm_mode_create_tv_properties(struct drm_device *dev, int num_modes,
				  char *modes[])
{
	struct drm_property *tv_selector;
	struct drm_property *tv_subconnector;
	int i;

	if (dev->mode_config.tv_select_subconnector_property)
		return 0;

	/*
	 * Basic connector properties
	 */
	tv_selector = drm_property_create_enum(dev, 0,
					  "select subconnector",
					  drm_tv_select_enum_list,
					  ARRAY_SIZE(drm_tv_select_enum_list));
	dev->mode_config.tv_select_subconnector_property = tv_selector;

	tv_subconnector =
		drm_property_create_enum(dev, DRM_MODE_PROP_IMMUTABLE,
				    "subconnector",
				    drm_tv_subconnector_enum_list,
				    ARRAY_SIZE(drm_tv_subconnector_enum_list));
	dev->mode_config.tv_subconnector_property = tv_subconnector;

	/*
	 * Other, TV specific properties: margins & TV modes.
	 */
	dev->mode_config.tv_left_margin_property =
		drm_property_create_range(dev, 0, "left margin", 0, 100);

	dev->mode_config.tv_right_margin_property =
		drm_property_create_range(dev, 0, "right margin", 0, 100);

	dev->mode_config.tv_top_margin_property =
		drm_property_create_range(dev, 0, "top margin", 0, 100);

	dev->mode_config.tv_bottom_margin_property =
		drm_property_create_range(dev, 0, "bottom margin", 0, 100);

	dev->mode_config.tv_mode_property =
		drm_property_create(dev, DRM_MODE_PROP_ENUM,
				    "mode", num_modes);
	for (i = 0; i < num_modes; i++)
		(void) drm_property_add_enum(dev->mode_config.tv_mode_property, i,
				      i, modes[i]);

	dev->mode_config.tv_brightness_property =
		drm_property_create_range(dev, 0, "brightness", 0, 100);

	dev->mode_config.tv_contrast_property =
		drm_property_create_range(dev, 0, "contrast", 0, 100);

	dev->mode_config.tv_flicker_reduction_property =
		drm_property_create_range(dev, 0, "flicker reduction", 0, 100);

	dev->mode_config.tv_overscan_property =
		drm_property_create_range(dev, 0, "overscan", 0, 100);

	dev->mode_config.tv_saturation_property =
		drm_property_create_range(dev, 0, "saturation", 0, 100);

	dev->mode_config.tv_hue_property =
		drm_property_create_range(dev, 0, "hue", 0, 100);

	return 0;
}

/**
 * drm_mode_create_scaling_mode_property - create scaling mode property
 * @dev: DRM device
 *
 * Called by a driver the first time it's needed, must be attached to desired
 * connectors.
 */
int drm_mode_create_scaling_mode_property(struct drm_device *dev)
{
	struct drm_property *scaling_mode;

	if (dev->mode_config.scaling_mode_property)
		return 0;

	scaling_mode =
		drm_property_create_enum(dev, 0, "scaling mode",
				drm_scaling_mode_enum_list,
				    ARRAY_SIZE(drm_scaling_mode_enum_list));

	dev->mode_config.scaling_mode_property = scaling_mode;

	return 0;
}

/**
 * drm_mode_create_dithering_property - create dithering property
 * @dev: DRM device
 *
 * Called by a driver the first time it's needed, must be attached to desired
 * connectors.
 */
int drm_mode_create_dithering_property(struct drm_device *dev)
{
	struct drm_property *dithering_mode;

	if (dev->mode_config.dithering_mode_property)
		return 0;

	dithering_mode =
		drm_property_create_enum(dev, 0, "dithering",
				drm_dithering_mode_enum_list,
				    ARRAY_SIZE(drm_dithering_mode_enum_list));
	dev->mode_config.dithering_mode_property = dithering_mode;

	return 0;
}

/**
 * drm_mode_create_dirty_property - create dirty property
 * @dev: DRM device
 *
 * Called by a driver the first time it's needed, must be attached to desired
 * connectors.
 */
int drm_mode_create_dirty_info_property(struct drm_device *dev)
{
	struct drm_property *dirty_info;

	if (dev->mode_config.dirty_info_property)
		return 0;

	dirty_info =
		drm_property_create_enum(dev, DRM_MODE_PROP_IMMUTABLE,
				    "dirty",
				    drm_dirty_info_enum_list,
				    ARRAY_SIZE(drm_dirty_info_enum_list));
	dev->mode_config.dirty_info_property = dirty_info;

	return 0;
}

/**
 * drm_mode_getconnector - get connector configuration
 * @inode: inode from the ioctl
 * @filp: file * from the ioctl
 * @cmd: cmd from ioctl
 * @arg: arg from ioctl
 *
 *
 * Construct a connector configuration structure to return to the user.
 *
 * Called by the user via ioctl.
 *
 * RETURNS:
 * Zero on success, errno on failure.
 */
/* LINTED E_FUNC_ARG_UNUSED */
int drm_mode_getconnector(DRM_IOCTL_ARGS)
{
	struct drm_mode_get_connector *out_resp = data;
	struct drm_mode_object *obj;
	struct drm_connector *connector;
	struct drm_display_mode *mode;
	int mode_count = 0;
	int props_count = 0;
	int encoders_count = 0;
	int ret = 0;
	int copied = 0;
	int i;
	struct drm_mode_modeinfo u_mode;
	struct drm_mode_modeinfo __user *mode_ptr;
	uint32_t __user *prop_ptr;
	uint64_t __user *prop_values;
	uint32_t __user *encoder_ptr;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return -EINVAL;

	(void) memset(&u_mode, 0, sizeof(struct drm_mode_modeinfo));

	DRM_DEBUG_KMS("[CONNECTOR:%d:?]\n", out_resp->connector_id);

	mutex_lock(&dev->mode_config.mutex);

	obj = drm_mode_object_find(dev, out_resp->connector_id,
				   DRM_MODE_OBJECT_CONNECTOR);
	if (!obj) {
		ret = -EINVAL;
		goto out;
	}
	connector = obj_to_connector(obj);

	props_count = connector->properties.count;

	for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
		if (connector->encoder_ids[i] != 0) {
			encoders_count++;
		}
	}

	if (out_resp->count_modes == 0) {
		connector->funcs->fill_modes(connector,
					     dev->mode_config.max_width,
					     dev->mode_config.max_height);
	}

	/* delayed so we get modes regardless of pre-fill_modes state */
	list_for_each_entry(mode, struct drm_display_mode, &connector->modes, head)
		mode_count++;

	out_resp->connector_id = connector->base.id;
	out_resp->connector_type = connector->connector_type;
	out_resp->connector_type_id = connector->connector_type_id;
	out_resp->mm_width = connector->display_info.width_mm;
	out_resp->mm_height = connector->display_info.height_mm;
	out_resp->subpixel = connector->display_info.subpixel_order;
	out_resp->connection = connector->status;
	if (connector->encoder)
		out_resp->encoder_id = connector->encoder->base.id;
	else
		out_resp->encoder_id = 0;

	/*
	 * This ioctl is called twice, once to determine how much space is
	 * needed, and the 2nd time to fill it.
	 */
	if ((out_resp->count_modes >= mode_count) && mode_count) {
		copied = 0;
		mode_ptr = (struct drm_mode_modeinfo *)(unsigned long)out_resp->modes_ptr;
		list_for_each_entry(mode, struct drm_display_mode, &connector->modes, head) {
			drm_crtc_convert_to_umode(&u_mode, mode);
			if (DRM_COPY_TO_USER(mode_ptr + copied,
					 &u_mode, sizeof(u_mode))) {
				ret = -EFAULT;
				goto out;
			}
			copied++;
		}
	}
	out_resp->count_modes = mode_count;

	if ((out_resp->count_props >= props_count) && props_count) {
		copied = 0;
		prop_ptr = (uint32_t *)(unsigned long)(out_resp->props_ptr);
		prop_values = (uint64_t *)(unsigned long)(out_resp->prop_values_ptr);
		for (i = 0; i < connector->properties.count; i++) {
			if (put_user(connector->properties.ids[i],
					     prop_ptr + copied)) {
					ret = -EFAULT;
					goto out;
				}

			if (put_user(connector->properties.values[i],
					     prop_values + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
		}
	}
	out_resp->count_props = props_count;

	if ((out_resp->count_encoders >= encoders_count) && encoders_count) {
		copied = 0;
		encoder_ptr = (uint32_t *)(unsigned long)(out_resp->encoders_ptr);
		for (i = 0; i < DRM_CONNECTOR_MAX_ENCODER; i++) {
			if (connector->encoder_ids[i] != 0) {
				if (put_user(connector->encoder_ids[i],
					     encoder_ptr + copied)) {
					ret = -EFAULT;
					goto out;
				}
				copied++;
			}
		}
	}
	out_resp->count_encoders = encoders_count;

out:
	mutex_unlock(&dev->mode_config.mutex);
	return ret;
}


int drm_mode_connector_update_edid_property(struct drm_connector *connector,
					    struct edid *edid)
{
	struct drm_device *dev = connector->dev;
	int ret, size;

	if (connector->edid_blob_ptr)
		drm_property_destroy_blob(dev, connector->edid_blob_ptr);

	/* Delete edid, when there is none. */
	if (!edid) {
		connector->edid_blob_ptr = NULL;
		ret = drm_object_property_set_value(&connector->base, dev->mode_config.edid_property, 0);
		return ret;
	}

	size = EDID_LENGTH * (1 + edid->extensions);
	connector->edid_blob_ptr = drm_property_create_blob(connector->dev,
							    size, edid);
	if (!connector->edid_blob_ptr)
		return -EINVAL;

	ret = drm_object_property_set_value(&connector->base,
					       dev->mode_config.edid_property,
					       connector->edid_blob_ptr->base.id);

	return ret;
}

int drm_mode_connector_set_obj_prop(struct drm_mode_object *obj,
					   struct drm_property *property,
					   uint64_t value)
{
	int ret = -EINVAL;
	struct drm_connector *connector = obj_to_connector(obj);

	/* Do DPMS ourselves */
	if (property == connector->dev->mode_config.dpms_property) {
		if (connector->funcs->dpms)
			(*connector->funcs->dpms)(connector, (int)value);
		ret = 0;
	} else if (connector->funcs->set_property)
		ret = connector->funcs->set_property(connector, property, value);

	/* store the property value if successful */
	if (!ret)
		ret = drm_object_property_set_value(&connector->base, property, value);
	return ret;
}

int drm_mode_connector_property_set_ioctl(DRM_IOCTL_ARGS)
{
	struct drm_mode_connector_set_property *conn_set_prop = data;
	struct drm_mode_obj_set_property obj_set_prop = {
		.value = conn_set_prop->value,
		.prop_id = conn_set_prop->prop_id,
		.obj_id = conn_set_prop->connector_id,
		.obj_type = DRM_MODE_OBJECT_CONNECTOR
	};

	/* It does all the locking and checking we need */
	return drm_mode_obj_set_property_ioctl(dev_id, dev, &obj_set_prop, file, ioctl_mode, credp);
}
