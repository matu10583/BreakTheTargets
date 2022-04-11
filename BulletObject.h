#pragma once
#include "Object.h"
class BulletObject :
    public Object
{
public:
    BulletObject(Object* parent,Object* shooter);
    void UpdateObject(float deltaTime)override;

    void SetLifeSpan(float l) { lifespan_ = l; }
    void SetVel(float v) { vel_ = v; }
private:

    void OnCollision(const CollisionInfo& ci)override;
    
    //消滅までの寿命。age>lifespanで消滅
    float lifespan_;
    float age_;
    //弾の速さ
    float vel_;

    //弾を撃ったオブジェクト
    class Object* shooter_;
    class LineColliderComponent* col_;
    class MoveComponent* move_;
    class MeshComponent* model_;
};

