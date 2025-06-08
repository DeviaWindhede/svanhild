#include "pch.h"
#include "Camera.h"

Camera::Camera(float aFoVAngle) : Camera(aFoVAngle, DEFAULT_ASPECT_RATIO, DEFAULT_NEAR, DEFAULT_FAR)
{
}

Camera::Camera(float aFoVAngle, float aAspectRatio, float aNearZ, float aFarZ) :
	nearPlane(aNearZ),
	farPlane(aFarZ),
	position(0, 0, 0),
	yaw(0),
	pitch(0)
{
	nearPlane = aNearZ;
	farPlane = aFarZ;
	SetFoV(aFoVAngle);
}

void Camera::SetFoV(float aFoV)
{
	fov = aFoV;

	// TODO: Fix aspect ratio getter
	projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov), DEFAULT_ASPECT_RATIO, nearPlane, farPlane);
}
