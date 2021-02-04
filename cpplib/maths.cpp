#include "maths.h"
#include <math.h>

Vector3::Vector3(Vector4 v) :
	x(v.x), y(v.y), z(v.z) {
}

float math::max(float a, float b) {
	return a >= b ? a : b;
}

float math::min(float a, float b) {
	return a <= b ? a : b;
}

int math::max(int a, int b) {
	return a >= b ? a : b;
}

int math::min(int a, int b) {
	return a <= b ? a : b;
}

float math::abs(float x) {
	return fabsf(x);
}

float math::sin(float x) {
	return sinf(x);
}

float math::cos(float x) {
	return cosf(x);
}

float math::tan(float x) {
	return tanf(x);
}

float math::atan2(float y, float x) {
	return atan2f(y, x);
}

float math::acos(float x) {
	return acosf(x);
}

float math::square(float x) {
	return x * x;
}

float math::sqrt(float x) {
	return sqrtf(x);
}

float math::pow(float x, float e) {
	return powf(x, e);
}

float math::sign(float x) {
	return x < 0.0f ? -1.0f : 1.0f;
}

float math::dot(Vector2 a, Vector2 b) {
	return a.x * b.x + a.y * b.y;
}

float math::dot(Vector3 a, Vector3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float math::dot(Vector4 a, Vector4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float math::length(Vector2 x) {
	return math::sqrt(math::length_squared(x));
}

float math::length(Vector3 x) {
	return math::sqrt(math::length_squared(x));
}

float math::length(Vector4 x) {
	return math::sqrt(math::length_squared(x));
}

float math::length_squared(Vector2 x) {
	return math::dot(x, x);
}

float math::length_squared(Vector3 x) {
	return math::dot(x, x);
}

float math::length_squared(Vector4 x) {
	return math::dot(x, x);
}

float math::floor(float x) {
	return floorf(x);
}

float math::deg2rad(float deg) {
	float rad = deg * math::PI / 180.0f;
	return rad;
}

float math::rad2deg(float rad) {
	float deg = rad * 180.0f / math::PI;
	return deg;
}

float math::lerp(float v1, float v2, float x) {
	return v1 * (1 - x) + v2 * x;
}

Vector3 math::lerp(Vector3 v1, Vector3 v2, float x) {
	Vector3 result = {};

	result.x = v1.x * (1 - x) + v2.x * x;
	result.y = v1.y * (1 - x) + v2.y * x;
	result.z = v1.z * (1 - x) + v2.z * x;

	return result;
}

Vector4 math::lerp(Vector4 v1, Vector4 v2, float x) {
	Vector4 result = {};

	result.x = v1.x * (1 - x) + v2.x * x;
	result.y = v1.y * (1 - x) + v2.y * x;
	result.z = v1.z * (1 - x) + v2.z * x;
	result.w = v1.w * (1 - x) + v2.w * x;

	return result;
}

Vector4 math::nlerp(Vector4 v1, Vector4 v2, float x) {
	Vector4 r = math::lerp(v1, v2, x);
	return math::normalize(r);
}

Vector3 operator*(float x, Vector3 v) {
	return v * x;
}

float math::fmod(float x, float m) {
	return fmodf(x, m);
}

int32_t math::mod(int32_t x, int32_t m) {
	return (x % m + m) % m;
}

Quaternion math::conjugate(Quaternion q) {
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}

Vector3 math::rotate(Vector3 v, Quaternion q) {
	Vector3 qv = Vector3(q.x, q.y, q.z);
	float s = q.w;
	Vector3 result = 2 * math::dot(qv, v) * qv + (s * s - math::dot(qv, qv)) * v + 2 * s * math::cross(qv, v);
	return result;
}

Vector3 math::rotate(Vector3 v, float angle, Vector3 axis) {
	float cos = math::cos(angle / 2.0f);
	float sin = math::sin(angle / 2.0f);
	axis = axis * sin;
	Quaternion q = Quaternion(axis.x, axis.y, axis.z, cos);
	Vector3 result = math::rotate(v, q);

	return result;
}

Vector3 math::cross(Vector3 a, Vector3 b) {
	Vector3 result;

	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;

	return result;
}

Vector2 math::normalize(Vector2 x) {
	float length = math::length(x);
	if (length < 0.001f) {
		return Vector2();
	}
	return x / length;
}

Vector3 math::normalize(Vector3 x) {
	float length = math::length(x);
	if (length < 0.001f) {
		return Vector3();
	}
	return x / length;
}

Vector4 math::normalize(Vector4 x) {
	float length = math::length(x);
	if (length < 0.001f) {
		return Vector4();
	}
	return x / length;
}

Matrix4x4 math::get_identity() {
	Matrix4x4 result = {};

	result[0] = result[5] = result[10] = result[15] = 1;

	return result;
}

Matrix4x4 math::get_translation(Vector3 v) {
	Matrix4x4 result = get_translation(v.x, v.y, v.z);
	return result;
}

Matrix4x4 math::get_translation(float x, float y, float z) {
	Matrix4x4 result = {};

	result[0] = 1;
	result[5] = 1;
	result[10] = 1;
	result[12] = x;
	result[13] = y;
	result[14] = z;
	result[15] = 1;

	return result;
}

Matrix4x4 math::get_scale(float scale) {
	Matrix4x4 result = {};

	result[0] = result[5] = result[10] = scale;
	result[15] = 1;

	return result;
}

Matrix4x4 math::get_scale(float s_x, float s_y, float s_z) {
	Matrix4x4 result = {};

	result[0] = s_x;
	result[5] = s_y;
	result[10] = s_z;
	result[15] = 1;

	return result;
}

Matrix4x4 math::get_scale(Vector3 scale) {
	return math::get_scale(scale.x, scale.y, scale.z);
}


Matrix4x4 math::get_rotation(float angle, Vector3 axis) {
	float c = math::cos(angle);
	float s = math::sin(angle);

	Matrix4x4 result;
	float ax = axis.x;
	float ay = axis.y;
	float az = axis.z;
	result[0] = c + (1 - c) * ax * ax;
	result[1] = (1 - c) * ax * ay + s * az;
	result[2] = (1 - c) * ax * az - s * ay;
	result[3] = 0;

	result[4] = (1 - c) * ax * ay - s * az;
	result[5] = c + (1 - c) * ay * ay;
	result[6] = (1 - c) * ay * az + s * ax;
	result[7] = 0;

	result[8] = (1 - c) * ax * az + s * ay;
	result[9] = (1 - c) * ay * az - s * ax;
	result[10] = c + (1 - c) * az * az;
	result[11] = 0;

	result[12] = 0;
	result[13] = 0;
	result[14] = 0;
	result[15] = 1;

	return result;
}

Matrix4x4 math::get_rotation(Quaternion q) {
	q = math::normalize(q);

	Matrix4x4 result = {};
	result[0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
	result[1] = 2 * q.x * q.y + 2 * q.w * q.z;
	result[2] = 2 * q.x * q.z - 2 * q.w * q.y;

	result[4] = 2 * q.x * q.y - 2 * q.w * q.z;
	result[5] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	result[6] = 2 * q.y * q.z + 2 * q.w * q.x;

	result[8] = 2 * q.x * q.z + 2 * q.w * q.y;
	result[9] = 2 * q.y * q.z - 2 * q.w * q.x;
	result[10] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;

	result[15] = 1;

	return result;
}

Matrix4x4 math::get_perspective_projection_dx_rh(float fov, float aspect_ratio, float near, float far) {
	Matrix4x4 result;

	float cot_fov = 1 / (math::tan(fov / 2.0f));
	result[0] = cot_fov / aspect_ratio;
	result[5] = cot_fov;
	result[10] = -far / (far - near);
	result[11] = -1.0f;
	result[14] = -near * far / (far - near);

	return result;
}

Matrix4x4 math::get_orthographics_projection_dx_rh(float left, float right, float bottom, float top, float near, float far) {
	Matrix4x4 result;

	result[0] = 2.0f / (right - left);
	result[5] = 2.0f / (top - bottom);
	result[10] = 1.0f / (far - near);
	result[12] = -(right + left) / (right - left);
	result[13] = -(top + bottom) / (top - bottom);
	result[14] = -(near) / (far - near);
	result[15] = 1;

	return result;
}

Matrix4x4 math::get_look_at(Vector3 eye, Vector3 target, Vector3 up) {
	Matrix4x4 matrix;
	Vector3 x, y, z;

	z = eye - target;
	z = math::normalize(z);

	x = math::cross(up, z);
	x = math::normalize(x);

	y = math::cross(z, x);
	y = math::normalize(y);

	matrix.v1 = Vector4(x.x, y.x, z.x, 0);
	matrix.v2 = Vector4(x.y, y.y, z.y, 0);
	matrix.v3 = Vector4(x.z, y.z, z.z, 0);
	matrix.v4 = Vector4(-math::dot(x, eye),
						-math::dot(y, eye),
						-math::dot(z, eye),
						1.0f);
	return matrix;
}

Matrix4x4 math::transpose(Matrix4x4 m) {
	Matrix4x4 result = {};
	result[0]  = m[0];
	result[1]  = m[4];
	result[2]  = m[8];
	result[3]  = m[12];

	result[4]  = m[1];
	result[5]  = m[5];
	result[6]  = m[9];
	result[7]  = m[13];

	result[8]  = m[2];
	result[9]  = m[6];
	result[10] = m[10];
	result[11] = m[14];

	result[12] = m[3];
	result[13] = m[7];
	result[14] = m[11];
	result[15] = m[15];

	return result;
}

Matrix4x4 math::invert(Matrix4x4 m) {
	Matrix4x4 inv;
	float det;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
		return math::get_identity();

	det = 1.0f / det;

	Matrix4x4 result;
	for (int i = 0; i < 16; i++)
		result[i] = inv[i] * det;

	return result;
}


float math::ray_plane_intersection(Vector3 ray_origin, Vector3 ray_direction, Vector3 plane_normal, float plane_distance) {
	float direction_dot = math::dot(ray_direction, plane_normal);
	if (math::abs(direction_dot) < 0.001f) {
		return -1.0f;
	}
	float intersection_time = -(math::dot(ray_origin, plane_normal) + plane_distance) / direction_dot;
	return intersection_time;
}

float math::ray_box_intersection(Vector3 ray_origin, Vector3 ray_direction, Vector3 box_position,
				    			 Vector3 x_axis, Vector3 y_axis, Vector3 z_axis) {
	return 0.0f;
}


// TODO: other source of randomness than rand()
#include <cstdlib>
float math::random_uniform(float low, float high) {
	float normalized = (rand() % 10000) / 10000.0f;
	float result = normalized * (high - low) + low;

	return result;
}

int math::random_uniform_int(int low, int high) {
	int result = (rand() % (high - low)) + low;
	return result;
}

Vector3 math::random_uniform_unit_sphere() {
	float azimuth = math::random_uniform(0, math::PI2);
	float polar = math::acos(2 * math::random_uniform() - 1);
	float r = math::pow(math::random_uniform(), 1.0f / 3.0f);
	
	Vector3 result;

	result.x = r * math::cos(azimuth) * math::sin(polar);
	result.y = r * math::cos(polar);
	result.z = r * math::sin(azimuth) * math::sin(polar);

	return result;
}

Vector3 math::random_uniform_unit_hemisphere() {
	float y = math::random_uniform(0.0f, 1.0f);
	float r = math::sqrt(1.0f - y * y);
	float phi = math::random_uniform(0.0f, math::PI2);

	float radius = math::random_uniform(0.0f, 1.0f);
	radius = math::sqrt(radius);
	
	Vector3 result;

	result.x = r * math::cos(phi) * radius;
	result.y = y * radius;
	result.z = r * math::sin(phi) * radius;

	return result;
}