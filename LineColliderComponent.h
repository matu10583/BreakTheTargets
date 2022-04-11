#pragma once
#include "ColliderComponent.h"
#include "Collider.h"
class LineColliderComponent :
    public ColliderComponent
{
public:
    LineColliderComponent(
        class Object* owner, const Line& line, bool istrriger = false);

    bool Intersect(ColliderComponent* other) override;
    bool Intersect(const Line& other, CollisionInfo* coInfo) override;
    bool Intersect(const Capsule& other, CollisionInfo* coInfo) override;
    bool Intersect(const AABB& other, CollisionInfo* coInfo) override;
    bool Intersect(const OBB& other, CollisionInfo* coInfo) override;

    void UpdateCollider()override;
    void SetLine(const Line& line);
private:
    Line worldLine_;
    Line localLine_;
};

