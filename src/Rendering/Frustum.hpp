#pragma once
#ifndef _SOURCE_RENDERING_FRUSTUM_HDR_
#define _SOURCE_RENDERING_FRUSTUM_HDR_

#include "World/Generation/Settings.hpp"

struct camera_frustum
{
	camera_frustum() noexcept = default;
	static constexpr double far_plane = 10000.0, near_plane = 0.05;

	struct frust_plane
	{
		vector3d normal = { 0.0f, 1.0f, 0.0f };
		double distance;

		frust_plane() noexcept = default;
		frust_plane(const vector3d &dist_vec, const vector3d &norm) noexcept;
		
		inline float neg_dist_to_plane(const vector3d &point) const noexcept {
			return static_cast<float>(distance - normal.dot(point));
		}
	};

	void update_frustum_vals(
		const vector3d &position,
		const vector3d &cam_front,
		const vector3d &cam_up,
		const vector3d &cam_right,
		double fov_y
	) noexcept;
	bool is_chunk_visible(const vector3d &corner) const noexcept;

	// The 'far' plane would prevent rendering of chunks further away than it so
	// it is not included in frustum checks. This also improves performance slightly.
	frust_plane top, near, left, right, bottom;
};

#endif // _SOURCE_RENDERING_FRUSTUM_HDR_
