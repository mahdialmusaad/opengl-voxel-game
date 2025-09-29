#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include "Application/Definitions.hpp"

class skybox_elems_obj
{
public:
	skybox_elems_obj() noexcept;
	void draw_skybox_elements() noexcept;

	static constexpr double day_seconds = 1200.0;
	static constexpr int stars_count = 20000;
	static constexpr int clouds_count = 300;

	~skybox_elems_obj();
private:
	GLuint m_clouds_vao, m_clouds_inst_vbo, m_clouds_vbo, m_clouds_ebo;
	GLuint m_skybox_vao, m_skybox_vbo;
	GLuint m_stars_vao, m_stars_vbo;
	GLuint m_planets_vao, m_planets_vbo, m_planets_ebo;

	void generate_clouds(std::mt19937 *gen, std::uniform_real_distribution<float> *dist) noexcept;
	void generate_stars(float init_random) noexcept;
	void generate_skybox() noexcept;
	void generate_planets() noexcept;
};

#endif
