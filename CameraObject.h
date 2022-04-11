#pragma once
#include "Object.h"


class CameraObject :
    public Object
{
public:
    enum Camera_Pos_State
    {


        CAMERA_POS_DEFAULT = 1,//通常
        CAMERA_POS_SHOOT = 2,//腰うち
        CAMERA_POS_FPS = 4,//FPS
        CAMERA_POS_TRANSIT_TO_DEFAULT = 8,//デフォルト方向に遷移してるか
        CAMERA_POS_TRANSIT_TO_NODEFAULT = 16,//デフォルト方向意外に遷移
    };

    CameraObject(class Object* parent);
    ~CameraObject();
    //SHOOTなら1,FPSなら2,それ以外は0
    int CanShoot() {
        if (cps_ & CAMERA_POS_SHOOT) {
            return 1;
        }
        else if (cps_ & CAMERA_POS_FPS) {
            return 2;
        }
        return 0;
    }
private:
    void UpdateObject(float deltaTime) override;
    void ActorInput(const InputState& inpuststate)override;

    const Vector3 POS_DEFAULT = Vector3(5, 25, -30);
    const Vector3 POS_SHOOT = Vector3(5, 20, -20);
    const Vector3 POS_FPS = Vector3(5, 15, 5);
    float invMoveTime_;//カメラ移動にかかる時間の逆数

    unsigned int cps_;
    class CameraComponent* cameraComp_;
    class CameraMoveComponent* moveComp_;

    float count_ = 0;
};

