#pragma once
#include "MathFunc.h"
#include <functional>
#include <math.h>
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"

using namespace DirectX;

//�j���[�g���@�ŋߎ��l�����߂�
 float
GetYFromXOnBezier(
	float x,
	const Vector2& a,
	const Vector2& b,
	int n
) {
	if (a.x == a.y && b.x == b.y) {
		//���`��ԂȂ̂ł��̂܂ܕԂ�
		return x;
	}
	//�_p�����߂�x�W�F�Ȑ��͎l�_a,b,c,d����������
	//(1-t)^3a+3(1-t)^2tb+3(1-t)t^2c+t^3d=p�̎�
	//�d�l��A�X�^�[�g��0�ōŌオ1�Ȃ̂ł���������Ĉ���a,b��p���Đ�����������
	//t^3(1+3a-3b)+t^2(3b-6a)+t(3a)=p�̎�p(x,y)
	//���̂Ƃ��^����ꂽx,a,b������������t���j���[�g���@�ŋ��߁A���̌�y�̒l��Ԃ�
	//�j���[�g���@��n��J��Ԃ�

	float t = x;
	float k0 = 1 + 3 * a.x - 3 * b.x;//t^3�̌W��
	float k1 = 3 * b.x - 6 * a.x;//t^2�̌W��
	float k2 = 3 * a.x;//t�̌W��

	constexpr float epsilon = 0.0005f;
	for (int i = 0; i < n; i++) {
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		if (fabs(ft) < epsilon) {
			break;
		}
		auto fdt = 3 * k0 * t * t + 2 * k1 * t + k2;
		if (fdt == 0)break;
		t -= ft / fdt;
	}
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

//z�������̕����Ɍ�������
 XMMATRIX
LookAtMatrix(
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
) {
	//���[�J���̎������߂�
	auto zaxis = lookat;
	zaxis.Normalize();
	auto yaxis = up;
	yaxis.Normalize();
	Vector3 xaxis;
	xaxis = Cross(yaxis, zaxis);
	xaxis.Normalize();
	yaxis = Cross(zaxis, xaxis);
	yaxis.Normalize();
	//lookat��up�����������̏ꍇ��right�ō��
	if (fabs(Dot(zaxis, yaxis)) == 1.0f) {
		xaxis = right;
		yaxis = Cross(zaxis, xaxis);
		yaxis.Normalize();
		xaxis = Cross(yaxis, zaxis);
		xaxis.Normalize();

	}
	//�s��̍쐬
	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = XMLoadFloat3(&xaxis);
	ret.r[1] = XMLoadFloat3(&yaxis);
	ret.r[2] = XMLoadFloat3(&zaxis);
	return ret;
}

//����̃x�N�g�������̕����Ɍ�������
 XMMATRIX
LookAtMatrix(
	const Vector3& zaxis,
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
) {
	//z�������̃x�N�g���Ɍ������邬�傤���
	//����̋t�s��i��]�Ȃ̂œ]�u�j��������z���Ɍ�������
	auto mat = LookAtMatrix(zaxis, up, right);
	//���̌��lookat�����Ɍ�������
	return XMMatrixTranspose(mat) *
		LookAtMatrix(lookat, up, right);

}
//�@����z�����@���������ۂ��̂ŕϊ�����
/*
 void
CalcVertNormal(
	const std::vector<Vector3>& verts,
	const std::vector<Index>& indis,
	std::vector<Vector3>& normals
) {
	//�אڂ���ʂ̖@���̔ԍ�
	std::vector<
		std::vector<int>> nextPlane(verts.size());
	std::vector<Vector3> planeNormal;
	//���ꂼ��̖ʂ̖@�����v�Z
	//�C���f�b�N�X�͉E���
	for (auto i : indis) {
		auto vec1 = verts[i.b] - verts[i.a];
		auto vec2 = verts[i.c] - verts[i.a];
		auto no = Cross(vec1, vec2);
		//���Ԗڂ̖@���ɗאڂ��邩���L�^
		auto idx = planeNormal.size();
		nextPlane[i.a].emplace_back(idx);
		nextPlane[i.b].emplace_back(idx);
		nextPlane[i.c].emplace_back(idx);
		//�@���̋L�^
		planeNormal.emplace_back(no);
	}
	//���_���ɒ��_�@�������߂�
	for (auto nPlane : nextPlane) {
		Vector3 sumNormal = Vector3::Zero();
		for (auto n : nPlane) {
			sumNormal += planeNormal[n];
		}
		sumNormal.Normalize();
		normals.emplace_back(sumNormal);
	}
}
*/
void
CalcVertNormal(
	std::vector<VertexData>& verts,
	const std::vector<Index>& indis
) {
	//�אڂ���ʂ̖@���̔ԍ�
	std::vector<
		std::vector<unsigned int>> nextPlane;
	nextPlane.resize(verts.size());
	std::vector<Vector3> planeNormal;
	//���ꂼ��̖ʂ̖@�����v�Z
	//�C���f�b�N�X�͉E���
	for (auto i : indis) {
		auto vec1 = verts[i.b].pos - verts[i.a].pos;
		auto vec2 = verts[i.c].pos - verts[i.a].pos;
		auto no = Cross(vec1, vec2);
		//���Ԗڂ̖@���ɗאڂ��邩���L�^
		auto idx = planeNormal.size();
		nextPlane[i.a].emplace_back(idx);
		nextPlane[i.b].emplace_back(idx);
		nextPlane[i.c].emplace_back(idx);
		//�@���̋L�^
		planeNormal.emplace_back(no);
	}
	//���_���ɒ��_�@�������߂�
	for (int i = 0; i < verts.size(); i++) {
		auto& nPlane = nextPlane[i];
		Vector3 sumNormal = Vector3::Zero();
		for (auto n : nPlane) {
			sumNormal += planeNormal[n];
		}
		sumNormal.Normalize();
		verts[i].normal = sumNormal;
	}
}

//�f�V�x���̑��ݕϊ�
#define MIN_DECIBEL -60//1/1000�{
 float
GetDBFromAmpRatio(float rat) {
	//�^�������ɍ��킹��
	rat = (rat > 0) ? rat : -rat;
	if (fabs(rat) < FLT_EPSILON) {
		//0��������
		return MIN_DECIBEL;
	}
	float db = 20 * std::log10f(rat);
	return db;
}

 float
GetAmpRatioFromDB(float db) {
	if (db < MIN_DECIBEL) {
		return 0;
	}
	db *= 0.05f;//(=/20)
	float rat = std::powf(10, db);
	return rat;
}