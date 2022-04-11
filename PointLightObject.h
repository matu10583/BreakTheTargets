#pragma once
#include "Object.h"
class PointLightObject :
    public Object
{
public:
    PointLightObject(
        Object* parent, const Vector3& color, float range, float radius);
    void SetLightParam(const Vector3& col, float range, float rad);
    const Vector3& GetColor()const;
    float GetRadius()const;
    float GetRange()const;
private:
    void UpdateObject(float deltaTime)override;
    void SetAdditionalDataFromJson(const nlohmann::json&)override;
    void GetAdditionalJsonFromObj(nlohmann::json&)override;

    class PointLightComponent* plcomp_;
    class MeshComponent* model_;
};

