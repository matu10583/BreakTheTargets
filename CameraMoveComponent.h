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
    //回転の最大速度
    float maxRotateVel_;
    //回転の感度
    float sensitivity_;
    //回転の最大値
    float maxAngle_;
    //回転の最小値
    float minAngle_;
};

