#pragma once
#include "Object.h"
#include "ColliderComponent.h"

class AABBColliderComponent;
struct Material;
class MeshComponent;
class PlaneObject :
    public Object
{
public:
    PlaneObject(class Object* parent, const Vector3& minP, const Vector3& maxP);
    void SetRepeatNum(unsigned int num) { repeatNum_ = num; }
    void AddMaterialInfo(const char* texPath,
        const char* normalPath = nullptr,
        const char* aoPath = nullptr);
    void CreateModel();
    void SetType(ColliderType t);
private:
    void UpdateObject(float deltaTime)override;
    AABBColliderComponent* aabbCol_;
    MeshComponent* aabbModel_;

    unsigned int repeatNum_ = 1;
    std::vector<Material> material_;
};

