#include "Component.h"
#include "Object.h"

Component::Component(Object* owner, int updateOrder):
state_(CActive)
{
	owner_ = owner;
	updateOrder_ = updateOrder;
	owner_->AddComponent(this);
}

Component::~Component() {
	owner_->RemoveComponent(this);
}

void
Component::Update(float deltaTime) {

}

void
Component::ProcessInput(const InputState& inputState) {

}

int 
Component::GetUpdateOrder() {
	return updateOrder_;
}

Object&
Component::GetOwner()const {
	return *owner_;
}