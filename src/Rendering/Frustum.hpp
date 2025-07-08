#pragma once
#ifndef _SOURCE_RENDERING_FRUSTUM_HDR_
#define _SOURCE_RENDERING_FRUSTUM_HDR_

#include "Application/Definitions.hpp"

struct CameraFrustum {
	struct FrustumPlane 
	{
		glm::dvec3 normal = { 0.0, 1.0, 0.0 };
		double distance = 0.0;

		FrustumPlane() noexcept {};
		FrustumPlane(const glm::dvec3 &distVec, const glm::dvec3 &_normal) noexcept : normal(_normal), distance(glm::dot(_normal, distVec)) {}
		double DistToPlane(const glm::dvec3 &point) const noexcept { return glm::dot(normal, point) - distance; }
	};

	FrustumPlane top;
	FrustumPlane bottom;

	FrustumPlane right;
	FrustumPlane left;

	FrustumPlane near;
};

#endif // _SOURCE_RENDERING_FRUSTUM_HDR_
