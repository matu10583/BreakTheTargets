#include "OBBColliderComponent.h"
#include "Object.h"

OBBColliderComponent::OBBColliderComponent(Object* owner, const OBB& obb, bool istrriger) :
	ColliderComponent(owner, istrriger)
{
	worldOBB_ = localOBB_ = obb;
	//AABBを計算.
	Vector3 vert[8];
	Vector3 ax[3];
	ax[0] = obb.sizedAxis_[0] / 2;
	ax[1] = obb.sizedAxis_[1] / 2;
	ax[2] = obb.sizedAxis_[2] / 2;

	vert[0] = obb.center_ + ax[0] + ax[1] + ax[2];
	vert[1] = obb.center_ + ax[0] + ax[1] - ax[2];
	vert[2] = obb.center_ + ax[0] - ax[1] + ax[2];
	vert[3] = obb.center_ + ax[0] - ax[1] - ax[2];
	vert[4] = obb.center_ - ax[0] + ax[1] + ax[2];
	vert[5] = obb.center_ - ax[0] + ax[1] - ax[2];
	vert[6] = obb.center_ - ax[0] - ax[1] + ax[2];
	vert[7] = obb.center_ - ax[0] - ax[1] - ax[2];

	for (int i = 0; i < 8; i++) {
		localAABB_.UpdateBox(vert[i]);
	}
}

bool
OBBColliderComponent::Intersect(ColliderComponent* other) {
	//衝突時の情報ももらう
	CollisionInfo ci;
	auto intersect = other->Intersect(worldOBB_, &ci);
	if (intersect) {
		//other向きのベクトルになっているため反転させる
		ci.outVec_ *= -1;
		ci.collider[0] = this;
		ci.collider[1] = other;
		collisionInfos_.emplace_back(ci);
	}
	return intersect;
}

bool
OBBColliderComponent::Intersect(const Capsule& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldOBB_, coInfo->outVec_);
	coInfo->outVec_ *= -1;
	return result;
}

bool
OBBColliderComponent::Intersect(const OBB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldOBB_, other, coInfo->outVec_);
	return result;
}

bool
OBBColliderComponent::Intersect(const Line& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldOBB_, coInfo->outVec_);
	coInfo->outVec_ *= -1;
	return result;
}

bool
OBBColliderComponent::Intersect(const AABB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldOBB_, other, coInfo->outVec_);
	return result;
}

void
OBBColliderComponent::UpdateCollider() {
	auto rot = owner_->GetWorldRot() + offsetRot_;
	auto rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	auto sca = owner_->GetWorldScale();
	auto scaMat = XMMatrixScaling(sca.x, sca.y, sca.z);
	auto pos = owner_->GetWorldPos() + offsetPos_;
	//localをコピーして回転,移動させる
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
	//obbの回転,移動
	worldOBB_ = localOBB_;
	worldOBB_.Scale(scaMat);
	worldOBB_.Rotate(rotMat);
	worldOBB_.center_ += pos;
}