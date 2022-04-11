#pragma once
#include "ColliderComponent.h"
#include "Collider.h"
class OBBColliderComponent :
    public ColliderComponent
{
public:
    OBBColliderComponent(
        class Object* owner, const OBB& capsule, bool istrriger = false);

    bool Intersect(ColliderComponent* other) override;
    bool Intersect(const Line& other, CollisionInfo* coInfo) override;
    bool Intersect(const OBB& other, CollisionInfo* coInfo) override;
    bool Intersect(const Capsule& other, CollisionInfo* coInfo) override;
    bool Intersect(const AABB& other, CollisionInfo* coInfo) override;

    void UpdateCollider()override;
private:
    OBB worldOBB_;
    OBB localOBB_;
};

