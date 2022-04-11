#pragma once
#include "Component.h"
#include "XMFLOAT_Helper.h"
#include <DirectXMath.h>

using namespace DirectX;

//�ړ����邽�߂̃R���|�[�l���g
class MoveComponent :
    public Component
{
public:
    MoveComponent(class Object* owner, int updateOrder = 100);
    void Update(float deltaTime)override;

    const Vector3& GetAngularVel()const;
    const Vector3& GetVelocity()const;
    const Vector3& GetInputVelocity()const;
    const Vector3& GetAccel()const;

    void SetAngularVel(float roll, float pitch, float yaw);
    void SetVelocity(float x, float y, float z);
    void SetAccel(float x, float y, float z);
    void SetMaxVel(float v) { maxSpeed_ = v; }

    void AddAngularVel(float roll, float pitch, float yaw);
    void AddVelocity(float x, float y, float z);
    void AddAccel(float x, float y, float z);
    void AddInputVelocity(float x, float y, float z) { inputVel_ += Vector3(x, y, z); }

    void SetGravity(const Vector3& g) { gravity_ = g; }
    void SetUseGravity(bool g) { useGravity_ = g; }
    bool GetUseGravity()const { return useGravity_; }
    void SetIsOnGround(bool ison) { isOnGround_ = ison; }
    bool IsOnGround()const { return isOnGround_; }
    //�����Ă��邩��Ԃ�
    bool DoRun();

private:
    //�p�x
    Vector3 angularVel_;
    //���x
    Vector3 vel_;
    //�������Z��K�v�Ƃ��Ȃ����񃊃Z�b�g����鑬�x
    Vector3 inputVel_;
    //�����x
    Vector3 accel_;
    //�n�ʂ̏ォ
    bool isOnGround_;
    //�d�͂��g����
    bool useGravity_;
    //�d�͉����x
    Vector3 gravity_;
    //�ő呬�x
    float maxSpeed_;

    //�n��ňړ����Ă��邩�ǂ���
    bool doRun_;
};

