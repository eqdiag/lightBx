#ifndef VEC_H
#define VEC_H

#include <iostream>

namespace math {

	const float NEARLY_ZERO = 0.00001f;

	/*Vec2*/

	class Vec2 {
	public:
		Vec2(float x = 0.0, float y = 0.0);

		/*Member access*/
		float x() const;
		float y() const;
		float r() const;
		float g() const;
		float operator[](size_t index) const;
		float& operator[](size_t index);


		/*Vector space operations*/
		Vec2 operator+(const Vec2& rhs) const;
		Vec2 operator+=(const Vec2& rhs);
		Vec2 operator-(const Vec2& rhs) const;
		Vec2 operator-=(const Vec2& rhs);
		Vec2 operator*(float rhs) const;
		Vec2 operator*=(float rhs);
		Vec2 operator/(float rhs) const;
		Vec2 operator/=(float rhs);
		bool operator==(const Vec2& rhs) const;
		Vec2 operator-() const;

		/*Get raw data location for sending to GPU*/
		float* getRawData();

		/*Inner product and cross product*/
		float dot(const Vec2& rhs) const;
		float norm2() const;
		float norm() const;
		Vec2 normalize() const;


	private:
		union {
			struct {
				float mX, mY;
			};
			float data[2] = {};
		};
	};

	std::ostream& operator<<(std::ostream& out, const Vec2& v);


	/*Vec3*/

	class Vec3 {
	public:
		Vec3(float x = 0.0, float y = 0.0, float z = 0.0);

		/*Member access*/
		float x() const;
		float y() const;
		float z() const;
		float r() const;
		float g() const;
		float b() const;
		float operator[](size_t index) const;
		float& operator[](size_t index);


		/*Vector space operations*/
		Vec3 operator+(const Vec3& rhs) const;
		Vec3 operator+=(const Vec3& rhs);
		Vec3 operator-(const Vec3& rhs) const;
		Vec3 operator-=(const Vec3& rhs);
		Vec3 operator*(float rhs) const;
		Vec3 operator*=(float rhs);
		Vec3 operator/(float rhs) const;
		Vec3 operator/=(float rhs);
		bool operator==(const Vec3& rhs) const;
		Vec3 operator-() const;

		/*Get raw data location for sending to GPU*/
		float* getRawData();

		/*Inner product and cross product*/
		float dot(const Vec3& rhs) const;
		Vec3 cross(const Vec3& rhs) const;
		float norm2() const;
		float norm() const;
		Vec3 normalize() const;


	private:
		union {
			struct {
				float mX, mY, mZ;
			};
			float data[3] = {};
		};
	};

	std::ostream& operator<<(std::ostream& out, const Vec3& v);

	/*Vec4*/

	class Vec4 {
	public:
		Vec4(float x = 0.0, float y = 0.0, float z = 0.0,float w = 1.0);

		/*Member access*/
		float x() const;
		float y() const;
		float z() const;
		float w() const;
		float r() const;
		float g() const;
		float b() const;
		float a() const;
		float operator[](size_t index) const;
		float& operator[](size_t index);


		/*Vector space operations*/
		Vec4 operator+(const Vec4& rhs) const;
		void operator+=(const Vec4& rhs);
		Vec4 operator-(const Vec4& rhs) const;
		void operator-=(const Vec4& rhs);
		Vec4 operator*(float rhs) const;
		void operator*=(float rhs);
		Vec4 operator/(float rhs) const;
		void operator/=(float rhs);
		bool operator==(const Vec4& rhs) const;
		Vec4 operator-() const;

		/*Get raw data location for sending to GPU*/
		float* getRawData();

		/*Inner product and cross product*/
		float dot(const Vec4& rhs) const;
		float norm2() const;
		float norm() const;
		Vec4 normalize() const;


	private:
		union {
			struct {
				float mX, mY, mZ,mW;
			};
			float data[4] = {};
		};
	};

	std::ostream& operator<<(std::ostream& out, const Vec4& v);

	/*Color3,Color4*/
	using Color3 = Vec3;
	using Color4 = Vec4;

}



#endif