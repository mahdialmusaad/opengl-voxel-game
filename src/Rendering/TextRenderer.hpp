#pragma once
#ifndef SOURCE_RENDERING_TEXTRENDERER_VXL_HDR
#define SOURCE_RENDERING_TEXTRENDERER_VXL_HDR

#include "Application/Definitions.hpp"

class text_renderer_obj
{
public:
	enum text_sets_en : uint8_t {
		ts_bg = 1,
		ts_bg_full_width = 2,
		ts_bg_full_height = 4,
		ts_shadow = 8,
		ts_in_inv = 16,
		ts_is_debug = 32,
		ts_dirty = 128
	};

	float text_width, text_height = 0.05f;
	int chat_line_char_limit = 60, chat_lines_limit = 11;

	struct text_obj
	{
		static constexpr float default_text_size = 0.005f;
		static constexpr float default_font_size = 12.0f;

		void init_vbo();
		void mark_needs_update() noexcept;
		
		inline const std::string &get_text() const noexcept { return m_text; }
		inline const vector2f &get_pos() const noexcept { return m_position; }
		inline GLuint get_vbo() const noexcept { return m_vbo; }
		inline float get_font_sz() const noexcept { return m_font_size; }

		void set_text(const std::string &new_txt) noexcept;
		void set_pos(const vector2f &new_pos) noexcept;
		void set_font_sz(float new_font_sz) noexcept;
		void set_ex_line_spacing(float new_ex_line_spc) noexcept;
		void set_ex_char_spacing(float new_ex_char_spc) noexcept;

		inline void remove_settings(uint8_t settings) noexcept { m_settings &= ~settings; }
		inline bool has_settings(uint8_t settings) const noexcept { return m_settings & settings; }
		inline void add_settings(uint8_t settings) noexcept { m_settings |= settings; }
		inline uint8_t get_settings() const noexcept { return m_settings; }

		int get_visible_chars() const noexcept { return m_displayed_chars; }
		int get_data_count() const noexcept;
		void update_visible_chars_count() noexcept;

		float get_transparency() const noexcept;
		bool is_visible() const noexcept;
		void set_visibility(bool visible) noexcept;
		void fade_after_time(double seconds = 5.0) noexcept;
		
		float get_sz_mult() const noexcept;
		float get_char_spacing() const noexcept;
		float get_line_spacing() const noexcept;
		float get_spc_sz() const noexcept;
		
		float get_char_displ_height() const noexcept;
		float get_char_displ_width(int char_ind) const noexcept;

		float get_width() const noexcept;
		float get_height() const noexcept;
		
		void set_position_y_relativeto(const text_renderer_obj::text_obj *rel_to, float offset = 0.01f) noexcept;
		void set_position_x_relativeto(const text_renderer_obj::text_obj *rel_to, float offset = 0.01f) noexcept;

		void upd_text_buf() noexcept;
		~text_obj() noexcept;
	private:
		float get_noclamp_transparency() const noexcept;

		std::string m_text;
		vector2f m_position;
		
		float m_font_size ;
		float m_extra_char_spacing = 0.0f;
		float m_extra_line_spacing = 0.0f;

		float m_fade_secs = 3.0f;
		float m_displayed_transparency = 1.0f;

		double m_hide_time;
		GLuint m_vbo = 0;
		int m_displayed_chars = 0;

		uint8_t m_settings = 0;
	};

	void init_renderer() noexcept;
	void upd_shader_uniform_vals() noexcept;

	void draw_all_texts(bool inv_is_open) const noexcept;
	void render_single(text_obj *text_object, bool switch_shader, bool inv_is_open) const noexcept;

	void mark_all_textobjs() noexcept;
	void update_marked_texts() noexcept;

	void init_text_obj(
		text_obj *to_init,
		vector2f pos, 
		std::string text, 
		uint8_t settings = 0,
		float font_size = text_obj::default_font_size,
		bool is_visible = true
	) noexcept;
	void init_text_obj(text_obj *to_init, const text_obj *original);

	~text_renderer_obj();
private:
	std::vector<text_obj*> m_text_objects;
	GLint m_textpos_uniform_loc;
	GLuint m_text_vao, m_text_quad_vbo;
} extern *glob_txt_rndr;

#endif // SOURCE_RENDERING_TEXTRENDERER_VXL_HDR
