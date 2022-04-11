#include "FreeCameraObject.h"
#include "Object.h"
#include "CameraComponent.h"
#include "CameraMoveComponent.h"
#include "PlayerMoveComponent.h"
#include "InputSystem.h"
#include "Collider.h"

FreeCameraObject::FreeCameraObject(Object* parent) :
	Object(parent)
{
	cameraComp_ = new CameraComponent(this);
	moveComp_ = new CameraMoveComponent(this);
	playerMoveComp_ = new PlayerMoveComponent(this);
	playerMoveComp_->SetUseGravity(false);
}

FreeCameraObject::~FreeCameraObject() {
}

void
FreeCameraObject::UpdateObject(float deltaTime) {
}