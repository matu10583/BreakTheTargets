#pragma once
#include "Component.h"
#include "Collider.h"
#include <vector>
#include "XMFLOAT_Helper.h"

//�Փˎ��̏��
struct CollisionInfo
{
	//�Փˎ��̒E�o����[1]����[0]�̌���
	Vector3 outVec_;
	//�Փˑ���
	const class ColliderComponent* collider[2];
};
//�R���C�_�[�̎��
enum ColliderType
{
	TYPE_NONE = 1,
	TYPE_PLAYER = 2,//�v���C���[
	TYPE_GROUND = 4,//�n��
	TYPE_GROUNDTRRIGER = 8,//���n�g���K�[
	TYPE_BULLET = 16,//�e��
	TYPE_WALL = 32,
	TYPE_TARGET = 64,//�ł��ꂽ�玀�ʂ��
};

class ColliderComponent :
    public Component
{
public:


    ColliderComponent(class Object* owner, bool istrriger, int updateOrder = 100);
	~ColliderComponent();

	const std::vector<CollisionInfo>& GetCollisionInfos()const;
	//�Փˏ��z������Z�b�g����
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

	//�R���C�_�[�̏�Ԃ��X�V����
	virtual void UpdateCollider() = 0;

	//���[�J����AABB��������Ă���
	//�f�o�b�O�p�ɔ��������������
	const AABB& GetLocalAABB() { return localAABB_; }



protected:
	std::vector<CollisionInfo> collisionInfos_;
	bool isTrigger_;
	AABB localAABB_;
	AABB worldAABB_;
	//�R���C�_�[���ʗp�̃^�C�v
	uint64_t type_;

	//�I�u�W�F�N�g���W����̃I�t�Z�b�g
	Vector3 offsetPos_;
	//�I�u�W�F�N�g��]����̃I�t�Z�b�g
	Vector3 offsetRot_;
};

