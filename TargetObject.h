#pragma once
#include "Object.h"
#include "json.hpp"
class TargetObject :
    public Object
{
public:
    enum MoveType
    {
        MOVE_NONE = 0,//�����Ȃ�
        MOVE_RANDOM = 1,//�����_���ȕ����֔��ł���
        MOVE_BEZIERLINE = 2,//�x�W�F�Ȑ��������
        MOVE_CIRCLE = 3//�~������
    };

    struct BezierInfo
    {
        Vector3 start;//�n�_
        Vector3 end;//�I�_
        Vector3 control;//�R���g���[���|�C���g
        BezierInfo():
            start(0, 0, 0),
            end(0, 0, 0),
            control(0, 0, 0){}
    };

    struct CircleInfo
    {
        CircleInfo():
            pitch(0),
            center(0, 0, 0),
            radius(1,1){}
        //���a�[���͂��肦�Ȃ�
        const Vector2& Rad() {
            if (radius.x <= 0) {
                radius.x = FLT_MIN;
            }
            if (radius.y <= 0) {
                radius.y = FLT_MIN;
            }
            return radius;
        }
        void SetRadius(const Vector2& rad) {
            radius = rad;
        }
        float pitch;//�s�b�`��]�p�x
        Vector3 center;
    private:
        Vector2 radius;
    };

    TargetObject(class Object* parent,
        class ScoreTextObject* score);

    //�^�[�Q�b�g�̎����A���x�̃��Z�b�g
    void Reset();

    void SetMoveType(int mv) { mvType_ = mv; age_ = 0; }
    int GetMoveType()const { return mvType_; }

    void SetBezierInfo(const BezierInfo& bz) { bzInfo_ = bz; }
    void SetCircleInfo(const CircleInfo& ci) { cInfo_ = ci; }
    const BezierInfo& GetBezierInfo() { return bzInfo_; }
    const CircleInfo& GetCircleInfo() { return cInfo_; }

    void SetSpan(float s) { lifespan_ = s;}
    float GetSpan()const { return lifespan_; }
private:
    void UpdateObject(float deltaTime)override;
    void OnCollision(const CollisionInfo& ci)override;
    void SetAdditionalDataFromJson(const nlohmann::json&)override;
    void GetAdditionalJsonFromObj(nlohmann::json&)override;

    ScoreTextObject* score_;
    class OBBColliderComponent* col1_;
    //���S�ɂ������ꏊ�̏Փ˔���
    OBBColliderComponent* col2_;
    OBBColliderComponent* col3_;
    class MoveComponent* move_;
    class MeshComponent* model_;

    float age_;
    float lifespan_;
    //�������̂ǂ����̕�����
    int dir_ = 1;

    const Vector3 GetPosFromBecier(float count);
    const Vector3 GetPosFromCircle(float count);

    BezierInfo bzInfo_;

    CircleInfo cInfo_;

    int mvType_;//�����̎��
};

