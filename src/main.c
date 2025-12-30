/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * UVC gadget test application
 *
 * Copyright (C) 2010-2018 Laurent Pinchart
 *
 * Contact: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "config.h"
#include "configfs.h"
#include "events.h"
#include "stream.h"
#include "libcamera-source.h"
#include "v4l2-source.h"
#include "test-source.h"
#include "jpg-source.h"
#include "slideshow-source.h"

#ifdef HAVE_LIBCAMERA
/* Validation of given option against a list of allowed camera modes */
static int is_camera_mode_valid(const char *mode, const char *valid_modes[])
{
	for (int i = 0; valid_modes[i] != NULL; i++) {
		if (strcmp(mode, valid_modes[i]) == 0)
			return 1;
	}
	return 0;
}
static const char *camera_valid_af_range_modes[] = {
	"normal",
	"macro",
	NULL
};
static const char *camera_valid_af_speed_modes[] = {
	"normal",
	"fast",
	NULL
};
static const char *camera_valid_awb_modes[] = {
	"auto",
	"incandescent",
	"tungsten",
	"fluorescent",
	"indoor",
	"daylight",
	"cloudy",
	NULL
};
static const char *camera_valid_exposure_modes[] = {
	"normal",
	"short",
	"sport", // rpicam-apps implementation of "short"
	"long",
	NULL
};
#endif

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [options] <uvc device>\n", argv0);
	fprintf(stderr, "Available options are\n");
#ifdef HAVE_LIBCAMERA
	fprintf(stderr, " -c|--camera <index|id>        libcamera camera name\n");
	fprintf(stderr, "    --autofocus-range <mode>   [libcamera] distances range of Autofocus (AF) scan\n");
	fprintf(stderr, "                                  values: ");
	for (int i = 0; camera_valid_af_range_modes[i] != NULL; i++)
		fprintf(stderr, "%s%s", camera_valid_af_range_modes[i], camera_valid_af_range_modes[i+1] ? ", " : "\n");
	fprintf(stderr, "    --autofocus-speed <mode>   [libcamera] AF lens speed when changing focus\n");
	fprintf(stderr, "                                  values: ");
	for (int i = 0; camera_valid_af_speed_modes[i] != NULL; i++)
		fprintf(stderr, "%s%s", camera_valid_af_speed_modes[i], camera_valid_af_speed_modes[i+1] ? ", " : "\n");
	fprintf(stderr, "    --awb <mode>               [libcamera] Auto White Balance (AWB) algorithm mode\n");
	fprintf(stderr, "                                  values: ");
	for (int i = 0; camera_valid_awb_modes[i] != NULL; i++)
		fprintf(stderr, "%s%s", camera_valid_awb_modes[i], camera_valid_awb_modes[i+1] ? ", " : "\n");
	fprintf(stderr, "    --exposure <mode>          [libcamera] AEGC algorithm exposure mode\n");
	fprintf(stderr, "                                  values: ");
	for (int i = 0; camera_valid_exposure_modes[i] != NULL; i++)
		fprintf(stderr, "%s%s", camera_valid_exposure_modes[i], camera_valid_exposure_modes[i+1] ? ", " : "\n");
	fprintf(stderr, "                                  (\"sport\" equals \"short\")");
#endif
	fprintf(stderr, " -d|--device <device>          V4L2 source device\n");
	fprintf(stderr, " -i|--image <image>            MJPEG image\n");
	fprintf(stderr, " -s|--slideshow <directory>    directory of slideshow images\n");
	fprintf(stderr, " -h|--help                     Print this help screen and exit\n");
	fprintf(stderr, "\n");
	fprintf(stderr, " <uvc device>                  UVC device instance specifier\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "  For ConfigFS devices the <uvc device> parameter can take the form of a shortened\n");
	fprintf(stderr, "  function specifier such as: 'uvc.0', or if multiple gadgets are configured, the\n");
	fprintf(stderr, "  gadget name should be included to prevent ambiguity: 'g1/functions/uvc.0'.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  For legacy g_webcam UVC instances, this parameter will identify the UDC that the\n");
	fprintf(stderr, "  UVC function is bound to.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  The parameter is optional, and if not provided the first UVC function on the first\n");
	fprintf(stderr, "  gadget identified will be used.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example usage:\n");
	fprintf(stderr, "    %s uvc.1\n", argv0);
	fprintf(stderr, "    %s g1/functions/uvc.1\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "    %s musb-hdrc.0.auto\n", argv0);
}

/* Necessary for and only used by signal handler. */
static struct events *sigint_events;

static void sigint_handler(int signal __attribute__((unused)))
{
	/* Stop the main loop when the user presses CTRL-C */
	events_stop(sigint_events);
}

int main(int argc, char *argv[])
{
	char *function = NULL;
#ifdef HAVE_LIBCAMERA
	char *camera = NULL;
	struct camera_controls camera_control_opts = {
		.af_range_mode = NULL,
		.af_speed_mode = NULL,
		.awb_mode = NULL,
		.exposure_mode = NULL,
	};
#endif
	char *cap_device = NULL;
	char *img_path = NULL;
	char *slideshow_dir = NULL;

	struct uvc_function_config *fc;
	struct uvc_stream *stream = NULL;
	struct video_source *src = NULL;
	struct events events;
	int ret = 0;
	int opt;
	int option_index = 0;

	#define OPT_AF_RANGE 1000
	#define OPT_AF_SPEED 1001
	#define OPT_AWB_MODE 1002
	#define OPT_EXP_MODE 1003
	struct option long_options[] = {
#ifdef HAVE_LIBCAMERA
		{ "camera",          required_argument, 0, 'c' },
		{ "autofocus-range", required_argument, 0, OPT_AF_RANGE },
		{ "autofocus-speed", required_argument, 0, OPT_AF_SPEED },
		{ "awb",             required_argument, 0, OPT_AWB_MODE },
		{ "exposure",        required_argument, 0, OPT_EXP_MODE },
#endif
		{ "device",          required_argument, 0, 'd' },
		{ "image",           required_argument, 0, 'i' },
		{ "slideshow",       required_argument, 0, 's' },
		{ "help",            no_argument,       0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = getopt_long(argc, argv, "c:d:i:s:h", long_options, &option_index)) != -1) {
		switch (opt) {
#ifdef HAVE_LIBCAMERA
		case 'c':
			camera = optarg;
			break;
		case OPT_AF_RANGE:
			if (!is_camera_mode_valid(optarg, camera_valid_af_range_modes)) {
				fprintf(stderr, "Invalid --autofocus-range value: %s\n", optarg);
				usage(argv[0]);
				return 1;
			}
			camera_control_opts.af_range_mode = optarg;
			break;
		case OPT_AF_SPEED:
			if (!is_camera_mode_valid(optarg, camera_valid_af_speed_modes)) {
				fprintf(stderr, "Invalid --autofocus-speed value: %s\n", optarg);
				usage(argv[0]);
				return 1;
			}
			camera_control_opts.af_speed_mode = optarg;
			break;
		case OPT_AWB_MODE:
			if (!is_camera_mode_valid(optarg, camera_valid_awb_modes)) {
				fprintf(stderr, "Invalid --awb value: %s\n", optarg);
				usage(argv[0]);
				return 1;
			}
			camera_control_opts.awb_mode = optarg;
			break;
		case OPT_EXP_MODE:
			if (!is_camera_mode_valid(optarg, camera_valid_exposure_modes)) {
				fprintf(stderr, "Invalid --exposure value: %s\n", optarg);
				usage(argv[0]);
				return 1;
			}
			camera_control_opts.exposure_mode = optarg;
			break;
#endif
		case 'd':
			cap_device = optarg;
			break;

		case 'i':
			img_path = optarg;
			break;

		case 's':
			slideshow_dir = optarg;
			break;

		case 'h':
			usage(argv[0]);
			return 0;

		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argv[optind] != NULL)
		function = argv[optind];

	fc = configfs_parse_uvc_function(function);
	if (!fc) {
		printf("Failed to identify function configuration\n");
		return 1;
	}

	if (cap_device != NULL && img_path != NULL) {
		printf("Both capture device and still image specified\n");
		printf("Please specify only one\n");
		return 1;
	}

	/*
	 * Create the events handler. Register a signal handler for SIGINT,
	 * received when the user presses CTRL-C. This will allow the main loop
	 * to be interrupted, and resources to be freed cleanly.
	 */
	events_init(&events);

	sigint_events = &events;
	signal(SIGINT, sigint_handler);

	/* Create and initialize a video source. */
	if (cap_device)
		src = v4l2_video_source_create(cap_device);
#ifdef HAVE_LIBCAMERA
	else if (camera) {
		src = libcamera_source_create(camera);
		libcamera_source_set_controls(src, &camera_control_opts);
	}
#endif
	else if (img_path)
		src = jpg_video_source_create(img_path);
	else if (slideshow_dir)
		src = slideshow_video_source_create(slideshow_dir);
	else
		src = test_video_source_create();
	if (src == NULL) {
		ret = 1;
		goto done;
	}

	if (cap_device)
		v4l2_video_source_init(src, &events);

#ifdef HAVE_LIBCAMERA
	if (camera)
		libcamera_source_init(src, &events);
#endif

	/* Create and initialise the stream. */
	stream = uvc_stream_new(fc->video);
	if (stream == NULL) {
		ret = 1;
		goto done;
	}

	uvc_stream_set_event_handler(stream, &events);
	uvc_stream_set_video_source(stream, src);
	uvc_stream_init_uvc(stream, fc);

	/* Main capture loop */
	events_loop(&events);

done:
	/* Cleanup */
	uvc_stream_delete(stream);
	video_source_destroy(src);
	events_cleanup(&events);
	configfs_free_uvc_function(fc);

	return ret;
}
