#pragma once
#include "MathFunc.h"
#include <functional>
#include <math.h>
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"

using namespace DirectX;

//ニュートン法で近似値を求める
 float
GetYFromXOnBezier(
	float x,
	const Vector2& a,
	const Vector2& b,
	int n
) {
	if (a.x == a.y && b.x == b.y) {
		//線形補間なのでそのまま返す
		return x;
	}
	//点pを求めるベジェ曲線は四点a,b,c,dをもちいて
	//(1-t)^3a+3(1-t)^2tb+3(1-t)t^2c+t^3d=pの式
	//仕様上、スタートは0で最後が1なのでそれを代入して引数a,bを用いて整理した式は
	//t^3(1+3a-3b)+t^2(3b-6a)+t(3a)=pの式p(x,y)
	//このとき与えられたx,a,bを代入した時のtをニュートン法で求め、その後yの値を返す
	//ニュートン法はn回繰り返す

	float t = x;
	float k0 = 1 + 3 * a.x - 3 * b.x;//t^3の係数
	float k1 = 3 * b.x - 6 * a.x;//t^2の係数
	float k2 = 3 * a.x;//tの係数

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

//z軸を特定の方向に向かせる
 XMMATRIX
LookAtMatrix(
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
) {
	//ローカルの軸を決める
	auto zaxis = lookat;
	zaxis.Normalize();
	auto yaxis = up;
	yaxis.Normalize();
	Vector3 xaxis;
	xaxis = Cross(yaxis, zaxis);
	xaxis.Normalize();
	yaxis = Cross(zaxis, xaxis);
	yaxis.Normalize();
	//lookatとupが同じ方向の場合はrightで作る
	if (fabs(Dot(zaxis, yaxis)) == 1.0f) {
		xaxis = right;
		yaxis = Cross(zaxis, xaxis);
		yaxis.Normalize();
		xaxis = Cross(yaxis, zaxis);
		xaxis.Normalize();

	}
	//行列の作成
	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = XMLoadFloat3(&xaxis);
	ret.r[1] = XMLoadFloat3(&yaxis);
	ret.r[2] = XMLoadFloat3(&zaxis);
	return ret;
}

//特定のベクトルを特定の方向に向かせる
 XMMATRIX
LookAtMatrix(
	const Vector3& zaxis,
	const Vector3& lookat,
	const Vector3& up,
	const Vector3& right
) {
	//z軸を特定のベクトルに向かせるぎょうれつ
	//これの逆行列（回転なので転置）をかけてz軸に向かせる
	auto mat = LookAtMatrix(zaxis, up, right);
	//その後にlookat方向に向かせる
	return XMMatrixTranspose(mat) *
		LookAtMatrix(lookat, up, right);

}
//法線はz軸が法線方向っぽいので変換する
/*
 void
CalcVertNormal(
	const std::vector<Vector3>& verts,
	const std::vector<Index>& indis,
	std::vector<Vector3>& normals
) {
	//隣接する面の法線の番号
	std::vector<
		std::vector<int>> nextPlane(verts.size());
	std::vector<Vector3> planeNormal;
	//それぞれの面の法線を計算
	//インデックスは右回り
	for (auto i : indis) {
		auto vec1 = verts[i.b] - verts[i.a];
		auto vec2 = verts[i.c] - verts[i.a];
		auto no = Cross(vec1, vec2);
		//何番目の法線に隣接するかを記録
		auto idx = planeNormal.size();
		nextPlane[i.a].emplace_back(idx);
		nextPlane[i.b].emplace_back(idx);
		nextPlane[i.c].emplace_back(idx);
		//法線の記録
		planeNormal.emplace_back(no);
	}
	//頂点事に頂点法線を求める
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
	//隣接する面の法線の番号
	std::vector<
		std::vector<unsigned int>> nextPlane;
	nextPlane.resize(verts.size());
	std::vector<Vector3> planeNormal;
	//それぞれの面の法線を計算
	//インデックスは右回り
	for (auto i : indis) {
		auto vec1 = verts[i.b].pos - verts[i.a].pos;
		auto vec2 = verts[i.c].pos - verts[i.a].pos;
		auto no = Cross(vec1, vec2);
		//何番目の法線に隣接するかを記録
		auto idx = planeNormal.size();
		nextPlane[i.a].emplace_back(idx);
		nextPlane[i.b].emplace_back(idx);
		nextPlane[i.c].emplace_back(idx);
		//法線の記録
		planeNormal.emplace_back(no);
	}
	//頂点事に頂点法線を求める
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

//デシベルの相互変換
#define MIN_DECIBEL -60//1/1000倍
 float
GetDBFromAmpRatio(float rat) {
	//真数条件に合わせる
	rat = (rat > 0) ? rat : -rat;
	if (fabs(rat) < FLT_EPSILON) {
		//0だったら
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