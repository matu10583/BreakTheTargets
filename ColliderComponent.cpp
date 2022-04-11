#include "ColliderComponent.h"
#include "PhysicsSystem.h"

ColliderComponent::ColliderComponent(Object* owner,bool istrriger, int updateOrder) :
	Component(owner, updateOrder),
isTrigger_(istrriger),
offsetPos_(Vector3::Zero()),
offsetRot_(Vector3::Zero()),
type_(TYPE_NONE)
{
	PhysicsSystem::Instance()->AddCollider(this);
}

ColliderComponent::~ColliderComponent(){
	PhysicsSystem::Instance()->RemoveCollider(this);
}

const std::vector<CollisionInfo>& 
ColliderComponent::GetCollisionInfos()const {
	return collisionInfos_;
}

bool 
ColliderComponent::BroadIntersect(const AABB& other)const {
	Vector3 outNorm;
	return Collide::Intersect(worldAABB_, other, outNorm);
}

const AABB& 
ColliderComponent::GetWorldAABB()const {
	return worldAABB_;
}

void
ColliderComponent::ClearCollisionInfo() {
	collisionInfos_.clear();
}