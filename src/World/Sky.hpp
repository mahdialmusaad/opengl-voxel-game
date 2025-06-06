#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include "Globals/Definitions.hpp"

class Skybox
{
public:
	Skybox() noexcept;

	constexpr static size_t AMOUNT_OF_CLOUDS = 300;
	constexpr static size_t AMOUNT_OF_STARS = 30000;

	constexpr static float dayNightTimeSeconds = 600.0f;
	constexpr static float starRotationalSpeed = 0.015f;
	constexpr static float dayNightTimeReciprocal = 1.0f / dayNightTimeSeconds;

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
