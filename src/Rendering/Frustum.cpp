#include "Frustum.hpp"

// Frustum culling - use to only render chunks that are within the player's view
// For now, only a sphere bounding box is used to simplify calculations done to determine visible chunks
// Original source can be found in https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling

camera_frustum::frust_plane::frust_plane(const vector3d &distance_vec, const vector3d &norm) noexcept
  : normal(norm.c_unit()), distance(normal.dot(distance_vec)) {}
void camera_frustum::update_frustum_vals(
	const vector3d &position,
	const vector3d &cam_front,
	const vector3d &cam_up,
	const vector3d &cam_right,
	double fov_y
) noexcept {
	// Calculate frustum values
	const double half_v_side = far_plane * ::tan(fov_y * 0.5);
	const double half_h_side = half_v_side * static_cast<double>(game.window_wh_aspect);
	const vector3d front_end = cam_front * far_plane;

	// Set each plane's values
	right_pl = { position, (front_end - cam_right * half_h_side).c_cross(cam_up) };
	left_pl = { position, cam_up.c_cross(front_end + cam_right * half_h_side) };
	
	top_pl = { position, cam_right.c_cross(front_end - cam_up * half_v_side) };
	bottom_pl = { position, (front_end + cam_up * half_v_side).c_cross(cam_right) };

	near_pl = { position + cam_front * near_plane, cam_front };
}

bool camera_frustum::is_chunk_visible(const vector3d &corner) const noexcept
{
	// Radius of a sphere that would encapsulate an entire chunk - calculated
	// by half the distance from two opposing corners of a cube (side length = chunk size)
	// (Using simple radius check for frustum culling to save on performance)
	constexpr float size_dbl = static_cast<double>(chunk_vals::size);
	constexpr float chunk_spherical_radius = math::cxpythagoras(math::cxpythagoras(size_dbl, size_dbl), size_dbl) * 0.5f;

	// Get the center of the chunk from given corner
	constexpr vector3d center_offset = vector3d(0.5 * chunk_vals::size);
	const vector3d center = corner + center_offset;
	
	return top_pl   .neg_dist_to_plane(center) <= chunk_spherical_radius &&
	       near_pl  .neg_dist_to_plane(center) <= chunk_spherical_radius &&
	       left_pl  .neg_dist_to_plane(center) <= chunk_spherical_radius &&
	       right_pl .neg_dist_to_plane(center) <= chunk_spherical_radius &&
	       bottom_pl.neg_dist_to_plane(center) <= chunk_spherical_radius;
}
