#pragma once
#include "Object.h"
//デバッグ用モデル
class Ruka :
    public Object
{
public:
    Ruka(class Object* parent);
private:
    void UpdateObject(float deltaTime)override;
    class AudioComponent* ac_;

    float count_ = 0;
};

