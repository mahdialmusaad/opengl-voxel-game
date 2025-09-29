#include "Game.hpp"

main_game_obj::main_game_obj() noexcept :
	events_handler(this),
	m_world(&m_player_inst.player)
{
	m_player_inst.world = &m_world;

	// Aspect ratio for GUI sizing and perspective matrix - do before text creation to use updated values
	aspect_changed();
	
	// Initialize global text renderer object
	glob_txt_rndr = &m_glob_text_rndr_obj;
	m_glob_text_rndr_obj.init_renderer(); 

	// Text objects creation
	const uint8_t bg_shadow = text_renderer_obj::ts_bg | text_renderer_obj::ts_shadow;
	const uint8_t bg_shadow_dbg = bg_shadow | text_renderer_obj::ts_is_debug;
	const float leftmost = -0.99f;

	// Combine static text for less draw calls and memory
	const std::string infofmt = fmt::format(
		"{}\n{}\nSeed: {}",
		glfwGetVersionString(),
		ogl::get_str(GL_RENDERER),
		m_world.world_noise_objs.elevation.seed
	);

	const unsigned def_font_size = text_renderer_obj::text_obj::default_font_size;
	m_glob_text_rndr_obj.init_text_obj(&m_static_info_txt, { leftmost, 0.99f }, infofmt, bg_shadow_dbg);

	// Normal chat text
	m_glob_text_rndr_obj.init_text_obj(&m_chat_txt, { leftmost, -0.15f }, "", bg_shadow, def_font_size, false);
	m_glob_text_rndr_obj.init_text_obj(&m_command_text, { leftmost, -0.74f }, "",
		bg_shadow | text_renderer_obj::ts_bg_full_width, def_font_size, false
	); // Command text (takes up full width)

	// Dynamic texts updated constantly to show game information (Y pos and text determined in game loop)
	m_glob_text_rndr_obj.init_text_obj(&m_game_info_txt, { leftmost, -1.0f }, "", bg_shadow_dbg);
	m_game_info_txt.set_position_y_relativeto(&m_static_info_txt); // Relative to static text object, already know width

	m_glob_text_rndr_obj.init_text_obj(&m_world_info_txt, { leftmost, -1.0f }, "", bg_shadow_dbg); // World information text
	m_glob_text_rndr_obj.init_text_obj(&m_perf_text, { leftmost, -1.0f }, "", bg_shadow_dbg); // Performance text

	m_player_inst.init_text(); // Initialize player text and other variables

	if (!game.is_first_open) return;
	game.is_first_open = false;

	formatter::log("Game inits complete (" +
		std::to_string((game.global_time - game.frame_delta_time) * 1000.0) + "ms static, " +
		std::to_string((glfwGetTime() - game.global_time) * 1000.0) + "ms constructor)"
	);
}

void main_game_obj::init_game(int argc, const char *const *const argv)
{	
	game.libraries_inited = true;
	game.frame_delta_time = glfwGetTime(); // Used to time this function
	
	// OGL rendering settings
	glEnable(GL_PROGRAM_POINT_SIZE); // Determine GL_POINTS size in shaders instead of from a global value
	glEnable(GL_DEPTH_TEST); // Enable depth testing so objects are rendered properly
	glDepthFunc(GL_LEQUAL); // Suitable depth testing function

	glEnable(GL_BLEND); // Transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Suitable blend function

	glEnable(GL_CULL_FACE); // Enable face culling
	glCullFace(GL_BACK); // The inside of objects are not seen normally so they can be ignored in rendering
	
	// Enable debug messages for OpenGL errors
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageCallback(callbacks_wrappers::gl_debug_out, nullptr);
	
	glfwSetWindowSizeLimits(game.window_ptr, 300, 200, GLFW_DONT_CARE, GLFW_DONT_CARE); // Enforce minimum window size
	glfwSwapInterval(1); // Swap buffers with screen's refresh rate
	
	// Determine hardware limits for compute shader work groups
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &game.max_wkgp_invocations);
	for (int axis = 0; axis < 3; ++axis) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, static_cast<GLuint>(axis), &game.max_wkgp_count[axis]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, static_cast<GLuint>(axis), &game.max_wkgp_size[axis]);
	}
	
	// Try to get executable's directory
	file_sys::get_exec_path(&game.current_exec_dir, *argv);
	if (!game.current_exec_dir.size()) game.current_exec_dir = *argv; // Use default if it was not changed
	file_sys::to_parent_dir(game.current_exec_dir);
	
	game.DB_exit_on_loaded = argc > 1; // Debug exit setting
	
	// Attempt to find resource folders - located in the same directory as exe
	game.resources_dir = game.current_exec_dir + "/Resources/";
	if (!file_sys::check_dir_exists(game.resources_dir)) formatter::custom_abort("Resources folder not found");

	// Get textures pointer and count from struct sizes
	constexpr size_t images_load_count =
		sizeof(shaders_obj::shader_texture_list) /
		sizeof(shaders_obj::shader_texture_list::texture_obj);
	shaders_obj::shader_texture_list::texture_obj *const textures_ptr = reinterpret_cast<
		shaders_obj::shader_texture_list::texture_obj*
	>(&game.shaders.textures);

	// Decode all textures with specified formats into a pixel array
	thread_ops::split(math::min(game.available_threads, static_cast<int>(images_load_count)), images_load_count,
	[&](int, size_t index, size_t end) {
		for (; index < end; ++index) {
			shaders_obj::shader_texture_list::texture_obj *const tex_data = textures_ptr + index;
			// Load in found image and get information data
			const std::string full_path = game.resources_dir + tex_data->filename + ".png";
			if (lodepng_decode_file(
				&tex_data->pixels, &tex_data->width, &tex_data->height,
				full_path.c_str(),
				tex_data->stored_col_fmt,
				tex_data->bit_depth
			)) throw file_sys::file_err(fmt::format("Could not find texture {}", full_path).c_str());
		}
	});

	// Create an OpenGL image texture object using pixel data and image information
	for (size_t i = 0; i < images_load_count; ++i) textures_ptr[i].add_ogl_img();

	game.shaders.init_shader_data(); // Init all shaders, UBOs and other related values

	game.shaders.sizes.vals.blocks = 16.0f / static_cast<float>(game.shaders.textures.blocks.width);
	game.shaders.sizes.vals.inventory = 1.0f / static_cast<float>(game.shaders.textures.text.width);
	game.shaders.sizes.vals.stars = 1.0f / static_cast<float>(skybox_elems_obj::stars_count);
	game.shaders.sizes.update_all();

	// Setup blocks shader uniform values
	const shaders_obj::shader_prog *const blocks_shader = &game.shaders.programs.blocks;
	blocks_shader->set_uint("ls", static_cast<GLuint>(chunk_vals::full_bits));
	blocks_shader->set_uint("s1", static_cast<GLuint>(chunk_vals::size_bits));
	blocks_shader->set_uint("s2", static_cast<GLuint>(chunk_vals::size_bits * 2));
	blocks_shader->set_uint("s3", static_cast<GLuint>(chunk_vals::size_bits * 3));

	chunk_vals::fill_lookup(); // Init lookup data
	game.global_time = glfwGetTime(); // Used for timing this init function and constructor
}

void main_game_obj::main_world_loop()
{
	double text_upd_time = 1.0; // Use for occasional game misc. updates
	double largest_frame_time = 0.0; // The largest amount of time between two frames
	int avg_fps_accumulated = 0; // Number of frames since last title update
	double prev_global_time = 0.0; // Previous executable time

	enum fps_counters_en { fc_current, fc_average, fc_lowest, fc_MAX };
	int fps_counters[fc_MAX];

	#if !defined(VOXEL_DEBUG)
	glfwSetInputMode(game.window_ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Capture mouse
	#endif

	game.is_loop_active = !game.DB_exit_on_loaded;
	if (game.DB_exit_on_loaded) formatter::log("Triggered debug game exit (extra args)");
	else formatter::log("Main loop enter");

	// Main game loop
	while (game.is_loop_active) {
		glfwPollEvents(); // Poll I/O such as key and mouse events

		// Time values
		game.global_time = glfwGetTime();
		game.frame_delta_time = game.global_time - prev_global_time;
		prev_global_time = game.global_time;

		game.ticked_delta_time = game.frame_delta_time * game.game_tick_speed;
		largest_frame_time = math::max(game.frame_delta_time, largest_frame_time);
		avg_fps_accumulated++;

		if ((game.cycle_day_seconds += game.ticked_delta_time) >= m_skybox.day_seconds) {
			game.cycle_day_seconds = 0.0;
			++game.world_day_counter;
		}

		m_player_inst.check_movement_input(); // Check for per-frame inputs
		m_player_inst.apply_velocity(); // Apply smoothed movement using velocity and run other position-related functions
		if (m_player_inst.player.moved) player_moved_update(); // Update matrices and frustum on position change
		m_world.generation_loop(true); // Possibly update chunks if results from threads are available

		update_frame_vals(); // Update shader UBO values (day/night cycle, sky colours)

		// Game rendering
		m_player_inst.draw_selected_outline();
		m_world.draw_entire_world();
		m_skybox.draw_skybox_elements();
		m_world.draw_enabled_borders();

		// Window title and other text update
		if ((text_upd_time += game.frame_delta_time) >= 0.05) {
			// Determine FPS values
			fps_counters[fc_average] = static_cast<int>(static_cast<double>(avg_fps_accumulated) / text_upd_time);
			fps_counters[fc_current] = static_cast<int>(1.0 / game.frame_delta_time);
			fps_counters[fc_lowest] = static_cast<int>(1.0 / largest_frame_time);

			upd_dynamic_text(fps_counters); // Update dynamic text values
 
			// Reset time values
			largest_frame_time = game.frame_delta_time;
			avg_fps_accumulated = 0;
			text_upd_time = 0.0f;
		}

		if (game.show_any_gui) {
			m_player_inst.draw_inventory_gui();
			m_glob_text_rndr_obj.update_marked_texts(); // Update any text object with changed properties
			m_glob_text_rndr_obj.draw_all_texts(m_player_inst.player.inventory_open);
		}

		glfwSwapBuffers(game.window_ptr); // Display new frame
	}

	game.is_active = false;
	game.is_loop_active = false;
	glfwSetInputMode(game.window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	
	if (!game.DB_exit_on_loaded) formatter::log("Game exit");
}

void main_game_obj::upd_dynamic_text(int *fps_counters) noexcept
{
	// Update window title
	const std::string fps_text = fmt::format("{} FPS | {} AVG | {} LOW", *fps_counters, fps_counters[1], fps_counters[2]);
	glfwSetWindowTitle(game.window_ptr, (game.def_title + "  -  " + fps_text).c_str());

	const world_acc_player &player = m_player_inst.player;
	const vector3d &vel = m_player_inst.get_current_velocity(); // Velocity shortcut

	static constexpr const char *const dir_compass_texts[6] = { "East", "West", "Up", "Down", "North", "South" };
	
	// Update dynamic information text
	static const std::string game_info_txt =
	"{}\n{} {} {} ({} {} {})\n"
	"Velocity: {:.2f} {:.2f} {:.2f}\n"
	"Yaw:{:.1f} Pitch:{:.1f} ({}, {})\n"
	"FOV:{:.1f} Speed:{:.1f}\n"
	"Flying: {} Noclip: {}";
	m_game_info_txt.set_text(fmt::format(game_info_txt, fps_text,
		formatter::group_flt(player.position.x), formatter::group_flt(player.position.y), formatter::group_flt(player.position.z),
		player.offset.x, player.offset.y, player.offset.z,
		vel.x, vel.y, vel.z,
		player.yaw, player.pitch,
		dir_compass_texts[player.look_dir_yaw], dir_compass_texts[player.look_dir_pitch],
		math::degrees(player.fov), player.active_speed, !player.gravity_effects, player.ignore_collision
	)); // Update first text info box
	
	// Determine if Y position for certain info boxes needs to be updated
	const bool do_text_y_update = m_world_info_txt.get_pos().y <= -1.0f;
	if (do_text_y_update) m_world_info_txt.set_position_y_relativeto(&m_game_info_txt);

	static const std::string world_info_txt =
	"Chunks: {} (Rendered: {})\n"
	"Triangles: {} (Rendered: {})\n"
	"Rnd.Dist: {} Generating: {} Ind.Calls: {}\n"
	"Time: {:.1f} (Cycle: {:.1f}, Day {})";

	m_world_info_txt.set_text(fmt::format(world_info_txt, 
		fmt::group_digits(m_world.rendered_map.size() * chunk_vals::y_count), fmt::group_digits(m_world.rendered_chunks_count),
		fmt::group_digits(m_world.existing_quads_count * 2), fmt::group_digits(m_world.rendered_squares_count * 2),
		m_world.get_rnd_dist(), game.do_generate_signal, fmt::group_digits(m_world.get_ind_calls()),
		game.global_time, game.cycle_day_seconds, game.world_day_counter
	)); // Update second text info box

	// Sort top 5 functions based on execution time
	typedef const voxel_global::perf_list::perf_obj *const perf_obj;
	constexpr size_t perfs_count = math::size(game.perf_leaderboard), perfs_last = perfs_count - 1;
	std::sort(game.perf_leaderboard, game.perf_leaderboard + perfs_count, [&](perf_obj a, perf_obj b) { return a->total > b->total; });
	std::string perf_leaderboard_txt;
	for (size_t i = 0; i < perfs_count; ++i) {
		perf_obj curr_perf = game.perf_leaderboard[i];
		perf_leaderboard_txt += fmt::format(
			"{}. {} {:.3f}s R:{}{}",
			i + 1, curr_perf->name, curr_perf->total, curr_perf->count, i == perfs_last ? "" : "\n"
		);
	}

	if (do_text_y_update) m_perf_text.set_position_y_relativeto(&m_world_info_txt);
	m_perf_text.set_text(perf_leaderboard_txt); // Update performance text

	// Check for any available screenshot text
	if (events_handler.screenshot_res_strs.size()) {
		for (const std::string &scrnst_res_txt : events_handler.screenshot_res_strs) events_handler.add_chat_text(scrnst_res_txt);
		m_chat_txt.fade_after_time();
		events_handler.main_clear_screenshot_txt = true;
		events_handler.screenshot_res_strs.clear();
		events_handler.main_clear_screenshot_txt = false;
	}
}

double main_game_obj::manage_game_time(bool is_changing_time, double t) noexcept
{
	if (is_changing_time) { // Change time to given seconds, updating day counter and cycle time.
		game.world_day_counter = static_cast<int64_t>(t / m_skybox.day_seconds);
		game.cycle_day_seconds = t - (static_cast<double>(game.world_day_counter) * m_skybox.day_seconds);
		return t;
	} else return game.cycle_day_seconds + (static_cast<double>(game.world_day_counter) * m_skybox.day_seconds); // Return world age
}

void main_game_obj::aspect_changed() noexcept
{
	// Calculate aspect ratio of new window size
	game.window_wh_aspect = static_cast<float>(game.window_width) / static_cast<float>(game.window_height);

	// Update text values
	m_glob_text_rndr_obj.text_width = text_renderer_obj::text_obj::default_text_size / game.window_wh_aspect;
	m_glob_text_rndr_obj.mark_all_textobjs();

	m_player_inst.update_inventory_buffers(); // Update player GUI

	if (m_static_info_txt.get_vbo()) { // Ensure text has been initialized first
		m_player_inst.update_item_text_pos();
		m_player_inst.update_selected_block_info_txt();
	} 

	m_player_inst.update_frustum_vals(); // Since the aspect changed, the frustum planes need to be recalculated.
	m_player_inst.player.moved = true; // Force a perspective matrix update, which uses the window aspect.
}

void main_game_obj::player_moved_update() noexcept
{
	// Calculate perspective and origin (no translation) matrices
	game.shaders.matrices.vals.origin = matrixf4x4::perspective(
		static_cast<float>(m_player_inst.player.fov),
		game.window_wh_aspect,
		static_cast<float>(m_player_inst.player.frustum.near_plane),
		static_cast<float>(m_player_inst.player.frustum.far_plane)
	) * m_player_inst.get_zero_matrix();
	game.shaders.matrices.update_specific(sizeof(matrixf4x4), sizeof(matrixf4x4)); // Update UBO matrix value

	m_world.sort_world_render(); // Sort the world buffers to determine what needs to be rendered
	m_player_inst.player.moved = false; // Use to check for next matrix and world buffer update
}

void main_game_obj::update_frame_vals() noexcept
{
	game.perfs.frame_cols.start_timer();
	const world_acc_player &player = m_player_inst.player;
	shaders_obj &shaders = game.shaders;

	// Update positions UBO
	shaders.positions.vals.player = player.position;
	const vector3d offset_glob_position = (player.offset * chunk_vals::size);
	shaders.positions.vals.offset = offset_glob_position;
	shaders.positions.vals.chunk = offset_glob_position - player.position;
	shaders.positions.vals.raycast = vector3d(player.selected_block_pos) - player.position;
	shaders.positions.update_all();

	// Triangle wave from 0 to 1, reaching 1 at halfway through an in-game day and returning back to 0:
	// 2 * abs( (x / t) - floor( (x / t) + 0.5 ) ) where x is the current time and t is the time for one full day
	const double time_div_day_secs = game.cycle_day_seconds / m_skybox.day_seconds;
	// 0 = midday, 1 = midnight
	const float rel_day_time = 2.0f * static_cast<float>(math::abs(time_div_day_secs - floor(time_div_day_secs + 0.5)));
	
	// Update star and sun/moon matrices
	const float rot_amnt = static_cast<float>(game.cycle_day_seconds / (m_skybox.day_seconds / math::two_pi<double>()));

	const matrixf4x4 &zero_mat = shaders.matrices.vals.origin;
	shaders.matrices.vals.stars = matrixf4x4::rotate(zero_mat, rot_amnt, { 0.7f, 0.5f, 0.2f });
	shaders.matrices.vals.planets = matrixf4x4::rotate(zero_mat, rot_amnt, { -1.0f, 0.0f, 0.0f });
	shaders.matrices.update_all();
	
	// Update time-dependent shader values
	shaders.times.vals.cycle = rel_day_time;
	constexpr float star_display_threshold = 0.4f;
	shaders.times.vals.stars = (rel_day_time - star_display_threshold) * (1.0f / (1.0f - star_display_threshold));
	shaders.times.vals.global = static_cast<float>(manage_game_time(false));
	shaders.times.vals.clouds = 1.1f - rel_day_time;
	// Fog variables
	const float render_dist = static_cast<float>(m_world.get_rnd_dist());
	shaders.times.vals.f_end =
		((math::max(render_dist, 1.0f) * chunk_vals::half) + (chunk_vals::half)) + 
		(rel_day_time * -chunk_vals::half) + // Variance over time - fog is heaviest during midnight
		(game.hide_world_fog * 1e20f); // Disable fog by making it extremely far away
	shaders.times.vals.f_range = 1.0f / render_dist; // Distance from clear to invisible (reciprocal for fast mul instead of slow div)
	shaders.times.update_all();

	constexpr vector3f sky_full_bright = vector3f(0.45f, 0.72f, 0.98f);
	constexpr vector3f sky_full_dark = vector3f(0.0f, 0.0f, 0.04f);
	const vector3f cur_sky_col = sky_full_bright.c_lerp(
		sky_full_dark,
		pow(rel_day_time, 2.0f) * (3.0f - 2.0f * rel_day_time)
	);
	
	// Clear background with the sky colour - other effects can be rendered on top
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(cur_sky_col.x, cur_sky_col.y, cur_sky_col.z, 1.0f);
	
	// Spikes to 1 around 0.5 progress (sunrise/sunset) but <0.0 otherwise (don't render)
	//game.twilight_colour_trnsp = (-400.0f * pow(rel_day_time - 0.5f, 2.0f)) + 1.0f;

	// Update colour vectors UBO
	constexpr vector3f sky_twilight_col = vector3f(0.788f, 0.533f, 0.278f);
	if (game.twilight_colour_trnsp > 0.0f) shaders.colours.vals.evening = cur_sky_col.c_lerp(sky_twilight_col, game.twilight_colour_trnsp);
	shaders.colours.vals.main = cur_sky_col;
	shaders.colours.vals.light = vector3f(1.1f - rel_day_time);
	shaders.colours.update_all();

	game.perfs.frame_cols.end_timer();
}

void main_game_obj::DB_func_perf() noexcept
{
	// Test function performance
	constexpr int_fast32_t times = 1000000;
	const double begin_time = glfwGetTime();
	const world_pos zero_pos{};
	for (int_fast32_t i = 0; i < times; ++i) m_world.find_chunk_at(&zero_pos);
	const double end_time = glfwGetTime(), func_total_time = end_time - begin_time;
	fmt::println(
		"*** {}s at {} loop(s) - {} funcs/s ***",
		func_total_time,
		fmt::group_digits(times),
		formatter::group_flt(times / func_total_time)
	);
}

void main_game_obj::DB_perlin_png() const noexcept
{
	// Test perlin noise results by creating an image
	const unsigned img_len = 1000;
	unsigned char *img_pixels = new unsigned char[img_len * img_len];
	const double factor = 0.01;

	float min = 100.0, max = -min;
	bool octave = true;

	for (unsigned i = 0; i < img_len; ++i) {
		for (unsigned j = 0; j < img_len; ++j) {
			const float res = octave ? m_world.world_noise_objs.elevation.octave(
				static_cast<double>(i) * factor,
				noise_def_vals::default_y_noise,
				static_cast<double>(j) * factor, 3
			) : m_world.world_noise_objs.elevation.noise(
				static_cast<double>(i) * factor,
				noise_def_vals::default_y_noise,
				static_cast<double>(j) * factor
			);
			const unsigned char pix = static_cast<unsigned char>(res * 255.0f);
			img_pixels[(i * img_len) + j] = 255 - pix;
			min = math::min(min, res); max = math::max(max, res);
		}
	}

	fmt::println("Min: {} Max: {}", min, max);
	lodepng_encode_file("perlin.png", img_pixels, img_len, img_len, LodePNGColorType::LCT_GREY, 8);
}
