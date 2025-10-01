#include "TextRenderer.hpp"

// Define global text renderer to be created by game instance
text_renderer_obj *glob_txt_rndr = nullptr;

void text_renderer_obj::init_renderer() noexcept
{
	// Setup text buffers to store instanced vertices
	m_text_vao = ogl::new_vao();
	m_text_quad_vbo = ogl::new_buf(GL_ARRAY_BUFFER);

	// Default instanced data for a text quad
	struct text_quad_vertices {
		text_quad_vertices(float _x, float _y) noexcept : x(_x), y(_y), x_ind(static_cast<uint32_t>(x)) {};
		float x, y;
		uint32_t x_ind; 
	} const quad_verts[4] = {
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f }
	};

	glBufferStorage(GL_ARRAY_BUFFER, sizeof quad_verts, quad_verts, 0);

	// Shader buffer attributes
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Data stride and offset for non-instanced attributes above (0, 1) 
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float[2]) + sizeof(uint32_t), nullptr);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(float[2]) + sizeof(uint32_t), reinterpret_cast<const void*>(sizeof(float[2])));

	// Instanced attributes for individual character data
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);

	// Update text shader uniform values with texture positions for each character
	upd_shader_uniform_vals();
}

void text_renderer_obj::upd_shader_uniform_vals() noexcept
{
	// Calculate size of a pixel on the font texture
	const float pixel_w = 1.0f / static_cast<float>(game.shaders.textures.text.width);

	// 95 unique characters (2 each for start and end coordinate) + end of image value
	// ** Shader uniform size should match --> Text.vert **
	const int num_texture_chars = sizeof game.ascii_char_sizes;
	const int pos_data_count = (num_texture_chars * 2) + 1;

	float text_uniform_pos[pos_data_count]; // Start and end positions of characters in the image
	
	// Each character is separated by a pixel on either side in the image texture (= 2 pixels between characters)
	float curr_img_offset = pixel_w; // Current coordinate of character (1 pixel gap to first character)
	for (int i = 0, n = 0; i < num_texture_chars; ++i) {
		const float char_sz = static_cast<float>(game.ascii_char_sizes[i]); // Get character size as decimal value
		text_uniform_pos[n++] = curr_img_offset; // Set start position of character texture
		text_uniform_pos[n++] = curr_img_offset + (pixel_w * char_sz); // Set end position of character texture
		curr_img_offset += pixel_w * (char_sz + 2.0f); // Advance to next character (each separated by 2 pixels)
	}
	
	text_uniform_pos[pos_data_count - 1] = 1.0f; // Texture end coordinate

	// Get location of positions array in shader and set uniform value
	m_textpos_uniform_loc = game.shaders.programs.text.get_loc("TP");
	glUniform1fv(m_textpos_uniform_loc, pos_data_count, text_uniform_pos);
}


void text_renderer_obj::draw_all_texts(bool inv_is_open) const noexcept
{
	// Render text if it should be visible
	int count = 0;
	for (const auto text_object : m_text_objects) render_single(text_object, !count++, inv_is_open);
}

void text_renderer_obj::render_single(text_obj *text_object, bool change_shader, bool inv_is_open) const noexcept
{
	if (text_object->has_settings(ts_dirty)) text_object->upd_text_buf(); // Update if still needs
	if (change_shader) game.shaders.programs.text.bind_and_use(m_text_vao); // Setup shader if needed

	
	// Check for visibility
	if (text_object->has_settings(ts_in_inv) != inv_is_open) return; // Inventory visibility check
	if (!game.display_debug_text && text_object->has_settings(ts_is_debug)) return; // Debug visibility check
	if (!text_object->is_visible() || (text_object->get_visible_chars() == 0 && !text_object->has_settings(ts_bg))) return; // General size check

	glBindBuffer(GL_ARRAY_BUFFER, text_object->get_vbo()); // Bind current text object's VBO

	// Set attribute layouts
	constexpr GLsizei stride = sizeof(float[4]) + sizeof(uint32_t[2]);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glVertexAttribIPointer(3, 2, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(sizeof(float[4])));

	if (game.is_wireframe_view) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // No wireframe for text
	
	// Render text object (4 vertices for a quad, using buffered instances count)
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, text_object->get_data_count());

	if (game.is_wireframe_view) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Set the wireframe mode to original
}

void text_renderer_obj::mark_all_textobjs() noexcept
{
	// Force recalculation of all text objects on the next update call below
	for (const auto text_object : m_text_objects) text_object->add_settings(ts_dirty);
}

void text_renderer_obj::update_marked_texts() noexcept
{
	// Update any texts determined to need... updating.
	game.perfs.text_update.start_timer();
	glBindVertexArray(m_text_vao);
	for (const auto text_object : m_text_objects) text_object->upd_text_buf();
	game.perfs.text_update.end_timer();
}


void text_renderer_obj::init_text_obj(
	text_obj *to_init,
	vector2f pos, 
	std::string text, 
	uint8_t settings,
	float font_size,
	bool is_visible
) noexcept {
	// Add to vector for destructor cleanup
	m_text_objects.emplace_back(to_init);

	// Set all values of given text object
	to_init->init_vbo();
	to_init->set_pos(pos);
	to_init->set_text(text);
	to_init->set_font_sz(font_size);
	to_init->add_settings(settings);
	to_init->set_visibility(is_visible);
}

void text_renderer_obj::init_text_obj(text_obj *to_init, const text_obj *original)
{
	// Create a text object using another for values
	init_text_obj(
		to_init,
		original->get_pos(),
		original->get_text(),
		original->get_settings(),
		original->get_font_sz(),
		original->get_transparency() > 0.0f
	);
}

text_renderer_obj::~text_renderer_obj()
{
	// Delete the renderer's VAO + VBO
	glDeleteBuffers(1, &m_text_quad_vbo);
	glDeleteVertexArrays(1, &m_text_vao);
}

// -------------------------------- text_obj --------------------------------
typedef text_renderer_obj::text_obj tr_tx_obj;

void tr_tx_obj::init_vbo() { if (!m_vbo) m_vbo = ogl::new_buf(GL_ARRAY_BUFFER); }
void tr_tx_obj::mark_needs_update() noexcept { add_settings(ts_dirty); }

void tr_tx_obj::set_text(const std::string &new_text) noexcept
{
	m_text = new_text;
	if (m_text.back() == '\n') m_text.pop_back();
	update_visible_chars_count();
	mark_needs_update();
}
void tr_tx_obj::set_pos(const vector2f &new_pos) noexcept
{
	m_position = new_pos;
	mark_needs_update();
}
void tr_tx_obj::set_font_sz(float new_font_sz) noexcept
{
	m_font_size = new_font_sz;
	mark_needs_update();
}
void tr_tx_obj::set_ex_line_spacing(float new_ex_line_spc) noexcept
{
	m_extra_line_spacing = new_ex_line_spc;
	mark_needs_update();
}
void tr_tx_obj::set_ex_char_spacing(float new_ex_char_spc) noexcept
{
	m_extra_char_spacing = new_ex_char_spc;
	mark_needs_update();
}

int tr_tx_obj::get_data_count() const noexcept
{
	return has_settings(ts_bg) + (get_visible_chars() * (has_settings(ts_shadow) + 1));
}
void tr_tx_obj::update_visible_chars_count() noexcept
{
	// Get the number of displayable characters
	m_displayed_chars = 0;
	const char *cstr_txt = m_text.data();
	for (size_t i = 0, sz = m_text.size(); i < sz; ++i) {
		char c = *cstr_txt++;
		m_displayed_chars += c > ' ' && c <= '~';
	}
}

float tr_tx_obj::get_transparency() const noexcept {
	return math::clamp(get_noclamp_transparency(), 0.0f, 1.0f);
}
float tr_tx_obj::get_noclamp_transparency() const noexcept {
	return static_cast<float>(m_hide_time - game.global_time) / m_fade_secs;
}

bool tr_tx_obj::is_visible() const noexcept { return m_displayed_transparency > 0.0f; }
void tr_tx_obj::set_visibility(bool visible) noexcept
{
	m_hide_time = visible ? math::lims<double>::max() : math::lims<double>::lowest();
	mark_needs_update();
}
void tr_tx_obj::fade_after_time(double seconds) noexcept
{
	m_hide_time = glfwGetTime() + seconds;
	mark_needs_update();
}

float tr_tx_obj::get_sz_mult() const noexcept { return m_font_size / default_font_size; }
float tr_tx_obj::get_char_spacing() const noexcept { return (::glob_txt_rndr->text_width + m_extra_char_spacing) * get_sz_mult(); }
float tr_tx_obj::get_line_spacing() const noexcept { return (::glob_txt_rndr->text_height + m_extra_line_spacing) * get_sz_mult(); }
float tr_tx_obj::get_spc_sz() const noexcept { return get_char_spacing() * 4.0f; }

float tr_tx_obj::get_char_displ_height() const noexcept { return ::glob_txt_rndr->text_height * get_sz_mult(); }
float tr_tx_obj::get_char_displ_width(int char_ind) const noexcept {
	return static_cast<float>(game.ascii_char_sizes[char_ind]) * ::glob_txt_rndr->text_width * get_sz_mult();
}

float tr_tx_obj::get_width() const noexcept
{
	// Precalculate widths to use
	const float char_spacing = get_char_spacing(), space_size = get_spc_sz();
	float curr_line_width = 0.0f; // Current width of line
	float record_line_width = 0.0f; // Largest recorded width

	const auto check_char = [&](int c) {
		switch (c) {
		case ' ':
			// Space character - just add width of a space
			curr_line_width += space_size;
			break;
		case '\n':
			// New line - Check if current line was largest and reset line width counter
			record_line_width = math::max(record_line_width, curr_line_width);
			curr_line_width = 0.0f;
			break;
		default:
			// Any other character - add the size of the character as well as character spacing
			curr_line_width += get_char_displ_width(c - 33) + char_spacing;
			break;
		}
	};

	// Calculate width of each line to determine the maximum width of the given string
	const char *text_ptr = m_text.data();
	while (*text_ptr) check_char(static_cast<int>(*text_ptr++));
	check_char('\n'); // Record last line's width as well

	// Return width of largest line
	return record_line_width;
}
float tr_tx_obj::get_height() const noexcept
{
	// Multiply line height (from font size and set line spacing) with number of lines
	return get_sz_mult() * (::glob_txt_rndr->text_height + m_extra_line_spacing) *
	       static_cast<float>(formatter::count_char(m_text, '\n') + 1); // Add 1 to include last line
}

void tr_tx_obj::set_position_y_relativeto(const tr_tx_obj *rel_to, float offset) noexcept
{
	m_position = { m_position.x, (rel_to->get_pos().y - rel_to->get_height()) - offset };
}
void tr_tx_obj::set_position_x_relativeto(const tr_tx_obj *rel_to, float offset) noexcept
{
	m_position = { (rel_to->get_pos().x + rel_to->get_width()) + offset, m_position.y };
}

void tr_tx_obj::upd_text_buf() noexcept
{
	// Determine if an update is required
	if (has_settings(ts_is_debug) && !game.display_debug_text) return;

	bool was_dirty = has_settings(ts_dirty);
	const float noclamp_trnsp = get_noclamp_transparency();
	if (!was_dirty && !math::in_range_incl(noclamp_trnsp, 0.0f, 1.0f)) return;

	m_displayed_transparency = math::clamp(noclamp_trnsp, 0.0f, 1.0f); // Set new transparency level to display
	
	// Bits arrangement: AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR
	// Initially starts out as white.
	uint32_t colour_int_val = 0xFFFFFF + (static_cast<int>(255.0f * m_displayed_transparency) << 24);

	// Check settings
	const int has_bg = has_settings(ts_bg);
	const bool has_shadow = has_settings(ts_shadow);

	const vector2f initial_pos = get_pos(); // Beginning position for new lines and background
	vector2f curr_pos = initial_pos; // Current position for text data calculation
	vector2f max_pos = initial_pos; // Bottom-right corner (for background, determined during update loop)

	// Text data struct (vec4 + uvec2)
	struct buf_char_inst {
		float x, y, w, h;
		uint32_t char_ind, col;
	} *text_buf_data = new buf_char_inst[get_data_count()];

	// Static values
	const float height = get_char_displ_height();
	const float line_spc = get_line_spacing();
	const float spc_sz = get_spc_sz();
	const float char_spc = get_char_spacing();
	const float texture_rel_height = height / static_cast<float>(game.shaders.textures.text.height);

	// TODO: Custom colour from hex format: e.g. "abc#000000123" - 'abc' white, '123' black
	//       ('##' should display just a '#', using full hex format)
	//       Will also need to update 'data count' accordingly.

	int data_ind = has_bg; // The background uses the first index if enabled
	const char *text_ptr = m_text.data();
	while (*text_ptr) {
		const int char_val = static_cast<int>(*text_ptr++);
		// For a new line character, reset the X position offset and set the Y position lower
		if (char_val == '\n') {
			curr_pos.y -= line_spc;
			curr_pos.x = initial_pos.x;
			max_pos.x = math::max(curr_pos.x, max_pos.x);
			max_pos.y = math::min(curr_pos.y, max_pos.y);
			continue;
		}
		// For a space, just increase the X position offset by an amount 
		if (char_val == ' ') {
			curr_pos.x += spc_sz;
			max_pos.x = math::max(curr_pos.x, max_pos.x);
			continue;
		}
		
		// Only characters starting after 'space' (ASCII 32) are displayed
		const int char_ind = char_val - 33;

		// Get display width of character
		const float char_width = get_char_displ_width(char_ind);

		// Create new character object with current values
		const buf_char_inst char_data {
			curr_pos.x, curr_pos.y, char_width, height,
			static_cast<uint32_t>(char_ind) << 1, colour_int_val
		};

		// Create a black and slightly offset copy for shadow text rendered underneath/before actual character
		if (has_shadow) {
			const uint32_t black_colour_int_val = colour_int_val & 0xFF000000;
			const buf_char_inst shadow_char_data = {
				curr_pos.x + (::glob_txt_rndr->text_width * 0.5f), 
				curr_pos.y - texture_rel_height, char_width, height,
				char_data.char_ind, black_colour_int_val
			};
			// Add shadow character to array
			text_buf_data[data_ind++] = shadow_char_data;
		}

		// Add character object to array
		text_buf_data[data_ind++] = char_data;
		
		// Advance in X axis by the character width and letter spacing
		curr_pos.x += char_width + char_spc;
		max_pos.x = math::max(curr_pos.x, max_pos.x);
	}

	// Add background if it was enabled - use initial and ending position to determine size
	// The background texture is made up of an entire white column in the texture so it appears as a box
	if (has_bg) {
		// Default background colour
		constexpr uint32_t bg_char = static_cast<uint32_t>((static_cast<int>(sizeof game.ascii_char_sizes) - 1) * 2);
		const uint32_t colour = static_cast<uint32_t>(127.0f * m_displayed_transparency);
		const uint32_t bg_rgba = colour + (colour << 8) + (colour << 16) + (colour << 24);

		const bool is_full_x = has_settings(ts_bg_full_width);
		const bool is_full_y = has_settings(ts_bg_full_height);
		
		// Start of background lines up exactly with text, increase width a bit for better appearance
		const float start_x = is_full_x ? -1.0f : initial_pos.x - 0.005f;
		const float start_y = is_full_y ? -1.0f : initial_pos.y;

		// Background X and Y sizes
		const float sz_x = is_full_x ? 2.0f : (max_pos.x - start_x) + 0.005f;
		const float sz_y = is_full_y ? 2.0f : (initial_pos.y - max_pos.y) + line_spc;

		// Place at first index so all text renders on top of background
		*text_buf_data = {
			start_x, start_y, sz_x, sz_y,
			bg_char, bg_rgba
		};
	}
	
	// Buffer calculated text data to the current text's array buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(buf_char_inst) * data_ind, text_buf_data, GL_DYNAMIC_DRAW);
	delete[] text_buf_data; // Free memory allocated by text data

	// If transparency is still being applied, do not remove the 'needs update' flag
	if (!math::in_range_excl(noclamp_trnsp, 0.0f, 1.0f)) remove_settings(ts_dirty); 
}

tr_tx_obj::~text_obj() noexcept { glDeleteBuffers(1, &m_vbo); }
