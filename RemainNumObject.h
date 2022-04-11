#pragma once
#include "Object.h"
class RemainNumObject :
    public Object
{

public:
    enum NumMode
    {
        MODE_BULLETNUM,
        MODE_TARGETNUM
    };
    RemainNumObject(Object* parent, int mode);
    void SetNum(int num) { remainNum_ = num; }
private:
    class TextSpriteComponent* text_;
    class PlayerObject* player_;

    void UpdateObject(float deltaTime)override;

    int numMode_;
    int remainNum_;

};

