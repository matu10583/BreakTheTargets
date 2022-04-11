#pragma once
#include "MoveComponent.h"
class CameraMoveComponent :
    public MoveComponent
{
public:
    CameraMoveComponent(class Object* owner, int updateOrder = 100);
    void ProcessInput(const struct InputState& inputState);

    //setter
    void SetMaxRotateVel(float);
    void SetSensitivity(float);

private:
    //‰ñ“]‚ÌÅ‘å‘¬“x
    float maxRotateVel_;
    //‰ñ“]‚ÌŠ´“x
    float sensitivity_;
    //‰ñ“]‚ÌÅ‘å’l
    float maxAngle_;
    //‰ñ“]‚ÌÅ¬’l
    float minAngle_;
};

