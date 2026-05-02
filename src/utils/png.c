#include "utils/png.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"
#include "directives/dos.h"

#include "io/files.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* 8-byte signature at the start of a PNG file. */
static const char png_signature[8] = { '\x89', 'P', 'N', 'G', '\r', '\n', '\x1a', '\n' };

/* Swap byte order of a 32 bit value. */
static inline uint32_t vxpng_swap32(uint32_t val)
{
	return (((val >> 24u) & 0x000000FFu) |
		((val <<  8u) & 0x00FF0000u) |
		((val >>  8u) & 0x0000FF00u) |
		((val << 24u) & 0xFF000000u));
}


/* Zlib decompression algorithm. */

#if defined (__has_include)
#define VX_HAS_INC(x) __has_include(x)
#else
#define VX_HAS_INC(x) 0
#endif

#if VX_HAS_INC(<zlib.h>) && !VX_WINDOWS
#define VX_EXT_ZLIB 1
#else
#define VX_EXT_ZLIB 0
#endif

#if VX_EXT_ZLIB
/* Using external Zlib. */
#include <zlib.h>

static void vxpng_inflate(uint8_t *result, uint8_t *stream, uint32_t stream_len, uint32_t result_len)
{
	z_stream zs_info;
	memset(&zs_info, 0, sizeof zs_info);
	inflateInit2(&zs_info, -MAX_WBITS);
	zs_info.avail_in = stream_len;
	zs_info.next_in = stream;
	zs_info.avail_out = result_len;
	zs_info.next_out = result;
	inflate(&zs_info, Z_FINISH);
	inflateEnd(&zs_info);
}

#else

/* The maximum number of bits in a code. */
#define VX_MAX_CODE_BITS (15)

/* Current values for inflation. */
typedef struct vxpng_inflate_state
{
	uint8_t *output;
	const uint8_t *input;

	int read_bits_buffer;
	int read_bits_count;
} vxpng_inflate_state;

/* Huffman code decoding tables. */
typedef struct vxpng_huffman_state
{
	int16_t *count;
	int16_t *symbol;
} vxpng_huffman_state;

/* Return a specific number of bits from the stream. */
static int vxpng_bits(vxpng_inflate_state *state, int bits_to_get)
{
	/* Load specific number of bits. */
	int val = state->read_bits_buffer;
	while (state->read_bits_count < bits_to_get) {
		val |= VX_CAST(int, *state->input++) << state->read_bits_count;
		state->read_bits_count += 8;
	}

	/* Drop specific number of bits and update buffer. */
	state->read_bits_buffer = val >> bits_to_get;
	state->read_bits_count -= bits_to_get;

	/* Return only needed number of bits. */
	return (int)(val & ((1L << bits_to_get) - 1));
}

/* Decode a value from the stream using the Huffman codes. */
static int vxpng_decode(vxpng_inflate_state *state, const vxpng_huffman_state *huffman)
{
	int first = 0, index = 0, code = 0;

	for (int len = 1; len <= VX_MAX_CODE_BITS; ++len) {
		code |= vxpng_bits(state, 1);
		const int count = huffman->count[len];
		if (code - count < first) return huffman->symbol[index + (code - first)];
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}

	return -1;
}

/* Construct the tables required to decode Huffman codes. */
static void vxpng_construct(vxpng_huffman_state *huffman, const int16_t *length, int length_count)
{
	/* Count number of codes of each length. */
	for (int len = 0; len <= VX_MAX_CODE_BITS; ++len) huffman->count[len] = 0;
	for (int symbol = 0; symbol < length_count; ++symbol) (huffman->count[length[symbol]])++;

	/* Offsets input symbol table for each length. */
	int16_t offsets[VX_MAX_CODE_BITS+1];
	offsets[1] = 0;
	/* Generate offsets into symbol table for each length for sorting. */
	for (int len = 1; len < VX_MAX_CODE_BITS; ++len) offsets[len + 1] = offsets[len] + huffman->count[len];

	/* Put symbols input table sorted by length, by symbol order within each length. */
	for (int symbol = 0; symbol < length_count; ++symbol) {
		if (length[symbol] == 0) continue;
		huffman->symbol[offsets[length[symbol]]++] = VX_CAST(int16_t, symbol);
	}
}

/* Decode literal/length and distance codes until an end-of-block code. */
static void vxpng_process(
	vxpng_inflate_state *state,
	const vxpng_huffman_state *len_codes,
	const vxpng_huffman_state *dist_codes
) {
	/* Size base for length codes 257-285. */
	static const int16_t len_base_codes[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	/* Extra bits for length codes 257-285. */
	static const int16_t lens_extra_bits[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
	/* Offset base for distance codes 0-29. */
	static const int16_t dist_base_codes[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
	/* Extra bits for distance codes 0-29. */
	static const int16_t dist_extra_bits[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

	/* Decode literals and length/distance pairs. */
	do {
		int symbol = vxpng_decode(state, len_codes);
		
		if (symbol < 256) *state->output++ = VX_CAST(uint8_t, symbol); /* Literal byte, just copy to output. */
		else if (symbol > 256) {
			/* Get count of symbols to repeat. */
			symbol -= 257;
			int length = len_base_codes[symbol] + vxpng_bits(state, lens_extra_bits[symbol]);

			/* Get backward distance of target symbols. */
			symbol = vxpng_decode(state, dist_codes);
			const int distance = dist_base_codes[symbol] + vxpng_bits(state, dist_extra_bits[symbol]);

			/* Copy 'length' bytes from 'distance' bytes back. */
			while (length--) {
				*state->output = *(state->output - distance);
				++state->output;
			}
		}
		else return; /* End of block symbol, stop decoding. */
	} while (1);
}

/* Process a 'static' codes block. */
static void vxpng_fixed(vxpng_inflate_state *state)
{
	int16_t len_counts[VX_MAX_CODE_BITS + 1], len_symbols[288];
	vxpng_huffman_state len_codes = { len_counts, len_symbols };
	
	int16_t lengths[288];
	int symbol;
	
	/* Create standard literal/length table. */
	for (symbol = 0; symbol < 144; symbol++) lengths[symbol] = 8;
	for (; symbol < 256; symbol++) lengths[symbol] = 9;
	for (; symbol < 280; symbol++) lengths[symbol] = 7;
	for (; symbol < 288; symbol++) lengths[symbol] = 8;
	vxpng_construct(&len_codes, lengths, 288);

	int16_t dist_counts[VX_MAX_CODE_BITS + 1], dist_symbols[30];
	vxpng_huffman_state dist_codes = { dist_counts, dist_symbols };

	/* Create standard distance table. */
	for (symbol = 0; symbol < 30; symbol++) lengths[symbol] = 5;
	vxpng_construct(&dist_codes, lengths, 30);

	vxpng_process(state, &len_codes, &dist_codes);
}

/* Process a 'dynamic' codes block. */
static void vxpng_dynamic(vxpng_inflate_state *state)
{
	/* Get number of lengths in each table. */
	const int NLEN  = vxpng_bits(state, 5) + 257;
	const int NDIST = vxpng_bits(state, 5) + 1;
	const int NCODE = vxpng_bits(state, 4) + 4;

	static const int8_t cl_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

	/* Read the code lengths of the.. code lengths, where unfilled are 0. */
	int16_t lengths[286 + 30];
	for (int index = 0; index < NCODE; ++index) lengths[cl_order[index]] = VX_CAST(int16_t, vxpng_bits(state, 3));
	for (int index = NCODE; index < 19; ++index) lengths[cl_order[index]] = 0;

	/* Use for now. */
	int16_t lens_counts[VX_MAX_CODE_BITS + 1], len_symbols[286];
	vxpng_huffman_state len_codes = { lens_counts, len_symbols };

	vxpng_construct(&len_codes, lengths, 19);

	/* Read length/literal and distance code length tables. */
	for (int index = 0, end = NLEN + NDIST; index < end;) {
		int symbol = vxpng_decode(state, &len_codes);
		if (symbol < 16) {
			lengths[index++] = VX_CAST(int16_t, symbol); /* Length input 0-15. */
			continue;
		}

		const int target_length = (symbol == 16) ? lengths[index - 1] : 0;

		/* Repeat instruction depending on given length. */
		if (symbol == 16) symbol =  3 + vxpng_bits(state, 2); /* Repeat last length  3 - 6   times; length in 2 next bits. */
		else if (symbol == 17) symbol =  3 + vxpng_bits(state, 3); /* Repeat last length  3 - 10  times; length in 3 next bits. */
		else symbol = 11 + vxpng_bits(state, 7); /* Repeat last length 11 - 138 times; length in 7 next bits. */

		/* Repeat last (or zero) length the given number of times. */
		while (symbol--) lengths[index++] = VX_CAST(int16_t, target_length);
	}

	int16_t dist_counts[VX_MAX_CODE_BITS + 1], dist_symbols[30];
	vxpng_huffman_state dist_codes = { dist_counts, dist_symbols };

	/* Construct decoding tables and begin decoding process. */
	vxpng_construct(&len_codes, lengths, NLEN);
	vxpng_construct(&dist_codes, lengths + NLEN, NDIST);
	vxpng_process(state, &len_codes, &dist_codes);
}

/* Original inflate implementation from zlib repo (zlib/contrib/puff). */
static void vxpng_inflate(uint8_t *result, const uint8_t *stream, uint32_t stream_len, uint32_t result_len)
{
	(void)(stream_len);
	(void)(result_len);

	vxpng_inflate_state state;
	memset(&state, 0, sizeof state);
	state.output = result;
	state.input = stream;

	do {
		const int BFINAL = vxpng_bits(&state, 1);
		const int BTYPE = vxpng_bits(&state, 2);

		if (BTYPE == 1) vxpng_fixed(&state);
		else if (BTYPE == 2) vxpng_dynamic(&state);
		else return;

		if (BFINAL) return;
	} while (1);
}

#endif /* defined(VX_EXTERNAL_ZLIB) */


/* Pixel filters removal. */


static uint8_t vxpng_paeth(uint8_t a, uint8_t b, uint8_t c)
{
	const int ia = VX_CAST(int, a), ib = VX_CAST(int, b), ic = VX_CAST(int, c);
	const int p = ia + ib - ic;
	const int pa = p > ia ? p - ia : ia - p;
	const int pb = p > ib ? p - ib : ib - p;
	const int pc = p > ic ? p - ic : ic - p;
	
	if (pa <= pb && pa <= pc) return a;
	else if (pb <= pc) return b;
	else return c;
}

static void vxpng_unfilter(uint8_t *pixel_data, uint32_t width, uint32_t height, uint32_t bytes_per_pixel)
{
	/* Previously decoded scanline (row of pixels).
	   First iteration has no previous scanline (no row 'above'), so starts out by using the empty scanline. */
	const uint8_t *above = pixel_data + (width * height * bytes_per_pixel) + height;
	const uint32_t scanline_width = (width * bytes_per_pixel);
	uint32_t scanline_index = 0u;
	
	do {
		/* First byte of each scanline determines filter method. */
		const uint8_t filter_method = pixel_data[scanline_index * (scanline_width + 1u)];

		/* Storing results in same array to reduce number of allocations. */
		uint8_t *output = pixel_data + (scanline_index * scanline_width);
		/* Actual filtered data - is offset from the output by 1 additional byte each iteration due to the filter byte. */
		const uint8_t *filtered = output + scanline_index + 1u;
		/* Previously decoded pixel. */
		const uint8_t *previous = output - bytes_per_pixel;
		/* Previously decoded pixel, but in the previous/above scanline. */
		const uint8_t *above_previous = above - bytes_per_pixel;

		uint32_t i = 0u;

		switch (filter_method)
		{
			default: break;
			case 0u: /* No filter applied, copy only. */
				memcpy(output, filtered, scanline_width);
				break;
			case 1u: /* 'Sub filter - reconstruct by adding on previously decoded corresponding byte. */
				for (; i < bytes_per_pixel; ++i) output[i] = filtered[i]; /* No previous pixel, only need to copy. */
				for (; i < scanline_width; ++i) output[i] = filtered[i] + previous[i];
				break;
			case 2u: /* Up filter - reconstruct by adding on corresponding decoded byte on the previous scanline. */
				for (; i < scanline_width; ++i) output[i] = filtered[i] + above[i];
				break;
			case 3u: /* Average filter - reconstruct by calculating the average of the above and previous pixels. */
				for (; i < bytes_per_pixel; ++i) output[i] = filtered[i] + (above[i] >> 1u); /* No previous pixel, only need to half pixel in previous scanline. */
				for (; i < scanline_width; ++i) output[i] = filtered[i] + VX_CAST(uint8_t, (previous[i] + above[i]) >> 1u);
				break;
			case 4u: /* Paeth filter - reconstruct by using Paeth predictor function on 3 unique positions (previous, above and above previous). */
				for (; i < bytes_per_pixel; ++i) output[i] = filtered[i]; /* No previous pixel, any value corresponding to a 'previous' pixel is 0. */
				for (; i < scanline_width; ++i) output[i] = filtered[i] + vxpng_paeth(previous[i], above[i], above_previous[i]);
				break;
		}
		
		if (++scanline_index >= height) break;
		above = output;
	} while (1);
}


/* PNG loading logic. */


/* Current PNG state. */
typedef struct png_state
{
	/* First 4 bytes are the type, 'length' bytes of content afterwards. */
	uint8_t *chunk_data;
	/* Length in bytes of the data pointer, excluding the chunk type (4 bytes). */
	uint32_t chunk_length;
	/* Resolution of the image. */
	uint32_t width, height;
	/* Number of bytes that make up each pixel. */
	uint8_t channels, bpp, is_filter, colour_type;
} png_state;

/* Get data of the current chunk from the PNG data. */
static void vxpng_get_next_chunk(char **file_data, png_state *state)
{
	memcpy(&state->chunk_length, *file_data, 4u);
	state->chunk_length = vxpng_swap32(state->chunk_length);
	state->chunk_data = VX_REINT_CAST(uint8_t *, *file_data + 4u);
	*file_data += 4u + 4u + state->chunk_length + 4u;
}

static void vxpng_process_header(png_state *state)
{
	memcpy(&state->width, state->chunk_data + 4u, 4u);
	state->width = vxpng_swap32(state->width); /* Get the image width in correct byte order (4 bytes after chunk type). */
	memcpy(&state->height, state->chunk_data + 8u, 4u);
	state->height = vxpng_swap32(state->height); /* Get the image height in correct byte order (4 bytes after width). */

	/* Assuming 8bpp. */
	const uint8_t colour_types  = state->chunk_data[13u];
	state->channels = VX_CAST(uint8_t,
		(colour_types & 2u ? 3u : 1u) + /* Bit 2 set = RGB (3 channels), otherwise grayscale (1 channel). */
		(colour_types & 4u ? 1u : 0u)   /* Bit 4 set = alpha, otherwise none (1 channel). */
	);
}

int vxpng_load(const char *path, uint8_t **pixels, uint32_t *VX_RESTRICT width, uint32_t *VX_RESTRICT height, int *num_channels)
{
	char *file_data = vxfile_read(path, VX_NULL), *file_stream = file_data + 8;

	/* Verify the file read and the PNG signature. */
	if (!file_data) {
	png_load_fail:
		VX_FREE(file_data);
		return 0;
	} else if (memcmp(file_data, png_signature, 8u)) goto png_load_fail;

	/* Get IHDR information (always first chunk). */
	png_state state;
	vxpng_get_next_chunk(&file_stream, &state);
	vxpng_process_header(&state);

	/* Search for IDAT chunk. */
	do vxpng_get_next_chunk(&file_stream, &state);
	while (memcmp(state.chunk_data, "IDAT", 4u) != 0);

	/* Allocate enough memory for filtered pixels and an extra scanline. */
	const uint32_t pixels_bytes_wfilter = (state.width * state.height * state.channels) + state.height;
	uint8_t *pixel_data = VX_CAST(uint8_t *, malloc(pixels_bytes_wfilter + state.width));
	if (!pixel_data) goto png_load_fail;
	else memset(pixel_data + pixels_bytes_wfilter, 0, state.width);

	/* Decompress data (skipping 4 bytes (chunk type) + 2 bytes (zlib header)), then reverse compression filters. */
	vxpng_inflate(pixel_data, state.chunk_data + 6, state.chunk_length, pixels_bytes_wfilter);
	VX_FREE(file_data);
	vxpng_unfilter(pixel_data, state.width, state.height, state.channels);

	/* Save results into given parameters. */
	*pixels = pixel_data;
	*height = state.height;
	*width = state.width;
	*num_channels = VX_CAST(int, state.channels);
	return 1;
}


/* Zlib compression algorithm. */

#if VX_EXT_ZLIB

static uint32_t vxpng_deflate(uint8_t *output_stream, uint8_t *input_stream, int32_t max_bytes)
{
	z_stream zs_info;
	memset(&zs_info, 0, sizeof zs_info);
	if (deflateInit(&zs_info, Z_DEFAULT_COMPRESSION) != Z_OK) return 0u;
	zs_info.avail_in = VX_CAST(uInt, max_bytes);
	zs_info.next_in = input_stream;
	zs_info.avail_out = VX_CAST(uInt, max_bytes);
	zs_info.next_out = output_stream;
	if (deflate(&zs_info, Z_FINISH) != Z_STREAM_END || deflateEnd(&zs_info) != Z_OK) return 0u;
	return VX_CAST(uint32_t, zs_info.next_out - output_stream);
}

#else

#include "directives/dmath.h"

#define VX_DEFLATE_MAX_OFF (1 << 15)
#define VX_DEFLATE_WIN_MSK (VX_DEFLATE_MAX_OFF - 1)

#define VX_DEFLATE_MIN_MATCH (4)
#define VX_DEFLATE_MAX_MATCH (258)

#define VX_DEFLATE_HASH_BITS (19u)

static const uint8_t png_mirror[256] = {
	#define R2(n) n, n + 128, n + 64, n + 192
	#define R4(n) R2(n), R2(n + 32), R2(n + 16), R2(n + 48)
	#define R6(n) R4(n), R4(n +  8), R4(n +  4), R4(n + 12)
	R6(0), R6(2), R6(1), R6(3)
	#undef R2
	#undef R4
	#undef R6
};

typedef struct vxpng_deflate_info
{
	int32_t bits, bits_count;
	int32_t tbl[(1u << VX_DEFLATE_HASH_BITS)];
	int32_t previous_window[VX_DEFLATE_MAX_OFF];
} vxpng_deflate_info;

static int32_t vxpng_deflate_ceilpow2(int32_t val)
{
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return ++val;
}

static int32_t vxpng_deflate_ilog2(int32_t val)
{
	if (val <= 0) return -1;
	int32_t largest_bit_index = 0;
	while (val >>= 1) ++largest_bit_index;
	return largest_bit_index;
}

static uint32_t vxpng_deflate_getu32(const void *stream)
{
	uint32_t val;
	memcpy(&val, stream, sizeof(val));
	return val;
}

static uint32_t vxpng_deflate_gethash32(const void *stream)
{
	return (vxpng_deflate_getu32(stream) * 0x9E377989u) >> (32u - VX_DEFLATE_HASH_BITS);
}

static void vxpng_deflate_write(uint8_t **output, vxpng_deflate_info *deflate_info, int32_t code, int32_t bit_count)
{
	deflate_info->bits |= (code << deflate_info->bits_count);
	deflate_info->bits_count += bit_count;

	while (deflate_info->bits_count >= 8) {
		**output = VX_CAST(uint8_t, deflate_info->bits & 0xFF);
		deflate_info->bits >>= 8;
		deflate_info->bits_count -= 8;
		++output;
	}
}

static void vxpng_deflate_match(uint8_t **stream, vxpng_deflate_info *deflate_info, int32_t dist, int32_t len)
{
	static const int16_t length_extra_min[] = { 0, 11, 19, 35, 67, 131 };
	static const int16_t distance_extra_min[] = { 0, 6, 12, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144, 12288, 24576 };
	static const int16_t length_min[] = { 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227 };
	static const int16_t distance_min[] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };

	/* Encode extra length bits. */
	int32_t lc = len;
	int32_t lx = vxpng_deflate_ilog2(len - 3) - 2;
	if (!(lx = (lx < 0) ? 0 : lx)) lc += 254;
	else if (len >= 258) { lx = 0; lc = 285; }
	else lc = ((lx - 1) << 2) + 265 + ((len - length_extra_min[lx]) >> lx);

	if (lc <= 279) vxpng_deflate_write(stream, deflate_info, png_mirror[(lc - 256) << 1], 7);
	else vxpng_deflate_write(stream, deflate_info, png_mirror[0xC0 - 280 + lc], 8);
	if (lx) vxpng_deflate_write(stream, deflate_info, len - length_min[lc - 265], lx);

	/* Encode extra distance bits. */
	int32_t dc = dist - 1;
	int32_t dx = vxpng_deflate_ilog2(vxpng_deflate_ceilpow2(dist) >> 2);
	if ((dx = (dx < 0) ? 0 : dx)) dc = ((dx + 1) << 1) + (dist > distance_extra_min[dx]);
	vxpng_deflate_write(stream, deflate_info, png_mirror[dc << 3], 5);
	if (dx) vxpng_deflate_write(stream, deflate_info, dist - distance_min[dc], dx);
}

static void vxpng_deflate_literal(uint8_t **stream, vxpng_deflate_info *deflate_info, int32_t c)
{
	if (c <= 143) vxpng_deflate_write(stream, deflate_info, png_mirror[0x30 + c], 8);
	else vxpng_deflate_write(stream, deflate_info, 1 + 2 * png_mirror[0x90 - 144 + c], 9);
}

static uint32_t vxpng_deflate(uint8_t *deflate_result, const uint8_t *data_stream, int32_t max_bytes)
{
	uint8_t *deflate_stream = deflate_result;

	vxpng_deflate_info *deflate_info = VX_CAST(vxpng_deflate_info *, malloc(sizeof(vxpng_deflate_info)));
	if (!deflate_info) return 0u;

	memset(deflate_info->tbl, -1, sizeof deflate_info->tbl);
	deflate_info->bits = deflate_info->bits_count = 0;

	/* Final block, since only using one. */
	vxpng_deflate_write(&deflate_stream, deflate_info, 0x01, 1);
	/* Using static Huffman codes. */
	vxpng_deflate_write(&deflate_stream, deflate_info, 0x01, 2);

	int32_t written_bytes = 0;
	while (written_bytes < max_bytes) {
		int32_t run, best_length = 0, dist = 0;
		const int32_t max_match = VX_INT_MIN(max_bytes - written_bytes, VX_DEFLATE_MAX_MATCH);

		if (max_match > VX_DEFLATE_MIN_MATCH) {
			const int32_t limit = VX_INT_MAX(written_bytes - VX_DEFLATE_MAX_OFF, -1);
			int32_t i = deflate_info->tbl[vxpng_deflate_gethash32(data_stream + written_bytes)];

			while (i > limit) {
				if (data_stream[i + best_length] == data_stream[written_bytes + best_length] &&
				   (vxpng_deflate_getu32(&data_stream[i]) == vxpng_deflate_getu32(&data_stream[written_bytes]))
				) {
					int32_t val = VX_DEFLATE_MIN_MATCH;
					while (val < max_match && data_stream[i + val] == data_stream[written_bytes + val]) ++val;
					
					if (val <= best_length) continue;
					best_length = val;
					dist = written_bytes - i;
					if (val == max_match) break;
				}

				i = deflate_info->previous_window[i & VX_DEFLATE_WIN_MSK];
			}
		}

		if (best_length >= VX_DEFLATE_MIN_MATCH) {
			vxpng_deflate_match(&deflate_stream, deflate_info, dist, best_length);
			run = best_length;
		} else {
			vxpng_deflate_literal(&deflate_stream, deflate_info, data_stream[written_bytes]);
			run = 1;
		}

		while (run-- != 0) {
			const uint32_t h = vxpng_deflate_gethash32(&data_stream[written_bytes]);
			deflate_info->previous_window[written_bytes & VX_DEFLATE_WIN_MSK] = deflate_info->tbl[h];
			deflate_info->tbl[h] = written_bytes++;
		}
	}

	vxpng_deflate_write(&deflate_stream, deflate_info, 0, 7);
	vxpng_deflate_write(&deflate_stream, deflate_info, 2, 10);
	vxpng_deflate_write(&deflate_stream, deflate_info, 2, 3);

	return VX_CAST(uint32_t, deflate_stream - deflate_result);
}

#endif /* defined(VX_EXTERNAL_ZLIB) */


/* PNG saving logic. */


/* Cyclic Redundancy Check lookup values. */
static const uint32_t crc_table[256] = {
	0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu, 0x076DC419u, 0x706AF48Fu, 0xE963A535u, 0x9E6495A3u,
	0x0EDB8832u, 0x79DCB8A4u, 0xE0D5E91Eu, 0x97D2D988u, 0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u, 0x90BF1D91u,
	0x1DB71064u, 0x6AB020F2u, 0xF3B97148u, 0x84BE41DEu, 0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u,
	0x136C9856u, 0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu, 0x14015C4Fu, 0x63066CD9u, 0xFA0F3D63u, 0x8D080DF5u,
	0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u, 0xA2677172u, 0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu,
	0x35B5A8FAu, 0x42B2986Cu, 0xDBBBC9D6u, 0xACBCF940u, 0x32D86CE3u, 0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u,
	0x26D930ACu, 0x51DE003Au, 0xC8D75180u, 0xBFD06116u, 0x21B4F4B5u, 0x56B3C423u, 0xCFBA9599u, 0xB8BDA50Fu,
	0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u, 0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du,
	0x76DC4190u, 0x01DB7106u, 0x98D220BCu, 0xEFD5102Au, 0x71B18589u, 0x06B6B51Fu, 0x9FBFE4A5u, 0xE8B8D433u,
	0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu, 0xE10E9818u, 0x7F6A0DBBu, 0x086D3D2Du, 0x91646C97u, 0xE6635C01u,
	0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu, 0x6C0695EDu, 0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u,
	0x65B0D9C6u, 0x12B7E950u, 0x8BBEB8EAu, 0xFCB9887Cu, 0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u, 0xFBD44C65u,
	0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u, 0x4ADFA541u, 0x3DD895D7u, 0xA4D1C46Du, 0xD3D6F4FBu,
	0x4369E96Au, 0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u, 0x44042D73u, 0x33031DE5u, 0xAA0A4C5Fu, 0xDD0D7CC9u,
	0x5005713Cu, 0x270241AAu, 0xBE0B1010u, 0xC90C2086u, 0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
	0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u, 0x59B33D17u, 0x2EB40D81u, 0xB7BD5C3Bu, 0xC0BA6CADu,
	0xEDB88320u, 0x9ABFB3B6u, 0x03B6E20Cu, 0x74B1D29Au, 0xEAD54739u, 0x9DD277AFu, 0x04DB2615u, 0x73DC1683u,
	0xE3630B12u, 0x94643B84u, 0x0D6D6A3Eu, 0x7A6A5AA8u, 0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u,
	0xF00F9344u, 0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu, 0xF762575Du, 0x806567CBu, 0x196C3671u, 0x6E6B06E7u,
	0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au, 0x67DD4ACCu, 0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u,
	0xD6D6A3E8u, 0xA1D1937Eu, 0x38D8C2C4u, 0x4FDFF252u, 0xD1BB67F1u, 0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu,
	0xD80D2BDAu, 0xAF0A1B4Cu, 0x36034AF6u, 0x41047A60u, 0xDF60EFC3u, 0xA867DF55u, 0x316E8EEFu, 0x4669BE79u,
	0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u, 0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu,
	0xC5BA3BBEu, 0xB2BD0B28u, 0x2BB45A92u, 0x5CB36A04u, 0xC2D7FFA7u, 0xB5D0CF31u, 0x2CD99E8Bu, 0x5BDEAE1Du,
	0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu, 0x026D930Au, 0x9C0906A9u, 0xEB0E363Fu, 0x72076785u, 0x05005713u,
	0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u, 0x92D28E9Bu, 0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u,
	0x86D3D2D4u, 0xF1D4E242u, 0x68DDB3F8u, 0x1FDA836Eu, 0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u, 0x18B74777u,
	0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu, 0x8F659EFFu, 0xF862AE69u, 0x616BFFD3u, 0x166CCF45u,
	0xA00AE278u, 0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u, 0xA7672661u, 0xD06016F7u, 0x4969474Du, 0x3E6E77DBu,
	0xAED16A4Au, 0xD9D65ADCu, 0x40DF0B66u, 0x37D83BF0u, 0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
	0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u, 0xBAD03605u, 0xCDD70693u, 0x54DE5729u, 0x23D967BFu,
	0xB3667A2Eu, 0xC4614AB8u, 0x5D681B02u, 0x2A6F2B94u, 0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu, 0x2D02EF8Du
};

/* Calculate CRC value for given data. */
static uint32_t vxpng_calc_crc(const uint8_t *data, size_t bytes)
{
	uint32_t crc = 0xFFFFFFFFu;
	while (bytes--) crc = crc_table[(crc ^ *data++) & 0xFFu] ^ (crc >> 8u);
	return crc ^ 0xFFFFFFFFu;
}

/* Copy data to stream and move forwards. */
static void vxpng_memcpy_advance(uint8_t **stream, const void *data, size_t bytes)
{
	memcpy(*stream, data, bytes);
	*stream += bytes;
}

int vxpng_save(const char *path, const uint8_t *pixels, uint32_t width, uint32_t height)
{
	if (!width || !height || !pixels) return 0;

	const uint32_t filtered_pixels_bytes = (width * height * 3u) + height;

	/* Chunk definitions. */

	#define BYTE(n, index) VX_CAST(uint8_t, (((n) >> index) & 0xffu))
	const uint8_t png_ihdr_chunk_nocrc[21] = {
		'\0', '\0', '\0', '\xd', /* 13 bytes of content. */
		'I', 'H', 'D', 'R', /* Identify as an IHDR chunk. */
		BYTE(width, 24),  BYTE(width, 16),  BYTE(width, 8),  BYTE(width, 0), /* 4 bytes of width. */
		BYTE(height, 24), BYTE(height, 16), BYTE(height, 8), BYTE(height, 0), /* 4 bytes of height. */
		'\x8', '\x2', '\x0', '\x0', '\x0' /* Image format (byte 1: 8bpp, byte 2: truecolour/RGB). */
	};
	const uint8_t png_idat_start[8] = {
		'\0', '\0', '\0', '\0', /* Content size, currently unknown. */
		'I', 'D', 'A', 'T', /* Identify as an IDAT chunk. */
	};
	#undef BYTE
	const char png_iend_chunk[12] = { '\0', '\0', '\0', '\0', 'I', 'E', 'N', 'D', '\xae', '\x42', '\x60', '\x82' };

	/* File creation of maximum amount of data possible. 63 extra bytes for signature and chunk metadata. */
	uint8_t *file_result = VX_CAST(uint8_t *, malloc(63u + filtered_pixels_bytes)), *stream = file_result;
	if (!file_result) {
	png_save_fail:
		VX_FREE(file_result);
		return 0;
	}

	/* Signature and IHDR. */
	vxpng_memcpy_advance(&stream, png_signature, 8u);
	vxpng_memcpy_advance(&stream, png_ihdr_chunk_nocrc, sizeof png_ihdr_chunk_nocrc);
	const uint32_t ihdr_crc_value = vxpng_swap32(vxpng_calc_crc(png_ihdr_chunk_nocrc + 4, (sizeof png_ihdr_chunk_nocrc) - 4u));
	vxpng_memcpy_advance(&stream, &ihdr_crc_value, sizeof ihdr_crc_value);

	/* IDAT header and data calculation. */
	uint8_t *const idat_start_ptr = stream;
	vxpng_memcpy_advance(&stream, png_idat_start, sizeof png_idat_start);

	/* Create pixel data that includes scanline filter information at the start of each row/scanline. */
	uint8_t *filtered_pixels = VX_CAST(uint8_t *, malloc(filtered_pixels_bytes));
	if (!filtered_pixels) goto png_save_fail;
	for (uint32_t scanline = 0u; scanline < height; ++scanline) {
		const uint32_t filter_byte_index = (scanline * width * 3u) + scanline;
		filtered_pixels[filter_byte_index] = 0u;
		memcpy(filtered_pixels + filter_byte_index + 1u, pixels + (width * scanline * 3u), width * 3u);
	}

	const uint32_t compressed_data_bytes = vxpng_deflate(stream, filtered_pixels, VX_CAST(int32_t, filtered_pixels_bytes));
	VX_FREE(filtered_pixels);
	if (!compressed_data_bytes) goto png_save_fail;
	stream += compressed_data_bytes;

	/* Calculate and add CRC checksum of IDAT name (4) and compressed data (variable). */
	const uint32_t idat_crc_value = vxpng_swap32(vxpng_calc_crc(idat_start_ptr + 4, 4u + compressed_data_bytes));
	vxpng_memcpy_advance(&stream, &idat_crc_value, sizeof idat_crc_value);

	/* Write size of IDAT content now that it is known. */
	const uint32_t num_written_endian = vxpng_swap32(compressed_data_bytes);
	memcpy(idat_start_ptr, &num_written_endian, sizeof num_written_endian);

	/* IEND header. */
	vxpng_memcpy_advance(&stream, png_iend_chunk, sizeof png_iend_chunk);

	if (vxfile_write(path, file_result, VX_CAST(size_t, stream - file_result)) == 0) goto png_save_fail;

	VX_FREE(file_result);
	return 1;
}
