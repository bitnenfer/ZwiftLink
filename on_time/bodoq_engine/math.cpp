#include "math.h"
#include <math.h>
#include <stdlib.h>
#include <random>

Float2 operator+(const Float2& a, const Float2& b)
{
    return Float2(a.x + b.x, a.y + b.y);
}

Float2 operator-(const Float2& a, const Float2& b)
{
    return Float2(a.x - b.x, a.y - b.y);
}

Float2 operator*(const Float2& a, const Float2& b)
{
    return Float2(a.x * b.x, a.y * b.y);
}

Float2 operator/(const Float2& a, const Float2& b)
{
    return Float2(a.x / b.x, a.y / b.y);
}

Float2 operator*(const Float2x2& m, const Float2& v)
{
    return Float2(
        m.data[0] * v.x + m.data[2] * v.y,
        m.data[1] * v.x + m.data[3] * v.y
    );
}

Float2 operator+(const Float2& a, const float& b)
{
    return Float2(a.x + b, a.y + b);
}

Float2 operator-(const Float2& a, const float& b)
{
    return Float2(a.x - b, a.y - b);
}

Float2 operator*(const Float2& a, const float& b)
{
    return Float2(a.x * b, a.y * b);
}

Float2 operator/(const Float2& a, const float& b)
{
    return Float2(a.x / b, a.y / b);
}

Float2 operator+(const float& b, const Float2& a)
{
    return Float2(a.x + b, a.y + b);
}

Float2 operator-(const float& b, const Float2& a)
{
    return Float2(b - a.x, b - a.y);
}

Float2 operator*(const float& b, const Float2& a)
{
    return Float2(a.x * b, a.y * b);
}

Float2 operator/(const float& b, const Float2& a)
{
    return Float2(b / a.x, b / a.y);
}

Float2 operator-(const Float2& a)
{
    return Float2(-a.x, -a.y);
}

Float2 operator*(const Matrix2D& a, const Float2& b)
{
    float x = b.x * a.a + b.y * a.c + a.tx;
    float y = b.x * a.b + b.y * a.d + a.ty;
    return Float2(x, y);
}

// Float3
Float3 operator+(const Float3& a, const Float3& b)
{
    return Float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Float3 operator-(const Float3& a, const Float3& b)
{
    return Float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Float3 operator*(const Float3& a, const Float3& b)
{
    return Float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Float3 operator/(const Float3& a, const Float3& b)
{
    return Float3(a.x / b.x, a.y / b.y, a.z / b.z);
}

Float3 operator*(const Float3x3& m, const Float3& v)
{
    return Float3(
        m.data[0] * v.x + m.data[3] * v.y + m.data[6] * v.z,
        m.data[1] * v.x + m.data[4] * v.y + m.data[7] * v.z,
        m.data[2] * v.x + m.data[5] * v.y + m.data[8] * v.z
    );
}

Float3 operator+(const Float3& a, const float& b)
{
    return Float3(a.x + b, a.y + b, a.z + b);
}

Float3 operator-(const Float3& a, const float& b)
{
    return Float3(a.x - b, a.y - b, a.z - b);
}

Float3 operator*(const Float3& a, const float& b)
{
    return Float3(a.x * b, a.y * b, a.z * b);
}

Float3 operator/(const Float3& a, const float& b)
{
    return Float3(a.x / b, a.y / b, a.z / b);
}

Float3 operator+(const float& b, const Float3& a)
{
    return Float3(a.x + b, a.y + b, a.z + b);
}

Float3 operator-(const float& b, const Float3& a)
{
    return Float3(b - a.x, b - a.y, b - a.z);
}

Float3 operator*(const float& b, const Float3& a)
{
    return Float3(a.x * b, a.y * b, a.z * b);
}

Float3 operator/(const float& b, const Float3& a)
{
    return Float3(b / a.x, b / a.y, b / a.z);
}

Float3 operator-(const Float3& a)
{
    return Float3(-a.x, -a.y, -a.z);
}

bool operator==(const Float3& a, const Float3& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!=(const Float3& a, const Float3& b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

// Float4
Float4 operator+(const Float4& a, const Float4& b)
{
    return Float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Float4 operator-(const Float4& a, const Float4& b)
{
    return Float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Float4 operator*(const Float4& a, const Float4& b)
{
    return Float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Float4 operator/(const Float4& a, const Float4& b)
{
    return Float4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

Float4 operator*(const Float4x4& m, const Float4& v)
{
    return Float4(
        m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12] * v.w,
        m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13] * v.w,
        m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14] * v.w,
        m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15] * v.w
    );
}

Float4 operator+(const Float4& a, const float& b)
{
    return Float4(a.x + b, a.y + b, a.z + b, a.w + b);
}

Float4 operator-(const Float4& a, const float& b)
{
    return Float4(a.x - b, a.y - b, a.z - b, a.w - b);
}

Float4 operator*(const Float4& a, const float& b)
{
    return Float4(a.x * b, a.y * b, a.z * b, a.w * b);
}

Float4 operator/(const Float4& a, const float& b)
{
    return Float4(a.x / b, a.y / b, a.z / b, a.w / b);
}

Float4 operator+(const float& b, const Float4& a)
{
    return Float4(a.x + b, a.y + b, a.z + b, a.w + b);
}

Float4 operator-(const float& b, const Float4& a)
{
    return Float4(b - a.x, b - a.y, b - a.z, b - a.w);
}

Float4 operator*(const float& b, const Float4& a)
{
    return Float4(a.x * b, a.y * b, a.z * b, a.w * b);
}

Float4 operator/(const float& b, const Float4& a)
{
    return Float4(b / a.x, b / a.y, b / a.z, b / a.w);
}

Float4 operator-(const Float4& a)
{
    return Float4(-a.x, -a.y, -a.z, -a.w);
}

Quat operator*(const Quat& a, const Quat& b)
{
    return Quat(
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}

Float2x2 operator*(const Float2x2& a, const Float2x2& b)
{
    Float2x2 c;
    for (int j = 0; j < 2; ++j) { // each column of B
        const float b0 = b.data[0 + j * 2];
        const float b1 = b.data[1 + j * 2];
        c.data[0 + j * 2] = a.data[0] * b0 + a.data[2] * b1;
        c.data[1 + j * 2] = a.data[1] * b0 + a.data[3] * b1;
    }
    return c;
}

Float3x3 operator*(const Float3x3& a, const Float3x3& b)
{
    Float3x3 c;
    for (int j = 0; j < 3; ++j) {
        const float b0 = b.data[0 + j * 3];
        const float b1 = b.data[1 + j * 3];
        const float b2 = b.data[2 + j * 3];
        c.data[0 + j * 3] = a.data[0] * b0 + a.data[3] * b1 + a.data[6] * b2;
        c.data[1 + j * 3] = a.data[1] * b0 + a.data[4] * b1 + a.data[7] * b2;
        c.data[2 + j * 3] = a.data[2] * b0 + a.data[5] * b1 + a.data[8] * b2;
    }
    return c;
}

Float4x4 operator*(const Float4x4& a, const Float4x4& b)
{
    Float4x4 c;
    for (int j = 0; j < 4; ++j) {
        const int bj = j * 4;
        const float b0 = b.data[bj + 0];
        const float b1 = b.data[bj + 1];
        const float b2 = b.data[bj + 2];
        const float b3 = b.data[bj + 3];
        // result column j = A * B.col(j)
        c.data[bj + 0] = a.data[0] * b0 + a.data[4] * b1 + a.data[8] * b2 + a.data[12] * b3;
        c.data[bj + 1] = a.data[1] * b0 + a.data[5] * b1 + a.data[9] * b2 + a.data[13] * b3;
        c.data[bj + 2] = a.data[2] * b0 + a.data[6] * b1 + a.data[10] * b2 + a.data[14] * b3;
        c.data[bj + 3] = a.data[3] * b0 + a.data[7] * b1 + a.data[11] * b2 + a.data[15] * b3;
    }
    return c;
}

float math::toRad(float d) { return math::PI * d / 180.0f; }

float math::toDeg(float r) { return 180.0f * r / math::PI; }

float math::cos(float t) { return cosf(t); }

float math::sin(float t) { return sinf(t); }

float math::tan(float t) { return tanf(t); }

float math::asin(float t) { return asinf(t); }

float math::acos(float t) { return acosf(t); }

float math::atan(float t) { return atanf(t); }

float math::atan2(float y, float x) { return atan2f(y, x); }

float math::pow(float x, float y) { return powf(x, y); }

float math::exp(float x) { return expf(x); }

float math::exp2(float x) { return exp2f(x); }

float math::log(float x) { return logf(x); }

float math::log2(float x) { return log2f(x); }

float math::sqrt(float x) { return sqrtf(x); }

float math::invSqrt(float x) { return 1.0f / sqrtf(x); }

float math::abs(float x) { return fabsf(x); }

float math::round(float x) { return roundf(x); }

float math::sign(float x) { return (x < 0.0f ? -1.0f : 1.0f); }

float math::floor(float x) { return floorf(x); }

float math::ceil(float x) { return ceilf(x); }

float math::fract(float x) { return x - floorf(x); }

float math::mod(float x, float y) { return fmodf(x, y); }

float math::clamp(float n, float x, float y) { return (n < x ? x : n > y ? y : n); }

float math::mix(float x, float y, float n) { return x * (1.0f - n) + y * n; }

float math::step(float edge, float x) { return (x < edge ? 0.0f : 1.0f); }

float math::smootStep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

float math::saturate(float Value)
{
    return math::clamp(Value, 0.0f, 1.0f);
}

float math::normalize(float v, float a, float b)
{
    float d = (b - a);
    return d != 0.0f ? (v - a) / d : 0.0f;
}

float math::randomFloat() {
    static std::random_device randDevice;
    static std::mt19937 mtGen(randDevice());
    static std::uniform_real_distribution<float> udt;
    return udt(mtGen);
}

uint32_t math::randomUint() {
    static std::random_device randDevice;
    static std::mt19937 mtGen(randDevice());
    static std::uniform_int_distribution<unsigned int> udt;
    return udt(mtGen);
}

float math::lengthSqr(const Float2& v)
{
    return v.x * v.x + v.y * v.y;
}

float math::length(const Float2& v)
{
    return math::sqrt(math::lengthSqr(v));
}

float math::dot(const Float2& a, const Float2& b)
{
    return a.x * b.x + a.y * b.y;
}

float math::distance(const Float2& a, const Float2& b)
{
    return math::length(a - b);
}

Float2 math::normalize(const Float2& v)
{
    Float2 r = v;
    float len = length(v);
    if (len != 0.0f)
    {
        r.x /= len;
        r.y /= len;
    }
    return r;
}

Float2 math::faceForward(const Float2& v, const Float2& incident, const Float2& reference)
{
    if (dot(incident, reference) >= 0.0f)
    {
        return -v;
    }
    return v;
}

Float2 math::reflect(const Float2& v, const Float2& n)
{
    return v - 2.0f * dot(v, n) * n;
}

Float2 math::refract(const Float2& v, const Float2& n, float ior)
{
    float idn = dot(v, n);
    float k = 1.0f - ior * ior * (1.0f - idn * idn);
    if (k < 0.0f) return Float2(0.0f);
    return ior * v - (ior * idn + math::sqrt(k)) * n;
}

float math::lengthSqr(const Float3& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float math::length(const Float3& v)
{
    return math::sqrt(lengthSqr(v));
}

float math::dot(const Float3& a, const Float3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float math::distance(const Float3& a, const Float3& b)
{
    return length(a - b);
}

Float3 math::normalize(const Float3& v)
{
    float len = math::length(v);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        return Float3(v.x * inv, v.y * inv, v.z * inv);
    }
    return v;
}

Float3 math::faceForward(const Float3& v, const Float3& incident, const Float3& reference)
{
    if (dot(incident, reference) >= 0.0f)
    {
        return -v;
    }
    return v;
}

Float3 math::reflect(const Float3& v, const Float3& n)
{
    return v - 2.0f * dot(v, n) * n;
}

Float3 math::refract(const Float3& v, const Float3& n, float ior)
{
    float idn = dot(v, n);
    float k = 1.0f - ior * ior * (1.0f - idn * idn);
    if (k < 0.0f) return Float3(0.0f);
    return ior * v - (ior * idn + math::sqrt(k)) * n;
}

Float3 math::cross(const Float3& a, const Float3& b)
{
    return Float3(a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

float math::lengthSqr(const Float4& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

float math::length(const Float4& v)
{
    return math::sqrt(lengthSqr(v));
}

float math::dot(const Float4& a, const Float4& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float math::distance(const Float4& a, const Float4& b)
{
    return length(a - b);
}

Float4 math::normalize(const Float4& v)
{
    float len = math::length(v);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        return Float4(v.x * inv, v.y * inv, v.z * inv, v.w * inv);
    }
    return v;
}

Float4 math::faceForward(const Float4& v, const Float4& incident, const Float4& reference)
{
    if (dot(incident, reference) >= 0.0f)
    {
        return -v;
    }
    return v;
}

Float4 math::reflect(const Float4& v, const Float4& n)
{
    return v - 2.0f * dot(v, n) * n;
}

Float4 math::refract(const Float4& v, const Float4& n, float ior)
{
    float idn = dot(v, n);
    float k = 1.0f - ior * ior * (1.0f - idn * idn);
    if (k < 0.0f) return Float4(0.0f);
    return ior * v - (ior * idn + math::sqrt(k)) * n;
}

Float2x2 math::transpose(const Float2x2& m)
{
    Float2x2 o = m;
    float t = m.data[1];
    o.data[1] = m.data[2];
    o.data[2] = t;
    return o;
}

Float2x2 math::rotate(const Float2x2& m, float r)
{
    float c = math::cos(r);
    float s = math::sin(r);
    float m00 = m.data[0]; float m01 = m.data[1];
    float m10 = m.data[2]; float m11 = m.data[3];

    Float2x2 o = m;
    o.data[0] = m00 * c + m10 * s;
    o.data[1] = m01 * c + m11 * s;
    o.data[2] = m00 * -s + m10 * c;
    o.data[3] = m01 * -s + m11 * c;
    return o;
}

Float2x2 math::scale(const Float2x2& m, const Float2& v)
{
    Float2x2 o = m;
    o.data[0] *= v.x; o.data[1] *= v.x;   // scale row 0 by x
    o.data[2] *= v.y; o.data[3] *= v.y;   // scale row 1 by y
    return o;
}

Float3x3 math::transpose(const Float3x3& m)
{
    Float3x3 o = m;
    float m1 = m.data[1];
    float m2 = m.data[2];
    float m5 = m.data[5];
    o.data[1] = m.data[3];
    o.data[2] = m.data[6];
    o.data[3] = m1;
    o.data[5] = m.data[7];
    o.data[6] = m2;
    o.data[7] = m5;
    return o;
}

Float3x3 math::rotate(const Float3x3& m, float r)
{
    Float3x3 o = m;
    float c = math::cos(r);
    float s = math::sin(r);
    float a00 = m.data[0]; float a01 = m.data[1]; float a02 = m.data[2];
    float a10 = m.data[3]; float a11 = m.data[4]; float a12 = m.data[5];
    o.data[0] = c * a00 + s * a10;
    o.data[1] = c * a01 + s * a11;
    o.data[2] = c * a02 + s * a12;
    o.data[3] = -s * a00 + c * a10;
    o.data[4] = -s * a01 + c * a11;
    o.data[5] = -s * a02 + c * a12;
    return o;
}

Float3x3 math::scale(const Float3x3& m, const Float3& v)
{
    Float3x3 o = m;
    o.data[0] *= v.x; o.data[1] *= v.x; o.data[2] *= v.x;
    o.data[3] *= v.y; o.data[4] *= v.y; o.data[5] *= v.y;
    o.data[6] *= v.z; o.data[7] *= v.z; o.data[8] *= v.z;
    return o;
}

Float3x3 math::translate(const Float3x3& m, const Float3& v)
{
    Float3x3 o = m;
    o.data[6] = m.data[0] * v.x + m.data[3] * v.y + m.data[6];
    o.data[7] = m.data[1] * v.x + m.data[4] * v.y + m.data[7];
    o.data[8] = m.data[2] * v.x + m.data[5] * v.y + m.data[8];
    return o;
}

Float4x4 math::transpose(const Float4x4& m)
{
    float m00 = m.data[0x0]; float m01 = m.data[0x1]; float m02 = m.data[0x2]; float m03 = m.data[0x3];
    float m10 = m.data[0x4]; float m11 = m.data[0x5]; float m12 = m.data[0x6]; float m13 = m.data[0x7];
    float m20 = m.data[0x8]; float m21 = m.data[0x9]; float m22 = m.data[0xA]; float m23 = m.data[0xB];
    float m30 = m.data[0xC]; float m31 = m.data[0xD]; float m32 = m.data[0xE]; float m33 = m.data[0xF];

    Float4x4 r;
    r.data[0x0] = m00; r.data[0x1] = m10; r.data[0x2] = m20; r.data[0x3] = m30;
    r.data[0x4] = m01; r.data[0x5] = m11; r.data[0x6] = m21; r.data[0x7] = m31;
    r.data[0x8] = m02; r.data[0x9] = m12; r.data[0xA] = m22; r.data[0xB] = m32;
    r.data[0xC] = m03; r.data[0xD] = m13; r.data[0xE] = m23; r.data[0xF] = m33;
    return r;
}

Float4x4 math::rotate(const Float4x4& m, const Float3& v)
{
    float cb = math::cos(v.x);
    float sb = math::sin(v.x);
    float ch = math::cos(v.y);
    float sh = math::sin(v.y);
    float ca = math::cos(v.z);
    float sa = math::sin(v.z);
    const float rm[] = {
            ch * ca, sh * sb - ch * sa * cb, ch * sa * sb + sh * cb, 0,
            sa, ca * cb, -ca * sb, 0,
            -sh * ca, sh * sa * cb + ch * sb, -sh * sa * sb + ch * cb, 0,
            0, 0, 0, 1
    };

    float m0 = rm[0] * m.data[0] + rm[1] * m.data[4] + rm[2] * m.data[8] + rm[3] * m.data[12];
    float m1 = rm[0] * m.data[1] + rm[1] * m.data[5] + rm[2] * m.data[9] + rm[3] * m.data[13];
    float m2 = rm[0] * m.data[2] + rm[1] * m.data[6] + rm[2] * m.data[10] + rm[3] * m.data[14];
    float m3 = rm[0] * m.data[3] + rm[1] * m.data[7] + rm[2] * m.data[11] + rm[3] * m.data[15];
    float m4 = rm[4] * m.data[0] + rm[5] * m.data[4] + rm[6] * m.data[8] + rm[7] * m.data[12];
    float m5 = rm[4] * m.data[1] + rm[5] * m.data[5] + rm[6] * m.data[9] + rm[7] * m.data[13];
    float m6 = rm[4] * m.data[2] + rm[5] * m.data[6] + rm[6] * m.data[10] + rm[7] * m.data[14];
    float m7 = rm[4] * m.data[3] + rm[5] * m.data[7] + rm[6] * m.data[11] + rm[7] * m.data[15];
    float m8 = rm[8] * m.data[0] + rm[9] * m.data[4] + rm[10] * m.data[8] + rm[11] * m.data[12];
    float m9 = rm[8] * m.data[1] + rm[9] * m.data[5] + rm[10] * m.data[9] + rm[11] * m.data[13];
    float m10 = rm[8] * m.data[2] + rm[9] * m.data[6] + rm[10] * m.data[10] + rm[11] * m.data[14];
    float m11 = rm[8] * m.data[3] + rm[9] * m.data[7] + rm[10] * m.data[11] + rm[11] * m.data[15];
    float m12 = rm[12] * m.data[0] + rm[13] * m.data[4] + rm[14] * m.data[8] + rm[15] * m.data[12];
    float m13 = rm[12] * m.data[1] + rm[13] * m.data[5] + rm[14] * m.data[9] + rm[15] * m.data[13];
    float m14 = rm[12] * m.data[2] + rm[13] * m.data[6] + rm[14] * m.data[10] + rm[15] * m.data[14];
    float m15 = rm[12] * m.data[3] + rm[13] * m.data[7] + rm[14] * m.data[11] + rm[15] * m.data[15];

    Float4x4 r;
    r.data[0] = m0;
    r.data[1] = m1;
    r.data[2] = m2;
    r.data[3] = m3;
    r.data[4] = m4;
    r.data[5] = m5;
    r.data[6] = m6;
    r.data[7] = m7;
    r.data[8] = m8;
    r.data[9] = m9;
    r.data[10] = m10;
    r.data[11] = m11;
    r.data[12] = m12;
    r.data[13] = m13;
    r.data[14] = m14;
    r.data[15] = m15;
    return r;
}

Float4x4 math::scale(const Float4x4& m, const Float3& v)
{
    Float4x4 r = m;
    r.data[0] *= v.x; r.data[1] *= v.x; r.data[2] *= v.x; r.data[3] *= v.x; // col 0
    r.data[4] *= v.y; r.data[5] *= v.y; r.data[6] *= v.y; r.data[7] *= v.y; // col 1
    r.data[8] *= v.z; r.data[9] *= v.z; r.data[10] *= v.z; r.data[11] *= v.z; // col 2
    return r;
}

Float4x4 math::translate(const Float4x4& m, const Float3& v)
{
    Float4x4 r = m;
    // last column = m * [v,1]
    r.data[12] = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12];
    r.data[13] = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13];
    r.data[14] = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14];
    r.data[15] = m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15];
    return r;
}

Float4x4 math::rotateX(const Float4x4& m, float rads)
{
    const float s = math::sin(rads), c = math::cos(rads);
    // R_x affects Y/Z columns
    Float4x4 r = m;
    // col 1
    float y0 = m.data[4], y1 = m.data[5], y2 = m.data[6], y3 = m.data[7];
    // col 2
    float z0 = m.data[8], z1 = m.data[9], z2 = m.data[10], z3 = m.data[11];
    // apply rotation to these columns
    r.data[4] = c * y0 + s * z0; r.data[5] = c * y1 + s * z1; r.data[6] = c * y2 + s * z2; r.data[7] = c * y3 + s * z3;
    r.data[8] = -s * y0 + c * z0; r.data[9] = -s * y1 + c * z1; r.data[10] = -s * y2 + c * z2; r.data[11] = -s * y3 + c * z3;
    return r;
}

Float4x4 math::rotateY(const Float4x4& m, float rads)
{
    const float s = math::sin(rads), c = math::cos(rads);
    Float4x4 r = m;
    // X/Z columns
    float x0 = m.data[0], x1 = m.data[1], x2 = m.data[2], x3 = m.data[3];
    float z0 = m.data[8], z1 = m.data[9], z2 = m.data[10], z3 = m.data[11];
    r.data[0] = c * x0 - s * z0; r.data[1] = c * x1 - s * z1; r.data[2] = c * x2 - s * z2; r.data[3] = c * x3 - s * z3;
    r.data[8] = s * x0 + c * z0; r.data[9] = s * x1 + c * z1; r.data[10] = s * x2 + c * z2; r.data[11] = s * x3 + c * z3;
    return r;
}

Float4x4 math::rotateZ(const Float4x4& m, float rads)
{
    const float s = math::sin(rads), c = math::cos(rads);
    Float4x4 r = m;
    // X/Y columns
    float x0 = m.data[0], x1 = m.data[1], x2 = m.data[2], x3 = m.data[3];
    float y0 = m.data[4], y1 = m.data[5], y2 = m.data[6], y3 = m.data[7];
    r.data[0] = c * x0 + s * y0; r.data[1] = c * x1 + s * y1; r.data[2] = c * x2 + s * y2; r.data[3] = c * x3 + s * y3;
    r.data[4] = -s * x0 + c * y0; r.data[5] = -s * x1 + c * y1; r.data[6] = -s * x2 + c * y2; r.data[7] = -s * x3 + c * y3;
    return r;
}

Float4x4 math::perspective(float fovy, float aspect, float zn, float zf)
{
    const float f = 1.0f / math::tan(fovy * 0.5f);
    Float4x4 m; // identity first
    // zero then set:
    for (int i = 0; i < 16; ++i) m.data[i] = 0.0f;

    m.data[0] = f / aspect; // col 0, row 0
    m.data[5] = f;          // col 1, row 1
    m.data[10] = (zf + zn) / (zn - zf); // col 2, row 2
    m.data[14] = (2.0f * zf * zn) / (zn - zf); // col 2, row 3
    m.data[11] = -1.0f;      // col 3, row 2
    // m.data[15] = 0
    return m;
}

Float4x4 math::orthographic(float l, float r, float b, float t, float zn, float zf)
{
    Float4x4 m; for (int i = 0; i < 16; ++i) m.data[i] = 0.0f;
    m.data[0] = 2.0f / (r - l);
    m.data[5] = 2.0f / (t - b);
    m.data[10] = -2.0f / (zf - zn);
    m.data[12] = -(r + l) / (r - l);
    m.data[13] = -(t + b) / (t - b);
    m.data[14] = -(zf + zn) / (zf - zn);
    m.data[15] = 1.0f;
    return m;
}

Float4x4 math::lookAt(const Float3& eye, const Float3& center, const Float3& up)
{
    Float3 f = math::normalize(Float3(center.x - eye.x, center.y - eye.y, center.z - eye.z));
    Float3 s = math::normalize(math::cross(f, up));   // right
    Float3 u = math::cross(s, f);                     // up corrected

    Float4x4 m; // identity then fill columns (basis vectors go in columns)
    m.data[0] = s.x; m.data[1] = u.x; m.data[2] = -f.x; m.data[3] = 0.0f;
    m.data[4] = s.y; m.data[5] = u.y; m.data[6] = -f.y; m.data[7] = 0.0f;
    m.data[8] = s.z; m.data[9] = u.z; m.data[10] = -f.z; m.data[11] = 0.0f;
    m.data[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    m.data[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    m.data[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    m.data[15] = 1.0f;
    return m;
}

Float4x4 math::invert(const Float4x4& m)
{
    float d0 = m.data[0] * m.data[5] - m.data[1] * m.data[4];
    float d1 = m.data[0] * m.data[6] - m.data[2] * m.data[4];
    float d2 = m.data[0] * m.data[7] - m.data[3] * m.data[4];
    float d3 = m.data[1] * m.data[6] - m.data[2] * m.data[5];
    float d4 = m.data[1] * m.data[7] - m.data[3] * m.data[5];
    float d5 = m.data[2] * m.data[7] - m.data[3] * m.data[6];
    float d6 = m.data[8] * m.data[13] - m.data[9] * m.data[12];
    float d7 = m.data[8] * m.data[14] - m.data[10] * m.data[12];
    float d8 = m.data[8] * m.data[15] - m.data[11] * m.data[12];
    float d9 = m.data[9] * m.data[14] - m.data[10] * m.data[13];
    float d10 = m.data[9] * m.data[15] - m.data[11] * m.data[13];
    float d11 = m.data[10] * m.data[15] - m.data[11] * m.data[14];
    float determinant = d0 * d11 - d1 * d10 + d2 * d9 + d3 * d8 - d4 * d7 + d5 * d6;

    if (determinant == 0.0f) return m;

    determinant = 1.0f / determinant;
    float m00 = (m.data[5] * d11 - m.data[6] * d10 + m.data[7] * d9) * determinant;
    float m01 = (m.data[2] * d10 - m.data[1] * d11 - m.data[3] * d9) * determinant;
    float m02 = (m.data[13] * d5 - m.data[14] * d4 + m.data[15] * d3) * determinant;
    float m03 = (m.data[10] * d4 - m.data[9] * d5 - m.data[11] * d3) * determinant;
    float m04 = (m.data[6] * d8 - m.data[4] * d11 - m.data[7] * d7) * determinant;
    float m05 = (m.data[0] * d11 - m.data[2] * d8 + m.data[3] * d7) * determinant;
    float m06 = (m.data[14] * d2 - m.data[12] * d5 - m.data[15] * d1) * determinant;
    float m07 = (m.data[8] * d5 - m.data[10] * d2 + m.data[11] * d1) * determinant;
    float m08 = (m.data[4] * d10 - m.data[5] * d8 + m.data[7] * d6) * determinant;
    float m09 = (m.data[1] * d8 - m.data[0] * d10 - m.data[3] * d6) * determinant;
    float m10 = (m.data[12] * d4 - m.data[13] * d2 + m.data[15] * d0) * determinant;
    float m11 = (m.data[9] * d2 - m.data[8] * d4 - m.data[11] * d0) * determinant;
    float m12 = (m.data[5] * d7 - m.data[4] * d9 - m.data[6] * d6) * determinant;
    float m13 = (m.data[0] * d9 - m.data[1] * d7 + m.data[2] * d6) * determinant;
    float m14 = (m.data[13] * d1 - m.data[12] * d3 - m.data[14] * d0) * determinant;
    float m15 = (m.data[8] * d3 - m.data[9] * d1 + m.data[10] * d0) * determinant;

    Float4x4 o;
    o.data[0] = m00;
    o.data[1] = m01;
    o.data[2] = m02;
    o.data[3] = m03;
    o.data[4] = m04;
    o.data[5] = m05;
    o.data[6] = m06;
    o.data[7] = m07;
    o.data[8] = m08;
    o.data[9] = m09;
    o.data[10] = m10;
    o.data[11] = m11;
    o.data[12] = m12;
    o.data[13] = m13;
    o.data[14] = m14;
    o.data[15] = m15;
    return o;
}

Quat math::fromEuler(const Float3& e)
{
    float cx = cosf(e.x * 0.5f), sx = sinf(e.x * 0.5f);
    float cy = cosf(e.y * 0.5f), sy = sinf(e.y * 0.5f);
    float cz = cosf(e.z * 0.5f), sz = sinf(e.z * 0.5f);

    Quat q;
    q.w = cz * cy * cx + sz * sy * sx;
    q.x = cz * cy * sx - sz * sy * cx;
    q.y = cz * sy * cx + sz * cy * sx;
    q.z = sz * cy * cx - cz * sy * sx;
    return q;
}

Float3 math::toEuler(const Quat& q)
{
    // roll (x-axis)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    float roll = atan2f(sinr_cosp, cosr_cosp);

    // pitch (y-axis)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    float pitch = (fabsf(sinp) >= 1.0f) ? copysignf(math::PI / 2.0f, sinp) : asinf(sinp);

    // yaw (z-axis)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    float yaw = atan2f(siny_cosp, cosy_cosp);

    return Float3(roll, pitch, yaw);
}

Quat math::rotateX(const Quat& q, float r)
{
    float s = sinf(r * 0.5f), c = cosf(r * 0.5f);
    Quat dx(s, 0.0f, 0.0f, c);
    return (dx * q);
}

Quat math::rotateY(const Quat& q, float r)
{
    float s = sinf(r * 0.5f), c = cosf(r * 0.5f);
    Quat dy(0.0f, s, 0.0f, c);
    return (dy * q);
}

Quat math::rotateZ(const Quat& q, float r)
{
    float s = sinf(r * 0.5f), c = cosf(r * 0.5f);
    Quat dz(0.0f, 0.0f, s, c);
    return (dz * q);
}

Float4x4 math::toFloat4x4(const Quat& q)
{
    const float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    const float xx = q.x * x2, yy = q.y * y2, zz = q.z * z2;
    const float xy = q.x * y2, xz = q.x * z2, yz = q.y * z2;
    const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;

    // column-major
    return Float4x4(
        1.0f - (yy + zz), xy + wz, xz - wy, 0.0f,
        xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f,
        xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix2D math::translate(const Matrix2D& m, float x, float y)
{
    Matrix2D o = m;
    float ttx = m.a * x + m.c * y + m.tx;
    float tty = m.b * x + m.d * y + m.ty;
    o.tx = ttx;
    o.ty = tty;
    return o;
}

Matrix2D math::translate(const Matrix2D& m, Float2& v)
{
    return translate(m, v.x, v.y);
}

Matrix2D math::scale(const Matrix2D& m, float x, float y)
{
    Matrix2D o = m;
    o.a = m.a * x;
    o.b = m.b * x;
    o.c = m.c * y;
    o.d = m.d * y;
    return o;
}

Matrix2D math::scale(const Matrix2D& m, Float2& v)
{
    return scale(m, v.x, v.y);
}

Matrix2D math::rotate(const Matrix2D& m, float r)
{
    float cr = math::cos(r);
    float sr = math::sin(r);
    float m0 = cr * m.a + sr * m.c;
    float m1 = cr * m.b + sr * m.d;
    float m2 = -sr * m.a + cr * m.c;
    float m3 = -sr * m.b + cr * m.d;
    Matrix2D o = m;
    o.a = m0;
    o.b = m1;
    o.c = m2;
    o.d = m3;
    return o;
}

inline bool isArrayNaN(const float* arr, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (isnan(arr[i]))
        {
            return true;
        }
    }
    return false;
}

inline bool isArrayInf(const float* arr, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (isnan(arr[i]))
        {
            return true;
        }
    }
    return false;
}

bool math::isNaN(float v)
{
    return isnan(v);
}

bool math::isNaN(Float2 v)
{
    return isArrayNaN(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isNaN(Float3 v)
{
    return isArrayNaN(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isNaN(Float4 v)
{
    return isArrayNaN(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isNaN(Float2x2 v)
{
    return isArrayNaN(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isNaN(Float3x3 v)
{
    return isArrayNaN(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isNaN(Float4x4 v)
{
    return isArrayNaN(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isNaN(Quat v)
{
    return isArrayNaN(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isNaN(Matrix2D v)
{
    return isArrayNaN(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isInf(float v)
{
    return isinf(v);
}

bool math::isInf(Float2 v)
{
    return isArrayInf(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isInf(Float3 v)
{
    return isArrayInf(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isInf(Float4 v)
{
    return isArrayInf(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isInf(Float2x2 v)
{
    return isArrayInf(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isInf(Float3x3 v)
{
    return isArrayInf(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isInf(Float4x4 v)
{
    return isArrayInf(v.data, sizeof(v.data) / sizeof(v.data[0]));
}

bool math::isInf(Quat v)
{
    return isArrayInf(v.components, sizeof(v.components) / sizeof(v.components[0]));
}

bool math::isInf(Matrix2D v)
{
    return isArrayInf(v.components, sizeof(v.components) / sizeof(v.components[0]));
}