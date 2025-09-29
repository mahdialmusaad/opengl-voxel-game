#pragma once
#ifndef _SOURCE_APPLICATION_GAME_HDR_
#define _SOURCE_APPLICATION_GAME_HDR_

#include "Callbacks.hpp"

class main_game_obj
{
	// Allow callbacks struct to access private members of this class
	friend struct game_callbacks;
	game_callbacks events_handler;
public:
	main_game_obj() noexcept;

	static void init_game(int argc, const char *const *const argv);
	void main_world_loop();
private:
	void DB_func_perf() noexcept;
	void DB_perlin_png() const noexcept;

	void aspect_changed() noexcept;
	void update_frame_vals() noexcept;
	void player_moved_update() noexcept;
	void upd_dynamic_text(int *fps_counters) noexcept;

	text_renderer_obj m_glob_text_rndr_obj;
	player_inst m_player_inst;
	world_obj m_world;
	skybox_elems_obj m_skybox;

	text_renderer_obj::text_obj
	   m_static_info_txt{},
	   m_game_info_txt{},
	   m_world_info_txt{},
	   m_chat_txt{},
	   m_command_text{},
	   m_perf_text{};

	double manage_game_time(bool is_changing_time, double t = 0.0) noexcept;
};

#endif // _SOURCE_APPLICATION_GAME_HDR_
