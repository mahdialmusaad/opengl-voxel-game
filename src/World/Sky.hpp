#pragma once
#ifndef _SOURCE_WORLD_SKY_HDR_
#define _SOURCE_WORLD_SKY_HDR_

#include <Utility/Definitions.hpp>

class WorldSky
{
public:
	WorldSky() noexcept;

	constexpr static int AMOUNT_OF_CLOUDS = 300;
	constexpr static int AMOUNT_OF_STARS = 30000;

	constexpr static float dayNightTimeSeconds = 60.0f;
	constexpr static float starRotationalSpeed = 0.015f;
	constexpr static float dayNightTimeReciprocal = 1.0f / dayNightTimeSeconds;

	static constexpr float triDist = static_cast<float>(Math::sqrt(0.75));

	void RenderClouds() const noexcept;
	void RenderSky() const noexcept;
	void RenderStars() const noexcept;
private:
	uint8_t m_cloudsVAO, m_skyboxVAO, m_starsVAO;
	void CreateClouds() noexcept;
	void CreateSkybox() noexcept;
	void CreateStars() noexcept;
};

#endif
