/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *   Damien Lespiau <damien.lespiau@intel.com>
 */

/*
 * Overview:
 *
 * For each pipe:
 *   - We start by taking a ref CRC of a 50% green fb blended on black
 *   - For each plane supporting the blend_func property:
 *   	- put a black fb on the primary plane
 *   	- put a 50% fb on a sprite plane
 *   	- take the CRC and compare it to the refrence CRC
 *
 * Now there are different ways to express 50% green sprite plane:
 *   - non pre-multipled alpha (0, 255, 0, 127)
 *   - pre-multipled alpha (0, 127, 0, 127)
 *   - full green with 50% plane alpha (0, 255, 0, 255) + alpha 50%
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "intel_chipset.h"
#include "drmtest.h"
#include "igt_core.h"
#include "igt_kms.h"
#include "igt_debugfs.h"

IGT_TEST_DESCRIPTION("Test we correctly blend ARGB8888 fbs.");

enum alpha_method {
	ALPHA_METHOD_NON_PREMULTIPLIED	= 1 << 0,
	ALPHA_METHOD_PREMULTIPLIED	= 1 << 1,
	ALPHA_METHOD_PLANE		= 1 << 2,
};

typedef struct {
	int drm_fd;
	igt_display_t display;
	igt_pipe_crc_t *pipe_crc;
	igt_crc_t reference_crc;
} test_t;

typedef union {
	uint32_t packed;
	struct {
		uint8_t alpha;
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	} unpacked;
} color_t;

#define COLOR_ALPHA(c)	(c->unpacked.alpha)
#define COLOR_RED(c)	(c->unpacked.red)
#define COLOR_GREEN(c)	(c->unpacked.green)
#define COLOR_BLUE(c)	(c->unpacked.blue)

static const color_t green_full = { .unpacked = { 255, 0, 255, 0 } };
static const color_t green_50_pre = { .unpacked = { 127, 0, 127, 0 } };
static const color_t green_50_non_pre = { .unpacked = { 127, 0, 255, 0 } };

static const char *alpha_method_str(enum alpha_method method)
{
	switch (method) {
	case ALPHA_METHOD_NON_PREMULTIPLIED:
		return "unpremultiplied";
	case ALPHA_METHOD_PREMULTIPLIED:
		return "premultiplied";
	case ALPHA_METHOD_PLANE:
		return "plane-alpha";
	}

	assert(0);
}

static void create_test_fb(test_t *test, drmModeModeInfo *mode, uint32_t format,
			   const color_t *color, struct igt_fb *fb /* out */)
{
	unsigned int fb_id;
	uint32_t *pixels;
	int i;

	fb_id = igt_create_fb(test->drm_fd, mode->hdisplay, mode->vdisplay,
			      DRM_FORMAT_ARGB8888, I915_TILING_NONE, fb);
	igt_assert(fb_id);

	pixels = gem_mmap(test->drm_fd, fb->gem_handle, fb->size,
			  PROT_READ | PROT_WRITE);


	for (i = 0; i < fb->size / 4; i++)
		pixels[i] = color->packed;
}

static void
test_grab_crc(test_t *test, igt_output_t *output, enum pipe pipe,
	      const color_t *fb_color, igt_crc_t *crc /* out */)
{
	struct igt_fb fb;
	drmModeModeInfo *mode;
	igt_plane_t *primary;
	char *crc_str;

	igt_output_set_pipe(output, pipe);

	primary = igt_output_get_plane(output, 0);

	mode = igt_output_get_mode(output);
	create_test_fb(test, mode, DRM_FORMAT_ARGB8888, fb_color, &fb);
	igt_plane_set_fb(primary, &fb);

	igt_display_commit(&test->display);

	igt_pipe_crc_collect_crc(test->pipe_crc, crc);

	igt_plane_set_fb(primary, NULL);
	igt_display_commit(&test->display);

	igt_remove_fb(test->drm_fd, &fb);

	crc_str = igt_crc_to_string(crc);
	igt_debug("CRC for a (%d,%d,%d,%d) fb: %s\n", COLOR_ALPHA(fb_color),
		  COLOR_RED(fb_color), COLOR_GREEN(fb_color),
		  COLOR_BLUE(fb_color), crc_str);
	free(crc_str);
}

static void test_init(test_t *test, enum pipe pipe, igt_output_t *output)
{
	test->pipe_crc = igt_pipe_crc_new(pipe, INTEL_PIPE_CRC_SOURCE_AUTO);
	test_grab_crc(test, output, pipe, &green_50_pre, &test->reference_crc);
	igt_output_set_pipe(output, pipe);
}

static void test_fini(test_t *test, enum pipe pipe, igt_output_t *output)
{
	igt_output_set_pipe(output, PIPE_ANY);
	igt_pipe_crc_free(test->pipe_crc);
}

static void
test_blend_with_output(test_t *test,
		       enum pipe pipe,
		       enum igt_plane plane_num,
		       igt_output_t *output,
		       unsigned int flags)
{
	igt_plane_t *plane;
	struct igt_fb fb;
	drmModeModeInfo *mode;
	const color_t *test_color = NULL;
	uint64_t blend_func, blend_color;
	igt_crc_t crc;

	igt_info("Testing connector %s using pipe %s plane %d\n",
		 igt_output_name(output), kmstest_pipe_name(pipe), plane_num);

	test_init(test, pipe, output);

	mode = igt_output_get_mode(output);
	plane = igt_output_get_plane(output, plane_num);

	blend_color = DRM_MODE_COLOR(0xffff, 0xffff, 0xffff, 0xffff);

	if (flags & ALPHA_METHOD_NON_PREMULTIPLIED) {
		test_color = &green_50_non_pre;
		blend_func = DRM_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
	} else if (flags & ALPHA_METHOD_PREMULTIPLIED) {
		test_color = &green_50_pre;
		blend_func = DRM_BLEND_FUNC(ONE, ONE_MINUS_SRC_ALPHA);
	} else if (flags & ALPHA_METHOD_PLANE) {
		test_color = &green_full;
		blend_func = DRM_BLEND_FUNC(CONSTANT_ALPHA,
					    ONE_MINUS_CONSTANT_ALPHA);
		blend_color = DRM_MODE_COLOR(0x7fff, 0xffff, 0xffff, 0xffff);
	}
	assert(test_color);

	create_test_fb(test, mode, DRM_FORMAT_ARGB8888, test_color, &fb);
	igt_plane_set_fb(plane, &fb);
	igt_plane_set_blend_func(plane, blend_func);
	igt_plane_set_blend_color(plane, blend_color);
	igt_display_commit(&test->display);

	igt_pipe_crc_collect_crc(test->pipe_crc, &crc);
	igt_assert_crc_equal(&crc, &test->reference_crc);

	igt_plane_set_fb(plane, NULL);
	igt_remove_fb(test->drm_fd, &fb);

	test_fini(test, pipe, output);
}

static void
test_blend(test_t *test, enum pipe pipe, enum igt_plane plane_num,
	   unsigned int flags)
{
	igt_output_t *output;

	igt_skip_on(pipe >= test->display.n_pipes);
	igt_skip_on(plane_num >= test->display.pipes[pipe].n_planes);

	for_each_connected_output(&test->display, output) {
		igt_plane_t *plane;

		plane = igt_output_get_plane(output, plane_num);

		igt_skip_on(!igt_plane_supports_blend_func(plane));
		igt_skip_on(!igt_plane_supports_blend_color(plane));

		test_blend_with_output(test, pipe, plane_num, output, flags);
	}
}

static void
run_tests_for_pipe_plane(test_t *test, enum pipe pipe, enum igt_plane plane)
{
	static const enum alpha_method methods[] = {
		ALPHA_METHOD_NON_PREMULTIPLIED,
		ALPHA_METHOD_PREMULTIPLIED,
		ALPHA_METHOD_PLANE,
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(methods); i++) {
		const enum alpha_method method = methods[i];

		igt_subtest_f("blend-%s-plane-%d-%s",
			      kmstest_pipe_name(pipe), plane,
			      alpha_method_str(method))
			test_blend(test, pipe, plane, method);
	}
}

static void run_tests_for_pipe(test_t *test, enum pipe pipe)
{
	int plane;

	for (plane = 0; plane < IGT_MAX_PLANES; plane++)
		run_tests_for_pipe_plane(test, pipe, plane);
}

static int plane_set_property(test_t *test, igt_plane_t *plane,
			       uint32_t prop_id, uint64_t value)
{
	return drmModeObjectSetProperty(test->drm_fd,
					plane->drm_plane->plane_id,
					DRM_MODE_OBJECT_PLANE, prop_id, value);
}

static int plane_get_property(test_t *test, igt_plane_t *plane,
			      const char *name, uint64_t *value /* out */)
{
	bool found;

	found = kmstest_get_property(test->drm_fd, plane->drm_plane->plane_id,
				     DRM_MODE_OBJECT_PLANE, name, NULL, value,
				     NULL);

	return found ? 0 : -EINVAL;
}

static struct {
	struct igt_fb fb;
} interface_data;

static void setup_display_for_interface_tests(test_t *test, igt_plane_t **plane)
{
	igt_output_t *output;

	/*
	 * Light up the first output. We only need the first overlay plane as
	 * it supports the property we want to test.
	 */
	for_each_connected_output(&test->display, output) {
		drmModeModeInfo *mode;

		mode = igt_output_get_mode(output);
		igt_create_color_fb(test->drm_fd, mode->hdisplay,
				    mode->vdisplay, DRM_FORMAT_XRGB8888,
				    LOCAL_DRM_FORMAT_MOD_NONE, 1., 1., 1.,
				    &interface_data.fb);

		*plane = igt_output_get_plane(output, IGT_PLANE_PRIMARY);
		igt_plane_set_fb(*plane, &interface_data.fb);

		*plane = igt_output_get_plane(output, IGT_PLANE_2);
		igt_plane_set_fb(*plane, &interface_data.fb);

		igt_skip_on(!igt_plane_supports_blend_func(*plane));
		igt_skip_on(!igt_plane_supports_blend_color(*plane));

		igt_display_commit(&test->display);

		break;
	}

}

static void cleanup_display_for_interface_tests(test_t *test)

{
	igt_remove_fb(test->drm_fd, &interface_data.fb);
}

static void run_interface_tests(test_t *test)
{
	igt_plane_t *plane = NULL;
	uint64_t color;
	int ret;

	/*
	 * We need to tup bring up pipe and plane or kernel will skip the
	 * checks :/. The "try" mode of the atomic ioctl could work there, but
	 * no libdrm support just yet.
	 */
	setup_display_for_interface_tests(test, &plane);
	igt_skip_on(plane == NULL);

	/* reserved bits */
	ret = plane_set_property(test, plane, plane->blend_func_property,
				 0xffff000000000000ULL);
	igt_assert_eq(ret, -EINVAL);

	/* should discard anything with AUTO that is not AUTO,AUTO */
	ret = plane_set_property(test, plane, plane->blend_func_property,
				 DRM_BLEND_FUNC(AUTO, SRC_ALPHA));
	igt_assert_eq(ret, -EINVAL);

	ret = plane_set_property(test, plane, plane->blend_func_property,
				 DRM_BLEND_FUNC(SRC_ALPHA, AUTO));
	igt_assert_eq(ret, -EINVAL);

	/* non supported alpha blending */
	ret = plane_set_property(test, plane, plane->blend_func_property,
				 DRM_BLEND_FUNC(SRC_ALPHA, SRC_ALPHA));
	igt_assert_eq(ret, -EINVAL);

	/* this one should work though! */
	ret = plane_set_property(test, plane, plane->blend_func_property,
				 DRM_BLEND_FUNC(AUTO, AUTO));
	igt_assert_eq(ret, 0);

	/* verify we can write a constant color */
	ret = plane_set_property(test, plane, plane->blend_color_property,
				 DRM_MODE_COLOR(0x1234, 0x5678,
						0x9abc, 0xdef0));
	igt_assert_eq(ret, 0);

	ret = plane_get_property(test, plane, "blend_color", &color);
	igt_assert_eq(ret, 0);
	igt_assert_eq(DRM_MODE_COLOR_ALPHA(color), 0x1234);
	igt_assert_eq(DRM_MODE_COLOR_RED(color), 0x5678);
	igt_assert_eq(DRM_MODE_COLOR_GREEN(color), 0x9abc);
	igt_assert_eq(DRM_MODE_COLOR_BLUE(color), 0xdef0);

	/* reset color to default state to not have bad surprises */
	ret = plane_set_property(test, plane, plane->blend_color_property,
				 DRM_MODE_COLOR(0xffff, 0xffff,
						0xffff, 0xffff));
	igt_assert_eq(ret, 0);

	cleanup_display_for_interface_tests(test);
}

static test_t test;

igt_main
{
	igt_fixture {
		test.drm_fd = drm_open_any_master();

		kmstest_set_vt_graphics_mode();

		igt_require_pipe_crc();
		igt_display_init(&test.display, test.drm_fd);
	}

	igt_subtest("blend-interface-tests")
		run_interface_tests(&test);

	for (int pipe = 0; pipe < I915_MAX_PIPES; pipe++)
		run_tests_for_pipe(&test, pipe);

	igt_fixture {
		igt_display_fini(&test.display);
	}
}
