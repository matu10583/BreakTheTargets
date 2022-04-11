#pragma once
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"
#include <vector>
#include <array>

using namespace DirectX;
//�Փː}�`�̒�`



//��
struct Line
{
	//�n�_
	Vector3 start_;
	//�I�_
	Vector3 end_;
	//����̓_��Ԃ�
	const Vector3 GetPointOnLine(float t)const;
	//vector��Ԃ�
	inline const Vector3 Vec()const {
		auto ret = end_ - start_;
		return ret;
	}
	void Rotate(const XMMATRIX& rotmat);
	void SetCenter(const Vector3& pos);
	//�ړ�
	void Move(const Vector3& pos);
	void Scale(const XMMATRIX& sca);

	//�_�Ɛ��̍ŏ������̓��
	static float MinDistPointSqr(const Vector3& p, const Line& l, Vector3& h, float& t);
	static float MinDistPoint(const Vector3& p, const Line& l, Vector3& h, float& t);
	//�������m�̍ŒZ����
	static float MinDistLineSegment(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	static float MinDistLineSegmentSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	//�������m�̍ŒZ����
	static float MinDistLine(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	static float MinDistLineSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);

};


//����
struct Plane
{
	//�@���x�N�g��
	Vector3 normal_;
	//���ʂƌ��_�Ƃ̋���(�����t��)
	float distance_;
	Plane(){}
	Plane(const Vector3& a, const Vector3& b, const Vector3& c);
	Plane(float a, float b, float c, float d);//���ʕ������̌W�����琶��
	Plane(const Vector3& normal, const Vector3& p);//�@���Ƃǂ̓_��ʂ邩
	float SignedMinDist(const Vector3& p)const;
	void GetModel(std::vector<VertexData>* vd, std::vector<Index>* indices)const;
};

//��
struct Sphere
{
	//���S���W
	Vector3 center_;
	//���a
	float radius_;
	//�_���܂܂�邩
	bool Contain(const Vector3& p)const;
	//���f���𓾂�
	void GetModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
};

//AABB
struct AABB
{
	AABB();
	//���W�l���ŏ��ɂȂ�_
	Vector3 minP_;
	//���W�l���ő�ɂȂ�_
	Vector3 maxP_;
	//�_���܂ނ悤��AABB���Čv�Z����
	void UpdateBox(const Vector3& p);
	//��ver.
	void UpdateBox(const Sphere& sp);
	//AABB����]�����ĐV����AABB�����
	//�����Ɖ�]������ƃ{�b�N�X���ł����Ȃ�̂�
	//�_���������f���Ȃ�ŏ���AABB���L�^���Ďg��
	void Rotate(const XMMATRIX& rotmat);
	//���S��ݒ�
	void SetCenter(const Vector3& pos);
	//�ړ�
	void Move(const Vector3& pos);
	//�X�P�[�����O
	void Scale(const XMMATRIX& sca);
	//AABB�̃��Z�b�g
	void Reset();
	//�_���܂܂�邩
	bool Contain(const Vector3& p)const;
	//��������̂��߂Ƀ��f�����𐶐�����
	void GetModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
	//��ʂ̃��f�����擾.uv���t��
	void GetPlaneModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
	//�_�Ƃ̍ŏ�����
	static float MinDistPointSqr(const AABB& aabb, const Vector3& p, Vector3& closestP);
	static float MinDistPoint(const AABB& aabb, const Vector3& p, Vector3& closestP);
};

//������
struct OBB
{
	//���S���W
	Vector3 center_;
	//��]
	void Rotate(const XMMATRIX& rotmat);
	//�X�P�[�����O
	void Scale(const XMMATRIX& scamat);
	//���S����ʂւ̃x�N�g��
	Vector3 sizedAxis_[3];
	//���_���v�Z����
	//�_���܂܂�邩
	bool Contain(const Vector3& p)const;
};

//�J�v�Z��
struct Capsule
{
	Line line_;
	float radius_;
	//�_���܂܂�邩
	bool Contain(const Vector3& p)const;
	void Rotate(const XMMATRIX& rotmat) { line_.Rotate(rotmat); }
	void Scale(const XMMATRIX& scamat) { line_.Rotate(scamat); }
	void SetCenter(const Vector3& pos) { line_.SetCenter(pos); }
	//�ړ�
	void Move(const Vector3& pos) { line_.Move(pos); }
};

struct ViewFrustum
{
	Plane planes_[4];//���ꂼ��̕���(near,far�͏���)
	//�v���W�F�N�V�����s��,�t�r���[�s�񂩂琶��
	ViewFrustum(const XMMATRIX& proj
		, const Vector3& eyepos, const Vector3& eyerot);

};

namespace Collide {

	bool Intersect(const Capsule& a, const Capsule& b, Vector3& outnorm);
	bool Intersect(const Capsule& a, const Line& b, Vector3& outnorm);
	bool Intersect(const Capsule& a, const AABB& b, Vector3& outnorm);
	bool Intersect(const Capsule& a, const OBB& b, Vector3& outnorm);
	bool Intersect(const AABB& a, const AABB& b, Vector3& outnorm);
	bool Intersect(const AABB& a, const Line& b, Vector3& outnorm);
	bool Intersect(const OBB& a, const AABB& b, Vector3& outnorm);
	bool Intersect(const OBB& a, const OBB& b, Vector3& outnorm);
	bool Intersect(const Line& a, const OBB& b, Vector3& outnorm);
	bool Intersect(const Sphere& a, const AABB& b, Vector3& outnorm);
	bool Intersect(const Sphere& a, const OBB& b, Vector3& outnorm);

	//�����������J�����O�p
	bool Intersect(const ViewFrustum& vf, const AABB& aabb);
}


