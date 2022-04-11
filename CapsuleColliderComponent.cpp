#include "CapsuleColliderComponent.h"
#include "ColliderComponent.h"
#include "Object.h"

CapsuleColliderComponent::CapsuleColliderComponent(Object* owner, const Capsule& capsule, bool istrriger) :
	ColliderComponent(owner, istrriger) 
{
	worldCapsule_ = localCapsule_ = capsule;
	//AABB���v�Z.�n�_�ƏI�_�ɂ����鋅���܂ނ悤�ɍL����
	Sphere sp[2];
	sp[0].center_ = localCapsule_.line_.start_;
	sp[1].center_ = localCapsule_.line_.end_;
	sp[0].radius_ = sp[1].radius_ = localCapsule_.radius_;
	localAABB_.UpdateBox(sp[0]);
	localAABB_.UpdateBox(sp[1]);
}

bool 
CapsuleColliderComponent::Intersect(ColliderComponent* other) {
	//�Փˎ��̏������炤
	CollisionInfo ci;
	auto intersect = other->Intersect(worldCapsule_, &ci);
	if (intersect) {
		//other�����̃x�N�g���ɂȂ��Ă��邽�ߔ��]������
		ci.outVec_ *= -1;
		ci.collider[0] = this;
		ci.collider[1] = other;
		collisionInfos_.emplace_back(ci);
	}
	return intersect;
}

bool
CapsuleColliderComponent::Intersect(const Capsule& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldCapsule_, other, coInfo->outVec_);
	return result;
}

bool
CapsuleColliderComponent::Intersect(const OBB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldCapsule_, other, coInfo->outVec_);
	return result;
}

bool
CapsuleColliderComponent::Intersect(const Line& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldCapsule_, other, coInfo->outVec_);
	return result;
}

bool
CapsuleColliderComponent::Intersect(const AABB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldCapsule_, other, coInfo->outVec_);
	return result;
}

void
CapsuleColliderComponent::UpdateCollider() {
	auto rot = owner_->GetWorldRot() + offsetRot_;
	auto rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	auto sca = owner_->GetWorldScale();
	auto scaMat = XMMatrixScaling(sca.x, sca.y, sca.z);
	auto pos = owner_->GetWorldPos() + offsetPos_;
	//local���R�s�[���ĉ�],�ړ�������
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
	//�J�v�Z���̉�],�ړ�
	worldCapsule_ = localCapsule_;
	worldCapsule_.Scale(scaMat);
	worldCapsule_.Rotate(rotMat);
	worldCapsule_.Move(pos);
}