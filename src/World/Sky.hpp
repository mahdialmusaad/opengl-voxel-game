#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include "Application/Definitions.hpp"

class Skybox
{
public:
	Skybox() noexcept;

	const float fullDaySeconds = 1200.0f;
	
	const GLsizei numStars = glm::max(static_cast<GLsizei>(0x7FFF), static_cast<GLsizei>(70000));
	const GLsizei numClouds = 300;

	void RenderClouds() const noexcept;
	void RenderSky() const noexcept;
	void RenderStars() const noexcept;
	void RenderPlanets() const noexcept;
	
	~Skybox() noexcept;
private:
	GLuint m_cloudsVAO, m_cloudsInstVBO, m_cloudsShapeVBO, m_cloudsShapeEBO;
	GLuint m_skyboxVAO, m_skyboxVBO, m_skyboxEBO;
	GLuint m_starsVAO, m_starsVBO;
	GLuint m_planetsVAO, m_planetsVBO, m_planetsEBO;

	void CreateClouds() noexcept;
	void CreateSkybox() noexcept;
	void CreatePlanets() noexcept;

	void CreateStars() noexcept;
	void CreateStarsImpl(glm::vec4 *data, int start, int end) noexcept;
};

#endif
