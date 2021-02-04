
#pragma once
#include <stdint.h>

// TODO: Later split into separate files for Vectors/Matrices

struct Vector2 {
	union {
		float v[2];
		struct {
			float x;
			float y;
		};
	};

	Vector2():
		x(0), y(0) {

	}

	Vector2(float x, float y) :
		x(x), y(y) {

	}

	Vector2 operator-(Vector2 v) {
		Vector2 result;

		result.x = this->x - v.x;
		result.y = this->y - v.y;

		return result;
	}

	Vector2 operator+(Vector2 v) {
		Vector2 result;

		result.x = this->x + v.x;
		result.y = this->y + v.y;

		return result;
	}

	Vector2 operator/(float x) {
		Vector2 result;

		result.x = this->x / x;
		result.y = this->y / x;

		return result;
	}

	Vector2 operator*(float x) {
		Vector2 result;

		result.x = this->x * x;
		result.y = this->y * x;

		return result;
	}

	Vector2& operator+=(Vector2 v) {
		this->x += v.x;
		this->y += v.y;

		return *this;
	}

	Vector2& operator-=(Vector2 v) {
		this->x -= v.x;
		this->y -= v.y;

		return *this;
	}

	Vector2& operator*=(float x) {
		this->x *= x;
		this->y *= x;

		return *this;
	}

};

struct Vector4;
struct Vector3 {
	union {
		float v[3];
		struct {
			float x;
			float y;
			float z;
		};
		struct {
			float r;
			float g;
			float b;
		};
	};

	Vector3() :
		v() {

	}

	Vector3(float x, float y, float z) :
		x(x), y(y), z(z) {

	}

	Vector3(Vector4 v);

	float& operator[](int index) {
		return v[index];
	}

	Vector3 operator-() {
		Vector3 result;

		result.x = -this->x;
		result.y = -this->y;
		result.z = -this->z;

		return result;
	}


	Vector3 operator-(Vector3 v) {
		Vector3 result;

		result.x = this->x - v.x;
		result.y = this->y - v.y;
		result.z = this->z - v.z;

		return result;
	}

	Vector3 operator+(Vector3 v) {
		Vector3 result;

		result.x = this->x + v.x;
		result.y = this->y + v.y;
		result.z = this->z + v.z;

		return result;
	}

	Vector3 operator+(float x) {
		Vector3 result;

		result.x = this->x + x;
		result.y = this->y + x;
		result.z = this->z + x;

		return result;
	}

	Vector3 operator-(float x) {
		Vector3 result;

		result.x = this->x - x;
		result.y = this->y - x;
		result.z = this->z - x;

		return result;
	}

	Vector3 operator*(float x) {
		Vector3 result;

		result.x = this->x * x;
		result.y = this->y * x;
		result.z = this->z * x;

		return result;
	}

	Vector3 operator/(float x) {
		Vector3 result;

		result.x = this->x / x;
		result.y = this->y / x;
		result.z = this->z / x;

		return result;
	}

	Vector3& operator+=(Vector3 v) {
		this->x += v.x;
		this->y += v.y;
		this->z += v.z;

		return *this;
	}

	Vector3& operator-=(Vector3 v) {
		this->x -= v.x;
		this->y -= v.y;
		this->z -= v.z;

		return *this;
	}
};

struct Vector4 {
	union {
		float v[4];
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			float r;
			float g;
			float b;
			float a;
		};
		struct {
			Vector3 xyz;
			float w;
		};
	};

	float& operator[](int index) {
		return v[index];
	}

	Vector4() :
		x(0), y(0), z(0), w(0) {

	}

	Vector4(float x, float y, float z, float w) :
		x(x), y(y), z(z), w(w) {

	}

	Vector4(Vector2 v, float z, float w):
		x(v.x), y(v.y), z(z), w(w) {

	}

	Vector4(Vector3 v, float w):
		x(v.x), y(v.y), z(v.z), w(w) {

	}

	Vector4 operator-() {
		Vector4 result;

		result.x = -this->x;
		result.y = -this->y;
		result.z = -this->z;
		result.w = -this->w;

		return result;
	}

	Vector4 operator+(Vector4 v) {
		Vector4 result;

		result.x = this->x + v.x;
		result.y = this->y + v.y;
		result.z = this->z + v.z;
		result.w = this->w + v.w;

		return result;
	}

	Vector4 operator-(Vector4 v) {
		Vector4 result;

		result.x = this->x - v.x;
		result.y = this->y - v.y;
		result.z = this->z - v.z;
		result.w = this->w - v.w;

		return result;
	}

	Vector4 operator*(float x) {
		Vector4 result;

		result.x = this->x * x;
		result.y = this->y * x;
		result.z = this->z * x;
		result.w = this->w * x;

		return result;
	}

	Vector4 operator*=(float x) {
		this->x = this->x * x;
		this->y = this->y * x;
		this->z = this->z * x;
		this->w = this->w * x;

		return *this;
	}

	Vector4& operator+=(Vector4 v) {
		this->x += v.x;
		this->y += v.y;
		this->z += v.z;
		this->w += v.w;

		return *this;
	}

	Vector4 operator/(float x) {
		Vector4 result;

		result.x = this->x / x;
		result.y = this->y / x;
		result.z = this->z / x;
		result.w = this->w / x;

		return result;
	}
};

typedef Vector4 Quaternion;
Vector3 operator*(float x, Vector3 v);

// TODO: switch to rows?
struct Matrix4x4 {
	union {
		float x[16];
		struct {
			// NOTE: these are columns
			Vector4 v1;
			Vector4 v2;
			Vector4 v3;
			Vector4 v4;
		};
	};

	Matrix4x4() :
		x{ 0 } {

	}

	float& operator[](int index) {
		return x[index];
	}

	Matrix4x4 operator *(Matrix4x4 m) {
		Matrix4x4 result;
		result[0] = x[0] * m[0] + x[4] * m[1] + x[8] * m[2] + x[12] * m[3];
		result[1] = x[1] * m[0] + x[5] * m[1] + x[9] * m[2] + x[13] * m[3];
		result[2] = x[2] * m[0] + x[6] * m[1] + x[10] * m[2] + x[14] * m[3];
		result[3] = x[3] * m[0] + x[7] * m[1] + x[11] * m[2] + x[15] * m[3];

		result[4] = x[0] * m[4] + x[4] * m[5] + x[8] * m[6] + x[12] * m[7];
		result[5] = x[1] * m[4] + x[5] * m[5] + x[9] * m[6] + x[13] * m[7];
		result[6] = x[2] * m[4] + x[6] * m[5] + x[10] * m[6] + x[14] * m[7];
		result[7] = x[3] * m[4] + x[7] * m[5] + x[11] * m[6] + x[15] * m[7];

		result[8] = x[0] * m[8] + x[4] * m[9] + x[8] * m[10] + x[12] * m[11];
		result[9] = x[1] * m[8] + x[5] * m[9] + x[9] * m[10] + x[13] * m[11];
		result[10] = x[2] * m[8] + x[6] * m[9] + x[10] * m[10] + x[14] * m[11];
		result[11] = x[3] * m[8] + x[7] * m[9] + x[11] * m[10] + x[15] * m[11];

		result[12] = x[0] * m[12] + x[4] * m[13] + x[8] * m[14] + x[12] * m[15];
		result[13] = x[1] * m[12] + x[5] * m[13] + x[9] * m[14] + x[13] * m[15];
		result[14] = x[2] * m[12] + x[6] * m[13] + x[10] * m[14] + x[14] * m[15];
		result[15] = x[3] * m[12] + x[7] * m[13] + x[11] * m[14] + x[15] * m[15];
		return result;
	}

	Vector4 operator *(Vector4 v) {
		Vector4 result;
		result[0] = x[0] * v[0] + x[4] * v[1] + x[8] * v[2] + x[12] * v[3];
		result[1] = x[1] * v[0] + x[5] * v[1] + x[9] * v[2] + x[13] * v[3];
		result[2] = x[2] * v[0] + x[6] * v[1] + x[10] * v[2] + x[14] * v[3];
		result[3] = x[3] * v[0] + x[7] * v[1] + x[11] * v[2] + x[15] * v[3];
		return result;
	}
};

namespace math {
	const float PIHALF = 1.57079633f;
	const float PI     = 3.14159265f;
	const float PI2    = 6.28318530f;
#undef max
#undef min
	float max(float a, float b);
	float min(float a, float b);
	int max(int a, int b);
	int min(int a, int b);
	float abs(float x);
	float sin(float x);
	float cos(float x);
	float tan(float x);
	float atan2(float y, float x);
	float acos(float x);
	float sqrt(float x);
	float square(float x);
	float pow(float x, float e);
	float sign(float x);
	float length(Vector2 x);
	float length(Vector3 x);
	float length(Vector4 x);
	float length_squared(Vector2 x);
	float length_squared(Vector3 x);
	float length_squared(Vector4 x);
	template<typename T>
	T clamp(T x, T min, T max) {
		if (x > max) return max;
		if (x < min) return min;
		return x;
	}
	float floor(float x);

	float lerp(float v1, float v2, float x);
	Vector3 lerp(Vector3 v1, Vector3 v2, float x);
	Vector4 lerp(Vector4 v1, Vector4 v2, float x);
	Vector4 nlerp(Vector4 v1, Vector4 v2, float x);

	Quaternion conjugate(Quaternion q);

	float deg2rad(float deg);
	float rad2deg(float rad);
		
	float fmod(float x, float m);
	int32_t mod(int32_t x, int32_t m);

	float dot(Vector2 a, Vector2 b);
	float dot(Vector3 a, Vector3 b);
	float dot(Vector4 a, Vector4 b);
	Vector3 cross(Vector3 a, Vector3 b);
	Vector2 normalize(Vector2);
	Vector3 normalize(Vector3);
	Vector4 normalize(Vector4);

	Vector3 rotate(Vector3 v, Quaternion q);
	Vector3 rotate(Vector3 v, float angle, Vector3 axis);

	Matrix4x4 get_identity();
	Matrix4x4 get_translation(Vector3 v);
	Matrix4x4 get_translation(float x, float y, float z);
	Matrix4x4 get_scale(float scale);
	Matrix4x4 get_scale(float s_x, float s_y, float s_z);
	Matrix4x4 get_scale(Vector3 scale);
	Matrix4x4 get_rotation(float angle, Vector3 axis);
	Matrix4x4 get_rotation(Quaternion q);

	Matrix4x4 invert(Matrix4x4 m);
	Matrix4x4 transpose(Matrix4x4 m);
#undef near
#undef far

	// NOTE: This matrix is for RH systems and near/far are reverted with respect to RH system. E.g. passing -near produces z value of 0;
	Matrix4x4 get_perspective_projection_dx_rh(float fov, float aspectRatio, float near, float far);
	Matrix4x4 get_orthographics_projection_dx_rh(float left, float right, float bottom, float top, float near, float far);

	Matrix4x4 get_look_at(Vector3 eye, Vector3 target, Vector3 up);

	float ray_plane_intersection(Vector3 ray_origin, Vector3 ray_direction, Vector3 plane_normal, float plane_distance);
	float ray_box_intersection(Vector3 ray_origin, Vector3 ray_direction, Vector3 box_position,
							   Vector3 x_axis, Vector3 y_axis, Vector3 z_axis);

	float random_uniform(float low = 0.0f, float high = 1.0f);
	int random_uniform_int(int low = 0, int high = 2);
	// Azimuth: 0 at +x axis in right handed system
	//			pi / 2 at +z axis in right handed system
	// Polar: 0 at the top
	Vector3 random_uniform_unit_sphere();
	Vector3 random_uniform_unit_hemisphere();
}

#ifdef CPPLIB_MATHS_IMPL
#include "maths.cpp"
#endif