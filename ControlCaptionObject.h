#pragma once
#include "Object.h"
//操作説明を表示する
class ControlCaptionObject :
    public Object
{
public:
    ControlCaptionObject(Object* parent);
private:
    class TextSpriteComponent* caps1_;
    class TextSpriteComponent* caps2_;
    class TextSpriteComponent* caps3_;

    void UpdateObject(float deltaTime)override{}
};

