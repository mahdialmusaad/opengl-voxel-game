#pragma once
#ifndef _SOURCE_RENDERING_FRSTCLL_HDR_
#define _SOURCE_RENDERING_FRSTCLL_HDR_

#include "Globals/Definitions.hpp"

struct FrustumPlane 
{
	glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
	float distance = 0.0f;

	FrustumPlane() noexcept {};
	FrustumPlane(const glm::vec3& distVec, const glm::vec3& _normal) noexcept : normal(_normal), distance(glm::dot(_normal, distVec)) {}
	constexpr float DistToPlane(const glm::vec3& point) const noexcept { return glm::dot(normal, point) - distance; }
};

struct CameraFrustum {
	FrustumPlane top;
	FrustumPlane bottom;

	FrustumPlane right;
	FrustumPlane left;

	FrustumPlane near;
};

struct FrustumSphere 
{
	glm::vec3 center;
	float radius = 0.0f;

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

#endif // _SOURCE_RENDERING_FRSTCLL_HDR_
