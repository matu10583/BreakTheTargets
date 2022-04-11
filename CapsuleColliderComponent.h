#pragma once
#include "ColliderComponent.h"
#include "Collider.h"
class CapsuleColliderComponent :
    public ColliderComponent
{
public:
    CapsuleColliderComponent(
        class Object* owner, const Capsule& capsule, bool istrriger = false);

    bool Intersect(ColliderComponent* other) override;
    bool Intersect(const OBB& other, CollisionInfo* coInfo) override;
    bool Intersect(const Line& other, CollisionInfo* coInfo) override;
    bool Intersect(const Capsule& other, CollisionInfo* coInfo) override;
    bool Intersect(const AABB& other, CollisionInfo* coInfo) override;

    void UpdateCollider()override;
private:
    Capsule worldCapsule_;
    Capsule localCapsule_;
};


