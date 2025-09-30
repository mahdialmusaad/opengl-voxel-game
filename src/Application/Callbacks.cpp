#include "Game.hpp"

game_callbacks::game_callbacks(main_game_obj *app_ptr) : m_app(app_ptr)
{
	// Set window event callbacks
	callbacks_wrappers::cb_wr = this;
	glfwSetFramebufferSizeCallback(game.window_ptr, callbacks_wrappers::window_resize_cb);
	glfwSetWindowCloseCallback(game.window_ptr, callbacks_wrappers::window_close_cb);
	glfwSetMouseButtonCallback(game.window_ptr, callbacks_wrappers::mouse_event_cb);
	glfwSetWindowPosCallback(game.window_ptr, callbacks_wrappers::window_move_cb);
	glfwSetCursorPosCallback(game.window_ptr, callbacks_wrappers::mouse_move_cb);
	glfwSetScrollCallback(game.window_ptr, callbacks_wrappers::scroll_event_cb);
	glfwSetCharCallback(game.window_ptr, callbacks_wrappers::char_event_cb);
	glfwSetKeyCallback(game.window_ptr, callbacks_wrappers::key_event_cb);
}

// The window is already defined in the global, so no need for the first argument
void game_callbacks::key_event(int key, int, int action, int)
{
	// Adding text to chat is handled by char_event_cb
	if (game.chatting) {
		if (action == GLFW_RELEASE) {
			game.glob_keyboard_state.erase(GLFW_KEY_SLASH);
			return;
		}

		const auto exit_chat = [&](bool do_fading) {
			m_app->m_command_text.set_text(""); // Empty player input
			m_app->m_command_text.set_visibility(false); // Hide command text
			// Change chat visibility
			if (do_fading) m_app->m_chat_txt.fade_after_time();
			else m_app->m_chat_txt.set_visibility(false);
			m_chat_history_ind = -1; // Reset chat history selector
			game.chatting = false;
		};

		// See if the player want to confirm chat message/command rather than adding more text to it
		if (key == GLFW_KEY_ENTER) {
			apply_cmd_text(); // Either add player message or execute command
			exit_chat(m_app->m_chat_txt.get_text().size()); // Temporarily visible if chat has text
		}
		// Exit chat if escape is pressed - hide chat instantly
		else if (key == GLFW_KEY_ESCAPE) {
			exit_chat(false);
			return; // Avoid triggering 'ESC' in input function to exit the game
		}
		// Chat message editing is done by below function but it does not include/detect backspace
		else if (key == GLFW_KEY_BACKSPACE) {
			std::string chat_text = m_app->m_command_text.get_text();
			if (chat_text.size()) { // Check if there is any text present
				chat_text.erase(chat_text.end() - 1); // Remove last char
				m_app->m_command_text.set_text(chat_text);
			}
		}
		// Set current message to the previous one if there is one
		else if (key == GLFW_KEY_UP && m_previous_chats.size() > static_cast<size_t>(m_chat_history_ind) + 1) {
			m_app->m_command_text.set_text(m_previous_chats[static_cast<size_t>(++m_chat_history_ind)]);
		}
		// Set current message to the next one if the selector is more than -1 (default text if going to present)
		else if (key == GLFW_KEY_DOWN && m_chat_history_ind > -1) {
			const std::string new_cmd_text = 
				--m_chat_history_ind == -1 ? 
				(game.is_normal_chat ? "/" : "") :
				m_previous_chats[static_cast<size_t>(m_chat_history_ind)];
			m_app->m_command_text.set_text(new_cmd_text);
		}
		// TODO: Handle text pasting
		//else if (game.is_ctrl_held && key == GLFW_KEY_V) add_cmd_text(glfwGetClipboardString(game.window_ptr));
	}

	// Determine if the input has a linked function and execute if so 
	apply_key_event(key, action);
}

void game_callbacks::char_event(unsigned codepoint)
{
	// Ignore unwanted initial input
	if (game.ignore_initial_chat_char) {
		game.ignore_initial_chat_char = false;
		return;
	}

	// Add inputted char into chat message
	if (game.chatting) {
		const char character = static_cast<char>(codepoint);
		//if (game.is_ctrl_held && (character == 'v' || character == 'V')) return; // CTRL+V is handled elsewhere
		add_cmd_text(std::string(1, character));
	}
}

void game_callbacks::mouse_event(int button, int action, int) noexcept
{
	if (game.chatting || m_app->m_player_inst.player.inventory_open) return;
	if (action == GLFW_PRESS) { // (for now) LMB = block breaking, RMB = block placing.
		if (button == GLFW_MOUSE_BUTTON_LEFT) m_app->m_player_inst.break_selected();
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) m_app->m_player_inst.place_selected();
	}
}

void game_callbacks::scroll_event(double, double yoffset) noexcept
{
	if (game.chatting) return;
	m_app->m_player_inst.upd_inv_scroll_relative(static_cast<float>(yoffset)); // Scroll through inventory
}

void game_callbacks::window_resize(int width, int height) noexcept
{
	// No need to update or recalculate if the game has just been minimized (dims = 0),
	// lower the FPS instead to decrease unecessary usage
	if (width + height <= 0) {
		game.is_window_iconified = true;
		glfwSwapInterval(game.screen_refresh_rate);
		return;
	} else if (game.is_window_iconified) { // Exiting out of minimized mode
		game.is_window_iconified = false;
		glfwSwapInterval(game.is_synced_fps); // Set to default game frame update state
	}

	// Update graphics viewport and settings
	glViewport(0, 0, width, height);

	game.window_width = width;
	game.window_height = height;

	// Scale GUI to accomodate the new window size
	m_app->aspect_changed();
}

void game_callbacks::mouse_move(double xpos, double ypos) noexcept
{
	if (game.chatting) return;

	// Avoid large jumps in camera direction when window gains focus
	if (game.window_focus_changed) {
		game.rel_mouse_xpos = xpos;
		game.rel_mouse_ypos = ypos;
		game.window_focus_changed = false;
	}
	else {
		// Move player camera
		m_app->m_player_inst.update_cam_dir_vars(xpos - game.rel_mouse_xpos, game.rel_mouse_ypos - ypos);
		game.rel_mouse_xpos = xpos; game.rel_mouse_ypos = ypos;
	}
}

void game_callbacks::window_move(int x, int y) noexcept
{
	game.window_xpos = x;
	game.window_ypos = y;
}

void game_callbacks::window_close() noexcept { game.is_loop_active = false; }

void game_callbacks::take_screenshot() noexcept
{
	static const std::string rel_screenshot_path = "Screenshots";
	bool file_error = false;

	// Ensure screenshot folder is valid by either creating if non-existent or replacing invalid folder
	try {
		// Create if there is no folder
		if (!file_sys::check_dir_exists(rel_screenshot_path)) file_sys::create_path(rel_screenshot_path);
		// Ensure it is a folder, not a file
		else if (file_sys::check_file_exists(rel_screenshot_path)) {
			file_sys::delete_path(rel_screenshot_path);
			file_sys::create_path(rel_screenshot_path);
		}
	} catch (const file_sys::file_err &err) { file_error = true; }

	static const std::string screenshot_fail_text = "Failed to save screenshot";

	if (file_error) {
		add_chat_text(screenshot_fail_text);
		m_app->m_chat_txt.fade_after_time();
		return;
	}

	const size_t main_width = static_cast<size_t>(game.window_width);
	const size_t main_height = static_cast<size_t>(game.window_height);
	const size_t main_pixels_bytes = 3 * main_width * main_height; // 3 bytes per pixel for RGB values
	unsigned char *pixels = new unsigned char[main_pixels_bytes];

	// Read pixels from screen to vector (must be on main thread)
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, game.window_width, game.window_height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// Complete rest of screenshot work on a separate thread to continue normal game logic
	game.taking_screenshot = true;
	std::thread([&](unsigned char *org_pixels, const vector2sz &dims, size_t total_bytes) {
		// File name for displaying and creating directory path
		const int cur_msecs = formatter::get_cur_msecs();
		const tm ct = formatter::get_cur_time();
		const std::string screenshot_filename = formatter::fmt(
			"Screenshot %02d-%02d-%d %02d-%02d-%02d-%03d.png",
			ct.tm_mday, ct.tm_mon + 1, ct.tm_year + 1900,
			ct.tm_hour, ct.tm_min, ct.tm_sec,
			cur_msecs
		);
		// Get full path of image to save to
		const std::string rel_dir_path = formatter::fmt("%s/%s", rel_screenshot_path, screenshot_filename);

		// Flip Y axis as OGL uses opposite Y coordinates (bottom-top instead of top-bottom)
		// Can be done in parallel to speed up large images
		unsigned char *flipped_pixels = new unsigned char[total_bytes];
		thread_ops::split(game.available_threads, dims.x, [&](int, size_t x_ind, size_t x_end) {
			for (; x_ind < x_end; ++x_ind) {
				for (size_t y = 0; y < dims.y; ++y) {
					const size_t base_flip_ind = (x_ind + y * dims.x) * 3;
					const size_t base_data_ind = (x_ind + (dims.y - 1 - y) * dims.x) * 3;
					flipped_pixels[base_flip_ind    ] = org_pixels[base_data_ind    ];
					flipped_pixels[base_flip_ind + 1] = org_pixels[base_data_ind + 1];
					flipped_pixels[base_flip_ind + 2] = org_pixels[base_data_ind + 2];
				}
			}
		});
		delete[] org_pixels; // No longer need original array

		// Save to PNG file in the directory constructed previously
		const bool success = lodepng_encode24_file(
			rel_dir_path.c_str(),
			flipped_pixels,
			static_cast<unsigned>(game.window_width),
			static_cast<unsigned>(game.window_height)
		) == 0;
		delete[] flipped_pixels;

		// Wait for main thread to finish displaying previous messages or for an exit
		while (main_clear_screenshot_txt) {
			if (!game.is_loop_active) goto thread_screenshot_ending;
			thread_ops::wait_avg_frame();
		}

		// Set result text to be displayed
		if (success) {
			if (game.is_loop_active) screenshot_res_strs.emplace_back("Screenshot saved as " + screenshot_filename);
			formatter::log("Screenshot taken");
		}
		else {
			if (game.is_loop_active) screenshot_res_strs.emplace_back(screenshot_fail_text);
			formatter::log(screenshot_fail_text);
		}
	thread_screenshot_ending:
		game.taking_screenshot = false;
	}, pixels, vector2sz(main_width, main_height), main_pixels_bytes).detach();
}

void game_callbacks::toggle_full_inv_display() noexcept
{
	// Determine if inventory elements should be rendered
	invert(m_app->m_player_inst.player.inventory_open); 
	// Free cursor if inventory is shown
	glfwSetInputMode(game.window_ptr, GLFW_CURSOR,
		m_app->m_player_inst.player.inventory_open ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
	);
	// Set mouse position to center
	glfwSetCursorPos(game.window_ptr,
		static_cast<double>(game.window_width) / 2.0,
		static_cast<double>(game.window_height) / 2.0
	);

	game.window_focus_changed = true;
	game.show_any_gui = true;
}

void game_callbacks::toggle_fullscreen() noexcept
{
	// TODO: Make fullscreen NOT crash OS
	return;
	
	game.window_focus_changed = true;

	if (!invert(game.is_window_fullscreen)) {
		glfwSetWindowMonitor(game.window_ptr, NULL,
			game.default_window_xpos,
			game.default_window_ypos,
			game.default_window_width,
			game.default_window_height, 0
		);
		return;
	}

	// Set fullscreen mode
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(game.window_ptr, monitor, 0, 0, mode->width, mode->height, 0);
}

void game_callbacks::apply_key_event(int key, int action) noexcept
{
	// Select the corresponding inventory slot if a number is pressed (only if not chatting)
	if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
		if (!game.chatting) m_app->m_player_inst.upd_inv_selected(key - GLFW_KEY_1);
		return;
	}

	// Add or remove from input information list
	if (action == GLFW_RELEASE) {
		game.glob_keyboard_state.erase(key);
		game.any_key_active = game.glob_keyboard_state.size();
		return;
	}
	else {
		game.glob_keyboard_state.insert({ key, action });
		game.any_key_active = true;
	}
	
	// Determine modifier key status
	switch (key)
	{
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL:
		game.is_ctrl_held = action != GLFW_RELEASE;
		return;
	case GLFW_KEY_LEFT_ALT:
	case GLFW_KEY_RIGHT_ALT:
		game.is_alt_held = action != GLFW_RELEASE;
		return;
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_SHIFT:
		game.is_shift_held = action != GLFW_RELEASE;
		return;
	default:
		break;
	}

	// Input types
	constexpr int no_ctrl = 1 << 3, no_alt = 1 << 4, no_shift = 1 << 5;
	constexpr int bypass_chat = 1 << 6;

	constexpr int press_bit = GLFW_PRESS;
	constexpr int repeatable_bits = press_bit | GLFW_REPEAT;

	// Get limit/clamp values
	using namespace val_limits;

	world_acc_player &plr = m_app->m_player_inst.player;

	// Functions for 'value' inputs
	const auto add_render_dist = [&](int change) {
		m_app->m_world.update_render_distance(m_app->m_world.get_rnd_dist() + change);
	};
	const auto add_cam_fov = [&](double change) {
		plr.fov = math::clamp(plr.fov + change, math::radians(fov_limits.min), math::radians(fov_limits.max));
		m_app->m_player_inst.update_frustum_vals();
		plr.moved = true;
	};
	
	// Specific key callbacks
	struct input_event {
		int key, accept_bits;
		std::function<void()> action;
	} static const all_key_events[] = {
	
	// Default controls
	{ GLFW_KEY_SLASH, press_bit, [&]{ game.is_normal_chat = true; begin_chatting(); }},
	{ GLFW_KEY_T, press_bit, [&]{ game.is_normal_chat = false; game.ignore_initial_chat_char = true; begin_chatting(); }},
	{ GLFW_KEY_ESCAPE, press_bit, [&]{ window_close(); }},
	
	// Toggle inputs
	{ GLFW_KEY_X, press_bit, [&]{ glfwSwapInterval(invert(game.is_synced_fps)); }},
	{ GLFW_KEY_E, press_bit, [&]{ toggle_full_inv_display(); }},
	{ GLFW_KEY_F, press_bit, [&]{ invert(game.hide_world_fog); }},
	{ GLFW_KEY_C, press_bit, [&]{ invert(plr.gravity_effects); }},
	{ GLFW_KEY_N, press_bit, [&]{ invert(plr.ignore_collision); }},
	{ GLFW_KEY_V, press_bit, [&]{ invert(game.do_generate_signal); }},

	// Value inputs
	// Render distance
	{ GLFW_KEY_LEFT_BRACKET, press_bit, [&]{ add_render_dist(1); }},
	{ GLFW_KEY_RIGHT_BRACKET, press_bit, [&]{ add_render_dist(-1); }},
	// Speed
	{ GLFW_KEY_COMMA, repeatable_bits, [&]{ plr.base_speed += 1.0; }},
	{ GLFW_KEY_PERIOD, repeatable_bits, [&]{ plr.base_speed -= 1.0; }},
	// FOV
	{ GLFW_KEY_I, repeatable_bits, [&]{ add_cam_fov(math::radians( 1.0)); }},
	{ GLFW_KEY_O, repeatable_bits, [&]{ add_cam_fov(math::radians(-1.0)); }},

	// Debug inputs
	{ GLFW_KEY_Z, press_bit, [&]{ 
		glPolygonMode(GL_FRONT_AND_BACK, invert(game.is_wireframe_view) ? GL_LINE : GL_FILL);
	}},
	{ GLFW_KEY_J, press_bit, [&]{ invert(game.display_chunk_borders); }},

	// Function inputs
	{ GLFW_KEY_F1, press_bit, [&]{ invert(game.show_any_gui); }},
	{ GLFW_KEY_F2, press_bit | bypass_chat, [&]{ take_screenshot(); }},
	{ GLFW_KEY_F3, press_bit | bypass_chat, [&]{ 
		glfwSetInputMode(game.window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		game.window_focus_changed = true;
	}},
	{ GLFW_KEY_F4, press_bit | bypass_chat, [&]{ invert(game.display_debug_text); }},
	{ GLFW_KEY_F11, press_bit | bypass_chat, [&]{ toggle_fullscreen(); }},

	};

	// Check if any input struct uses the pressed key
	const input_event *event_end_ptr = all_key_events + math::size(all_key_events);
	const input_event *found = std::find_if(all_key_events, event_end_ptr, [&](const input_event &id) { return id.key == key; });

	if (found != event_end_ptr) {
		// Check if input action matches (or disallows certain modifier) and run the associated function if it is
		// Disallow modifiers CTRL, ALT and SHIFT
		if ((found->accept_bits & no_ctrl) && game.is_ctrl_held) return; 
		if ((found->accept_bits & no_alt) && game.is_alt_held) return;
		if ((found->accept_bits & no_shift) && game.is_shift_held) return;

		// Disallow whilst chatting unless bypass is specified
		if (!(found->accept_bits & bypass_chat) && game.chatting) return;

		if (action & found->accept_bits) found->action(); // Determine type of action
	}
}

void game_callbacks::add_chat_text(std::string message, bool auto_new_line) noexcept
{
	if (!message.size()) return;
	to_chat_fmt_text(message); // Format text for chat

	// Append formatted text to current chat - assume previous text is formatted already
	std::string cur_chat_text = m_app->m_chat_txt.get_text();
	if (auto_new_line && cur_chat_text.size()) cur_chat_text += '\n';
	cur_chat_text += message;

	// Remove previous lines if the whole text has too many lines
	const int lines_to_remove = formatter::count_char(cur_chat_text, '\n') - glob_txt_rndr->chat_lines_limit;
	if (lines_to_remove > 0) cur_chat_text = cur_chat_text.substr(formatter::get_nth_found(cur_chat_text, '\n', lines_to_remove) + 1);

	// Update text with new message
	m_app->m_chat_txt.set_text(cur_chat_text);
}

void game_callbacks::to_chat_fmt_text(std::string &text) const noexcept
{
	const size_t line_char_limit = glob_txt_rndr->chat_line_char_limit;
	size_t space_ind = 0;

	// Traverse through each word of the text, replacing spaces with new lines so each line
	// doesn't go beyond the line character limit. If a single word is larger than this limit,
	// split it into multiple lines so it fits within this limit.
	
	bool loop = true;
	while (loop) {
		size_t next_space_ind = text.find(' ', space_ind);
		loop = next_space_ind != std::string::npos;
		if (!loop) next_space_ind = text.size();

		if (next_space_ind - space_ind > line_char_limit) {
			space_ind += line_char_limit;
			for (; space_ind < next_space_ind; space_ind += line_char_limit) text.insert(space_ind, 1, '\n');
		}
		else if (next_space_ind - text.find_last_of('\n', next_space_ind) > line_char_limit) text.insert(space_ind, 1, '\n');

		space_ind = ++next_space_ind;
	}
}

void game_callbacks::begin_chatting() noexcept
{
	game.chatting = true;
	game.show_any_gui = true;
	m_app->m_player_inst.player.inventory_open = false;

	// Make command text and chat log visible (if chat log has text)
	m_app->m_command_text.set_visibility(true);
	m_app->m_chat_txt.set_visibility(m_app->m_chat_txt.get_text().size());
}

void game_callbacks::add_cmd_text(std::string new_txt) noexcept {
	m_app->m_command_text.set_text(m_app->m_command_text.get_text() + new_txt);
}

void game_callbacks::apply_cmd_text()
{	
	// Command struct
	struct command_handler_obj {
		typedef std::vector<int> int_vec_t;
		command_handler_obj(
			const std::string &name,
			const std::string &args_names_txt,
			const std::string &action_description,
			const std::function<void()> &action_func,
			const std::function<void()> &value_query_func = nullptr,
			const int_vec_t acceptable_any_num_args = { -2 }
		) noexcept :
		   cmd_base_name(name),
		   description(action_description),
		   arguments_names_list(args_names_txt),
		   action_function(action_func),
		   value_query_function(value_query_func)
		{
			// Determine number of arguments from arguments list:
			// Empty with query function: 1 argument
			// Empty without query function: -1 arguments (no argument checks)
			// All normal (e.g. 'x y z'): Number of spaces + 1 -> 2 + 1 = 3
			// Normal + optional (e.g. 'a b c *d *e *f'):
			//   Same as above but also includes another with optional argument(s) included -> 3, 4, 5, 6

			// If the 'all_acceptable_num_args' argument is not the default (-2), use the given value instead
			if (acceptable_any_num_args.size() && acceptable_any_num_args[0] != -2) {
				all_acceptable_num_args = acceptable_any_num_args;
				return;
			}

			if (!args_names_txt.size()) {
				all_acceptable_num_args = { value_query_function ? 1 : -1 };
			} else {
				const int arg_indesc_count = formatter::count_char(args_names_txt, ' ') + 1;
				const int arg_optionals_count = formatter::count_char(args_names_txt, '*');
				if (arg_optionals_count) {
					for (int i = arg_indesc_count - arg_optionals_count; i <= arg_indesc_count; ++i) {
						all_acceptable_num_args.emplace_back(i);
					}
				} else all_acceptable_num_args = int_vec_t{ arg_indesc_count };
			}
		}

		const std::string cmd_base_name, description, arguments_names_list;
		
		const std::function<void()> action_function;
		const std::function<void()> value_query_function;

		std::vector<int> all_acceptable_num_args;
	};

	const char *default_query_txt = "*value";

	const auto display_help_section = [&](const command_handler_obj *all_cmds_ptr, size_t size, int page) {
		// Instead of storing the entire command text, it can be constructed at run-time
		// to avoid taking up too much memory passively. The unique command syntax information
		// text below still needs to be stored, however.

		// Page 1 is the first page - advance through help text depending on text lines limit and page
		static const std::string base_info_txt =
			"Commands start with the character '/' and are formatted as such: '/name arg1 arg2'...\n"
			"An optional argument is marked with an asterik: '/name arg1 *arg2'.\n"
			"Entering some commands without any arguments will return the current value(s), e.g: "
			"'/time' returns the current in-game time in seconds. "
			"A tilde (~) in some arguments is treated as the current value: /tp 0 ~ 0 -> '~' is replaced with your Y position. "
			"Numbers and negation can be used: /tp 0 -~5 0 -> tilde expression becomes -(Ypos + 5).\n";

		std::string display_txt = base_info_txt;
		page = math::max(--page, 0); // Page 1 is the start, decrement for zero-indexing

		// Add descriptions and arguments of all commands
		while (size--) {
			const command_handler_obj *cmd = &all_cmds_ptr[size];
			if (cmd->description[0] == '_') continue; // Ignore debug commands
			const std::string &args_txt = cmd->arguments_names_list.size() ?
			      cmd->arguments_names_list :
			      (cmd->value_query_function == nullptr ? "(None)" : default_query_txt);
			display_txt += formatter::fmt(
				"\n/%s %s -- Arguments: %s",
				cmd->cmd_base_name.c_str(),
				cmd->description.c_str(),
				args_txt.c_str()
			);
		}
		
		// Determine section to show depending on the page using chat-formatted version of the help text
		to_chat_fmt_text(display_txt);
		const size_t start = formatter::get_nth_found(display_txt, '\n', page * glob_txt_rndr->chat_lines_limit);
		const size_t end = formatter::get_nth_found(display_txt, '\n', (page + 1) * glob_txt_rndr->chat_lines_limit);
		display_txt = display_txt.substr(start, end - start) + " ";
		
		add_chat_text(display_txt, false); // Add help message to chat
	};

	// Shortcut variables
	world_obj &world = m_app->m_world;
	world_acc_player &plr = m_app->m_player_inst.player;
	const vector3d &plr_pos = plr.position;
	const world_pos plr_block_pos = chunk_vals::to_block_pos(&plr_pos);
	player_inst &plr_funcs_obj = m_app->m_player_inst;

	// Determine if a query should display a message
	m_query_chat = true;
	bool no_query_conv = false;

	command_handler_obj *all_cmds_ptr;
	size_t all_cmds_count;
	
	using namespace val_limits;
	const auto IArg = [&](int i){ return int_arg<pos_t>(i); };

	static command_handler_obj cmds_list[] = {
	// Function commands

	{ "tp",
		"x y z *yaw *pitch",
		"Teleports you to the specified x, y, z position. Optionally also sets the camera's direction", 
	[&]{
		conv_cmd({ tcv(0, plr_pos.x), tcv(1, plr_pos.y), tcv(2, plr_pos.z), tcv(3, plr.yaw), tcv(4, plr.pitch) });
		const vector3d pos = { dbl_arg(0), dbl_arg(1), dbl_arg(2) };
		plr_funcs_obj.set_new_position(pos);
		if (has_arg(3)) {
			plr.yaw = dbl_arg(3);
			if (has_arg(4)) plr.pitch = dbl_arg(4);
			plr_funcs_obj.update_cam_dir_vars();
		}
	}, [&]{
		query_mult("position is", plr_pos);
		query_mult("camera direction is", vector2d(plr.yaw, plr.pitch), 3);
		no_query_conv = true;
	}},
	{ "fill",
		"xFrom yFrom zFrom xTo yTo zTo block",
		"Fills from the 'from' position to the 'to' position with the specified block ID",
	[&]{
		conv_cmd({
			tcv(0, plr_block_pos.x), tcv(1, plr_block_pos.y), tcv(2, plr_block_pos.z),
			tcv(3, plr_block_pos.x), tcv(4, plr_block_pos.y), tcv(5, plr_block_pos.z) 
		});
		const uintmax_t changed = world.fill_blocks(
			{ IArg(0), IArg(1), IArg(2) },
			{ IArg(3), IArg(4), IArg(5) },
			int_arg<block_id_t, block_id>(6)
		);
		if (changed) add_chat_text(formatter::fmt(
				"Too many blocks to change (%ju > 32768). Try filling a smaller area.",
				changed
			));
	}},
	{ "set",
		"x y z block",
		"Sets the given position to the given block.",
	[&]{
		const world_pos set_block_pos = { int_arg<pos_t>(0), int_arg<pos_t>(1), int_arg<pos_t>(2) };
		world.set_block_at_to(&set_block_pos, int_arg<block_id_t, block_id>(3));
	}},
	{ "rr",
		"intervals",
		"Sets the number of screen updates to wait before swapping buffers.",
	[&]{
		glfwSwapInterval(int_arg<int>(0));
	}},
	{ "dcmp",
		"id",
		"_Creates a new file on the same directory with the ASM code for a given shader program ID",
	[&]{
		std::vector<char> binary(65535);
		GLenum format = 0; GLsizei length = 0;
		glGetProgramBinary(int_arg<GLuint>(0), 65535, &length, &format, binary.data());
		std::ofstream("shaderbinary.txt").write(binary.data(), length); 
	}, [&]{
		no_query_conv = true;
		if (!m_query_chat) return;
		std::string result = "Programs: ";
		const shaders_obj::shader_prog *const progs_ptr = reinterpret_cast<shaders_obj::shader_prog*>(&game.shaders.programs);
		constexpr size_t progs_count = sizeof game.shaders.programs / sizeof *progs_ptr;
		for (size_t i = 0; i < progs_count; ++i) result += formatter::fmt("'%s': %u, ", progs_ptr[i].base_name.c_str(), progs_ptr[i].program);
		add_chat_text(result.substr(0, result.size() - 2)); // Remove trailing space and comma
	}},

	// Set/query commands

	{ "time", 
		"",
		"Changes the in-game time (in seconds)",
	[&]{ m_app->manage_game_time(true, dbl_arg(0)); },
	[&]{ query("time", m_app->manage_game_time(false)); }
	},
	{ "speed", "", "Changes movement speed",
		[&]{ plr.base_speed = dbl_arg(0); }, [&]{ query("speed", plr.active_speed); }
	},
	{ "tick", "", formatter::fmt("Changes the tick speed (how fast natural events occur) [%.1f, %.1f]", tick_limits.min, tick_limits.max),
		[&]{ game.game_tick_speed = dbl_arg(0, -100.0, 100.0); }, [&]{ query("tick", game.game_tick_speed); }
	},
	{ "sens", "", "Sets the in-game mouse sensitivity.",
		[&]{ game.rel_mouse_sens = dbl_arg(0); }, [&]{ query("sensitivity", game.rel_mouse_sens); }
	},
	{ "rd",
		"",
		formatter::fmt("Changes the world's render distance [%d, %d]", render_limits.min, render_limits.max),
	[&]{ world.update_render_distance(int_arg<int32_t>(0, render_limits.min, render_limits.max)); },
	[&]{ query("render distance", m_app->m_world.get_rnd_dist()); }
	},
	{ "fov",
		"",
		formatter::fmt("Sets the camera FOV. [%.1f, %.1f]", fov_limits.min, fov_limits.max),
	[&]{
		plr.fov = math::radians(dbl_arg(0, fov_limits.min, fov_limits.max));
		m_app->m_player_inst.update_frustum_vals();
		plr.moved = true;
	},
	[&]{ query("FOV", math::degrees(plr.fov)); }
	},
	{ "test",
		"*x *y *z *w",
		"_Sets 4 values for run-time testing", 
	[&]{ for (int i=0;i<4;++i) if (has_arg(i)) game.runtime_test_vec[i] = dbl_arg(i); },
	[&]{ query_mult("debug values are", game.runtime_test_vec); }
	},

	// No argument commands

	{ "clear",
		"",
		"Clears the chat.",
	[&]{ m_app->m_chat_txt.set_text(""); }
	},
	{ "trytxt",
		"",
		"_Used to test the text renderer.",
	[&]{ std::string msg; for (char x = '!'; x <= '~'; ++x) { msg += x; } add_chat_text(msg); }
	},
	{ "exit",
		"",
		"Exits the game.",
	[&]{ window_close(); }
	},

	// Other commands

	{ "help",
		"*page",
		"Prints out some helpful text depending on the given page.",
	[&]{ display_help_section(all_cmds_ptr, all_cmds_count, has_arg(0) ? int_arg<int>(0) : 0); }
	},
	};

	all_cmds_ptr = cmds_list;
	all_cmds_count = math::size(cmds_list);
	command_handler_obj* found;

	{
		// Check if there is any text in the first place
		std::string cur_usr_cmd = m_app->m_command_text.get_text();
		if (!cur_usr_cmd.size()) return;
	
		// Add current chat input to history - 50 max for now
		m_previous_chats.insert(m_previous_chats.begin(), cur_usr_cmd);
		if (m_previous_chats.size() > 50) m_previous_chats.pop_back();
	
		// Determine if it is a chat message or command
		if (cur_usr_cmd[0] != '/') {
			add_chat_text("[YOU] " + cur_usr_cmd);
			return;
		}
		
		const std::string cmd_name = cur_usr_cmd.substr(1, cur_usr_cmd.find(' ') - 1); // Get command name
		const size_t arg_ind = cur_usr_cmd.find(' ');

		// Remove 'name' section and trailing spaces
		cur_usr_cmd = cur_usr_cmd.substr(
			(arg_ind == std::string::npos ? cmd_name.size() : arg_ind) + 1,
			cur_usr_cmd.find_last_not_of(' ')
		); 
		// Search command list to see if the given command name matches
		found = std::find_if(
			cmds_list,
			cmds_list + all_cmds_count,
			[&](const command_handler_obj &c) { return c.cmd_base_name == cmd_name; }
		);
		// Warn for invalid commands
		if (found == cmds_list + all_cmds_count) {
			add_chat_text(formatter::fmt("/%s is not a valid command. Type /help for help.", cmd_name.c_str())); 
			return;
		}

		m_cmd_args.clear(); // Clear arguments list

		// Get each argument from the command given using spaces
		if (cur_usr_cmd.size()) formatter::split_str(m_cmd_args, cur_usr_cmd, ' ');
		
		// Remove any invalid/empty arguments
		m_cmd_args.erase(std::remove_if(
			m_cmd_args.begin(),
			m_cmd_args.end(),
			[&](const std::string &v) { return v.find_first_not_of(' ') == std::string::npos; }
		), m_cmd_args.end());
	}

	const int num_cmd_args = static_cast<int>(m_cmd_args.size());

	// Attempt to execute command
	try {
		// Check for value query
		bool is_specifc_help = num_cmd_args == 1 && m_cmd_args[0] == "?";
		bool cmd_has_query = found->value_query_function != nullptr;
		bool should_do_query = !is_specifc_help && cmd_has_query;

		// Print out current value(s)
		if (should_do_query && num_cmd_args == 0) {
			found->value_query_function();
			return;
		}

		// Check for correct arguments amount (-1 means ignore check)
		if (is_specifc_help || (
			found->all_acceptable_num_args[0] != -1 &&
			std::find(
				found->all_acceptable_num_args.begin(),
				found->all_acceptable_num_args.end(),
				num_cmd_args
			) == found->all_acceptable_num_args.end())
		) {
			add_chat_text(formatter::fmt("Usage: /%s %s", found->cmd_base_name.c_str(),
				cmd_has_query && !found->arguments_names_list.size() ?
					default_query_txt :
					found->arguments_names_list.c_str()
			));
			return;
		}
		
		m_query_chat = false; // Using query to convert rather than display value(s)
		if (cmd_has_query) found->value_query_function();
		if (cmd_has_query && !no_query_conv) {
			/*
			   Use query string to convert specific arguments:

			   Example: { i, "12.3", true } is returned, only one conversion is made,
			   using 'i' as the starting argument index.
			   
			   If { 2, "12.3 64.5 7.2", true } is returned,
			   3 conversions are done (3 numbers) starting from the 2nd argument index.
			*/
			int i = cvd.index;
			std::vector<std::string> each_conv;
			formatter::split_str(each_conv, cvd.str_arg, ' ');
			for (const std::string &str : each_conv) conv_cmd({ str, i++, cvd.decimal });
		}
		found->action_function();
	}
	// Warn about found invalid argument if any
	catch (const std::invalid_argument&) {
		add_chat_text(formatter::fmt(
			"Unexpected '%s' at command argument %d",
			get_arg(m_current_cmd_index).c_str(),
			m_current_cmd_index + 1)
		);
	}
	// Warn about converting extremely large numbers
	catch (const std::out_of_range&) {
		add_chat_text(formatter::fmt(
			"Argument %d (%.5s...) is too large",
			m_current_cmd_index + 1,
			get_arg(m_current_cmd_index))
		);
	}
}

bool game_callbacks::has_arg(int index) { return static_cast<int>(m_cmd_args.size()) > index; }

std::string &game_callbacks::get_arg(int index)
{
	m_current_cmd_index = index;
	return m_cmd_args[static_cast<size_t>(index)];
}

void game_callbacks::conv_cmd(const std::vector<tilde_conversion_data> &args_convs) {
	for (const auto &val : args_convs) conv_cmd(val);
}
void game_callbacks::conv_cmd(const tilde_conversion_data &data)
{
	if (!has_arg(data.index)) return; // Check if argument is in range
	
	std::string &arg = get_arg(data.index); // Get the command argument
	const std::string &actual = data.str_arg; // Get the current value as a string
	if (!actual.size()) return;

	if (formatter::count_char(arg, '~') != 1) return; // Convert if only 1 tilde is found
	const size_t tilde_index = arg.find('~'); // Index of tilde
	if (tilde_index > 1) throw std::invalid_argument(""); // A tilde index of >1 is invalid

	std::string after_tilde_text = arg.substr(tilde_index + 1); // Text after tilde character
	if (!after_tilde_text.size()) after_tilde_text = "0"; // Default to "0" if empty
	const char before_tilde_char = tilde_index ? arg[0] : '\0'; // Character before tilde character

	// Check for a possible negative symbol before the tilde or for empty text
	const bool is_neg = before_tilde_char == '-';

	// No other non-zero character before the tilde is allowed
	if (!is_neg && static_cast<int>(before_tilde_char)) throw std::invalid_argument("");

	// Combine all parts of the argument into one number (add number after tilde, use negation if found)
	if (data.decimal) arg = std::to_string(
			(::strtod(actual.c_str(), nullptr) + ::strtod(after_tilde_text.c_str(), nullptr)) *
			(is_neg ? -1.0 : 1.0)
		);
	else arg = std::to_string(
			formatter::conv_to_imax(actual) +
			(formatter::conv_to_imax(after_tilde_text) *
			static_cast<intmax_t>(is_neg ? -1 : 1))
		);
}
