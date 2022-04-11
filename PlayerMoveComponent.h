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
    //回転の最大速度
    float maxRotateVel_;
    //前後左右のみの移動の速度
    float horizontalVel_;
    //ジャンプの加速度
    float jumpAcc_;
    //回転の感度
    float sensitivity_;

    //ジャンプできるかのフラグ
    bool canJump_;
};

