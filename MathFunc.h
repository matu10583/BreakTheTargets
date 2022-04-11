#pragma once
#include "XMFLOAT_Helper.h"
#include <vector>

//ニュートン法で近似値を求める
extern float
GetYFromXOnBezier(
	float x,
	const Vector2& a,
	const Vector2& b,
	int n
);

//z軸を特定の方向に向かせる
extern XMMATRIX
LookAtMatrix(
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
);

//特定のベクトルを特定の方向に向かせる
extern XMMATRIX
LookAtMatrix(
	const Vector3& zaxis,
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
);
//法線はz軸が法線方向っぽいので変換する
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

//デシベルの相互変換
extern float
GetDBFromAmpRatio(float rat);

extern float
GetAmpRatioFromDB(float db);