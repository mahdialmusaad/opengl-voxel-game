#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include "Application/Definitions.hpp"

class Skybox
{
public:
	Skybox() noexcept;

	static const float fullDaySeconds;
	static const int numStars;
	static const int numClouds;

	void RenderClouds() const noexcept;
	void RenderSky() const noexcept;
	void RenderStars() const noexcept;
	void RenderSunAndMoon() const noexcept;
	
	~Skybox() noexcept;
private:
	GLuint m_cloudsVAO, m_cloudsInstVBO, m_cloudsShapeVBO, m_cloudsShapeEBO;
	GLuint m_skyboxVAO, m_skyboxVBO, m_skyboxEBO;
	GLuint m_starsVAO, m_starsVBO;
	GLuint m_sunMoonVAO, m_sunMoonVBO;

	void CreateClouds() noexcept;
	void CreateSkybox() noexcept;
	void CreateStars() noexcept;
	void CreateSunAndMoon() noexcept;
};

#endif
