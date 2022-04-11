#pragma once
#include "MoveComponent.h"
class PlayerMoveComponent :
    public MoveComponent
{
public:
    PlayerMoveComponent(class Object* owner, int updateOrder=100);
    void ProcessInput(const struct InputState& inputState)override;

    //getter&setter
    
    void SetHorizontalVel(float);
    void SetMaxRotateVel(float);
    void SetSensitivity(float);
    void SetJumpAccel(float);
    void SetCanJump(bool);


    
private:
    //��]�̍ő呬�x
    float maxRotateVel_;
    //�O�㍶�E�݂̂̈ړ��̑��x
    float horizontalVel_;
    //�W�����v�̉����x
    float jumpAcc_;
    //��]�̊��x
    float sensitivity_;

    //�W�����v�ł��邩�̃t���O
    bool canJump_;
};

