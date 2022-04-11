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
	enum State//�I�u�W�F�N�g�̏��
	{
		Active,
		InActive,
		Dead
	};

	Object(Object* parent, bool iskinematic = true);
	virtual ~Object();

	void Update(float deltaTime);
	void DeleteDeadObject();//�u�]�I�u�W�F�N�g�̍폜
	void AddComponent(Component* comp);
	void RemoveComponent(Component* comp);

	void ProcessInput(const struct InputState& inputState);

	void UpdateOnCollision(const struct CollisionInfo& ci);

	//json�f�[�^�𔽉f������
	void SetObjDataFromJson(const nlohmann::json&);
	//json�f�[�^���擾����
	nlohmann::json GetJsonFromObj(std::string objID, std::string objName);

	void AddChild(Object* child);
	void RemoveChild(Object* child);

	//Get&Set
	const State GetState()const;
	void SetState(State st);
	//���[�J�����W
	void SetLocalPos(float x, float y, float z);
	void SetLocalRot(float x, float y, float z);
	void SetLocalScale(const Vector3& scale) { scale_ = scale; }
	const Vector3& GetLocalPos()const;
	const Vector3& GetLocalRot()const;
	const Vector3 GetLocalScale()const { return scale_; }
	//���[���h���W
	void SetWorldPos(float x, float y, float z);
	void SetWorldRot(float x, float y, float z);
	void SetWorldScale(const Vector3& scale);
	const Vector3 GetWorldPos()const;
	const Vector3 GetWorldRot()const;
	const Vector3 GetWorldScale()const;
	//�O�����̎�
	const Vector3 GetForward()const;
	//�E�����̎�
	const Vector3 GetRight()const;
	//������̎�
	const Vector3 GetUp()const;
	//kinematic��
	bool IsKinematic()const;
	void SetIsKinematic(bool flag) { isKinematic = flag; }
	//parent�̏��
	Object* GetParent()const { return parent_; }

	void AddLocalRot(float roll, float pitch, float yaw);
	void AddLocalPos(float x, float y, float z);
	void AddWorldRot(float roll, float pitch, float yaw);
	void AddWorldPos(float x, float y, float z);

	//true�Ȃ��MeshComponent�����Ă�ꍇ�̂ݕ`�悷��
	void SetDraw(bool flag) {
		isDraw = flag; }
	bool IsDraw()const { return isDraw; }

private:
	void UpdateComponents(float deltaTime);
	void UpdateChildren(float deltaTime);
	virtual void UpdateObject(float deltaTime)=0;
	virtual void OnCollision(const CollisionInfo& ci);

	//JSON�t�@�C���֌W�B�ǉ��̐ݒ肪����ꍇ�͂������I�[�o�[���C�h����
	virtual void SetAdditionalDataFromJson(const nlohmann::json&){}
	virtual void GetAdditionalJsonFromObj(nlohmann::json&) {}

	virtual void ActorInput(const InputState& inputState);

	State state_;
	std::vector<Component*> components_;
	std::vector<Object*> children_;
	std::vector<Object*> pendingObjects_;
	bool updatingObject_;
	Object* parent_;
	//�S�����[�J�����W
	Vector3 position_;
	Vector3 rotation_;
	//�{��
	Vector3 scale_;
	
	bool isKinematic;

	bool isDraw;//�`�悷�邩�ǂ���
};

