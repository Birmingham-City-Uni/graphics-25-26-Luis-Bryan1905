# pragma once
# include <Eigen/Dense>

// This convenience function converts a 3D vector to a 4D homogeneous vector,
// setting the w component to 1.
Eigen::Vector4f vec3ToVec4(const Eigen::Vector3f& v)
{
	Eigen::Vector4f output;
	output << v.x(), v.y(), v.z(), 1.0f;
	return output;
}

// This is the 2D "cross product" function you implemented last week.
float vec2Cross(const Eigen::Vector2f& v0, const Eigen::Vector2f& v1)
{
	return v0.x() * v1.y() - v0.y() * v1.x();
}

float vec2CrossXY(const Eigen::Vector3f& v0, const Eigen::Vector3f& v1)
{
	return v0.x() * v1.y() - v0.y() * v1.x();
}

Eigen::Vector2f v2(const Eigen::Vector3f& v)
{
	return v.block<2, 1>(0, 0);
}

// ============ TASK 2 =================
Eigen::Matrix4f translationMatrix(const Eigen::Vector3f& t)
{
	// *** Your code here ***
	Eigen::Matrix4f TranslationMatrix = Eigen::Matrix4f::Identity();
	TranslationMatrix.block<3, 1>(0, 3) = t;

	return TranslationMatrix;
}

// Implement this function that makes a uniform scaling matrix
Eigen::Matrix4f scaleMatrix(float s)
{
	// *** Your code here ***
	Eigen::Matrix4f ScalingMatrix = Eigen::Matrix4f::Identity();

	ScalingMatrix(0, 0) = s;
	ScalingMatrix(1, 1) = s;
	ScalingMatrix(2, 2) = s;


	return ScalingMatrix;
}

Eigen::Matrix4f rotateXMatrix(float theta)
{
	// *** Your code here ***
	Eigen::Matrix4f RotatingXMatrix = Eigen::Matrix4f::Identity();
	float cos = std::cos(theta);
	float sin = std::sin(theta);
	// rotation around X:
	// [  1  0  0 ]
	// [  0  c -s ]
	// [  0  s  c ]

	RotatingXMatrix(0, 0) = 1;  RotatingXMatrix(0, 1) = 0;   RotatingXMatrix(0, 2) = 0;

	RotatingXMatrix(1, 0) = 0;  RotatingXMatrix(1, 1) = cos; RotatingXMatrix(1, 2) = -sin;

	RotatingXMatrix(2, 0) = 0;  RotatingXMatrix(2, 1) = sin; RotatingXMatrix(2, 2) = cos;


	return RotatingXMatrix;
}
Eigen::Matrix4f rotateYMatrix(float theta)
{
	Eigen::Matrix4f RotatingYMatrix = Eigen::Matrix4f::Identity();
	float cos = std::cos(theta);
	float sin = std::sin(theta);
	// rotation around Y:
	// [  c  0  s ]
	// [  0  1  0 ]
	// [ -s  0  c ]
	RotatingYMatrix(0, 0) = cos;   RotatingYMatrix(0, 1) = 0; RotatingYMatrix(0, 2) = sin;

	RotatingYMatrix(1, 0) = 0;     RotatingYMatrix(1, 1) = 1; RotatingYMatrix(1, 2) = 0;

	RotatingYMatrix(2, 0) = -sin;  RotatingYMatrix(2, 1) = 0; RotatingYMatrix(2, 2) = cos;


	return RotatingYMatrix;
}

Eigen::Matrix4f rotateZMatrix(float theta)
{
	Eigen::Matrix4f RotatingZMatrix = Eigen::Matrix4f::Identity();
	float cos = std::cos(theta);
	float sin = std::sin(theta);
	// rotation around Z:
	// [  c -s  0 ]
	// [  s  c  0 ]
	// [  0  0  1 ]
	RotatingZMatrix(0, 0) = cos;   RotatingZMatrix(0, 1) = -sin; RotatingZMatrix(0, 2) = 0;
	RotatingZMatrix(1, 0) = sin;   RotatingZMatrix(1, 1) = cos;  RotatingZMatrix(1, 2) = 0;
	RotatingZMatrix(2, 0) = 0;     RotatingZMatrix(2, 1) = 0;    RotatingZMatrix(2, 2) = 1;
}

template<typename T> T coeffWiseMultiply(const T& l, const T& r)
{
	return (l.array() * r.array()).matrix();
}

bool outsideClipBox(const Eigen::Vector4f& v)
{
	return
		v.x() < -1.f ||
		v.y() < -1.f ||
		v.z() < -1.f ||
		v.x() > 1.f ||
		v.y() > 1.f ||
		v.z() > 1.f;
}

