#include "CameraComponent.h"
#include "Object.h"
#include "Dx12Wrapper.h"

CameraComponent::CameraComponent(Object* owner, int updateOrder) :
	Component(owner, updateOrder)
{

}

void
CameraComponent::SetCameraSetting(
	const Vector3& eyePos, const Vector3& eyeRot) {
	Dx12Wrapper* dx12 = Dx12Wrapper::Instance();
	dx12->SetCameraSetting(eyePos, eyeRot);
}

void
CameraComponent::Update(float deltaTime) {
	Vector3 eyepos = owner_->GetWorldPos();
	Vector3 eyerot = owner_->GetWorldRot();
	
	SetCameraSetting(
		eyepos,
		eyerot
	);
}