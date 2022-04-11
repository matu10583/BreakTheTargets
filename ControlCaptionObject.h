#pragma once
#include "Object.h"
//‘€ìà–¾‚ğ•\¦‚·‚é
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

