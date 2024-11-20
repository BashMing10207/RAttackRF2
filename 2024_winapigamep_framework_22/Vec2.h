#pragma once
#include"pch.h"
#include<assert.h>
struct Vec2
{
public:
	float x = 0.f;
	float y = 0.f;
public:
	Vec2() = default;
	Vec2(float _x, float _y) : x(_x), y(_y) {}
	Vec2(POINT _pt) : x((float)_pt.x), y((float)_pt.y) {}
	//Vec2(int _x, int _y) : x((float)_x), y((float)_y) {}
	Vec2(const Vec2& _other) : x(_other.x), y(_other.y) {}
public:
	Vec2 operator + (const Vec2& _vOther)
	{
		return Vec2(x + _vOther.x, y + _vOther.y);
	}
	Vec2 operator + (Vec2& _vOther)
	{
		return Vec2(x + _vOther.x, y + _vOther.y);
	}
	Vec2 operator - (const Vec2& _vOther)
	{
		return Vec2(x - _vOther.x, y - _vOther.y);
	}
	Vec2 operator * (const Vec2& _vOther)
	{
		return Vec2(x * _vOther.x, y * _vOther.y);
	}
	Vec2 operator * (float _val)
	{
		return Vec2(x * _val, y * _val);
	}
	Vec2 operator / (float _val)
	{
		return Vec2(x / _val, y / _val);
	}
	Vec2 operator / (const Vec2& _vOther)
	{
		assert(!(0.f == _vOther.x || 0.f == _vOther.y));
		return Vec2(x / _vOther.x, y / _vOther.y);
	}
	void operator+=(const Vec2& _other)
	{
		x += _other.x;
		y += _other.y;
	}
	void operator-=(const Vec2& _other)
	{
		x -= _other.x;
		y -= _other.y;
	}
	bool operator==(Vec2& _other)
	{
		bool a = true;
		a &= x == _other.x;
		a&= y == _other.y;
		return a;
	}

	static float Lerp(float a, float b, float t)
	{
		return a * (t - 1) + b * t;
	}

	double magnitude() const {
		return sqrt(x * x + y * y);
	}
	double dot(const Vec2& other) const {
		return x * other.x + y * other.y;
	}
	float LengthSquared()
	{
		return x * x + y * y;
	}
	float Length()
	{
		return ::sqrt(LengthSquared());
	}
	Vec2 Normalize()
	{
		float len = Length();
		// 0�̸� �ȵ�.
		if (len < FLT_EPSILON)
			return Vec2(0,0);
		return Vec2(x / len, y / len);
	}
	static Vec2 Lerpv(Vec2 a, Vec2 b, float t)
	{
		return Vec2(Lerp(a.x, b.x,t), Lerp(a.y, b.y,t));
	}
	float Dot(Vec2 _other)
	{
		return x * _other.x + y * _other.y;
	}
	float Cross(Vec2 _other)
	{
		// z���� ���´ٰ� ����
		return x * _other.y - y * _other.x;
	}
	static Vec2 A2BLineAndPoint( Vec2& A,Vec2& B, Vec2& P) {
		// ���� AP�� AB ����
		Vec2 AP = P - A;
		Vec2 AB = B - A;

		// ������ ���� ���� ���
		double AB_squared = AB.dot(AB);

		// ������ ���̰� 0�� ���, A�� �� P ������ ���� ��ȯ
		if (AB_squared == 0.0) {
			return AP;
		}

		// AB ���Ϳ� ���� AP ������ ���� ���� t ���
		double t = AP.dot(AB) / AB_squared;

		// t�� [0, 1] ������ ����� ������ ���� �� �ϳ��κ��� �ִ� �Ÿ� ���� ��ȯ
		if (t < 0.0) {
			return AP;  // A���� P���� ����
		}
		else if (t > 1.0) {
			return P - B;  // B���� P���� ����
		}

		// ������ ��ǥ ���
		Vec2 projection = A + AB * t;

		// ���������� �� P�� ���� �ִ� �Ÿ� ����
		return P - projection;
	}

};