#ifndef _SOURCE_APPLICATION_CALLBACKS_HDR_
#define _SOURCE_APPLICATION_CALLBACKS_HDR_

#include "Player/Player.hpp"
#include "World/Sky.hpp"

class main_game_obj;

struct game_callbacks
{
	game_callbacks(main_game_obj *app_ptr);
	std::vector<std::string> screenshot_res_strs;
	bool main_clear_screenshot_txt = false;

	void key_event(int key, int scancode, int action, int mods);
	void mouse_event(int button, int action, int) noexcept;
	void mouse_move(double xpos, double ypos) noexcept;
	void window_resize(int _width, int _height) noexcept;
	void scroll_event(double, double yoffset) noexcept;
	void window_move(int x, int y) noexcept;
	void char_event(unsigned codepoint);
	void window_close() noexcept;
	
	void apply_key_event(int key, int action) noexcept;

	void take_screenshot() noexcept;
	void toggle_full_inv_display() noexcept;
	void toggle_fullscreen() noexcept;

	void add_chat_text(std::string message, bool auto_new_line = true) noexcept;
	void to_chat_fmt_text(std::string &text) const noexcept;
	void begin_chatting() noexcept;

	void add_cmd_text(std::string new_txt) noexcept;
	void apply_cmd_text();
private:
	struct tilde_conversion_data {
		tilde_conversion_data(
			const std::string &s,
			int i,
			bool b
		) noexcept : str_arg(s), index(i), decimal(b) {}
		std::string str_arg;
		int index;
		bool decimal;
	} cvd { "", 0, false };

	bool invert(bool &b) const noexcept { b = !b; return b; }

	main_game_obj *m_app = nullptr;
	
	std::vector<std::string> m_previous_chats;
	std::vector<std::string> m_cmd_args;
	int m_chat_history_ind = -1;
	int m_current_cmd_index = 0;
	bool m_query_chat = true;
	
	bool has_arg(int index);
	std::string &get_arg(int index);
	
	template<typename T, typename C = T> C int_arg(int index,
		T min = math::lims<T>::lowest(),
		T max = math::lims<T>::max()
	) { return static_cast<C>(math::clamp(static_cast<T>(formatter::strtoimax(get_arg(index))), min, max)); }
	double dbl_arg(int index,
		double min = math::lims<double>::lowest(),
		double max = math::lims<double>::max()
	) { return math::clamp(std::stod(get_arg(index)), min, max); }
	float flt_arg(int index,
		float min = math::lims<float>::lowest(),
		float max = math::lims<float>::max()
	) { return math::clamp(std::stof(get_arg(index)), min, max); }

	void conv_cmd(const std::vector<tilde_conversion_data> &args_convs);
	void conv_cmd(const tilde_conversion_data &data);

	template<typename T> bool isdec() const noexcept { return math::lims<T>::is_iec559; } 
	template<typename T> tilde_conversion_data tcv(int index, T val) const noexcept {
		return { fmt::to_string(val), index, isdec<T>() };
	}

	template<typename T> void query(const std::string &name, T value) noexcept {
		const std::string as_str = fmt::format(std::is_integral<T>::value ? "{}" : "{:.3f}", value);
		if (m_query_chat) add_chat_text(fmt::format("Current {} is {}", name, as_str));
		cvd = tilde_conversion_data(as_str, 0, isdec<T>());
	}

	template<typename T, int L> void query_mult(
		const std::string &name,
		const vector<T, L> &value,
		int start_ind = 0
	) noexcept {
		std::string fmt_str;
		for (int i = 0, m = L - 1; i < L; ++i) fmt_str += fmt::format("{:.3f}{}", value[i], i != m ? " " : "");
		if (m_query_chat) add_chat_text(fmt::format("Current {}: {}", name, fmt_str));
		cvd = tilde_conversion_data(fmt_str, start_ind, isdec<T>());
	}
};

// OpenGL I/O callbacks can only be set as global functions -
// using wrappers for the actual functions defined in the 'callbacks' struct above
namespace callbacks_wrappers {
	static game_callbacks *cb_wr;
	inline void char_event_cb(GLFWwindow*, unsigned codepoint) { cb_wr->char_event(codepoint); }
	inline void window_resize_cb(GLFWwindow*, int width, int height) { cb_wr->window_resize(width, height); }
	inline void key_event_cb(GLFWwindow*, int key, int sc, int action, int mods) { cb_wr->key_event(key, sc, action, mods); }
	inline void mouse_event_cb(GLFWwindow*, int button, int action, int mods) { cb_wr->mouse_event(button, action, mods); }
	inline void scroll_event_cb(GLFWwindow*, double xoffset, double yoffset) { cb_wr->scroll_event(xoffset, yoffset); }
	inline void mouse_move_cb(GLFWwindow*, double xpos, double ypos) { cb_wr->mouse_move(xpos, ypos); }
	inline void window_move_cb(GLFWwindow*, int x, int y) { cb_wr->window_move(x, y); }
	inline void window_close_cb(GLFWwindow*) { cb_wr->window_close(); }

	inline APIENTRY void gl_debug_out(GLenum source, GLenum type, unsigned id, GLenum severity, GLsizei, const char *message, const void*) {
		if (id == 131185 || id == 131154) return;
		formatter::warn(fmt::format("{} (ID {}) - {}{}{}", message, id,
			ogl::err_src_str(source), ogl::err_typ_str(type), ogl::err_svt_str(severity))
		);
	}
};

#endif // _SOURCE_APPLICATION_CALLBACKS_HDR_
