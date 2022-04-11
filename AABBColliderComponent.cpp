#include "AABBColliderComponent.h"
#include "Object.h"

AABBColliderComponent::AABBColliderComponent(
	Object* owner, const AABB& aabb, bool istrriger) :
	ColliderComponent(owner, istrriger)
{
	worldAABB_ = aabb;
	localAABB_ = aabb;
}

bool
AABBColliderComponent::Intersect(ColliderComponent* other) {
	//�Փˎ��̏������炤
	CollisionInfo ci;
	auto intersect = other->Intersect(worldAABB_, &ci);
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
AABBColliderComponent::Intersect(const Capsule& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldAABB_, coInfo->outVec_);
	//�֐��̓s����t
	coInfo->outVec_ *= -1;
	return result;
}

bool
AABBColliderComponent::Intersect(const OBB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldAABB_, coInfo->outVec_);
	//�֐��̓s����t
	coInfo->outVec_ *= -1;
	return result;
}

bool
AABBColliderComponent::Intersect(const Line& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldAABB_, other, coInfo->outVec_);
	return result;
}

bool
AABBColliderComponent::Intersect(const AABB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldAABB_, other, coInfo->outVec_);
	return result;
}

void
AABBColliderComponent::UpdateCollider() {
	auto rot = owner_->GetWorldRot() + offsetRot_;
	auto rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	auto sca = owner_->GetWorldScale();
	auto scaMat = XMMatrixScaling(sca.x, sca.y, sca.z);
	auto pos = owner_->GetWorldPos() + offsetPos_;
	//local���R�s�[���ĉ�],�ړ�,�X�P�[��������
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
}