#include "Definitions.hpp"

// -------------------- thread_ops --------------------

void thread_ops::split(int thread_count, size_t work_max_index, thread_function_t work_func)
{
	if (thread_count <= 0) throw std::invalid_argument("");
	if (work_max_index == 0) return;
	if (thread_count == 1) {
		work_func(0, 0, work_max_index);
		return;
	}

	std::thread *work_threads = new std::thread[thread_count - 1];

	// Calculate amount of indexes per thread
	const size_t each_thread_share = work_max_index / static_cast<size_t>(thread_count);
	// If the work index size is not a multiple of the number of threads, there will be some left over.
	// This 'leftover' work is spread amongst threads, with each thread taking one until there are no extras.
	int remainder_work = static_cast<int>(work_max_index - (each_thread_share * static_cast<size_t>(thread_count)));

	size_t curr_array_index = 0;
	int spawned_threads = 0;

	for (int thread_index = 0, last = thread_count - 1; thread_index < thread_count; ++thread_index) {
		const size_t index_start = curr_array_index; // Starting index
		curr_array_index += each_thread_share + (remainder_work-- > 0 ? 1 : 0); // Spread extra work
		if (index_start == curr_array_index) break; // No more work left

		// Last 'thread' can just be the calling thread
		if (thread_index == last || (each_thread_share == 0 && remainder_work <= 0)) {
			work_func(thread_index, index_start, curr_array_index);
			break;
		} else {
			// Create a new thread to work using on specific indexes
			work_threads[thread_index] = std::thread(work_func, thread_index, index_start, curr_array_index);
			++spawned_threads;
		}
	}

	// Join all created threads
	for (int thread_index = 0; thread_index < spawned_threads; ++thread_index) work_threads[thread_index].join();
	delete[] work_threads; // Clear thread array
}
void thread_ops::wait_avg_frame() noexcept
{
	// Sleep for 1 screen frame
	const int64_t frame_ms = static_cast<int64_t>(ceil(1000.0 / game.screen_refresh_rate));
	std::this_thread::sleep_for(std::chrono::milliseconds(frame_ms));
}

// -------------------- formatter -------------------- 

void formatter::log(const std::string &t) noexcept { printf("[ %.3fms ] %s\n", glfwGetTime() * 1000.0, t.c_str()); }
void formatter::warn(const std::string &t, unsigned warn_bits) noexcept
{
	// Different warning type prefixes from bitmap
	std::string warn_prefix;
	const auto add_warning = [&](unsigned bit, const char *warn_add) {
		if (warn_bits & bit) warn_prefix += '[' + std::string(warn_add) + "] ";
	};

	add_warning(WARN_FILE, "FILE"); // File error
	warn_prefix += "[WARNING] "; // Default warning is implied

	// Log with warning text at the start so these messages are easily
	// differentiable from normal logs
	printf("%s", warn_prefix.c_str());
	log(t);
}

void formatter::_fmt_abort() noexcept
{
	printf("[ERROR] "); // Error output before timestamp to make it more noticeable
	formatter::log(game.error_msg); // Print out stored error message
	::exit(game.error_code); // Exit with given code
}

void formatter::init_abort(int code) noexcept
{
	// Set global exit message with specific message depending on given fail code
	game.error_code = code;
	switch (game.frame_update_interval)
	{
	case VOXEL_ERR_GLFW_INIT:
		game.current_exec_dir = "Failed to initialize GLFW";
		break;
	case VOXEL_ERR_WINDOW_INIT:
		game.current_exec_dir = "Failed to create window";
		break;
	case VOXEL_ERR_LOADER:
		game.current_exec_dir = "Failed to initialize function loader";
		break;
	default:
		game.current_exec_dir = "Unknown"; 
		break;
	}
	_fmt_abort();
}

void formatter::custom_abort(const std::string &err) noexcept
{
	// Fail with custom error message
	game.error_msg = err;
	game.error_code = VOXEL_ERR_CUSTOM;
	_fmt_abort();
}

int formatter::get_cur_msecs() noexcept
{
	const double cur_time = glfwGetTime();
	const int cur_sec_time = static_cast<int>(cur_time - floor(cur_time));
	return static_cast<int>(cur_sec_time * 1000.0);
}

tm formatter::get_cur_time() noexcept
{
	const time_t time_secs = time(NULL); // Get current time as long
	tm time_vals_result; // Resulting time structure

	#if defined(VOXEL_UNIX)
		// Fill time struct with values based on current time;
		// UNIX systems have the current time as the first arg.
		::localtime_r(&time_secs, &time_vals_result);
	#elif defined(VOXEL_MSVC)
		// Thanks, Microsoft. (#3)
		::localtime_s(&time_vals_result, &time_secs);
	#else
		// Default fallback with mutex to avoid thread-safety issues on plain 'localtime'.
		static std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);
		time_vals_result = *::localtime(&time_secs);
	#endif

	return time_vals_result; // Return filled time structure
}

int formatter::count_char(const std::string &str, const char c) noexcept {
	return static_cast<int>(std::count(str.begin(), str.end(), c));
}
bool formatter::str_ends_with(const std::string &str, const std::string &ending) noexcept {
	return str.size() < ending.size() ? false : str.compare(str.size() - ending.size(), ending.size(), ending) == 0;
}
void formatter::split_str(std::vector<std::string> &vec, const std::string &str, const char sp) noexcept
{
	size_t last_index = 0;
	// Use 'do...while' as 'last_index' is initially 0 and only changed as part of the split logic - 
	// a 'while loop' will not be entered at all as 'last_index' is checked first - and evaluates to false.
	do {
		size_t split_ind = str.find(sp, last_index); // Get next index of split character in string
		const std::string arg = str.substr(last_index, split_ind - last_index); // Get substring of text between split character
		vec.emplace_back(arg); // Add text in between split character to given vector
		last_index = ++split_ind; // Start of next substring is after split character so it doesn't get detected again
	} while (last_index);
	// If there is no space at the end, 'split_ind' will become std::string::npos so the '.substr' uses the rest of the string.
	// Since 'last_index' is set to 1 more than 'split_ind' (pre-inc), it overflows to 0 (size_t is unsigned) and evaluates to false.
	
}
size_t formatter::get_nth_found(const std::string &str, const char find, int nth) noexcept
{
	if (nth < 1) return 0;
	size_t index = std::string::npos;
	do index = str.find(find, ++index); while (--nth && index != std::string::npos);
	return index;
}
intmax_t formatter::conv_to_imax(const std::string &number_str_text) { return ::strtoimax(number_str_text.c_str(), NULL, 10); }

// -------------------- ogl -------------------- 

GLuint ogl::new_vao() noexcept
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	return vao;
}
GLuint ogl::new_buf(GLenum type) noexcept
{
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(type, buf);
	return buf;
}

const char *ogl::get_str(GLenum string_identifier) noexcept
{
	return reinterpret_cast<const char*>(glGetString(string_identifier));
}

const char *ogl::get_shader_str(GLenum shader_type) noexcept
{
	switch (shader_type) {
		case GL_COMPUTE_SHADER:  return "compute";
		case GL_VERTEX_SHADER:   return "vertex";
		case GL_FRAGMENT_SHADER: return "fragment";
		case GL_PROGRAM:         return "program";
		default: return nullptr;
	}
}

const char *ogl::err_svt_str(GLenum severity) noexcept
{
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:            return "[High]";
		case GL_DEBUG_SEVERITY_MEDIUM:          return "[Medium]";
		case GL_DEBUG_SEVERITY_LOW:             return "[Low]";
		case GL_DEBUG_SEVERITY_NOTIFICATION:    return "[Notif]";
		default: return nullptr;
	}
}
const char *ogl::err_src_str(GLenum source) noexcept
{
	switch (source) {
		case GL_DEBUG_SOURCE_API:               return "[API]";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     return "[Window]";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:   return "[Shader]";
		case GL_DEBUG_SOURCE_THIRD_PARTY:       return "[External]";
		case GL_DEBUG_SOURCE_APPLICATION:       return "[Appl]";
		case GL_DEBUG_SOURCE_OTHER:             return "[Other]";
		default: return nullptr;
	} 
}

const char *ogl::err_typ_str(GLenum type) noexcept
{
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               return "[Error]";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "[Deprecated]";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "[Undefined]";
		case GL_DEBUG_TYPE_PORTABILITY:         return "[Portability]";
		case GL_DEBUG_TYPE_PERFORMANCE:         return "[Performance]";
		case GL_DEBUG_TYPE_MARKER:              return "[Marker]";
		case GL_DEBUG_TYPE_PUSH_GROUP:          return "[Push]";
		case GL_DEBUG_TYPE_POP_GROUP:           return "[Pop]";
		case GL_DEBUG_TYPE_OTHER:               return "[Other]";
		default: return nullptr;
	}
}

// -------------------- shaders_obj -------------------- 

void shaders_obj::init_shader_data()
{
	// Setup buffers and shader UBO code for shared program data
	std::string *ubo_list = new std::string[game.shaders.all_ubos.size()];
	for (shader_ubo_base *ubo : game.shaders.all_ubos) ubo->init(ubo_list);

	// GLSL code golfing to reduce bytes of string
	// (but some outside indentation is added to reduce eye strain).

	// Trying to avoid reliance on file read speeds by including shader
	// code in source file, allowing large memory savings as it is
	// easier to implement and change global code included in all shaders
	// (e.g. '#version xxx core') instead of manually writing in each shader.

	// First argument/program variable name for shader use case
	// Character '\2' separates code outside from code inside 'main()'
	// Fragment code is given separate ubo and texture settings to add and then
	// follows the same outer/inner separation rule.

	// TODO: uniform+layout shortcut

	programs.blocks.init("Blocks", ubo_list,
		430, ubo_matrices | ubo_times | ubo_positions | ubo_sizes, tex_none,
		// Outer code
		"\n#extension GL_ARB_shader_draw_parameters : require\n"
		"struct od{double x,z;float y;uint f;};"
		"layout(std430,binding=0)readonly restrict buffer O{od ol[];};"
		"uniform uint ls;uniform uint s1;uniform uint s2;uniform uint s3;" // Uniforms
		"layout(location=0)in uint bd;" // X bits for X,Y,Z depending on settings then rest is texture
		"layout(location=1)in vec4 sXZ;"
		"layout(location=2)in vec4 sYZ;"
		"layout(location=3)in vec4 sYW;"
		"out vec3 z;\2"
		// Inner code
		"const od cd=ol[gl_DrawIDARB];"
		"dvec3 b_pos=dvec3("
			"bd&ls,"
			"(bd>>s1)&ls,"
			"(bd>>s2)&ls"
		")+dvec3(cd.x,cd.y,cd.z);"
		     "if(cd.f==0)b_pos+=sXZ.wyx;"
		"else if(cd.f==1)b_pos+=sYZ.zyx;"
		"else if(cd.f==2)b_pos+=sYZ.ywx;"
		"else if(cd.f==3)b_pos+=sYW.yzx;"
		"else if(cd.f==4)b_pos+=sYZ.xyw;"
		           "else b_pos+=sXZ.xyz;"
		"const vec3 rl=vec3(b_pos-P_player.xyz);"
		"gl_Position=M_origin*vec4(rl,1.0);"
		"z=vec3("
			"((sYZ.x+float(bd>>s3)))*S_blocks,"
			"sYW.y,"
			"clamp((T_f_end-length(rl-vec3(0.0,rl.y*0.9,0.0)))*T_f_range,0.0,1.0)"
		");",
		430, ubo_colours, tex_blocks,
		"in vec3 z;out vec4 f;\2"
		"f=mix(C_main,texture(TX_blocks,z.xy),z.z);"
		"if(f.a==0.0)discard;"
	);
	programs.border.init("Border", ubo_list,
		430, ubo_matrices | ubo_positions, tex_none,
		"layout(location=0)in vec3 p;\2"
		"gl_Position=M_origin*vec4(p+P_chunk.xyz,1.0);",
		330, ubo_none, 0,
		"out vec4 c;\2c=vec4(1.0);"
	);
	programs.clouds.init("Clouds", ubo_list,
		430, ubo_matrices | ubo_times | ubo_positions, tex_none,
		// Outer code
		"layout(location=0)in vec3 B;" // Base cloud cube coords
		"layout(location=1)in vec4 d;" // Instanced cloud data (X, Z, W, H)
		"out vec4 c;\2"
		// Inner code
		"const vec3 m=B*d.w;"
		"vec4 r=vec4(dvec3(d.xyz+m)-P_player.xyz,1.0);"
		"r.z+=T_global;"
		"gl_Position=M_origin*r;"
		"c=vec4(vec3(T_clouds),clamp((T_f_end-length(r.xyz-vec3(0.0,r.y*0.7,0.0))*0.3)*T_f_range,0.0,0.8));",
		430, ubo_colours, tex_none,
		"in vec4 c;out vec4 f;\2"
		"f=vec4(mix(C_main,vec4(c.xyz,1.0),c.w));"
		"if(c.w==0.0)discard;"
	);
	programs.inventory.init("Inventory", ubo_list,
		430, ubo_sizes, tex_none,
		// Outer code
		"uniform const vec4 T[4]={"
			"vec4(0.5,0.0,0.2,0.0)," // Background
			"vec4(0.0,1.0,0.0,0.428571)," // Unequipped
			"vec4(0.0,1.0,0.428571,0.428571)," // Equipped
			"vec4(0.0,0.166666,0.9285714,0.0714285714)" // Crosshair
		"};"
		"layout(location=0)in vec4 a;" // X, Y, W, H
		"layout(location=1)in uint b;" // Texture + type (bit 0)
		"layout(location=2)in vec3 B;" // Base quad coords
		"out vec2 c;out flat int i;\2"
		// Inner code
		"const uint I=b>>1;"
		"if((b&1)==1){"
			"i=1;"
			"c=vec2((B.x+float(I))*S_blocks,B.z);"
		"}else{"
			"i=0;"
			"const vec4 t=T[I];"
			"c=vec2((B.x*t.y)+t.x,(B.y*t.w)+t.z);"
		"}"
		"gl_Position=vec4(a.x+(B.x*a.z),a.y+(B.y*a.w),0.0,1.0);",
		430, ubo_none, tex_blocks | tex_inventory,
		"out vec4 f;in vec2 c;in flat int i;\2"
		"f=texture(i==0?TX_inventory:TX_blocks,c);"
	);
	programs.outline.init("Outline", ubo_list,
		430, ubo_matrices | ubo_positions, tex_none,
		"layout(location=0)in vec3 B;\2"
		"gl_Position=M_origin*vec4(B+P_raycast.xyz,1.0);",
		330, ubo_none, tex_none,
		"out vec4 f;\2"
		"f=vec4(vec3(0.0),1.0);"
	);
	programs.planets.init("Planets", ubo_list,
		430, ubo_matrices, tex_none,
		"layout(location=0)in mat2x4 d;" // { X, Y, Z, 1 }, { R, G, B, A }
		"out vec4 c;\2"
		"gl_Position=(M_planets*d[0]).xyzz;"
		"c=d[1];",
		330, ubo_none, tex_none,
		"" // Empty for simple f=c fragment shader
	);
	programs.sky.init("Sky", ubo_list,
		430, ubo_matrices | ubo_colours | ubo_positions, tex_none,
		"layout(location=0)in vec3 p;out vec4 c;\2"
		"gl_Position=(M_planets*vec4(p,0.0)).xyzz;"
		"c=vec4(1.0,0.0,0.0,1.0);",//mix(C_main,C_evening,1.0-abs(p.y*3.0))
		330, ubo_none, tex_none,
		""
	);
	programs.stars.init("Stars", ubo_list,
		430, ubo_matrices | ubo_times, tex_none,
		// Outer code
		"layout(location=0)in vec3 p;"
		"uniform const vec3 C[16]={" 
			"vec3(1.0),vec3(1.0),vec3(1.0),vec3(1.0),"
			"vec3(1.0),vec3(1.0),vec3(1.0),vec3(1.0),"
			"vec3(1.0),vec3(1.0),vec3(1.0),vec3(1.0),"
			"vec3(1.0,1.0,0.6),vec3(1.0,1.0,0.6),"
			"vec3(1.0,0.6,0.6),vec3(0.6,0.6,1.0)"
		"};"
		"out vec4 c;\2"
		// Inner code
		"c=vec4(C[gl_VertexID&15],T_stars);"
		"gl_PointSize=1.0+fract(length(p*gl_VertexID))*1.5;"
		"gl_Position=(M_stars*vec4(p+vec3(gl_VertexID/gl_PointSize)*1e-5,0.0)).xyzz;",
		330, ubo_none, tex_none,
		""
	);
	programs.text.init("Text", ubo_list,
		430, ubo_none, tex_none,
		// Outer code
		"layout(location=0)in vec2 G;"
		"layout(location=1)in uint I;"
		"layout(location=2)in vec4 a;" // X, Y, W, H
		"layout(location=3)in uvec2 b;" // Char, col
		"uniform float TP[191];" // Uniforms
		"out vec2 t;out vec4 c;\2"
		// Inner code
		"gl_Position=vec4(a.x+(G.x*a.z),a.y-(G.y*a.w),0.0,1.0);"
		"t=vec2(TP[b.x+I],G.y);"
		"c=vec4("
			"(b.y&0xFF)/255.0,"
			"((b.y>>8)&0xFF)/255.0,"
			"((b.y>>16)&0xFF)/255.0,"
			"((b.y>>24)&0xFF)/255.0"
		");",
		330, ubo_none, tex_text,
		"out vec4 f;in vec2 t; in vec4 c;\2"
		"f=texture(TX_text,t)*c;"
	);

	delete[] ubo_list; // Compute shaders do not need UBO definitions, clean memory

	// Compute shaders: Follows same '\2' separation rule as normal shaders.
	// Only contain one set of settings + code combo and do not use textures or UBOs.
	computes.faces.init("CM_faces",
		// Outer code
		"layout(local_size_x=32)in;"
		"layout(std430,binding=1)writeonly restrict buffer K{"
			"uint R[];"
		"};"
		"uniform int cz;uniform int qz;uniform ivec3 D[6];" // Uniforms
		"int L(int x){return(cz+x)%cz;}"
		"bool B(int x){return x<0||x>=cz;}\2"
		// Inner code
		"const int G=int(gl_GlobalInvocationID.x);"
		"const int i=G/6;"
		"const ivec3 C={"
			"i/qz,(i/cz)%cz,i%cz"
		"};"
		"const int f=G%6;"
		"const ivec3 N=C+D[f];"
		"if(G<qz*cz*6)"
			"R[G]=(((L(N.x)*qz)+(L(N.y)*cz)+(L(N.z)))<<3)+((B(N.x)||B(N.y)||B(N.z))?f:6);"
	);
	computes.stars.init("CM_stars",
		// Outer code
		"layout(local_size_x=32)in;"
		"layout(std430,binding=1)writeonly restrict buffer K{"
			"float S[];"
		"};"
		"uniform uint SC;uniform float I;" // Uniforms
		"float R(){"
			"uint x=floatBitsToUint(gl_GlobalInvocationID.x);"
			"x^=x*87983473u;"
			"return float(x&0x7FFFFFFF)/float(0x7FFFFFFF);"
		"}\2"
		// Inner code
		"const float i=R();"
		"const float t=3.883222077*i*float(SC)*I;"
		"const float y=1.0-i*2.0;"
		"const float r=sqrt(1.0-y*y);"
		"const uint G=gl_GlobalInvocationID.x*3;"
		"S[G]=cos(t)*r;"
		"S[G+1]=y;"
		"S[G+2]=sin(t)*r;"
	);
}

// -------------------- shaders_obj::(x)_prog -------------------- 

void shaders_obj::shader_prog::init(
	const std::string &given_name, const std::string *const ubo_list,
	unsigned version, unsigned incl_ubos, unsigned incl_texs,
	const std::string &vertex_code,
	unsigned frag_version, unsigned frag_incl_ubos, unsigned frag_incl_texs,
	const std::string &fragment_code
) {
	base_name = given_name;

	// Use default assignment if fragment shader is empty (un-const)
	if (!fragment_code.size()) const_cast<std::string&>(fragment_code) = "out vec4 f;in vec4 c;\2f=c;";

	// Specific character that would not be typed usually to
	// determine the end of outer code and start of main function code
	const size_t vertex_inner_ind = vertex_code.find(2);
	const size_t fragment_inner_ind = fragment_code.find(2);

	// Create new program and store returned identifier
	program = glCreateProgram();

	texture_res tex_res; // Included textures result for uniform setting later on

	// Create vertex shader from given settings and code
	create_shader(
		&tex_res, vertex, GL_VERTEX_SHADER, ubo_list,
		version, incl_ubos, incl_texs,
		vertex_code.substr(0, vertex_inner_ind),
		vertex_code.substr(vertex_inner_ind + 1)
	);

	// Same for fragment, using specific part of code string
	create_shader(
		&tex_res, fragment, GL_FRAGMENT_SHADER, ubo_list,
		frag_version, frag_incl_ubos, frag_incl_texs,
		fragment_code.substr(0, fragment_inner_ind),
		fragment_code.substr(fragment_inner_ind + 1)
	);

	// Attach compiled vertex and fragment code
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	// Delete standalone vertex and fragment shaders as the program has been created
	glDetachShader(program, vertex);
	glDeleteShader(vertex);
	glDetachShader(program, fragment);
	glDeleteShader(fragment);
	
	if (shaderorprog_has_error(program, GL_PROGRAM)) formatter::custom_abort("");

	// Set sampler uniform values for included textures
	// Texture bind ids start from 1 but texture units start from 0, so subtract 1
	for (const texture_res_obj &tex : tex_res) set_int(tex.texture_name.c_str(), tex.texture_id - 1);
}

void shaders_obj::compute_prog::init(const std::string &given_name, const std::string &compute_code)
{
	base_name = given_name;

	// Create compute shader with given code and version number
	const std::string empty_str;
	const size_t sep_char_ind = compute_code.find(2);

	// Create new program and store returned identifier
	program = glCreateProgram();

	// Create compute shader from given arguments
	create_shader(
		nullptr, compute, GL_COMPUTE_SHADER, &empty_str,
		430, 0, 0,
		compute_code.substr(0, sep_char_ind),
		compute_code.substr(sep_char_ind + 1)
	);

	// Attach compute shader to created program
	glAttachShader(program, compute);
	glLinkProgram(program);
	
	// Delete standalone compute shader from ID
	glDetachShader(program, compute);
	glDeleteShader(compute);

	if (shaderorprog_has_error(program, GL_PROGRAM)) formatter::custom_abort("");
}


void shaders_obj::base_prog::create_shader(
	texture_res *tex_res, GLuint &shader_id, GLenum type, const std::string *const ubo_list,
	unsigned version, unsigned incl_ubos, unsigned incl_texs,
	const std::string &outer_code, const std::string &inner_code
) {
	// Create string with enough memory to avoid excessive reallocations
	std::string result;
	result.reserve(1024);
	
	// Version number at the top
	result = "#version " + std::to_string(version) + " core\n";

	// Add chosen UBOs
	// TODO: UBO binding for each shader
	if (!incl_ubos) goto skip_incl_ubos; // I'd prefer not to indent a bunch of code just for this
	for (size_t i = 0; i < game.shaders.all_ubos.size(); ++i) {
		const shader_ubo_base *const curr_ubo = game.shaders.all_ubos[i];
		if (incl_ubos & (1u << curr_ubo->defined_id)) result += ubo_list[curr_ubo->defined_id];
	}
skip_incl_ubos:

	// Add chosen textures
	const std::string base_sampler = "uniform sampler2D TX_";
	std::string textures_incl_res;

	constexpr size_t textures_count = sizeof(shader_texture_list) / sizeof(shader_texture_list::texture_obj);
	const shader_texture_list::texture_obj *const textures_ptr = reinterpret_cast<shader_texture_list::texture_obj*>(&game.shaders.textures);
	const GLuint first_bind = textures_ptr->bind_id;

	if (!incl_texs) goto skip_incl_texs;
	for (size_t i = 0; i < textures_count; ++i) {
		decltype(textures_ptr) curr_tex = textures_ptr + i;
		if (incl_texs & (1u << (curr_tex->bind_id - first_bind))) {
			const std::string texture_def = base_sampler + curr_tex->filename;
			tex_res->emplace_back(texture_res_obj{ static_cast<GLint>(curr_tex->bind_id), texture_def.substr(18) });
			result += texture_def + ';';
		}
	}
skip_incl_texs:

	// Add given outer code and main code
	result += outer_code.substr(0, outer_code.find(2)) + "void main(){" + inner_code + '}';
	
	// Create given type of shader with constructed GLSL code and compile
	shader_id = glCreateShader(type);
	const char *const c_str_data = result.c_str();
	glShaderSource(shader_id, 1, &c_str_data, nullptr);
	glCompileShader(shader_id);
	
	// Check for any errors during shader code compilation
	if (shaderorprog_has_error(shader_id, type)) formatter::custom_abort("");
}

bool shaders_obj::base_prog::shaderorprog_has_error(GLuint id, GLenum type) noexcept
{
	// Check for any errors in the given shader/program ID
	GLenum status_enum_check;
	void (*get_iv_func_ptr)(GLuint, GLenum, GLint*);
	void (*get_ilog_func_ptr)(GLuint, GLint, GLsizei*, GLchar*);

	if (type != GL_PROGRAM) { // Function pointers and status enum for shaders
		status_enum_check = GL_COMPILE_STATUS;
		get_iv_func_ptr = glGetShaderiv;
		get_ilog_func_ptr = glGetShaderInfoLog;
	} else { // Different pointers for programs
		status_enum_check = GL_LINK_STATUS;
		get_iv_func_ptr = glGetProgramiv;
		get_ilog_func_ptr = glGetProgramInfoLog;
	}

	GLint success_value; // GL_FALSE = error, GL_TRUE = success
	get_iv_func_ptr(id, status_enum_check, &success_value);
	if (success_value == GL_TRUE) return false; // No errors
	
	// Get length of error message and use it to get the error message into a char array
	get_iv_func_ptr(id, GL_INFO_LOG_LENGTH, &success_value);
	GLchar *const info_log = new GLchar[success_value];
	get_ilog_func_ptr(id, success_value, nullptr, info_log);

	formatter::warn(base_name + ' ' + ogl::get_shader_str(type) + " shader error:\n" + info_log);
	delete[] info_log; // Clear error message

	return true; // An error occurred
}


void shaders_obj::base_prog::use() const noexcept { glUseProgram(program); }
void shaders_obj::base_prog::bind_and_use(GLuint vao) noexcept { glBindVertexArray(vao); use(); }

GLint shaders_obj::base_prog::get_loc(const char *name) const noexcept { use(); return glGetUniformLocation(program, name); }
void shaders_obj::base_prog::set_flt(const char *name, GLfloat val) const noexcept { glUniform1f(get_loc(name), val); }
void shaders_obj::base_prog::set_uint(const char *name, GLuint val) const noexcept { glUniform1ui(get_loc(name), val); }
void shaders_obj::base_prog::set_int(const char *name, GLint val) const noexcept { glUniform1i(get_loc(name), val); }

void shaders_obj::base_prog::destroy_now() noexcept { glDeleteProgram(program); program = 0; }
shaders_obj::base_prog::~base_prog() { if (program) glDeleteProgram(program); }

// -------------------- shader_ubo_obj --------------------

shaders_obj::shader_ubo_base::shader_ubo_base(
	char prefix, ubo_glob_types_en val_type,
	const std::string &shader_sepd_names
) noexcept :
	start_char(prefix),
	bytes_type(val_type),
	starting_vals(shader_sepd_names)
{
	defined_id = static_cast<int>(game.shaders.all_ubos.size());
	game.shaders.all_ubos.emplace_back(this); // Add to base UBO list
} 

void shaders_obj::shader_ubo_base::init(std::string *const ubo_list)
{
	ubo = ogl::new_buf(GL_UNIFORM_BUFFER);

	// Buffer correct amount of bytes depending on given type enum
	// and the number of commas + 1 in the variables string
	const int num_bytes = game.shaders.glob_types_data[bytes_type].bytes * (formatter::count_char(starting_vals, ',') + 1);
	glBufferStorage(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(num_bytes), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, defined_id, ubo); // Bind UBO to a specific... binding.

	// Used for UBO definition construction
	const std::string base_type_spaced = game.shaders.glob_types_data[bytes_type].name + ' ';

	// Char + underscore prefix to go at the start of each variable name
	// to easily differentiate normal variables from UBO variables
	const std::string ubo_name_prefix = std::string(1, start_char) + '_';

	// Insert variable prefixes, using comma separator to determine where variables are
	for (size_t ind = 0; ind < starting_vals.size(); ++ind) {
		if (starting_vals[ind] == ',') {
			starting_vals.insert(ind + 1, ubo_name_prefix);
			ind += ubo_name_prefix.size();
		}
	}
	starting_vals.insert(0, ubo_name_prefix); // Also add for beginning variable

	// Insert type at the very beginning
	starting_vals.insert(0, base_type_spaced);
	if (starting_vals.back() != ';') starting_vals += ';'; // Add final semicolon if there isn't one already
	// Should now have comma-separated variable definition: type name1,name2...;

	// UBO starting declaration - creating full definition with binding and name
	std::string base_ubo = "layout(std140,binding=x)uniform UBx{";
	const char ind_as_char = static_cast<char>('0' + defined_id);
	base_ubo[22] = ind_as_char; base_ubo[34] = ind_as_char;
	ubo_list[defined_id] = base_ubo + starting_vals + "};"; // Add to the resulting UBO list

	// Clear - no longer needed for execution
	starting_vals.clear();
	starting_vals.shrink_to_fit();
}

void shaders_obj::shader_ubo_base::update_all() const noexcept
{
	// Update the entire UBO
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, get_data_size(), get_data_ptr());
}

void shaders_obj::shader_ubo_base::update_specific(size_t bytes, size_t offset) const noexcept
{
	// Update a given part of the UBO
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, bytes, get_data_ptr());
}

shaders_obj::shader_ubo_base::~shader_ubo_base() { glDeleteBuffers(1, &ubo); }

// -------------------- shaders_obj::texture_obj -------------------- 

void shaders_obj::shader_texture_list::texture_obj::add_ogl_img() noexcept
{
	if (bind_id) return;
	glGenTextures(1, &bind_id); // Create a new texture object
	
	// Set the newly created texture as the currently bound one
	glActiveTexture(GL_TEXTURE0 + bind_id - 1); // GL_TEXTURE0, GL_TEXTURE1, ...
	glBindTexture(GL_TEXTURE_2D, bind_id);

	// Determine pixel colour to show depending on texture coordinates - nearest or combined
	// Optionally determine mipmap level selection method with same options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap_param == GL_DONT_CARE ? GL_NEAREST : mipmap_param);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Texture coordinate overflow handling
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_param);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_param);

	// Determine 'border' colour with clamp wrap setting
	if (wrap_param == GL_CLAMP_TO_BORDER) {
		const float border_cols[4] { 1.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_cols);
	}

	// Save the given texture data into the currently bound texture object
	glTexImage2D(
		GL_TEXTURE_2D, 0, ogl_display_fmt,
		static_cast<GLsizei>(width), static_cast<GLsizei>(height),
		0, static_cast<GLenum>(ogl_display_fmt),
		GL_UNSIGNED_BYTE, pixels
	);

	// Create mipmaps (with bias setting for different LODs) for the texture 
	if (mipmap_param != GL_DONT_CARE) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, lod_bias);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	::free(pixels); // No longer need image data - loaded for use in shaders
}

shaders_obj::shader_texture_list::texture_obj::~texture_obj() { glDeleteTextures(1, &bind_id); }

// -------------------- file_sys -------------------- 

void file_sys::to_parent_dir(std::string &dir) noexcept
{
	dir = dir.substr(0, dir.find_last_of("/\\"));
}
bool file_sys::check_dir_exists(std::string path)
{
	struct stat info;
	return ::stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}
bool file_sys::check_file_exists(std::string path) 
{
	struct stat info;
	return ::stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFREG);
}
void file_sys::delete_path(std::string path)
{
	if (std::remove(path.c_str()) != 0) throw file_sys::file_err("Could not delete path");
}
void file_sys::create_path(std::string path)
{
	const char *cstr_path = path.c_str();
	int err = 1;

	// Create a directory in the given path
	#if defined(VOXEL_WINDOWS)
	// Thanks, Microsoft. (#4)
	err = ::_mkdir(cstr_path);
	#else
	// Make the given path with specific permissions
	err = ::mkdir(cstr_path, 0755);
	#endif

	if (err) throw file_sys::file_err("Could not create path"); // Throw on error
}

void file_sys::read(const std::string &filename, std::string &contents, bool message)
{
	// Setup file stream (moves to eof with ate bit)
	std::ifstream file_stream(filename, std::ios::binary | std::ios::ate);
	
	// Check if stream is valid
	if (!file_stream.good()) throw file_sys::file_err("Could not read file");

	const std::streampos file_size = file_stream.tellg(); // Get file size
	file_stream.seekg(std::ios::beg); // Move reader to beginning
	contents.resize(static_cast<size_t>(file_size)); // Resize string to fit entire file
	file_stream.read(&contents[0], static_cast<std::streamsize>(file_size)); // Read file to contents string

	// Log file read message if requested
	if (message && game.is_active) formatter::log(formatter::fmt("File '%s' read from disk.", filename.c_str()));
}

void file_sys::get_exec_path(std::string *exec_dir, const char *const argv_first)
{
	// TODO: realloc instead for some implems + calloc instead of malloc when you do so
	// Thanks, different operating systems.
	formatter::auto_delete<char> path_buf;
#if defined(VOXEL_UNIX)
	ssize_t buf_size = PATH_MAX;
	path_buf.data = new char[buf_size];

	const auto resolve_symlink = [&](const char *path) {
		buf_size = 0;
	readlink_repeat_alloc:
		buf_size += PATH_MAX;
		path_buf.data = new char[buf_size];
		const ssize_t readlink_res = ::readlink(path, path_buf.data, buf_size);
		if (readlink_res == buf_size) { // Too small 
			delete[] path_buf.data;
			goto readlink_repeat_alloc;
		} else if (readlink_res == -1) return false;
		return true;
	};

	// Why?
#  if defined(VOXEL_LINUX)
	if (resolve_symlink("/proc/self/exe")) *exec_dir = path_buf.data;
	VOXEL_UNUSED(argv_first);
#  elif defined(VOXEL_FREEBSD)
	if (resolve_symlink("/proc/curproc/file")) *exec_dir = path_buf.data;
	VOXEL_UNUSED(argv_first);
#  elif defined(VOXEL_SOLARIS)
	if (resolve_symlink("/proc/self/path/a.out")) *exec_dir = path_buf.data;
	VOXEL_UNUSED(argv_first);
#  else
	// Default UNIX fallback
	struct stat given_status; // Check given argv status
	if (::lstat(argv_first, &given_status) == 0 && (given_status.st_mode & S_IFLNK)) {
		// Resolve given symbolic link if it is one and set it as the executable path
		if (resolve_symlink(argv_first)) { *exec_dir = path_buf.data; return; }
	}

	if (*argv_first == '/') return; // Already full path
	const std::string argv_first_str = argv_first;
	if (argv_first_str.find('/') != std::string::npos) { // Relative path, append to cwd
		while (!::getcwd(path_buf.data, buf_size)) {
			if (buf_size += PATH_MAX > 0x4000) formatter::custom_abort("Could not getcwd");
			delete[] path_buf.data;
			path_buf.data = new char[buf_size];
		}
		*exec_dir = path_buf.data + std::string(argv_first);
		return;
	}
	
	if (const char *const env = ::getenv("argv[0]")) *exec_dir = env + argv_first_str; // Search PATH for last resort
#  endif
#else
#  if defined(VOXEL_WINDOWS)
	path_buf.data = new char[PATH_MAX];
	if (::_get_pgmptr(&path_buf.data) == 0) *exec_dir = path_buf.data;
	else goto fallback_exec_dir;
#  elif defined(VOXEL_APPLE)
	uint32_t buf_size = PATH_MAX;
apple_get_exec_path:
	path_buf.data = new char[buf_size];
	if (::_NSGetExecutablePath(path_buf.data, &buf_size) == 0) *exec_dir = path_buf.data;
	else {
		delete[] path_buf.data;
		buf_size += PATH_MAX; // Just in case, allocate some extra bytes
		goto apple_get_exec_path;
	}
#  else
	goto fallback_exec_dir; // No implementation found, use argv
#  endif
	return;
fallback_exec_dir:
	*exec_dir = argv_first;
#endif
}

// -------------------- voxel_global -------------------- 

void voxel_global::init() noexcept
{
	// Get display information
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);

	// TODO: Settings file for startup values
	
	// Screen refresh rate for minimized slow-down
	game.screen_refresh_rate = mode->refreshRate;

	// Window dims at startup
	window_width = mode->width / 3;
	window_height = mode->height / 3;

	// Window position at startup (middle)
	window_xpos = (mode->width / 2) - (window_width / 2);
	window_ypos = (mode->height / 2) - (window_height / 2);

	// Set window defaults
	default_window_xpos = window_xpos;
	default_window_ypos = window_ypos;
	default_window_width = window_width;
	default_window_height = window_height;

	// Init perf objects pointer list
	// TODO: struct-as-ptr result function?
	const perf_list::perf_obj *const perfs_ptr = reinterpret_cast<perf_list::perf_obj*>(&perfs);
	for (size_t i = 0; i < math::size(perf_leaderboard); ++i) perf_leaderboard[i] = perfs_ptr + i;

	available_threads = math::max(1, static_cast<int>(std::thread::hardware_concurrency())); // Set thread count (1 as fallback)

	const tm ct = formatter::get_cur_time();
	game_started_time = ct; // Set game start time

	// Output current time
	printf("Started on %02d/%02d/%d %02d:%02d:%02d\n",
		ct.tm_mday, ct.tm_mon + 1, ct.tm_year + 1900, ct.tm_hour, ct.tm_min, ct.tm_sec
	);
}

voxel_global::~voxel_global()
{
	delete[] faces_lk_ptr; // Clean memory from lookup table
	// Possibly wait for screenshot to finish, but include a timeout just in case
	int frame_counts = 0;
	const int max_frames = game.screen_refresh_rate * 5;
	while (taking_screenshot && frame_counts++ < max_frames) thread_ops::wait_avg_frame();
}

voxel_global::global_cleaner::~global_cleaner()
{
	// As per the standard, members are destroyed on reverse order of declaration.
	// Therefore, this 'cleaner' object should be placed at the top to guarantee
	// GLFW termination occurs at the very end, after buffers are deleted.
	glfwTerminate(); // Terminate GLFW library
}
