#pragma once
#include "Object.h"
class TargetFactoryObject :
    public Object
{
public:
    TargetFactoryObject(Object* parent, class ScoreTextObject* score);
    void UpdateObject(float deltaTime)override;
private:
    ScoreTextObject* score_;
    std::vector<class TargetObject*> targets_;
};

