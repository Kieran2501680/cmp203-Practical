#pragma once
#define SKTBD_PI 3.141592654f

//#define GLM_FORCE_SSE2
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/ext.hpp"
#include "glm/gtx/matrix_decompose.hpp"

// TODO: Remove this! and use glm functions everywhere!
typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;
typedef glm::ivec2 int2;
typedef glm::ivec3 int3;
typedef glm::ivec4 int4;
typedef glm::uvec2 uint2;
typedef glm::uvec3 uint3;
typedef glm::uvec4 uint4;
typedef glm::vec4 vector;
typedef glm::mat4 matrix, float4x4;

typedef glm::mat<4, 4, glm::f32, glm::defaultp>	float4x3;

//struct BoundSphere
//{
//	float3 Center;
//	float Radius;
//};

//struct BoundBox
//{
//	float3 Center;
//	float3 Extents;
//};


struct Transform
{
	Transform():
		Translation(0.f, 0.f, 0.f),
		Rotation(1.f, 0.f, 0.f, 0.f),
		Scale(1.f, 1.f, 1.f)
	{
		
	};
	
	const glm::mat4x4 AsMatrix() const {
		return glm::translate(Translation) * glm::toMat4(Rotation) * glm::scale(Scale);
	}

	Transform operator*(const Transform& A) {
		Transform T = {};
		auto m = AsMatrix() * A.AsMatrix();
		T = m;
		return T;
	}
	// <summary>
	// Operator overload for 4x4 matrix decomposition. Extracts the translation, rotation and scale
	// properties encoded in the matrix.
	// </summary>
	__inline auto operator= (const glm::mat4x4& mat) -> Transform& {
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(mat, Scale, Rotation, Translation, skew, perspective);
		return *this;
	}

	float AsEulerX() const { return glm::eulerAngles(Rotation).x; }
	float AsEulerY() const { return glm::eulerAngles(Rotation).y; }
	float AsEulerZ() const { return glm::eulerAngles(Rotation).z; }

	glm::vec3 AsEuler() const { return glm::eulerAngles(Rotation); }

	void SetEulerX(float x) { SetEulerAngles(x, AsEulerY(), AsEulerZ()); }
	void SetEulerY(float y) { SetEulerAngles(AsEulerX(), y, AsEulerZ()); }
	void SetEulerZ(float z) { SetEulerAngles(AsEulerX(), AsEulerY(), z); }

	void SetEulerAngles(float x, float y, float z) 
	{ 
		Rotation = glm::toQuat(glm::yawPitchRoll(y, x, z));
	}
	// Add a rotation quat to current transform rotation
	void Rotate(const glm::quat& deltaRot)
	{
		Rotation *= deltaRot;
	};
	// Add a rotation from x,y,z to current transform rotation
	void Rotate(const glm::vec3& eulerDeltaRot)
	{
		Rotate(glm::quat(eulerDeltaRot));
	};

	glm::vec3 GetForwardVector() const {
		auto fvec = glm::normalize(Rotation * glm::vec3(0, 0, 1));
		return fvec;
	}

	glm::vec3 const GetRightVector() const {
		auto fvec = glm::normalize(Rotation * glm::vec3(1, 0, 0));
		return fvec;
	}

	void LookAt(glm::vec3 target) {
		float3 dir = glm::normalize(target - Translation);
		Rotation = glm::quatLookAtLH(dir, float3(0,1,0));	// Set the rotation, by giving it  a target direction and up vector
	}

	glm::vec3 const GetUpVector() const {
		glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);  // Set w to 1 for the following matrix multiplication
		up = Rotation * up;
		return up;
	}

	glm::vec3 Translation;
	glm::quat Rotation;
	glm::vec3 Scale;
};