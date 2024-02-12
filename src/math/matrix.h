#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>

#include "math/vec.h"


namespace math {

	/*Mat4*/

	/*Note: data[i][j] is the (j,i) entry of the matrix due to column-major order*/

	class Mat4 {
	public:
		Mat4(
			float m11 = 1.0, float m12 = 0.0, float m13 = 0.0, float m14 = 0.0,
			float m21 = 0.0, float m22 = 1.0, float m23 = 0.0, float m24 = 0.0,
			float m31 = 0.0, float m32 = 0.0, float m33 = 1.0, float m34 = 0.0,
			float m41 = 0.0, float m42 = 0.0, float m43 = 0.0, float m44 = 1.0
		);

		Mat4(
			const Vec4& c0,
			const Vec4& c1,
			const Vec4& c2,
			const Vec4& c3
		);

		/*Matrix element read/write access*/
		Vec4 operator[](size_t index) const;
		Vec4& operator[](size_t index);

		/*Matrix algebra*/
		Mat4 operator+(const Mat4& rhs) const;
		Mat4 operator-(const Mat4& rhs) const;
		Vec4 operator*(const Vec4& rhs) const;
		Mat4 operator*(const Mat4& rhs) const;
		Mat4 operator*(const float rhs) const;
		Mat4 transpose() const;


		/*Get raw data location for sending to GPU*/
		float* getRawData();

		/*Static methods for generating common matrices*/
		static Mat4 identity();
		static Mat4 lookAt(const Vec3& eye,const Vec3& center,const Vec3& up);
		
		//Scale
		static Mat4 fromScale(const Vec3& v);
		static Mat4 fromScale(float x, float y, float z);

		//Rotation
		static Mat4 fromRotateXAxis(float angle);
		static Mat4 fromRotateYAxis(float angle);
		static Mat4 fromRotateZAxis(float angle);
		static Mat4 fromAxisAngle(float angle, const Vec3& axis);
		//TODO: rotation from quaternion

		//Translation
		static Mat4 fromTranslation(const Vec3& v);
		static Mat4 fromTranslation(float x, float y, float z);

		//Projection matrices
		static Mat4 orthographicProjection(float left, float right, float bottom, float top, float near, float far);
		static Mat4 orthographicProjectionVk(float left, float right, float bottom, float top, float near, float far);


		static Mat4 perspectiveProjection(float fovy, float aspectRatio, float near, float far);
		static Mat4 perspectiveProjectionVk(float fovy, float aspectRatio, float near, float far);



	private:
		union {
			Vec4 cols[4] = {};
			float data[16];
		};

	};

	std::ostream& operator<<(std::ostream& out, const Mat4& v);

}

#endif