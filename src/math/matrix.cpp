#include "math/matrix.h"

#include <cassert>
#include <cmath>

math::Mat4::Mat4(
	float m11, float m12, float m13, float m14,
	float m21, float m22, float m23, float m24,
	float m31, float m32, float m33, float m34,
	float m41, float m42, float m43, float m44) :
	cols{
		Vec4{m11,m12,m13,m14},
		Vec4{m21,m22,m23,m24},
		Vec4{m31,m32,m33,m34},
		Vec4{m41,m42,m43,m44}
	}
{

}

math::Mat4::Mat4(const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3):
	cols{c0,c1,c2,c3}
{

}


math::Vec4 math::Mat4::operator[](size_t index) const
{
	assert(index <= 3);
	return cols[index];
}

math::Vec4& math::Mat4::operator[](size_t index)
{
	assert(index <= 3);
	return cols[index];
}

math::Mat4 math::Mat4::operator+(const Mat4& rhs) const
{
	return Mat4{
		cols[0] + rhs[0],
		cols[1] + rhs[1],
		cols[2] + rhs[2],
		cols[3] + rhs[3]
	};
}

math::Mat4 math::Mat4::operator-(const Mat4& rhs) const
{
	return Mat4{
		cols[0] - rhs[0],
		cols[1] - rhs[1],
		cols[2] - rhs[2],
		cols[3] - rhs[3]
	};
}

math::Vec4 math::Mat4::operator*(const Vec4& rhs) const
{
	return math::Vec4{
		Vec4{cols[0][0],cols[1][0],cols[2][0],cols[3][0]}.dot(rhs),
		Vec4{cols[0][1],cols[1][1],cols[2][1],cols[3][1]}.dot(rhs),
		Vec4{cols[0][2],cols[1][2],cols[2][2],cols[3][2]}.dot(rhs),
		Vec4{cols[0][3],cols[1][3],cols[2][3],cols[3][3]}.dot(rhs)
	};
}

math::Mat4 math::Mat4::operator*(const Mat4& rhs) const
{
	return Mat4{
		(*this) * rhs[0],
		(*this) * rhs[1],
		(*this) * rhs[2],
		(*this) * rhs[3],
	};
}

math::Mat4 math::Mat4::operator*(const float rhs) const
{

	return Mat4{
		cols[0]*rhs,
		cols[1] * rhs,
		cols[2] * rhs,
		cols[3] * rhs
	};
}

math::Mat4 math::Mat4::transpose() const
{
	Mat4 m{};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m[i][j] = (*this)[j][i];
		}
	}
	return m;
}


float* math::Mat4::getRawData()
{
	return (float*)data;
}

math::Mat4 math::Mat4::identity()
{
	return Mat4{};
}

math::Mat4 math::Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
	auto front = (center - eye).normalize();
	auto right = front.cross(up).normalize();
	auto newUp = right.cross(front).normalize();

	Mat4 m{};
	m[0] = Vec4{ right.x(),newUp.x(),-front.x(),0.0 };
	m[1] = Vec4{ right.y(),newUp.y(),-front.y(),0.0};
	m[2] = Vec4{ right.z(),newUp.z(),-front.z(),0.0};
	m[3] = Vec4{-eye.dot(right),-eye.dot(newUp),eye.dot(front)};
	return m;
}

math::Mat4 math::Mat4::fromScale(const Vec3& v) 
{
	return Mat4{
		Vec4{v.x(),0.0,0.0,0.0},
		Vec4{0.0,v.y(),0.0,0.0},
		Vec4{0.0,0.0,v.z(),0.0},
		Vec4{0.0,0.0,0.0,1.0}
	};
}

math::Mat4 math::Mat4::fromScale(float x, float y, float z) 
{
	return Mat4{
		Vec4{x,0.0,0.0,0.0},
		Vec4{0.0,y,0.0,0.0},
		Vec4{0.0,0.0,z,0.0},
		Vec4{0.0,0.0,0.0,1.0}
	};
}

math::Mat4 math::Mat4::fromRotateXAxis(float angle)
{
	float c = cos(angle);
	float s = sin(angle);

	return Mat4{
			Vec4{1.0,0.0,0.0,0.0},
			Vec4{0.0,c,s,0.0},
			Vec4{0.0,-s,c,0.0},
			Vec4{0.0,0.0,0.0,1.0}
	};
}

math::Mat4 math::Mat4::fromRotateYAxis(float angle)
{
	float c = cos(angle);
	float s = sin(angle);

	return Mat4{
		Vec4{c,0.0,-s,0.0},
		Vec4{0.0,1.0,0.0,0.0},
		Vec4{s,0.0,c,0.0},
		Vec4{0.0,0.0,0.0,1.0}
	};
}

math::Mat4 math::Mat4::fromRotateZAxis(float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	return Mat4{
		Vec4{c,s,0.0,0.0},
		Vec4{-s,c,0.0,0.0},
		Vec4{0.0,0.0,1.0,0.0},
		Vec4{0.0,0.0,0.0,1.0}
	};
}

math::Mat4 math::Mat4::fromAxisAngle(float angle, const Vec3& axis) 
{
	float c = cos(angle);
	float s = sin(angle);
	Vec3 u = axis.normalize();

	float uxx = u.x() * u.x();
	float uxy = u.x() * u.y();
	float uxz = u.x() * u.z();
	float uyy = u.y() * u.y();
	float uyz = u.y() * u.z();
	float uzz = u.z() * u.z();

	//TODO: Fix

	return Mat4{};
}


math::Mat4 math::Mat4::fromTranslation(const Vec3& v) 
{
	return Mat4{
		Vec4{1.0,0.0,0.0,0.0},
		Vec4{0.0,1.0,0.0,0.0},
		Vec4{0.0,0.0,1.0,0.0},
		Vec4{v.x(),v.y(),v.z(),1.0}
	};
}

math::Mat4 math::Mat4::fromTranslation(float x, float y, float z) 
{
	return Mat4{
		Vec4{1.0,0.0,0.0,0.0},
		Vec4{0.0,1.0,0.0,0.0},
		Vec4{0.0,0.0,1.0,0.0},
		Vec4{x,y,z,1.0}
	};
}

math::Mat4 math::Mat4::orthographicProjection(float left, float right, float bottom, float top, float near, float far)
{
	Mat4 m{};
	m[0][0] = 2.0f / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[2][2] = -2.0f / (far - near);
	m[3][0] = - (right + left) / (right - left);
	m[3][1] = - (top + bottom) / (top - bottom);
	m[3][2] = - (far + near) / (far - near);
	return m;
}

math::Mat4 math::Mat4::orthographicProjectionVk(float left, float right, float bottom, float top, float near, float far)
{
	Mat4 m{};
	m[0][0] = 2.0f / (right - left);
	m[1][1] = -2.0f / (bottom - top);
	m[2][2] = -1.0f / (far - near);
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (bottom - top);
	m[3][2] = -near / (far - near);
	return m;
}

math::Mat4 math::Mat4::perspectiveProjection(float fovy, float aspectRatio, float near, float far)
{
	Mat4 m{};
	float ty = tan(fovy * 0.5);
	float tx = ty * aspectRatio;
	m[0][0] = 1.0 / tx;
	m[1][1] = 1.0 / ty;
	m[2][2] = - (far + near) / (far - near);
	m[3][2] = - 2 * far * near / (far - near);
	m[2][3] = - 1.0;
	m[3][3] = 0.0;

	return m;
}

math::Mat4 math::Mat4::perspectiveProjectionVk(float fovy, float aspectRatio, float near, float far)
{
	Mat4 m{};
	float ty = tan(fovy * 0.5);
	float tx = ty * aspectRatio;
	m[0][0] = 1.0 / tx;
	m[1][1] = -1.0 / ty;
	m[2][2] = -far / (far - near);
	m[3][2] = - far * near / (far - near);
	m[2][3] = -1.0;
	m[3][3] = 0.0;

	return m;
}

std::ostream& math::operator<<(std::ostream& out, const Mat4& v)
{
	out << "| " << v[0][0] << " " << v[1][0] << " " << v[2][0] << " " << v[3][0] << " |\n";
	out << "| " << v[0][1] << " " << v[1][1] << " " << v[2][1] << " " << v[3][1] << " |\n";
	out << "| " << v[0][2] << " " << v[1][2] << " " << v[2][2] << " " << v[3][2] << " |\n";
	out << "| " << v[0][3] << " " << v[1][3] << " " << v[2][3] << " " << v[3][3] << " |\n";

	return out;
}
