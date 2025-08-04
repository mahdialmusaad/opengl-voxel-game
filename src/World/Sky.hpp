#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include "Application/Definitions.hpp"

class Skybox
{
public:
	Skybox() noexcept;
	void RenderSkyboxElements() noexcept;

	const float fullDaySeconds = 1200.0f;
	static const int numStars = 100000;
	static const int numClouds = 300;

	~Skybox() noexcept;
private:
	GLuint m_cloudsVAO, m_cloudsInstVBO, m_cloudsVBO, m_cloudsEBO;
	GLuint m_skyboxVAO, m_skyboxVBO, m_skyboxEBO;
	GLuint m_starsVAO, m_starsVBO;
	GLuint m_planetsVAO, m_planetsVBO, m_planetsEBO;

	void CreateClouds() noexcept;
	void CreateStars() noexcept;
	void CreateSkybox() noexcept;
	void CreatePlanets() noexcept;
};

#endif
