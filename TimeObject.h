#pragma once
#include "Object.h"
#include "SpriteComponent.h"
#include "TextSpriteComponent.h"
class TimeObject :
    public Object
{
public:
    TimeObject(class Object*, int stageMode);
    float GetTime()const { return nowTime_; }
    void SetTime(float t) { nowTime_ = t; }

private:
    void UpdateObject(float deltaTime)override;

    //�R���|�[�l���g
    //�\������
    class TextSpriteComponent* nowTimeImg_;
    float nowTime_;

    int stageMode_;
};

