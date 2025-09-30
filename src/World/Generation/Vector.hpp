#pragma once
#ifndef _SOURCE_GENERATION_VECTOR_HDR_
#define _SOURCE_GENERATION_VECTOR_HDR_

#include <type_traits>
#include <limits.h>
#include <limits>
#include <math.h>

// Mathematical functions
namespace math {
	template<typename T> using lims = std::numeric_limits<T>;
	template<typename T, size_t N> constexpr size_t size(T(&)[N]) noexcept { return N; }

	template<typename T> constexpr T   abs   (T val) noexcept { return val < 0 ? -val : val; }
	template<typename T> constexpr int sign  (T val) noexcept { return val < 0 ? -1   :   1; }
	template<typename T> constexpr int sign01(T val) noexcept { return val < 0 ?  0   :   1; }

	template<typename T> constexpr int bits(T val, int bits_count = 0) noexcept {
		return val != 0 ? bits(val / 2, bits_count + 1) : bits_count;
	}
	constexpr int bitwise_ind(int index, int res = 1) {
		return index <= 0 ? res : bitwise_ind(index - 1, res * 2); 
	};

	inline int abs(int val) noexcept {
		const int mask = val >> ((sizeof(int) * CHAR_BIT) - 1);
		return (val + mask) ^ mask;
	}

	template<typename T> T fract(T val) noexcept { return abs(val - trunc(val)); }

	template<typename T> constexpr T in_range_incl(T val, T min, T max) noexcept { return val >= min && val <= max; }
	template<typename T> constexpr T in_range_excl(T val, T min, T max) noexcept { return val >  min && val <  max; }

	template<typename T> constexpr T min(T a, T b) noexcept { return a < b ? a : b; }
	template<typename T> constexpr T max(T a, T b) noexcept { return a > b ? a : b; }
	template<typename T> constexpr T clamp(T val, T min, T max) noexcept { return val < min ? min : val > max ? max : val; }
	template<typename A, typename B> constexpr A max_conv(A a, B b) noexcept { return a > static_cast<A>(b) ? a : static_cast<A>(b); }

	template<typename T> T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }

	template<typename T> constexpr T round_up_to_x(T val, T x) noexcept { return val + (x - (val % x)); }
	template<typename T> constexpr T round_down_to_x(T val, T x) noexcept { return val - (val % x); }

	template<typename T> constexpr T sqrt_impl(T x, T c, T p = T{}) noexcept {
		return (c == p) ? c : sqrt_impl(x, static_cast<T>(0.5f) * (c + x / c), c);
	}
	template<typename T> constexpr T cxsqrt(T x) noexcept { return sqrt_impl(x, x); }

	template<typename T> constexpr T cxpythagoras(T a, T b) noexcept { return cxsqrt(a * a + b * b); }

	template<typename T> constexpr T pi() noexcept {
		return static_cast<T>(3.141592653589793238462643383279502884197169399);
	}
	template<typename T> constexpr T half_pi() noexcept { return pi<T>() / T(2); }
	template<typename T> constexpr T two_pi() noexcept { return pi<T>() * T(2); }

	template<typename T> constexpr T radians(T degrees) noexcept {
		return degrees * static_cast<T>(0.017453292519943295769236907684886127134428718);
	}
	template<typename T> constexpr T degrees(T radians) noexcept {
		return radians * static_cast<T>(57.29577951308232087679815481410517033240547246);
	}
}

#define VOXEL_ASSERT_VECTOR_TYPE static_assert(\
	std::is_integral<T>::value || std::is_floating_point<T>::value,\
	"Must be either an integral or decimal type")\

// Vector struct templates and definitions

template <typename T, int L>
struct vector {
	VOXEL_ASSERT_VECTOR_TYPE;
	static_assert(L > 1, "Length must be larger than 1");
	static_assert(L < 5, "No definitions for vectors of size >4");
};

template <typename T>
struct vector<T, 2> {
	typedef vector<T, 2> vec_t;
	typedef vec_t &ref_t;
	typedef const vec_t &cref_t;

	T x, y;

	constexpr vector() noexcept : x(static_cast<T>(0)), y(x) {}
	constexpr explicit vector(T scalar) noexcept : x(scalar), y(scalar) {}
	constexpr vector(cref_t other) noexcept : x(other.x), y(other.y) {}
	template<typename X, typename Y> constexpr vector(X _x, Y _y) noexcept
	  : x(static_cast<T>(_x)), y(static_cast<T>(_y)) {}
	template<typename O> constexpr vector(const vector<O, 2> &other) noexcept
	  : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}
	
	T &operator[](int index) noexcept { return index ? y : x; }
	const T &operator[](int index) const noexcept { return index ? y : x; }

	ref_t operator=(cref_t rhs) noexcept { x = rhs.x; y = rhs.y; return *this; }
	template<typename O> ref_t operator=(const vector<O, 2> &rhs) {
		x = static_cast<T>(rhs.x);
		y = static_cast<T>(rhs.y);
		return *this;
	}

	bool operator!=(cref_t rhs) const noexcept { return x != rhs.x || y != rhs.y; }
	bool operator==(cref_t rhs) const noexcept { return x == rhs.x && y == rhs.y; }

	vec_t operator-() const noexcept { return vec_t(-x, -y); }

	vec_t operator+(cref_t rhs) const noexcept { return { x + rhs.x, y + rhs.y }; }
	vec_t operator-(cref_t rhs) const noexcept { return { x - rhs.x, y - rhs.y }; }
	ref_t operator+=(cref_t rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
	ref_t operator-=(cref_t rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }

	template<typename I> vec_t operator*(I scalar) const noexcept { return { x * scalar, y * scalar }; }
	template<typename I> vec_t operator/(I scalar) const noexcept { return { x / scalar, y / scalar }; }
	template<typename I> ref_t operator*=(I scalar) noexcept { x *= scalar; y *= scalar; return *this; }
	template<typename I> ref_t operator/=(I scalar) noexcept { x /= scalar; y /= scalar; return *this; }
	
	T dot(cref_t other) const noexcept { return x * other.x + y * other.y; }
	T sqr_length() const noexcept { return dot(*this); }
	T length() const noexcept { return sqrt(sqr_length()); }

	vec_t c_lerp(cref_t b, float t) const noexcept { return vec_t(math::lerp(x, b.x, t), math::lerp(y, b.y, t)); }
	ref_t to_lerp(cref_t b, float t) noexcept { x = math::lerp(x, b.x, t); y = math::lerp(y, b.y, t); return *this; }

	vec_t c_unit() const noexcept { return *this / length(); }
	ref_t to_unit() const noexcept { *this /= length(); return *this; }
};

template <typename T>
struct vector<T, 3> {
	typedef vector<T, 3> vec_t;
	typedef vec_t &ref_t;
	typedef const vec_t &cref_t;

	T x, y, z;

	constexpr vector() noexcept : x(static_cast<T>(0)), y(x), z(x) {}
	constexpr explicit vector(T scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
	constexpr vector(cref_t other) noexcept : x(other.x), y(other.y), z(other.z) {}
	template<typename X, typename Y, typename Z> constexpr vector(X _x, Y _y, Z _z) noexcept
	  : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
	template<typename O> constexpr vector(const vector<O, 3> &other) noexcept
	  : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

	T &operator[](int index) noexcept { return !index ? x : index == 1 ? y : z; }
	const T &operator[](int index) const noexcept { return !index ? x : index == 1 ? y : z; }

	ref_t operator=(cref_t rhs) noexcept { x = rhs.x; y = rhs.y; z = rhs.z; return *this; }
	template<typename O> ref_t operator=(const vector<O, 3> &rhs) {
		x = static_cast<T>(rhs.x);
		y = static_cast<T>(rhs.y);
		z = static_cast<T>(rhs.z);
		return *this;
	}

	bool operator!=(cref_t rhs) const noexcept { return x != rhs.x || y != rhs.y || z != rhs.z; }
	bool operator==(cref_t rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }

	vec_t operator-() const noexcept { return vec_t(-x, -y, -z); }

	vec_t operator+(cref_t rhs) const noexcept { return { x + rhs.x, y + rhs.y, z + rhs.z }; }
	vec_t operator-(cref_t rhs) const noexcept { return { x - rhs.x, y - rhs.y, z - rhs.z }; }
	ref_t operator+=(cref_t rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	ref_t operator-=(cref_t rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }

	template<typename I> vec_t operator*(I i) const noexcept { return { x * i, y * i, z * i }; }
	template<typename I> vec_t operator/(I i) const noexcept { return { x / i, y / i, z / i }; }
	template<typename I> ref_t operator*=(I i) noexcept { x *= i; y *= i; z *= i; return *this; }
	template<typename I> ref_t operator/=(I i) noexcept { x /= i; y /= i; z /= i; return *this; }

	T dot(cref_t other) const noexcept { return x * other.x + y * other.y + z * other.z; }
	T sqr_length() const noexcept { return dot(*this); }
	T length() const noexcept { return sqrt(sqr_length()); }

	vec_t c_lerp(cref_t b, float t) const noexcept {
		return vec_t(math::lerp(x, b.x, t), math::lerp(y, b.y, t), math::lerp(z, b.z, t));
	}
	ref_t to_lerp(cref_t b, float t) noexcept {
		x = math::lerp(x, b.x, t);
		y = math::lerp(y, b.y, t);
		z = math::lerp(z, b.z, t);
		return *this;
	}

	vec_t c_unit() const noexcept { return *this / length(); }
	ref_t to_unit() noexcept { *this /= length(); return *this; }

	vec_t c_cross(cref_t b) const noexcept { return vec_t(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
	ref_t to_cross(cref_t b) noexcept {
		const T ox = x, oy = y;
		x = y  * b.z - z  * b.y;
		y = z  * b.x - ox * b.z;
		z = ox * b.y - oy * b.x;
		return *this;
	}

	vector<T, 2> xz() const noexcept { return vector<T, 2>(x, z); }
};

template <typename T>
struct vector<T, 4> {
	typedef vector<T, 4> vec_t;
	typedef vec_t &ref_t;
	typedef const vec_t &cref_t;

	T x, y, z, w;

	constexpr vector() noexcept : x(static_cast<T>(0)), y(x), z(x), w(x) {}
	constexpr explicit vector(T scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}
	constexpr vector(cref_t other) noexcept : x(other.x), y(other.y), z(other.z), w(other.w) {}
	template<typename O> constexpr vector(const vector<O, 4> &other) noexcept
	  : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}
	template<typename A, typename B> constexpr vector(vector<A, 2> a, vector<B, 2> b) noexcept
	  : x(static_cast<T>(a.x)), y(static_cast<T>(a.y)), z(static_cast<T>(b.x)), w(static_cast<T>(b.y)) {}
	template<typename X, typename Y, typename Z, typename W> constexpr vector(X _x, Y _y, Z _z, W _w) noexcept
	  : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)), w(static_cast<T>(_w)) {}
	
	T &operator[](int index) noexcept { return !index ? x : index == 1 ? y : index == 2 ? z : w; }
	const T &operator[](int index) const noexcept { return !index ? x : index == 1 ? y : index == 2 ? z : w; }

	ref_t operator=(cref_t rhs) noexcept { x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w; return *this; }
	template<typename O> ref_t operator=(const vector<O, 4> &rhs) {
		x = static_cast<T>(rhs.x);
		y = static_cast<T>(rhs.y);
		z = static_cast<T>(rhs.z);
		w = static_cast<T>(rhs.w);
		return *this;
	}
	template<typename O> ref_t operator=(const vector<O, 3> &rhs) {
		x = static_cast<T>(rhs.x);
		y = static_cast<T>(rhs.y);
		z = static_cast<T>(rhs.z);
		return *this;
	}
	template<typename O> ref_t operator=(const vector<O, 2> &rhs) {
		x = static_cast<T>(rhs.x);
		y = static_cast<T>(rhs.y);
		return *this;
	}

	bool operator!=(cref_t rhs) const noexcept { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }
	bool operator==(cref_t rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

	vec_t operator-() const noexcept { return vec_t(-x, -y, -z, -w); }

	vec_t operator+(cref_t rhs) const noexcept { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
	vec_t operator-(cref_t rhs) const noexcept { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
	ref_t operator+=(cref_t rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	ref_t operator-=(cref_t rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
	
	template<typename I> vec_t operator*(I i) const noexcept { return { x * i, y * i, z * i, w * i }; }
	template<typename I> vec_t operator/(I i) const noexcept { return { x / i, y / i, z / i, w / i }; }
	template<typename I> ref_t operator*=(I i) noexcept { x *= i; y *= i; z *= i; w *= i; return *this; }
	template<typename I> ref_t operator/=(I i) noexcept { x /= i; y /= i; z /= i; w /= i; return *this; }

	T dot(cref_t other) const noexcept { return x * other.x + y * other.y + z * other.z + w * other.w; }
	T sqr_length() const noexcept { return dot(*this); }
	T length() const noexcept { return sqrt(sqr_length()); }

	vec_t c_lerp(cref_t b, float t) const noexcept {
		return vec_t(math::lerp(x, b.x, t), math::lerp(y, b.y, t), math::lerp(z, b.z, t), math::lerp(w, b.w, t));
	}
	ref_t to_lerp(cref_t b, float t) noexcept {
		x = math::lerp(x, b.x, t);
		y = math::lerp(y, b.y, t);
		z = math::lerp(z, b.z, t);
		w = math::lerp(w, b.w, t);
		return *this;
	}

	vec_t c_unit() const noexcept { return *this / length(); }
	ref_t to_unit() const noexcept { *this /= length(); return *this; }
};

struct vec_hash {
	template<typename T> size_t operator()(vector<T, 2> vec) const noexcept {
		return static_cast<size_t>(vec.x ^ (vec.y ^ (vec.x >> 1)));
	}
	template<typename T> size_t operator()(vector<T, 3> vec) const noexcept {
		return static_cast<size_t>(vec.x ^ (((vec.y << 1) ^ (vec.z << 1)) >> 1));
	}
	template<typename T> size_t operator()(vector<T, 4> vec) const noexcept {
		return static_cast<size_t>(vec.x ^ (((vec.y << 1) ^ ((vec.z << 1)) ^ vec.w) >> 1));
	}
};

// Vector typedefs
// Default integer types
typedef vector<int, 2> vector2i;
typedef vector<int, 3> vector3i;
typedef vector<int, 4> vector4i;

typedef vector<unsigned, 2> vector2ui;
typedef vector<unsigned, 3> vector3ui;
typedef vector<unsigned, 4> vector4ui;

// 8-bit types
typedef vector<int8_t, 2> vector2i8;
typedef vector<int8_t, 3> vector3i8;
typedef vector<int8_t, 4> vector4i8;

typedef vector<uint8_t, 2> vector2ui8;
typedef vector<uint8_t, 3> vector3ui8;
typedef vector<uint8_t, 4> vector4ui8;

// 16-bit types
typedef vector<int16_t, 2> vector2i16;
typedef vector<int16_t, 3> vector3i16;
typedef vector<int16_t, 4> vector4i16;

typedef vector<uint16_t, 2> vector2iu16;
typedef vector<uint16_t, 3> vector3iu16;
typedef vector<uint16_t, 4> vector4iu16;

// 32-bit types
typedef vector<int32_t, 2> vector2i32;
typedef vector<int32_t, 3> vector3i32;
typedef vector<int32_t, 4> vector4i32;

typedef vector<uint32_t, 2> vector2iu32;
typedef vector<uint32_t, 3> vector3iu32;
typedef vector<uint32_t, 4> vector4iu32;

// 64-bit types
typedef vector<int64_t, 2> vector2i64;
typedef vector<int64_t, 3> vector3i64;
typedef vector<int64_t, 4> vector4i64;

typedef vector<uint64_t, 2> vector2iu64;
typedef vector<uint64_t, 3> vector3iu64;
typedef vector<uint64_t, 4> vector4iu64;

// Pointer-length types
typedef vector<size_t, 2> vector2sz;
typedef vector<size_t, 3> vector3sz;
typedef vector<size_t, 4> vector4sz;

// Floating-point types
typedef vector<float, 2> vector2f;
typedef vector<float, 3> vector3f;
typedef vector<float, 4> vector4f;

typedef vector<double, 2> vector2d;
typedef vector<double, 3> vector3d;
typedef vector<double, 4> vector4d;


// Matrix struct template and definitions

template <typename T, int R, int C>
struct matrix { VOXEL_ASSERT_VECTOR_TYPE; };

// Currently no need for other sized matrices (e.g. 3x3), may be added if there is.

template<typename T>
struct matrix<T, 4, 4> {
	typedef matrix<T, 4, 4> mat_t;
	typedef vector<T, 4> in_vec_t;

	typedef mat_t &ref_t;
	typedef const mat_t &cref_t;

	in_vec_t vx, vy, vz, vw;
	
	constexpr matrix() noexcept : vx(static_cast<T>(0)), vy(vx), vz(vx), vw(vx) {}
	constexpr matrix(cref_t other) noexcept : vx(other.vx), vy(other.vy), vz(other.vz), vw(other.vw) {}
	ref_t operator=(cref_t rhs) noexcept { vx = rhs.vx; vy = rhs.vy; vz = rhs.vz; vw = rhs.vw; return *this; }

	mat_t operator*(cref_t rhs) noexcept {
		mat_t result;
		result.vx = (vx * rhs.vx.x) + (vy * rhs.vx.y) + (vz * rhs.vx.z) + (vw * rhs.vx.w);
		result.vy = (vx * rhs.vy.x) + (vy * rhs.vy.y) + (vz * rhs.vy.z) + (vw * rhs.vy.w);
		result.vz = (vx * rhs.vz.x) + (vy * rhs.vz.y) + (vz * rhs.vz.z) + (vw * rhs.vz.w);
		result.vw = (vx * rhs.vw.x) + (vy * rhs.vw.y) + (vz * rhs.vw.z) + (vw * rhs.vw.w);
		return result;
	}

	in_vec_t &operator[](int index) noexcept { return !index ? vx : index == 1 ? vy : index == 2 ? vz : vw; };
	const in_vec_t &operator[](int index) const noexcept { return !index ? vx : index == 1 ? vy : index == 2 ? vz : vw; }

	static mat_t look_at(vector<T, 3> eye, vector<T, 3> center, vector<T, 3> up) noexcept {
		const vector<T, 3> f = (center - eye).to_unit();
		const vector<T, 3> s = f.c_cross(up).to_unit();
		const vector<T, 3> u = s.c_cross(f);

		mat_t result;
		result.vx.x =  s.x;
		result.vy.x =  s.y;
		result.vz.x =  s.z;
		result.vw.x = -s.dot(eye);

		result.vx.y =  u.x;
		result.vy.y =  u.y;
		result.vz.y =  u.z;
		result.vw.y = -u.dot(eye);

		result.vx.z = -f.x;
		result.vy.z = -f.y;
		result.vz.z = -f.z;
		result.vw.z =  f.dot(eye);

		result.vw.w = static_cast<T>(1);

		return result;
	}

	static mat_t rotate(const mat_t &mat, T angle, const vector<T, 3> &axis) noexcept {
		const T c = cos(angle);
		const T s = sin(angle);
		
		const vector<T, 3> ax = axis.c_unit();
		const vector<T, 3> ot = axis * (static_cast<T>(1) - c);

		mat_t rot;
		rot.vx.x = fma(ax.x, ot.x, c);
		rot.vx.y = ot.x * ax.y + s * ax.z;
		rot.vx.z = ot.x * ax.z - s * ax.y;

		rot.vy.x = ot.y * ax.x - s * ax.z;
		rot.vy.y = fma(ax.y, ot.y, c);
		rot.vy.z = ot.y * ax.z + s * ax.x;

		rot.vz.x = ot.z * ax.x + s * ax.y;
		rot.vz.y = ot.z * ax.y - s * ax.x;
		rot.vz.z = fma(ax.z, ot.z, c);

		mat_t result;
		result.vx = (mat.vx * rot.vx.x) + (mat.vy * rot.vx.y) + (mat.vz * rot.vx.z);
		result.vy = (mat.vx * rot.vy.x) + (mat.vy * rot.vy.y) + (mat.vz * rot.vy.z);
		result.vz = (mat.vx * rot.vz.x) + (mat.vy * rot.vz.y) + (mat.vz * rot.vz.z);
		result.vw = mat.vw;
		return result;
	}

	static mat_t perspective(T fov_y, T aspect, T near_plane_dist, T far_plane_dist) noexcept {
		const T half_tangent = tan(fov_y / static_cast<T>(2));

		mat_t result{};
		result.vx.x = static_cast<T>(1) / (aspect * half_tangent);
		result.vy.y = static_cast<T>(1) / (half_tangent);
		result.vz.z = -(far_plane_dist + near_plane_dist) / (far_plane_dist - near_plane_dist);
		result.vz.w = -static_cast<T>(1);
		result.vw.z = -(static_cast<T>(2) * far_plane_dist * near_plane_dist) / (far_plane_dist - near_plane_dist);

		return result;
	}
};

typedef matrix<float, 4, 4> matrixf4x4;

#endif // _SOURCE_GENERATION_VECTOR_HDR_
