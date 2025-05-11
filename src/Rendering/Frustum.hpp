#pragma once
#ifndef _SOURCE_RENDERING_FRSTCLL_HDR_
#define _SOURCE_RENDERING_FRSTCLL_HDR_

#include "Globals/Definitions.hpp"

struct CameraFrustum {
	struct FrustumPlane 
	{
		glm::dvec3 normal = { 0.0, 1.0, 0.0 };
		double distance = 0.0;

		FrustumPlane() noexcept {};
		FrustumPlane(const glm::dvec3& distVec, const glm::dvec3& _normal) noexcept : normal(_normal), distance(glm::dot(_normal, distVec)) {}
		constexpr float DistToPlane(const glm::dvec3& point) const noexcept { return glm::dot(normal, point) - distance; }
	};

	FrustumPlane top;
	FrustumPlane bottom;

	FrustumPlane right;
	FrustumPlane left;

	FrustumPlane near;
};

/*
struct FrustumSphere 
{
	glm::dvec3 center;
	double radius = 0.0;

	FrustumSphere(const glm::vec3& center, float radius) noexcept : center(center), radius(radius) {}

	constexpr bool OnPlane(const FrustumPlane& plane) const noexcept { return plane.DistToPlane(center) > -radius; }

	constexpr bool OnFrustum(const CameraFrustum& frustum) const noexcept {
		return OnPlane(frustum.top)	&&
			OnPlane(frustum.bottom) &&
			OnPlane(frustum.right)	&&
			OnPlane(frustum.left)	&&
			OnPlane(frustum.near);
	}
};
*/

#endif // _SOURCE_RENDERING_FRSTCLL_HDR_
