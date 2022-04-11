#pragma once
#include <DirectXMath.h>
#include "XMFLOAT_Helper.h"
#include <vector>
#include <array>

using namespace DirectX;
//衝突図形の定義



//線
struct Line
{
	//始点
	Vector3 start_;
	//終点
	Vector3 end_;
	//線上の点を返す
	const Vector3 GetPointOnLine(float t)const;
	//vectorを返す
	inline const Vector3 Vec()const {
		auto ret = end_ - start_;
		return ret;
	}
	void Rotate(const XMMATRIX& rotmat);
	void SetCenter(const Vector3& pos);
	//移動
	void Move(const Vector3& pos);
	void Scale(const XMMATRIX& sca);

	//点と線の最小距離の二乗
	static float MinDistPointSqr(const Vector3& p, const Line& l, Vector3& h, float& t);
	static float MinDistPoint(const Vector3& p, const Line& l, Vector3& h, float& t);
	//線分同士の最短距離
	static float MinDistLineSegment(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	static float MinDistLineSegmentSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	//直線同士の最短距離
	static float MinDistLine(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);
	static float MinDistLineSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
		float& t1, float& t2);

};


//平面
struct Plane
{
	//法線ベクトル
	Vector3 normal_;
	//平面と原点との距離(符号付き)
	float distance_;
	Plane(){}
	Plane(const Vector3& a, const Vector3& b, const Vector3& c);
	Plane(float a, float b, float c, float d);//平面方程式の係数から生成
	Plane(const Vector3& normal, const Vector3& p);//法線とどの点を通るか
	float SignedMinDist(const Vector3& p)const;
	void GetModel(std::vector<VertexData>* vd, std::vector<Index>* indices)const;
};

//球
struct Sphere
{
	//中心座標
	Vector3 center_;
	//半径
	float radius_;
	//点が含まれるか
	bool Contain(const Vector3& p)const;
	//モデルを得る
	void GetModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
};

//AABB
struct AABB
{
	AABB();
	//座標値が最小になる点
	Vector3 minP_;
	//座標値が最大になる点
	Vector3 maxP_;
	//点を含むようにAABBを再計算する
	void UpdateBox(const Vector3& p);
	//玉ver.
	void UpdateBox(const Sphere& sp);
	//AABBを回転させて新たなAABBを作る
	//ずっと回転させるとボックスがでかくなるので
	//点が多いモデルなら最初のAABBを記録して使う
	void Rotate(const XMMATRIX& rotmat);
	//中心を設定
	void SetCenter(const Vector3& pos);
	//移動
	void Move(const Vector3& pos);
	//スケーリング
	void Scale(const XMMATRIX& sca);
	//AABBのリセット
	void Reset();
	//点が含まれるか
	bool Contain(const Vector3& p)const;
	//判定可視化のためにモデル情報を生成する
	void GetModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
	//上面のモデルを取得.uv情報付き
	void GetPlaneModel(
		std::vector<VertexData>& vd, std::vector<Index>& indices)const;
	//点との最小距離
	static float MinDistPointSqr(const AABB& aabb, const Vector3& p, Vector3& closestP);
	static float MinDistPoint(const AABB& aabb, const Vector3& p, Vector3& closestP);
};

//直方体
struct OBB
{
	//中心座標
	Vector3 center_;
	//回転
	void Rotate(const XMMATRIX& rotmat);
	//スケーリング
	void Scale(const XMMATRIX& scamat);
	//中心から面へのベクトル
	Vector3 sizedAxis_[3];
	//頂点を計算する
	//点が含まれるか
	bool Contain(const Vector3& p)const;
};

//カプセル
struct Capsule
{
	Line line_;
	float radius_;
	//点が含まれるか
	bool Contain(const Vector3& p)const;
	void Rotate(const XMMATRIX& rotmat) { line_.Rotate(rotmat); }
	void Scale(const XMMATRIX& scamat) { line_.Rotate(scamat); }
	void SetCenter(const Vector3& pos) { line_.SetCenter(pos); }
	//移動
	void Move(const Vector3& pos) { line_.Move(pos); }
};

struct ViewFrustum
{
	Plane planes_[4];//それぞれの平面(near,farは除く)
	//プロジェクション行列,逆ビュー行列から生成
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

	//しすいだいカリング用
	bool Intersect(const ViewFrustum& vf, const AABB& aabb);
}


