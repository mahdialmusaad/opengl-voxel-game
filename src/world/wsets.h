#pragma once
#ifndef SOURCE_WORLD_SETS_VXL_HDR
#define SOURCE_WORLD_SETS_VXL_HDR
/* World settings. */

/* Dimensions. */

/* Number of blocks in the X axis of a chunk. */
#define VX_WLD_CHUNK_XBLKS (32)
/* Number of blocks in the Y axis of a chunk. */
#define VX_WLD_CHUNK_YBLKS (32)
/* Number of blocks in the Z axis of a chunk. */
#define VX_WLD_CHUNK_ZBLKS (32)

/* Number of chunks a region should have in the X axis. */
#define VX_WLD_REGION_XDIM (32)
/* Number of chunks a region should have in the Y axis. */
#define VX_WLD_REGION_YDIM (32)
/* Number of chunks a region should have in the Z axis. */
#define VX_WLD_REGION_ZDIM (32)

/* Logic rates. */

/* Time to wait if no work was done [0-999]. */
#define VX_WLD_MILLISECONDS_REST (17u)
/* Maximum number of chunks to generate per thread iteration. */
#define VX_WLD_MAX_GENERATE (256u)
/* Maximum number of chunks to mesh overall (split between threads). */
#define VX_WLD_MAX_MESH (256u)
/* Maximum number of mesh updates to apply per frame. */
#define VX_WLD_MAX_CONSUME (512u)

/* Noise. */

/* How far each block travels in the noise map. */
#define VX_WLD_NOISE_TRAVEL (1 / 512.0)
/* Multiplier for noise result. */
#define VX_WLD_NOISE_MULT (48.0)

/* Generation. */

/* Maximum Y position for bodies of water. */
#define VX_WLD_WATER_LEVEL (-18)

#endif
