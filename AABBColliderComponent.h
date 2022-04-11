#pragma once
#include "ColliderComponent.h"
#include "Collider.h"
class AABBColliderComponent :
    public ColliderComponent
{
public:
    AABBColliderComponent(
        class Object* owner, const AABB& aabb, bool istrriger = false);

    bool Intersect(ColliderComponent* other) override;
    bool Intersect(const Line& other, CollisionInfo* coInfo) override;
    bool Intersect(const Capsule& other, CollisionInfo* coInfo) override;
    bool Intersect(const AABB& other, CollisionInfo* coInfo) override;
    bool Intersect(const OBB& other, CollisionInfo* coInfo) override;

    void UpdateCollider()override;
private:
    //AABB‚Íe‚ª‚Á‚Ä‚é‚â‚Â‚ğg‚¤


};

