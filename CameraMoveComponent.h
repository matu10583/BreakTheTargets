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
    //��]�̍ő呬�x
    float maxRotateVel_;
    //��]�̊��x
    float sensitivity_;
    //��]�̍ő�l
    float maxAngle_;
    //��]�̍ŏ��l
    float minAngle_;
};

