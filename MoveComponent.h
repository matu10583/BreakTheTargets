#pragma once
#include "Component.h"
#include "XMFLOAT_Helper.h"
#include <DirectXMath.h>

using namespace DirectX;

//移動するためのコンポーネント
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
    //走っているかを返す
    bool DoRun();

private:
    //角度
    Vector3 angularVel_;
    //速度
    Vector3 vel_;
    //物理演算を必要としない毎回リセットされる速度
    Vector3 inputVel_;
    //加速度
    Vector3 accel_;
    //地面の上か
    bool isOnGround_;
    //重力を使うか
    bool useGravity_;
    //重力加速度
    Vector3 gravity_;
    //最大速度
    float maxSpeed_;

    //地上で移動しているかどうか
    bool doRun_;
};

