#pragma once
#include "Object.h"
#include "json.hpp"
class TargetObject :
    public Object
{
public:
    enum MoveType
    {
        MOVE_NONE = 0,//動かない
        MOVE_RANDOM = 1,//ランダムな方向へ飛んでいく
        MOVE_BEZIERLINE = 2,//ベジェ曲線上を往復
        MOVE_CIRCLE = 3//円上を回る
    };

    struct BezierInfo
    {
        Vector3 start;//始点
        Vector3 end;//終点
        Vector3 control;//コントロールポイント
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
        //半径ゼロはありえない
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
        float pitch;//ピッチ回転角度
        Vector3 center;
    private:
        Vector2 radius;
    };

    TargetObject(class Object* parent,
        class ScoreTextObject* score);

    //ターゲットの寿命、速度のリセット
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
    //中心にちかい場所の衝突判定
    OBBColliderComponent* col2_;
    OBBColliderComponent* col3_;
    class MoveComponent* move_;
    class MeshComponent* model_;

    float age_;
    float lifespan_;
    //今往復のどっちの方向か
    int dir_ = 1;

    const Vector3 GetPosFromBecier(float count);
    const Vector3 GetPosFromCircle(float count);

    BezierInfo bzInfo_;

    CircleInfo cInfo_;

    int mvType_;//動きの種類
};

