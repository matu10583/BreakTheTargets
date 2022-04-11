#include "PlayerMoveComponent.h"
#include "InputSystem.h"
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"

using namespace DirectX;

PlayerMoveComponent::PlayerMoveComponent(Object* owner, int updateOrder):
MoveComponent(owner, updateOrder),
maxRotateVel_(5),
horizontalVel_(100),
jumpAcc_(250),
sensitivity_(0.1f),
canJump_(true)
{
	SetUseGravity(true);
}

void 
PlayerMoveComponent::ProcessInput(const InputState& inputState) {
	//‘€ì
	Vector3 moveAcc = Vector3::Zero();
	Vector3 moveVel = Vector3(0, 0, 0);
	auto hv = horizontalVel_;
	float sen = sensitivity_;
	if (inputState.kb.IsMappedButtonOn(READY_SHOOT)) {
		hv /= 2;
		sen /= 2;
	}
	if (inputState.kb.IsMappedButtonOn(MOVE_FORWAD)) {
		moveVel.z += hv;
	}
	if (inputState.kb.IsMappedButtonOn(MOVE_BACK)) {
		moveVel.z -= hv;
	}
	if (inputState.kb.IsMappedButtonOn(MOVE_RIGHT)) {
		moveVel.x += hv;
	}
	if (inputState.kb.IsMappedButtonOn(MOVE_LEFT)) {
		moveVel.x -= hv;
	}
	if (inputState.kb.IsMappedButtonOn(MOVE_JUMP)) {
		//ƒWƒƒƒ“ƒvˆ—‚ð“ü‚ê‚é
		if (IsOnGround() && GetUseGravity()) {
			moveAcc.y += jumpAcc_;
		}
	}
	float mag = moveVel.Magnitude();
	if (mag > hv) {
		//Å‘å‘¬“x‚É‡‚í‚¹‚é
		moveVel = moveVel * hv / mag;
	}

	AddInputVelocity(
		moveVel.x, moveVel.y, moveVel.z
	);
	AddAccel(
		moveAcc.x, moveAcc.y, moveAcc.z
	);

	//U‚èŒü‚«
	auto mouseVel = inputState.ms.GetMoveVec();
	float rot = sen * mouseVel.x;
	if (rot > maxRotateVel_) {
		rot = maxRotateVel_;
	}

	SetAngularVel(0, rot, 0);
}

void
PlayerMoveComponent::SetHorizontalVel(float hvel) {
	horizontalVel_ = hvel;
}

void
PlayerMoveComponent::SetMaxRotateVel(float max) {
	maxRotateVel_ = max;
}

void
PlayerMoveComponent::SetSensitivity(float sen) {
	sensitivity_ = sen;
}

void
PlayerMoveComponent::SetJumpAccel(float a) {
	jumpAcc_ = a;
}

void
PlayerMoveComponent::SetCanJump(bool flag) {
	canJump_ = flag;
}