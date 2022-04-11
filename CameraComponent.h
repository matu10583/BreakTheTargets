#pragma once
#include "Component.h"
#include "XMFLOAT_Helper.h"
#include <DirectXMath.h>

using namespace DirectX;

class CameraComponent :
    public Component
{
public:
    CameraComponent(class Object* owner, int updateOrder = 100);
    void Update(float deltaTime)override;
private:
    void SetCameraSetting(const Vector3& eyePos, const Vector3& eyeRot);

};

