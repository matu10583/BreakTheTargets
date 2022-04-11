#include "LineColliderComponent.h"
#include "Object.h"

LineColliderComponent::LineColliderComponent(Object* owner, const Line& line, bool istrriger) :
	ColliderComponent(owner, istrriger)
{
	worldLine_ = localLine_ = line;
	//AABB‚ğŒvZ.
	localAABB_.UpdateBox(line.start_);
	localAABB_.UpdateBox(line.end_);
}

bool
LineColliderComponent::Intersect(ColliderComponent* other) {
	//Õ“Ë‚Ìî•ñ‚à‚à‚ç‚¤
	CollisionInfo ci;
	auto intersect = other->Intersect(worldLine_, &ci);
	if (intersect) {
		//otherŒü‚«‚ÌƒxƒNƒgƒ‹‚É‚È‚Á‚Ä‚¢‚é‚½‚ß”½“]‚³‚¹‚é
		ci.outVec_ *= -1;
		ci.collider[0] = this;
		ci.collider[1] = other;
		collisionInfos_.emplace_back(ci);
	}
	return intersect;
}

bool
LineColliderComponent::Intersect(const Line& other, CollisionInfo* coInfo) {
	coInfo->outVec_ = Vector3::Zero();
	//ü•ª“¯m‚ÌÕ“Ë‚Íg‚í‚È‚³‚»‚¤‚È‚Ì‚ÅÀ‘•‚µ‚È‚¢
	//‚Æ‚è‚ ‚¦‚¸false‚É‚µ‚Ä‚¨‚­
	return false;
}

bool
LineColliderComponent::Intersect(const OBB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(worldLine_, other, coInfo->outVec_);
	return result;
}

bool
LineColliderComponent::Intersect(const Capsule& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldLine_, coInfo->outVec_);
	//ŠÖ”‚Ì“s‡ã‹t
	coInfo->outVec_ *= -1;
	return result;
}

bool
LineColliderComponent::Intersect(const AABB& other, CollisionInfo* coInfo) {
	auto result = Collide::Intersect(other, worldLine_, coInfo->outVec_);
	//ŠÖ”‚Ì“s‡ã‹t
	coInfo->outVec_ *= -1;
	return result;
}

void
LineColliderComponent::UpdateCollider() {
	auto rot = owner_->GetWorldRot() + offsetRot_;
	auto rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	auto sca = owner_->GetWorldScale();
	auto scaMat = XMMatrixScaling(sca.x, sca.y, sca.z);
	auto pos = owner_->GetWorldPos() + offsetPos_;
	//local‚ğƒRƒs[‚µ‚Ä‰ñ“],ˆÚ“®‚³‚¹‚é
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
	//‰ñ“],ˆÚ“®
	worldLine_ = localLine_;
	worldLine_.Scale(scaMat);
	worldLine_.Rotate(rotMat);
	worldLine_.Move(pos);
}

void
LineColliderComponent::SetLine(const Line& line) {
	localLine_ = line;
	localAABB_.Reset();
	localAABB_.UpdateBox(line.start_);
	localAABB_.UpdateBox(line.end_);
}