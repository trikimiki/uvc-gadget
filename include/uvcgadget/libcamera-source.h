/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libcamera video source
 *
 * Copyright (C) 2022 Ideas on Board Oy.
 * Copyright (C) 2022 Kieran Bingham
 *
 * Contact: Kieran Bingham <kieran.bingham@ideasonboard.com>
 */
#ifndef __LIBCAMERA_SOURCE_H__
#define __LIBCAMERA_SOURCE_H__

#include "video-source.h"

struct events;
struct video_source;
/* Container for libcamera options */
struct camera_arguments {
	char *af_range_mode;
	char *af_speed_mode;
	char *awb_mode;
	char *exposure_mode;

	// colour-gains | awbgains
	float colour_gain_r;
	float colour_gain_b;

	float lens_position;

	float brightness;
	float contrast;
	float saturation;
	float sharpness;

	int debug_report_enabled;
};

#ifdef __cplusplus
extern "C" {
#endif

struct video_source *libcamera_source_create(const char *devname);
void libcamera_source_set_controls(struct video_source *src, struct camera_arguments *input_controls);
void libcamera_source_init(struct video_source *src, struct events *events);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBCAMERA_SOURCE_H__ */
