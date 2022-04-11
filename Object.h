#pragma once
#include <vector>
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"
#include "json.hpp"

using namespace DirectX;

class Game;
class Component;

class Object
{

public:
	enum State//オブジェクトの状態
	{
		Active,
		InActive,
		Dead
	};

	Object(Object* parent, bool iskinematic = true);
	virtual ~Object();

	void Update(float deltaTime);
	void DeleteDeadObject();//志望オブジェクトの削除
	void AddComponent(Component* comp);
	void RemoveComponent(Component* comp);

	void ProcessInput(const struct InputState& inputState);

	void UpdateOnCollision(const struct CollisionInfo& ci);

	//jsonデータを反映させる
	void SetObjDataFromJson(const nlohmann::json&);
	//jsonデータを取得する
	nlohmann::json GetJsonFromObj(std::string objID, std::string objName);

	void AddChild(Object* child);
	void RemoveChild(Object* child);

	//Get&Set
	const State GetState()const;
	void SetState(State st);
	//ローカル座標
	void SetLocalPos(float x, float y, float z);
	void SetLocalRot(float x, float y, float z);
	void SetLocalScale(const Vector3& scale) { scale_ = scale; }
	const Vector3& GetLocalPos()const;
	const Vector3& GetLocalRot()const;
	const Vector3 GetLocalScale()const { return scale_; }
	//ワールド座標
	void SetWorldPos(float x, float y, float z);
	void SetWorldRot(float x, float y, float z);
	void SetWorldScale(const Vector3& scale);
	const Vector3 GetWorldPos()const;
	const Vector3 GetWorldRot()const;
	const Vector3 GetWorldScale()const;
	//前向きの軸
	const Vector3 GetForward()const;
	//右向きの軸
	const Vector3 GetRight()const;
	//上向きの軸
	const Vector3 GetUp()const;
	//kinematicか
	bool IsKinematic()const;
	void SetIsKinematic(bool flag) { isKinematic = flag; }
	//parentの情報
	Object* GetParent()const { return parent_; }

	void AddLocalRot(float roll, float pitch, float yaw);
	void AddLocalPos(float x, float y, float z);
	void AddWorldRot(float roll, float pitch, float yaw);
	void AddWorldPos(float x, float y, float z);

	//trueならばMeshComponentがついてる場合のみ描画する
	void SetDraw(bool flag) {
		isDraw = flag; }
	bool IsDraw()const { return isDraw; }

private:
	void UpdateComponents(float deltaTime);
	void UpdateChildren(float deltaTime);
	virtual void UpdateObject(float deltaTime)=0;
	virtual void OnCollision(const CollisionInfo& ci);

	//JSONファイル関係。追加の設定がある場合はここをオーバーライドする
	virtual void SetAdditionalDataFromJson(const nlohmann::json&){}
	virtual void GetAdditionalJsonFromObj(nlohmann::json&) {}

	virtual void ActorInput(const InputState& inputState);

	State state_;
	std::vector<Component*> components_;
	std::vector<Object*> children_;
	std::vector<Object*> pendingObjects_;
	bool updatingObject_;
	Object* parent_;
	//全部ローカル座標
	Vector3 position_;
	Vector3 rotation_;
	//倍率
	Vector3 scale_;
	
	bool isKinematic;

	bool isDraw;//描画するかどうか
};

