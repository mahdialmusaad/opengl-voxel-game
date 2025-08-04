#pragma once
#ifndef _SOURCE_RENDERING_FRUSTUM_HDR_
#define _SOURCE_RENDERING_FRUSTUM_HDR_

#include "Application/Definitions.hpp"

struct CameraFrustum
{
	CameraFrustum() noexcept = default;
	static constexpr double farPlaneDistance = 10000.0, nearPlaneDistance = 0.05;

	struct FrustumPlane
	{
		glm::dvec3 normal = { 0.0f, 1.0f, 0.0f };
		double distance;

		FrustumPlane() noexcept = default;
		FrustumPlane(const glm::dvec3 &distVec, const glm::dvec3 &norm) noexcept;

		float NDistToPlane(const glm::dvec3 &point) const noexcept;
	};

	void UpdateFrustum(const glm::dvec3 &position, const glm::dvec3 &cFront, const glm::dvec3 &cUp, const glm::dvec3 &cRight, double fov) noexcept;

	bool SphereInFrustum(const glm::dvec3 &center, double radius) const noexcept;

	// The 'far' plane would prevent rendering of chunks further away than it so
	// it is not included in frustum checks. This also improves performance slightly.
	FrustumPlane top, near, left, right, bottom;
};

#endif // _SOURCE_RENDERING_FRUSTUM_HDR_
