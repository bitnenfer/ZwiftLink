#pragma once

#include <stdint.h>

#undef min
#undef max

struct Float2
{
	Float2() : x(0.0f), y(0.0f) {}
	Float2(float x) : x(x), y(x) {}
	Float2(float x, float y) : x(x), y(y) {}
	Float2(const Float2& other) : x(other.x), y(other.y) {}
	
	float& operator[](uint32_t i) { return components[i]; }

	union
	{
		struct { float x, y; };
		float components[2];
	};
};

struct Float3
{
	Float3() : x(0.0f), y(0.0f), z(0.0f) {}
	Float3(float x) : x(x), y(x), z(x) {}
	Float3(float x, float y, float z) : x(x), y(y), z(z) {}
	Float3(const Float3& other) : x(other.x), y(other.y), z(other.z) {}
	Float3& operator=(const Float3& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}
	float& operator[](uint32_t i) { return components[i]; }

	union
	{
		struct { float x, y, z; };
		float components[3];
	};
};

struct Float4
{
	Float4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	Float4(float x) : x(x), y(x), z(x), w(x) {}
	Float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Float4(const Float4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	float& operator[](uint32_t i) { return components[i]; }

	union
	{
		struct { float x, y, z, w; };
		float components[4];
	};
};

struct Float2x2
{
	Float2x2() :
		Float2x2(1, 0, 0, 1)
	{
	}

	Float2x2(float a, float b, float c, float d)
	{
		data[0] = a; data[2] = c; // col 0
		data[1] = b; data[3] = d; // col 1
	}

	Float2x2(const Float2x2& other)
	{
		row0 = other.row0;
		row1 = other.row1;
	}
	float& operator[](uint32_t i) { return data[i]; }

	union
	{
		float data[4];
		Float2 rows[2];
		struct
		{
			Float2 row0;
			Float2 row1;
		};
	};
};

struct Float3x3
{
	Float3x3() :
		Float3x3(1.0f, 0.0f, 0.0f,
				 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 1.0f)
	{
	}

	Float3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i)
	{
		data[0] = a; data[3] = d; data[6] = g; // col 0
		data[1] = b; data[4] = e; data[7] = h; // col 1
		data[2] = c; data[5] = f; data[8] = i; // col 2
	}
	Float3x3(const Float3x3& other)
	{
		row0 = other.row0;
		row1 = other.row1;
		row2 = other.row2;
	}
	float& operator[](uint32_t i) { return data[i]; }

	union
	{
		float data[9];
		Float3 rows[3];
		struct
		{
			Float3 row0;
			Float3 row1;
			Float3 row2;
		};
	};
};

struct Float4x4
{
	Float4x4() :
		Float4x4(1.0f, 0.0f, 0.0f, 0.0f,
				 0.0f, 1.0f, 0.0f, 0.0f,
				 0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f)
	{
	}

	Float4x4(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p)
	{
		data[0] = a; data[4] = e; data[8] = i;  data[12] = m; // col 0
		data[1] = b; data[5] = f; data[9] = j;  data[13] = n; // col 1
		data[2] = c; data[6] = g; data[10] = k;  data[14] = o; // col 2
		data[3] = d; data[7] = h; data[11] = l;  data[15] = p; // col 3
	}

	Float4x4(const Float4x4& other)
	{
		row0 = other.row0;
		row1 = other.row1;
		row2 = other.row2;
		row3 = other.row3;
	}
	float& operator[](uint32_t i) { return data[i]; }

	union
	{
		float data[16];
		Float4 rows[4];
		struct
		{
			Float4 row0;
			Float4 row1;
			Float4 row2;
			Float4 row3;
		};
	};
};

struct Quat
{
	Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	Quat(float x) : x(x), y(x), z(x), w(x) {}
	Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Quat(const Quat& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	float& operator[](uint32_t i) { return components[i]; }

	union
	{
		struct { float x, y, z, w; };
		float components[4];
	};
};

struct Matrix2D
{
	Matrix2D()
	{
		a = 1.0f;
		b = 0.0f;
		c = 0.0f;
		d = 1.0f;
		tx = 0.0f;
		ty = 0.0f;
	}

	Matrix2D(const Matrix2D& Other)
	{
		ab = Other.ab;
		cd = Other.cd;
		txty = Other.txty;
	}

	Matrix2D(float a, float b, float c, float d, float tx, float ty) :
		a(a), b(b), c(c), d(d), tx(tx), ty(ty)
	{
	}

	float& operator[](uint32_t i) { return components[i]; }
	union
	{
		float components[6];
		struct { float a, b, c, d, tx, ty; };
		struct { Float2 ab, cd, txty; };
	};
};

Float2x2 operator*(const Float2x2 & a, const Float2x2 & b);
Float3x3 operator*(const Float3x3& a, const Float3x3& b);
Float4x4 operator*(const Float4x4& a, const Float4x4& b);

Float2 operator+(const Float2& a, const Float2& b);
Float2 operator-(const Float2& a, const Float2& b);
Float2 operator*(const Float2& a, const Float2& b);
Float2 operator/(const Float2& a, const Float2& b);
Float2 operator*(const Float2x2& a, const Float2& b);
Float2 operator+(const Float2& a, const float& b);
Float2 operator-(const Float2& a, const float& b);
Float2 operator*(const Float2& a, const float& b);
Float2 operator/(const Float2& a, const float& b);
Float2 operator+(const float& a, const Float2& b);
Float2 operator-(const float& a, const Float2& b);
Float2 operator*(const float& a, const Float2& b);
Float2 operator/(const float& a, const Float2& b);
Float2 operator-(const Float2& a);
Float2 operator*(const Matrix2D& a, const Float2& b);

Float3 operator+(const Float3& a, const Float3& b);
Float3 operator-(const Float3& a, const Float3& b);
Float3 operator*(const Float3& a, const Float3& b);
Float3 operator/(const Float3& a, const Float3& b);
Float3 operator*(const Float3x3& a, const Float3& b);
Float3 operator+(const Float3& a, const float& b);
Float3 operator-(const Float3& a, const float& b);
Float3 operator*(const Float3& a, const float& b);
Float3 operator/(const Float3& a, const float& b);
Float3 operator+(const float& a, const Float3& b);
Float3 operator-(const float& a, const Float3& b);
Float3 operator*(const float& a, const Float3& b);
Float3 operator/(const float& a, const Float3& b);
Float3 operator-(const Float3& a);
bool operator==(const Float3& a, const Float3& b);
bool operator!=(const Float3& a, const Float3& b);

Float4 operator+(const Float4& a, const Float4& b);
Float4 operator-(const Float4& a, const Float4& b);
Float4 operator*(const Float4& a, const Float4& b);
Float4 operator/(const Float4& a, const Float4& b);
Float4 operator*(const Float4x4& a, const Float4& b);
Float4 operator+(const Float4& a, const float& b);
Float4 operator-(const Float4& a, const float& b);
Float4 operator*(const Float4& a, const float& b);
Float4 operator/(const Float4& a, const float& b);
Float4 operator+(const float& a, const Float4& b);
Float4 operator-(const float& a, const Float4& b);
Float4 operator*(const float& a, const Float4& b);
Float4 operator/(const float& a, const Float4& b);
Float4 operator-(const Float4& a);

Quat operator*(const Quat& a, const Quat& b);

namespace math
{
	constexpr float PI = 3.14159265359f;
	constexpr float TAU = PI * 2.0f;

	float toRad(float d);
	float toDeg(float r);
	float cos(float t);
	float sin(float t);
	float tan(float t);
	float asin(float t);
	float acos(float t);
	float atan(float t);
	float atan2(float y, float x);
	float pow(float x, float y);
	float exp(float x);
	float exp2(float x);
	float log(float x);
	float log2(float x);
	float sqrt(float x);
	float invSqrt(float x);
	float abs(float x);
	float round(float x);
	float sign(float x);
	float floor(float x);
	float ceil(float x);
	float fract(float x);
	float mod(float x, float y);
	float clamp(float n, float x, float y);
	float mix(float x, float y, float n);
	float step(float edge, float x);
	float smootStep(float edge0, float edge1, float x);
	float saturate(float Value);
	float normalize(float Value, float MinValue, float MaxValue);
	float randomFloat();
	uint32_t randomUint();

	template<typename T> T min(T Lhs, T Rhs) { return Lhs < Rhs ? Lhs : Rhs; }
	template<typename T> static T max(T Lhs, T Rhs) { return Lhs > Rhs ? Lhs : Rhs; }

	float lengthSqr(const Float2& v);
	float length(const Float2& v);
	float dot(const Float2& a, const Float2& b);
	float distance(const Float2& a, const Float2& b);
	Float2 normalize(const Float2& v);
	Float2 faceForward(const Float2& v, const Float2& incident, const Float2& reference);
	Float2 reflect(const Float2& v, const Float2& n);
	Float2 refract(const Float2& v, const Float2& n, float ior);

	float lengthSqr(const Float3& v);
	float length(const Float3& v);
	float dot(const Float3& a, const Float3& b);
	float distance(const Float3& a, const Float3& b);
	Float3 normalize(const Float3& v);
	Float3 faceForward(const Float3& v, const Float3& incident, const Float3& reference);
	Float3 reflect(const Float3& v, const Float3& n);
	Float3 refract(const Float3& v, const Float3& n, float ior);
	Float3 cross(const Float3& v, const Float3& n);

	float lengthSqr(const Float4& v);
	float length(const Float4& v);
	float dot(const Float4& a, const Float4& b);
	float distance(const Float4& a, const Float4& b);
	Float4 normalize(const Float4& v);
	Float4 faceForward(const Float4& v, const Float4& incident, const Float4& reference);
	Float4 reflect(const Float4& v, const Float4& n);
	Float4 refract(const Float4& v, const Float4& n, float ior);

	Float2x2 transpose(const Float2x2& m);
	Float2x2 rotate(const Float2x2& m, float r);
	Float2x2 scale(const Float2x2& m, const Float2& v);

	Float3x3 transpose(const Float3x3& m);
	Float3x3 rotate(const Float3x3& m, float r);
	Float3x3 scale(const Float3x3& m, const Float3& v);
	Float3x3 translate(const Float3x3& m, const Float3& v);

	Float4x4 transpose(const Float4x4& m);
	Float4x4 rotate(const Float4x4& m, const Float3& v);
	Float4x4 scale(const Float4x4& m, const Float3& v);
	Float4x4 translate(const Float4x4& m, const Float3& v);
	Float4x4 rotateX(const Float4x4& m, float r);
	Float4x4 rotateY(const Float4x4& m, float r);
	Float4x4 rotateZ(const Float4x4& m, float r);
	Float4x4 perspective(float fovy, float aspect, float nearBound, float farBound);
	Float4x4 orthographic(float left, float right, float bottom, float top, float nearBound, float farBound);
	Float4x4 lookAt(const Float3& eye, const Float3& center, const Float3& up);
	Float4x4 invert(const Float4x4& m);

	Quat fromEuler(const Float3& e);
	Float3 toEuler(const Quat& q);
	Quat rotateX(const Quat& q, float r);
	Quat rotateY(const Quat& q, float r);
	Quat rotateZ(const Quat& q, float r);
	Float4x4 toFloat4x4(const Quat& q);

	Matrix2D translate(const Matrix2D& m, float x, float y);
	Matrix2D translate(const Matrix2D& m, Float2& v);
	Matrix2D scale(const Matrix2D& m, float x, float y);
	Matrix2D scale(const Matrix2D& m, Float2& v);
	Matrix2D rotate(const Matrix2D& m, float r);

	bool isNaN(float v);
	bool isNaN(Float2 v);
	bool isNaN(Float3 v);
	bool isNaN(Float4 v);
	bool isNaN(Float2x2 v);
	bool isNaN(Float3x3 v);
	bool isNaN(Float4x4 v);
	bool isNaN(Quat v);
	bool isNaN(Matrix2D v);

	bool isInf(float v);
	bool isInf(Float2 v);
	bool isInf(Float3 v);
	bool isInf(Float4 v);
	bool isInf(Float2x2 v);
	bool isInf(Float3x3 v);
	bool isInf(Float4x4 v);
	bool isInf(Quat v);
	bool isInf(Matrix2D v);

	template<typename T>
	bool isValid(T v)
	{
		return !isNaN(v) && !isInf(v);
	}
}
