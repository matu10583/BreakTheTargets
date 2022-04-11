#include "MoveComponent.h"
#include "XMFLOAT_Helper.h"
#include "Object.h"


MoveComponent::MoveComponent(Object* owner, int updateOrder):
Component(owner, updateOrder),
isOnGround_(false),
useGravity_(false)
{
	angularVel_ = Vector3(0, 0, 0);
	vel_ = Vector3(0, 0, 0);
	accel_ = Vector3(0, 0, 0);
	gravity_ = Vector3(0, -30, 0);
	maxSpeed_ = 50;
}

void 
MoveComponent::Update(float deltaTime) {
	doRun_ = false;
	//�d�͉����x���f
	if (!isOnGround_ && useGravity_) {
		accel_ += gravity_;
	}
	//�����x���f
	if (!accel_.ApproxZero()) {
		vel_ += accel_ * deltaTime;
	}


	//���x���f
	if (!angularVel_.ApproxZero()) {
		Vector3 angle = angularVel_ * deltaTime;
		owner_->AddLocalRot(angle.x, angle.y, angle.z);
	}

	if (!vel_.ApproxZero() ||
		!inputVel_.ApproxZero()){
		if (vel_.MagnitudeSqr() > maxSpeed_ * maxSpeed_) {
			//�����ł��ǂ�����C��R�ōő呬�x�����݂���̂�
			//�����˂������Ȃ��悤�ɑ��x��������
			vel_.Normalize();
			vel_ *= maxSpeed_;
		}
		if (!isOnGround_ && useGravity_) {
			//�󂿂イ���x�͒x������
			inputVel_ *= 0.5;
		}
		else if(!inputVel_.ApproxZero()) {
			//�n��ňړ����Ă���
			doRun_ = true;
		}

		Vector3 pos = (vel_ + inputVel_) * deltaTime;
		owner_->AddLocalPos(
			pos.x, pos.y, pos.z);
		inputVel_ = Vector3::Zero();
	}
}

const Vector3& 
MoveComponent::GetAngularVel()const {
	return angularVel_;
}
const Vector3& 
MoveComponent::GetVelocity()const {
	return vel_;
}

const Vector3&
MoveComponent::GetInputVelocity()const {
	return inputVel_;
}

const Vector3&
MoveComponent::GetAccel()const {
	return accel_;
}

void 
MoveComponent::SetAngularVel(float roll, float pitch, float yaw) {
	angularVel_ = Vector3(roll, pitch, yaw);
}

void 
MoveComponent::SetVelocity(float x, float y, float z) {
	vel_ = Vector3(x, y, z);
}

void
MoveComponent::SetAccel(float x, float y, float z) {
	accel_ = Vector3(x, y, z);
}

void
MoveComponent::AddAngularVel(float roll, float pitch, float yaw) {
	angularVel_ += Vector3(roll, pitch, yaw);
}

void
MoveComponent::AddVelocity(float x, float y, float z) {
	vel_ += Vector3(x, y, z);
}

void
MoveComponent::AddAccel(float x, float y, float z) {
	accel_ += Vector3(x, y, z);
}

bool 
MoveComponent::DoRun() {
	return doRun_;
}