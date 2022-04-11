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
    
    //���ł܂ł̎����Bage>lifespan�ŏ���
    float lifespan_;
    float age_;
    //�e�̑���
    float vel_;

    //�e���������I�u�W�F�N�g
    class Object* shooter_;
    class LineColliderComponent* col_;
    class MoveComponent* move_;
    class MeshComponent* model_;
};

