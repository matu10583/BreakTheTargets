#pragma once
#include "XMFLOAT_Helper.h"
#include <vector>

//�j���[�g���@�ŋߎ��l�����߂�
extern float
GetYFromXOnBezier(
	float x,
	const Vector2& a,
	const Vector2& b,
	int n
);

//z�������̕����Ɍ�������
extern XMMATRIX
LookAtMatrix(
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
);

//����̃x�N�g�������̕����Ɍ�������
extern XMMATRIX
LookAtMatrix(
	const Vector3& zaxis,
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
);
//�@����z�����@���������ۂ��̂ŕϊ�����
/*
extern void 
CalcVertNormal(
	const std::vector<Vector3>& verts,
	const std::vector<Index>& indis,
	std::vector<Vector3>& normals
);
*/
extern void
CalcVertNormal(
	std::vector<VertexData>& verts,
	const std::vector<Index>& indis
);

//�f�V�x���̑��ݕϊ�
extern float
GetDBFromAmpRatio(float rat);

extern float
GetAmpRatioFromDB(float db);