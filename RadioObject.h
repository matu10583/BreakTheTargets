#pragma once
#include "Object.h"
class RadioObject :
    public Object
{
public:
    RadioObject(class Object* parent);
private:
    void UpdateObject(float deltaTime)override{
    
    }

    class AudioComponent* bgm_;
    class OBBColliderComponent* obb_;
    class MeshComponent* model_;
};

