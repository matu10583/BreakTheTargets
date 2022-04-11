#pragma once
#include "Object.h"


class CameraObject :
    public Object
{
public:
    enum Camera_Pos_State
    {


        CAMERA_POS_DEFAULT = 1,//�ʏ�
        CAMERA_POS_SHOOT = 2,//������
        CAMERA_POS_FPS = 4,//FPS
        CAMERA_POS_TRANSIT_TO_DEFAULT = 8,//�f�t�H���g�����ɑJ�ڂ��Ă邩
        CAMERA_POS_TRANSIT_TO_NODEFAULT = 16,//�f�t�H���g�����ӊO�ɑJ��
    };

    CameraObject(class Object* parent);
    ~CameraObject();
    //SHOOT�Ȃ�1,FPS�Ȃ�2,����ȊO��0
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
    float invMoveTime_;//�J�����ړ��ɂ����鎞�Ԃ̋t��

    unsigned int cps_;
    class CameraComponent* cameraComp_;
    class CameraMoveComponent* moveComp_;

    float count_ = 0;
};

