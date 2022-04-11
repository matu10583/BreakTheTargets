#pragma once
#include "Component.h"
#include "Collider.h"
#include <vector>
#include "XMFLOAT_Helper.h"

//衝突時の情報
struct CollisionInfo
{
	//衝突字の脱出方向[1]から[0]の向き
	Vector3 outVec_;
	//衝突相手
	const class ColliderComponent* collider[2];
};
//コライダーの種類
enum ColliderType
{
	TYPE_NONE = 1,
	TYPE_PLAYER = 2,//プレイヤー
	TYPE_GROUND = 4,//地面
	TYPE_GROUNDTRRIGER = 8,//着地トリガー
	TYPE_BULLET = 16,//弾丸
	TYPE_WALL = 32,
	TYPE_TARGET = 64,//打たれたら死ぬやつら
};

class ColliderComponent :
    public Component
{
public:


    ColliderComponent(class Object* owner, bool istrriger, int updateOrder = 100);
	~ColliderComponent();

	const std::vector<CollisionInfo>& GetCollisionInfos()const;
	//衝突情報配列をリセットする
	void ClearCollisionInfo();

	bool BroadIntersect(const AABB& other)const;
	const AABB& GetWorldAABB()const;

	bool IsTrigger() const { return isTrigger_; }
	uint64_t GetType()const { return type_; }
	void SetType(uint64_t t) { type_ = t; }
	void SetOfstPos(float x, float y, float z) { offsetPos_ = Vector3(x, y, z); }
	void SetOfstRot(float x, float y, float z) { offsetRot_ = Vector3(x, y, z); }

	virtual bool Intersect(
		ColliderComponent* other) = 0;

	virtual bool Intersect(
		const Line& other, CollisionInfo* coInfos) = 0;
	virtual bool Intersect(
		const AABB& other, CollisionInfo* coInfos) = 0;
	virtual bool Intersect(
		const Capsule& other, CollisionInfo* coInfos) = 0;
	virtual bool Intersect(
		const OBB& other, CollisionInfo* coInfos) = 0;

	//コライダーの状態を更新する
	virtual void UpdateCollider() = 0;

	//ローカルのAABBをもらってくる
	//デバッグ用に判定を可視化させる
	const AABB& GetLocalAABB() { return localAABB_; }



protected:
	std::vector<CollisionInfo> collisionInfos_;
	bool isTrigger_;
	AABB localAABB_;
	AABB worldAABB_;
	//コライダー識別用のタイプ
	uint64_t type_;

	//オブジェクト座標からのオフセット
	Vector3 offsetPos_;
	//オブジェクト回転からのオフセット
	Vector3 offsetRot_;
};

