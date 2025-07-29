#include "Frustum.hpp"

// Frustum culling - use to only render chunks that are within the player's view
// For now, only a sphere bounding box is used to simplify calculations done to determine visible chunks
// Original source can be found in https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling

// Frustum plane

CameraFrustum::FrustumPlane::FrustumPlane(const glm::dvec3 &distVec, const glm::dvec3 &norm) noexcept : normal(glm::normalize(norm)), distance(glm::dot(normal, distVec)) {}
float CameraFrustum::FrustumPlane::DistToPlane(const glm::dvec3 &point) const noexcept { return glm::dot(normal, point) - distance; }
void CameraFrustum::UpdateFrustum(const glm::dvec3 &position, const glm::dvec3 &cFront, const glm::dvec3 &cUp, const glm::dvec3 &cRight, double fov) noexcept
{
	// Calculate frustum values
	const double halfVside = farPlaneDistance * glm::tan(fov * 0.5);
	const double halfHside = halfVside * game.aspect;
	const glm::dvec3 frontMultFar = farPlaneDistance * cFront;

	// Set each plane's values
	right = { position, glm::cross(frontMultFar - cRight * halfHside, cUp) };
	left = { position, glm::cross(cUp, frontMultFar + cRight * halfHside) };
	
	top = { position, glm::cross(cRight, frontMultFar - cUp * halfVside) };
	bottom = { position, glm::cross(frontMultFar + cUp * halfVside, cRight) };

	near = { position + nearPlaneDistance * cFront, cFront };
}

// Sphere culling check

bool CameraFrustum::SphereInFrustum(const glm::dvec3 &center, double radius) const noexcept
{
	return top   .DistToPlane(center) > -radius &&
	       near  .DistToPlane(center) > -radius &&
	       left  .DistToPlane(center) > -radius &&
	       right .DistToPlane(center) > -radius &&
	       bottom.DistToPlane(center) > -radius;
}
