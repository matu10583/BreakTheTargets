#pragma once
#include "Object.h"
#include <string>
class ModelObject :
    public Object
{
public:
    ModelObject(Object* parent);

    void CreateModel(const char* fileName);
    const std::string& GetFileName()const {
        return fileName_;
    }

private:
    void UpdateObject(float deltaTime)override{}
    void SetAdditionalDataFromJson(const nlohmann::json&)override;
    void GetAdditionalJsonFromObj(nlohmann::json&)override;
    
    class MeshComponent* model_;
    class OBBColliderComponent* col_;

    std::string fileName_;
};

