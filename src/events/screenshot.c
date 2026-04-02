#include "events/screenshot.h"
#include "events/chat.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"

#include "values/state.h"

#include "utils/png.h"

#include "io/format.h"
#include "io/files.h"
#include "io/logs.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VX_RELATIVE_SCREENSHOT_DIR "screenshots/"

/* Create a directory for the screenshots folder and returns a useable filename. */
static char *vxshot_create_directory(VX_NO_ARG)
{
	char *dir_path = vxfmt_text("%s" VX_RELATIVE_SCREENSHOT_DIR, vxfile_exec_dir);
	if (!dir_path) return VX_NULL;

	int io_result = 1;
	if (!vxfile_directory_exists(dir_path)) {
		if (vxfile_file_exists(dir_path)) vxfile_remove_file(dir_path);
		io_result = vxfile_create_directory(dir_path);
	}

	VX_FREE(dir_path);
	if (!io_result) return VX_NULL;

	const time_t cur_time = time(NULL);
	const struct tm ct = *localtime(&cur_time);
	return vxfmt_text(
		"%s" VX_RELATIVE_SCREENSHOT_DIR "Screenshot %02d-%02d-%d %02d-%02d-%02d.png",
		vxfile_exec_dir, ct.tm_mday, ct.tm_mon + 1, ct.tm_year + 1900, ct.tm_hour, ct.tm_min, ct.tm_sec
	);
}

void vxshot_take_screenshot(VX_NO_ARG)
{
	uint8_t *pixels = VX_NULL;
	char *full_path;
	int success = 0;
	
	if (!(full_path = vxshot_create_directory())) goto screenshot_fail_exit;
	if (!(pixels = VX_CAST(uint8_t *, malloc(VX_CAST(size_t, vxstate_vals.window_width * vxstate_vals.window_height) * 3u)))) goto screenshot_fail_exit;

	gl.ReadBuffer(GL_FRONT);
	gl.ReadPixels(0, 0, vxstate_vals.window_width, vxstate_vals.window_height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (intmax_t x = 0; x < vxstate_vals.window_width; ++x) {
		for (intmax_t y = 0; y < vxstate_vals.window_height / 2; ++y) {
			#define VX_SWAP(n, a, b) const uint8_t tmp_##n = a; a = b; b = tmp_##n
			const intmax_t base_flip_ind = (x + y * vxstate_vals.window_width) * 3;
			const intmax_t base_data_ind = (x + (vxstate_vals.window_height - 1 - y) * vxstate_vals.window_width) * 3;
			VX_SWAP(r, pixels[base_flip_ind + 0], pixels[base_data_ind + 0]);
			VX_SWAP(g, pixels[base_flip_ind + 1], pixels[base_data_ind + 1]);
			VX_SWAP(b, pixels[base_flip_ind + 2], pixels[base_data_ind + 2]);
		}
	}

	success = vxpng_save(full_path, pixels, VX_CAST(uint32_t, vxstate_vals.window_width), VX_CAST(uint32_t, vxstate_vals.window_height));
	VX_FREE(pixels);
	
screenshot_fail_exit:
	if (success) {
		char *text = vxfmt_text("Saved as '%s'", full_path);
		vxlog_msg(VX_LOG_DEFAULT_BIT, text);
		vxchat_add_text(full_path);
		VX_FREE(text);
	} else {
		const char fail_text[] = "Failed to save screenshot.";
		vxlog_msg(VX_LOG_WARNING_BIT, fail_text);
		vxchat_add_text(fail_text);
	}

	VX_FREE(full_path);
}
