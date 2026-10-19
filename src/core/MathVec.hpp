#pragma once
#include <cmath>
#include <algorithm>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        float l = length();
        return (l > 1e-6f) ? (*this / l) : Vec2(0.0f, 0.0f);
    }

    Vec2 rotated(float angleRad) const {
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        return {x * c - y * s, x * s + y * c};
    }

    static float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    static float cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
    static Vec2 cross(float w, const Vec2& r) { return {-w * r.y, w * r.x}; }
    static float dist(const Vec2& a, const Vec2& b) { return (a - b).length(); }

    static float distToSegment(const Vec2& p, const Vec2& a, const Vec2& b) {
        Vec2 ab = b - a;
        Vec2 ap = p - a;
        float lenSq = ab.lengthSq();
        if (lenSq <= 1e-6f) return ap.length();
        float t = std::clamp(dot(ap, ab) / lenSq, 0.0f, 1.0f);
        Vec2 closest = a + ab * t;
        return (p - closest).length();
    }
};