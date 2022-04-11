#include "CameraMoveComponent.h"
#include "InputSystem.h"
#include "Object.h"
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

CameraMoveComponent::CameraMoveComponent(Object* owner, int updateOrder) :
	MoveComponent(owner, updateOrder),
	maxRotateVel_(3),
	sensitivity_(0.1f),
	maxAngle_(M_PI*75.0f/180.0f),
	minAngle_(-M_PI*75.0f/180.0f)
{
}

void
CameraMoveComponent::ProcessInput(const InputState& inputState) {
	auto mouseVel = inputState.ms.GetMoveVec();
	auto rot = mouseVel.y * sensitivity_;
	if (rot > maxRotateVel_) {
		rot = maxRotateVel_;
	}
	auto owner_rot = owner_->GetLocalRot();
	//Ç±ÇÍà»è„â∫Çå¸Ç©Ç»Ç¢
	if (owner_rot.x > maxAngle_) {
		owner_->SetLocalRot(maxAngle_, owner_rot.y, owner_rot.z);
		rot = 0;
	}
	//Ç±ÇÍà»è„è„Çå¸Ç©Ç»Ç¢
	if (owner_rot.x < minAngle_) {
		owner_->SetLocalRot(minAngle_, owner_rot.y, owner_rot.z);
		rot=0;
	}
	SetAngularVel(rot, 0, 0);
}

void
CameraMoveComponent::SetMaxRotateVel(float vel) {
	maxRotateVel_ = vel;
}

void
CameraMoveComponent::SetSensitivity(float sen) {
	sensitivity_ = sen;
}