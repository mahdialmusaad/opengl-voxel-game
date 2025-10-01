#pragma once
#ifndef SOURCE_APPLICATION_DEFS_VXL_HDR
#define SOURCE_APPLICATION_DEFS_VXL_HDR

#if !defined(NDEBUG)
#define VOXEL_DEBUG
#endif

#if defined(__cplusplus)
#define VOXEL_CPP
#else
#define VOXEL_RESTRICT restrict
#endif

#define VOXEL_UNUSED(value) do { (void)(value); } while (0)

#define VOXEL_ERR_GLFW_INIT -1
#define VOXEL_ERR_WINDOW_INIT -2
#define VOXEL_ERR_LOADER -3
#define VOXEL_ERR_CUSTOM -4

#if defined(VOXEL_CPP)
#define LODEPNG_NO_COMPILE_CPP
extern "C" {
#endif

// glad - OpenGL API function loader
#include "glad/voxel_glad.h"

// GLFW - Window and input library
#include "glfw/include/GLFW/glfw3.h"

// lodepng - PNG encoder and decoder
#include "lodepng/lodepng.h"

// C libraries
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>

#if defined(VOXEL_CPP)
}
#endif

// C++ libraries
#include <mutex>
#include <thread>
#include <condition_variable>

#include <string>
#include <fstream>

#include <random>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <unordered_map>

// Local vector header
#include "World/Generation/Vector.hpp"

// Compiler directives
#if defined(__MINGW32__)
#define VOXEL_MINGW
#endif
#if defined(__clang__)
#define VOXEL_CLANG
#elif defined(VOXEL_MINGW) || defined(__GNUC__) || defined(__GNUG__)
#define VOXEL_GCC
#  if !defined(VOXEL_RESTRICT)
#  define VOXEL_RESTRICT __restrict__
#  endif
#endif
#if defined(_MSC_VER)
#define VOXEL_MSVC
#  if !defined(VOXEL_RESTRICT)
#  define VOXEL_RESTRICT __restrict
#  endif
#endif

// OS directives
#if defined(__linux__)
#define VOXEL_LINUX
#endif
#if defined(__APPLE__) && defined(__MACH__)
#define VOXEL_APPLE
#include <mach-o/dyld.h>
#endif
#if defined(__unix__) || defined(unix) || defined(__unix) || defined(VOXEL_APPLE)
#define VOXEL_UNIX
#include <unistd.h>
#endif
#if defined(sun) || defined(__sun)
#  if defined(__SVR4) || defined(__svr4__)
#  define VOXEL_SOLARIS
#  else
#  define VOXEL_SUNOS
#  endif
#endif
#if defined(__FreeBSD__)
#define VOXEL_FREEBSD
#endif
#if defined(__ANDROID__)
#define VOXEL_ANDROID
#endif
#if defined(__CYGWIN__)
#define VOXEL_CYGWIN
#endif
#if defined(_WIN32) || defined(VOXEL_MINGW) || defined(VOXEL_CYGWIN)
#define VOXEL_WINDOWS
#define WIN32_LEAN_AND_MEAN
// Thanks, Microsoft. (#1)
// Who thought that #define-ing a bunch of macros with extremely
// simple and common names like 'max' would be a good idea?
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#  if !defined(SSIZE_T)
#  define SSIZE_T signed long long int
#endif
#define ssize_t SSIZE_T
#  if defined(_WIN64) || defined(VOXEL_CYGWIN)
#  define VOXEL_WIN64
#  endif
#endif

#if !defined(PATH_MAX)
#  if defined(MAX_PATH)
   // Thanks, Microsoft. (#2)
#  define PATH_MAX MAX_PATH
#  endif
#endif

#if !defined(APIENTRY)
#define APIENTRY
#endif

typedef int64_t pos_t; // Type used for positioning
typedef vector<pos_t, 3> world_pos; // 3D position
typedef vector<pos_t, 2> world_xzpos; // 2D position

// For printing format
#define VXF64 "%" PRIi64

typedef uint32_t quad_data_t; // Data per world quad

// OpenGL function shorcuts
namespace ogl 
{
	GLuint new_buf(GLenum type) noexcept;
	GLuint new_vao() noexcept;
	
	const char *get_str(GLenum string_identifier) noexcept;
	const char *get_shader_str(GLenum shader_type) noexcept;
	
	const char *err_svt_str(GLenum severity) noexcept;
	const char *err_src_str(GLenum source) noexcept;
	const char *err_typ_str(GLenum type) noexcept;
};

// Thread-related functions
namespace thread_ops {
	typedef std::function<void(int, size_t, size_t)> thread_function_t;
	void split(int thread_count, size_t work_array_size, thread_function_t individual_work_func);
	void wait_avg_frame() noexcept;
}

// Logging and text formatting
namespace formatter
{
	template<typename T>
	struct auto_delete { T *data = nullptr; ~auto_delete() { if (data) delete[] data; } };

	enum warn_bits_en : unsigned {
		WARN_FILE
	};

	void warn(const std::string &t, unsigned warn_bits = 0u) noexcept;
	void log(const std::string &t) noexcept;
	void _fmt_abort() noexcept;
	void init_abort(int code) noexcept;
	void custom_abort(const std::string &err) noexcept;

	template <class...A> std::string fmt(const std::string& fmt_string, A... args)
	{
		const ssize_t fmtd_sz = static_cast<ssize_t>(::snprintf(nullptr, 0, fmt_string.c_str(), args...) + 1);
		if (fmtd_sz <= 0) return fmt_string; // Fallback on error
		char *const fmt_buf = static_cast<char*>(malloc(static_cast<size_t>(fmtd_sz))); 
		::snprintf(fmt_buf, fmtd_sz, fmt_string.c_str(), args...);
		return fmt_buf;
	}

	template <typename T> std::string specific()
	{
		if (!std::is_integral<T>::value) return "%f";
		if (std::is_same<T, intmax_t>::value) return "%ji";
		if (std::is_same<T, uintmax_t>::value) return "%ju";
		if (std::is_same<T, size_t>::value) return "%zi";
		if (std::is_same<T, ssize_t>::value) return "%zd";
		if (std::is_unsigned<T>::value) return "%u";
		else return "%d";
	}

	int get_cur_msecs() noexcept;
	tm get_cur_time() noexcept;

	int count_char(const std::string &str, const char c) noexcept;
	bool str_ends_with(const std::string &str, const std::string &ending) noexcept;
	void split_str(std::vector<std::string> &vec, const std::string &str, const char sp) noexcept;
	size_t get_nth_found(const std::string &str, const char find, int nth) noexcept;
	intmax_t conv_to_imax(const std::string &num_as_text);
	
	template <typename T> std::string group_num(T val, unsigned decimals = 3u) noexcept
	{
		std::string as_str = std::to_string(val);
		size_t decm_ind = as_str.find('.'); // Find decimal index (npos if none)

		if (decm_ind == std::string::npos) decm_ind = as_str.size(); // no decimal found, use size for index calculation
		else {
			as_str = as_str.substr(0, decm_ind + ++decimals); // Remove any extra decimals
			as_str.insert(as_str.size(), decimals - (as_str.size() - decm_ind), '0'); // Pad with 0s if there aren't enough
		}
		
		const bool minus = as_str[0] == '-'; // Check for minus sign
		if (decm_ind - minus < 4) return as_str; // Ignore small numbers with no separator needed
		
		// Add thousands separator every 3 digits while accounting for negative sign
		for (ssize_t ind = static_cast<ssize_t>(decm_ind) - 3; ind > minus; ind -= 3) as_str.insert(ind, 1, ','); 

		return as_str;
	}
};

// Shader loader and manager
struct shaders_obj
{
private:
	struct shader_types_data { std::string name; int bytes; };
	enum ubo_glob_types_en : int { U_float, U_double, U_vec4, U_dvec4, U_mat4, U_MAX };
	const shader_types_data glob_types_data[ubo_glob_types_en::U_MAX] = {
		{ "float", sizeof(GLfloat) }, { "double", sizeof(GLdouble) },
		{ "vec4", sizeof(GLfloat[4]) }, { "dvec4", sizeof(GLdouble[4]) },
		{ "mat4", sizeof(GLfloat[4][4]) }
	};
	typedef matrixf4x4 mat4;
	typedef vector4d dvec4;
	typedef vector4f vec4;

public:
	// Textures list
	struct shader_texture_list {
	struct texture_obj {
		texture_obj(
			std::string file_name, LodePNGColorType stored_col_type, unsigned stored_bit_depth,
			GLint displayed_col_type, GLint wrap_parameter, GLint mipmap_parameter, float mip_lod_bias
		) noexcept :
			filename(file_name), stored_col_fmt(stored_col_type), bit_depth(stored_bit_depth),
			ogl_display_fmt(displayed_col_type), wrap_param(wrap_parameter), mipmap_param(mipmap_parameter),
			lod_bias(mip_lod_bias)
		{};

		void add_ogl_img() noexcept;

		const std::string filename;
		GLuint bind_id = 0;
		unsigned char *pixels = nullptr;
		const LodePNGColorType stored_col_fmt;
		unsigned width, height, bit_depth;
		const GLint ogl_display_fmt;
		const GLint wrap_param;
		const GLint mipmap_param;
		const float lod_bias;

		~texture_obj();
	} blocks    = { "blocks",    LodePNGColorType::LCT_RGBA, 8u, GL_RGBA, GL_REPEAT, GL_DONT_CARE, 1.0f }, // Blocks atlas texture
	  inventory = { "inventory", LodePNGColorType::LCT_RGBA, 8u, GL_RGBA, GL_REPEAT, GL_DONT_CARE, 1.0f }, // Inventory elements texture
	  text      = { "text",      LodePNGColorType::LCT_RGBA, 8u, GL_RGBA, GL_REPEAT, GL_DONT_CARE, 1.0f }; // ASCII text list texture
	// TODO: text 1bpp, inv 8bpp
	} textures;
	enum tex_incl_en : int { tex_none = 0, tex_blocks = 1, tex_inventory = 2, tex_text = 4 };

	struct shader_ubo_base;
	std::vector<shader_ubo_base*> all_ubos; // List of all base UBOs

	struct shader_ubo_base {
		shader_ubo_base(char prefix, ubo_glob_types_en val_type, const std::string &shader_sepd_names) noexcept;
		void init(std::string *const ubo_list);

		char start_char;
		int bytes_type;
		std::string starting_vals;
		GLuint ubo = 0;
		int defined_id = 0;

		virtual const void *get_data_ptr() const noexcept { return nullptr; }
		virtual size_t get_data_size() const noexcept { return 0; }
		void update_all() const noexcept;
		void update_specific(size_t bytes, size_t offset) const noexcept;
		~shader_ubo_base();
	};

	// This allows parity between shader and CPU code variable names whilst using struct
	// members instead of relying on remembering the order of variable declarations.
	// It also means the string and member version of a value are defined together so they
	// can be changed easily without having to make any changes to the source file.

	// Each UBO, as part of the base constructor, is initialized automatically
	// during shader loading to construct the shader code from the given information - adding
	// a new UBO only requires a new entry below.
	
	#define UBO_DEF(name, prefix, type, ...) enum UBO_##name##_en { ubo_##name = math::bitwise_ind(__LINE__ - UBO_IND_SUB - 1) };\
	struct UBO_##name : public shader_ubo_base {\
		UBO_##name(const std::string &shader_sepd_names) : shader_ubo_base(prefix, U_##type, shader_sepd_names) {}\
		struct { type __VA_ARGS__; } vals;\
		const void *get_data_ptr() const noexcept override { return &vals; };\
		size_t get_data_size() const noexcept override { return sizeof(vals); }\
	} name{#__VA_ARGS__};\

	// UBO definitions
	// Does not work if you space out the variable names due to string interpretation
	// Using __LINE__ macro to create enum counter for each UBO (bitwise or),
	// so keep them next to each other with no spacing to work properly
	enum ubo_sub_en { ubo_none = 0, UBO_IND_SUB = __LINE__ };
	UBO_DEF(matrices, 'M', mat4, origin,stars,planets);
	UBO_DEF(times, 'T', float, cycle,f_end,f_range,stars,global,clouds);
	UBO_DEF(colours, 'C', vec4, main,evening,light); 
	UBO_DEF(positions, 'P', dvec4, raycast,player,chunk,offset);
	UBO_DEF(sizes, 'S', float, blocks,inventory,stars);
	#undef UBO_DEF

	struct base_prog {
		base_prog() noexcept = default;

		struct texture_res_obj { GLint texture_id; std::string texture_name; };
		typedef std::vector<texture_res_obj> texture_res;

		void create_shader(
			texture_res *tex_res, GLuint &shader_id, GLenum type, const std::string *const ubo_list,
			unsigned version, unsigned incl_ubos, unsigned incl_texs,
			const std::string &outer_code, const std::string &inner_code
		);

		bool shaderorprog_has_error(GLuint id, GLenum type) noexcept;
		void bind_and_use(GLuint vao) noexcept;

		GLuint program = 0;
		std::string base_name;

		void use() const noexcept;
		GLint get_loc(const char *name) const noexcept;

		void set_flt(const char *name, GLfloat val) const noexcept;
		void set_uint(const char *name, GLuint val) const noexcept;
		void set_int(const char *name, GLint val) const noexcept;
		
		void destroy_now() noexcept;
		~base_prog();
	};

	struct shader_prog : base_prog
	{
		void init(
			const std::string &given_name, const std::string *const ubo_list,
			unsigned version, unsigned incl_ubos, unsigned incl_texs,
			const std::string &vertex_code,
			unsigned frag_version, unsigned frag_incl_ubos, unsigned frag_incl_texs,
			const std::string &fragment_code
		);
		GLuint vertex = 0, fragment = 0;
	};
	struct compute_prog : base_prog
	{
		void init(const std::string &given_name, const std::string &compute_code);
		GLuint compute = 0;
	};

	struct shader_progs_list { shader_prog blocks, clouds, inventory, outline, sky, stars, text, planets, border; } programs;
	struct compute_progs_list { compute_prog faces, stars; } computes;

	void init_shader_data();
};

// File system functions
namespace file_sys
{
	void read(const std::string &filename, std::string &contents, bool log);

	bool check_dir_exists(std::string path);
	bool check_file_exists(std::string path);

	void get_exec_path(std::string *result, const char *const argv_first);
	void to_parent_dir(std::string &directory) noexcept;

	void delete_path(std::string path);
	void create_path(std::string path);

	struct file_err : public std::exception { 
		const char *err_message;
		file_err(const char *msg) {
			formatter::warn(msg, formatter::WARN_FILE);
			err_message = msg;
		}
		const char *what() const noexcept override { return err_message; } 
	};
};

// Global object for game data
struct voxel_global
{
	void init() noexcept;

	GLFWwindow *window_ptr;
	uint32_t *faces_lk_ptr;
	struct global_cleaner { ~global_cleaner(); } cleaner;
	shaders_obj shaders;
	
	int window_xpos, window_ypos, default_window_xpos, default_window_ypos;
	int window_width, window_height, default_window_width, default_window_height;
	int frame_update_interval = 1, screen_refresh_rate;
	float window_wh_aspect;
	
	vector4d runtime_test_vec = vector4d(1.0);
	double game_tick_speed = 1.0;
	double ticked_delta_time = 0.016;
	double global_time = 0.0;
	double cycle_day_seconds = 0.0;

	int available_threads, generation_thread_count;
	vector3i max_wkgp_count, max_wkgp_size;
	union { int max_wkgp_invocations; int error_code; };
	
	bool any_key_active = false;
	bool libraries_inited = false;
	bool taking_screenshot = false;
	bool window_focus_changed = true;
	bool chatting = false;
	bool ignore_initial_chat_char = false;
	bool is_normal_chat = false;
	bool is_window_iconified = false;
	bool is_game_server = false;
	bool do_generate_signal = true;
	bool is_synced_fps = true;
	bool show_any_gui = true;
	bool is_wireframe_view = false;
	bool is_window_fullscreen = false;
	bool is_active = true;
	bool is_loop_active = false;
	bool DB_exit_on_loaded = false;
	bool display_debug_text = true;
	bool display_chunk_borders = false;
	bool hide_world_fog = false;
	bool is_first_open = true;

	float twilight_colour_trnsp = -1.0f;
	
	intmax_t world_day_counter = 0;
	uintmax_t game_frame_counter = 0;

	tm game_started_time;
	
	double rel_mouse_xpos = 0.0, rel_mouse_ypos = 0.0, rel_mouse_sens = 0.1;
	double frame_delta_time = 0.016;
	
	union { std::string current_exec_dir{}; std::string error_msg; };
	const std::string def_title = "Voxels 1.0.0";
	std::string resources_dir;
	
	bool is_ctrl_held, is_shift_held, is_alt_held;
	std::unordered_map<int, int> glob_keyboard_state;

	const uint8_t ascii_char_sizes[95] = {
	// !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
	   1, 3, 6, 5, 9, 6, 1, 2, 2, 5, 5, 2, 5, 1, 3,
	// 0  1  2  3  4  5  6  7  8  9
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	// :  ;  <  =  >  ?  @
	   1, 2, 4, 5, 4, 5, 6,
	// A  B  C  D  E  F  G  H  I  J  K  L  M
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	// N  O  P  Q  R  S  T  U  V  W  X  Y  Z
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	// [  \  ]  ^  _  `
	   2, 3, 2, 5, 5, 2,
	// a  b  c  d  e  f  g  h  i  j  k  l  m
	   4, 4, 4, 4, 4, 4, 4, 4, 1, 3, 4, 3, 5,
	// n  o  p  q  r  s  t  u  v  w  x  y  z
	   4, 4, 4, 4, 4, 4, 3, 4, 5, 5, 5, 4, 4,
	// {  |  }  ~
	   3, 1, 3, 6,
	// Background character (custom)
	   1
	};

	struct perf_list
	{
		struct perf_obj
		{
			perf_obj(const char *_name) noexcept : name(_name) {}

			std::string name;
			double total = 0.0;
			uintmax_t count = 0;

			void start_timer() noexcept { m_start_time = glfwGetTime(); }
			void end_timer() noexcept { total += glfwGetTime() - m_start_time; ++count; }
		private:
			double m_start_time;
		};

		#define PERF_DEF(name) perf_obj name = #name
		PERF_DEF(movement);
		PERF_DEF(text_update);
		PERF_DEF(frame_cols);
		PERF_DEF(render_sort);
		PERF_DEF(buf_update);
		#undef PERF_DEF
	} perfs;

	const perf_list::perf_obj *perf_leaderboard[sizeof(perf_list) / sizeof(perf_list::perf_obj)];
	~voxel_global();
} extern game;

#endif // SOURCE_APPLICATION_DEFS_VXL_HDR
