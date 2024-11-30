#pragma once

#include "MathDefines.h"
#include <DirectXMath.h>

class Camera
{
private:
	static constexpr float DEFAULT_NEAR = 0.1f;
	static constexpr float DEFAULT_FAR = 1000000.0f;
	static constexpr float DEFAULT_FOV = 110.0f;
	static constexpr float DEFAULT_ASPECT_RATIO = 1920.0f / 1080.0f;
public:
	Camera(float aFoVAngle);
	Camera(	float aFoVAngle = DEFAULT_FOV,
			float aAspectRatio = DEFAULT_ASPECT_RATIO,
			float aNearZ = DEFAULT_NEAR,
			float aFarZ = DEFAULT_FAR
	);

	void SetFoV(float aFoV);

	inline DirectX::XMMATRIX Projection() const { return projection; }
	DirectX::XMMATRIX View() const { return DirectX::XMMatrixInverse(nullptr, Transform()); }
	DirectX::XMMATRIX Transform() const
	{ 
		return DirectX::XMMatrixMultiply(
			DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, 0),
			DirectX::XMMatrixTranslation(position.x, position.y, position.z)
		);
	}

	Vector3f Forward() const {
		DirectX::XMMATRIX transform = Transform();
		return { transform.r[2].m128_f32[0], transform.r[2].m128_f32[1], transform.r[2].m128_f32[2] };;
	}

	Vector3f Up() const {
		DirectX::XMMATRIX transform = Transform();
		return { transform.r[1].m128_f32[0], transform.r[1].m128_f32[1], transform.r[1].m128_f32[2] };
	}

	Vector3f Right() const {
		DirectX::XMMATRIX transform = Transform();
		return { transform.r[0].m128_f32[0], transform.r[0].m128_f32[1], transform.r[0].m128_f32[2] };
	}

	DirectX::XMMATRIX projection;
	Vector3f position;
	float yaw;
	float pitch;
	float nearPlane;
	float farPlane;
	float fov;
};

