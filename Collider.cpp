#include "Collider.h"
#include "MathFunc.h"
#include <algorithm>
#include <math.h>


//Line
const Vector3
Line::GetPointOnLine(float t)const {
	auto ret = start_ + (end_ - start_) * t;
	return ret;
}

float
Line::MinDistPointSqr(const Vector3& p, const Line& l, Vector3& h, float& t) {
	const auto sp = p - l.start_;
	const auto ep = p - l.end_;
	const auto se = l.end_ - l.start_;
	const auto es = -se;

	//線の内側に点がある。
	auto len = Dot(se, sp);
	t = len / se.Magnitude();
	h = l.start_ + se * t;

	if (Dot(sp, se) <= 0) {
		//start側の外側に点がある
		h = l.start_;
		return sp.MagnitudeSqr();
	}
	if (Dot(es, ep) <= 0) {
		//end側の外側に点がある
		h = l.end_;
		return ep.MagnitudeSqr();
	}

	return len;
}

float Line::MinDistPoint(const Vector3& p, const Line& l, Vector3& h, float& t) {
	return sqrt(MinDistPointSqr(p, l, h, t));
}

void
Line::Rotate(const XMMATRIX& rotMat) {
	start_ = XMVector3Transform(XMLoadFloat3(&start_), rotMat);
	end_ = XMVector3Transform(XMLoadFloat3(&end_), rotMat);
}

void 
Line::SetCenter(const Vector3& center) {
	auto c = (start_ + end_) / 2;
	auto moveVec = center - c;
	Move(moveVec);
}

void
Line::Move(const Vector3& moveVec) {
	start_ += moveVec;
	end_ += moveVec;
}

void
Line::Scale(const XMMATRIX& mat) {
	start_ = Vector3(XMVector3Transform(
		XMLoadFloat3(&start_), mat
	));
	end_ = Vector3(XMVector3Transform(
		XMLoadFloat3(&end_), mat
	));
}


//線分：l1, l2
//線分における推薦の足：p1,p2
//垂線のベクトル係数:t1, t2
float
Line::MinDistLineSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
	float& t1, float& t2){
	auto l1v = l1.end_ - l1.start_;
	auto l2v = l2.end_ - l2.start_;
	//平行なら|l1v|*|l2v|
	auto l1Dl2 = Dot(l1v, l2v);
	auto D = l1Dl2 * l1Dl2 - l1v.MagnitudeSqr() * l2v.MagnitudeSqr();
	if (abs(D) < FLT_EPSILON) {
		//平行なので始点同士の距離
		auto len = MinDistPoint(l1.start_, l2, p2, t2);
		p1 = l1.start_;
		t1 = 0.0f;

		return len;
	}
	float l1Dl1 = l1v.MagnitudeSqr();
	float l2Dl2 = l2v.MagnitudeSqr();
	auto l2sl1s = l1.start_ - l2.start_;
	t1 = (l1Dl2 * Dot(l2v, l2sl1s) - l2Dl2 * Dot(l1v, l2sl1s)) /
		(l1Dl1 * l2Dl2 - l1Dl2 * l1Dl2);
	p1 = l1.GetPointOnLine(t1);
	t2 = Dot(l2v, p1 - l2.start_) / l2Dl2;
	p2 = l2.GetPointOnLine(t2);

	return (p2 - p1).Magnitude();
}

float Line::MinDistLine(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
	float& t1, float& t2) {
	return sqrt(MinDistLineSqr(l1, l2, p1, p2, t1, t2));
}


void clamp0_1(float& v) {
	if (v < 0.0f) v = 0.0f;
	else if (v > 1.0f) v = 1.0f;
}

float
Line::MinDistLineSegmentSqr(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
	float& t1, float& t2) {
	auto l1v = l1.end_ - l1.start_;
	auto l2v = l2.end_ - l2.start_;
	//平行なら|l1v|*|l2v|
	auto l1Dl2 = Dot(l1v, l2v);
	auto D = l1Dl2 * l1Dl2 - l1v.MagnitudeSqr() * l2v.MagnitudeSqr();
	if (abs(D) < FLT_EPSILON) {
		//平行なので端点を適当に決める
		t1 = 0.0f;
		p1 = l1.start_;
		float len = MinDistPoint(p1, l2, p2, t2);
		if (0.0f <= t2 && t2 <= 1.0f) {
			return len;
		}
	}
	else {
		//２直線の最短距離を求め、仮のパラメーターを決める
		float len = MinDistLine(l1, l2, p1, p2, t1, t2);
		if (
			0.0f <= t1 && t1 <= 1.0 &&
			0.0f <= t2 && t2 <= 1.0
			) {
			//線分の内側に来たので終わり
			return len;
		}
	}

	//まだ垂線の足が外にあるので続行
	clamp0_1(t1);
	p1 = l1.GetPointOnLine(t1);
	float len = MinDistPoint(p1, l2, p2, t2);
	if (0.0f <= t2 && t2 <= 1.0f) {
		return len;
	}

	//まだ垂線の足が外にあるので続行
	clamp0_1(t2);
	p2 = l2.GetPointOnLine(t2);
	len = MinDistPoint(p2, l1, p1, t1);
	if (0.0f <= t2 && t2 <= 1.0f) {
		return len;
	}

	//端点どうしが最短
	clamp0_1(t1);
	p1 = l1.GetPointOnLine(t1);
	return (p2 - p1).MagnitudeSqr();
}

float Line::MinDistLineSegment(const Line& l1, const Line& l2, Vector3& p1, Vector3& p2,
	float& t1, float& t2) {
	return sqrt(MinDistLineSegmentSqr(l1, l2, p1, p2, t1, t2));
}

//Plane
Plane::Plane(const Vector3& a, const Vector3& b, const Vector3& c) {
	//外積から法線を求める
	const auto v1 = b - a;
	const auto v2 = c - a;
	auto norm = Cross(v1, v2);
	norm.Normalize();
	normal_ = norm;
	//法線に点の位置ベクトルを投影
	distance_ = -Dot(a, norm);
}

Plane::Plane(const Vector3& normal, const Vector3& p) {
	//外積から法線を求める
	normal_ = normal;
	//法線に点の位置ベクトルを投影
	distance_ = -Dot(p, normal);
}

float
Plane::SignedMinDist(const Vector3& p)const {
	//法線に点の位置ベクトルを投影する
	//そこからdistanceを引くと表裏の符号付き距離が得られる
	auto pdist = Dot(p, normal_);
	return pdist + distance_;
}

Plane::Plane(float a, float b, float c, float d) {
	auto nor = Vector3(a, b, c);
	auto mag = nor.Magnitude();
	normal_ = nor / mag;
	distance_ = d / mag;
}



//Sphere

bool
Sphere::Contain(const Vector3& p)const {
	auto dist = center_ - p;
	if (dist.MagnitudeSqr() <= radius_ * radius_) {
		return true;
	}
	return false;
}

void
Sphere::GetModel(
	std::vector<VertexData>& vd, std::vector<Index>& indices
)const {
	//断面円上の点8個を4断面分求める。なお上下の点はかぶるので合計26頂点
	vd.resize(26);
	vd[0].pos = Vector3(0, radius_, 0);
	vd[0].uv = Vector2(0, 0);//uvは無理……
	vd[0].normal = Vector3(0, 1, 0);
	Vector3 pos = Vector3::Zero();
	for (int i = 0; i < 3; i++) {
		auto k = -(i - 1);//1~-1
		pos.y = radius_ * std::sin(XM_PIDIV4 * k);
		auto rad = radius_ * std::cos(XM_PIDIV4 * k);
		for (int j = 0; j < 8; j++) {
			pos.x = rad * std::cos(XM_PIDIV4 * j);
			pos.z = rad * std::sin(XM_PIDIV4 * j);
			vd[i * 8 + j + 1].pos = pos;
			vd[i * 8 + j + 1].uv = Vector2::Zero();
			//法線は原点から頂点の方向
			auto norm = pos;
			norm.Normalize();
			vd[i * 8 + j + 1].normal = norm;
		}
	}
	vd[25].pos = Vector3(0, -radius_, 0);
	vd[25].uv = Vector2::Zero();
	vd[25].normal = Vector3(0, -1, 0);

	//インデックスを決める.数は三角形8*2個、四角形8*2個で48個
	indices.resize(48);
	for (int i = 0; i < 8; i++) {
		//上面0~7
		auto i2 = i + 1;//二段目に対応する頂点番号
		indices[i] = Index(0, (i2 % 8) + 1, i2);
		//2段目8~23
		auto i3 = i + 9;//三段目に対応する頂点番号
		indices[2 * i + 8] = Index(i2, (i2 % 8) + 1, i3);
		indices[2 * i + 9] = Index((i2 % 8) + 1, (i3 % 8) + 9, i3);
		//3段目24~39
		auto i4 = i + 17;//4段目に対応する頂点番号
		indices[2 * i + 24] = Index(i3, (i3 % 8) + 9, i4);
		indices[2 * i + 25] = Index((i3 % 8) + 9, (i4 % 8) + 17, i4);
		//下面40~47
		indices[i+40] = Index(25, i4, (i4 % 8) + 17);
	}
}

//AABB
AABB::AABB():
	maxP_(-FLT_MAX, -FLT_MAX, -FLT_MAX),
	minP_(FLT_MAX, FLT_MAX, FLT_MAX)
{
}

void
AABB::UpdateBox(const Vector3& p) {
	minP_.x = std::min(minP_.x, p.x);
	minP_.y = std::min(minP_.y, p.y);
	minP_.z = std::min(minP_.z, p.z);

	maxP_.x = std::max(maxP_.x, p.x);
	maxP_.y = std::max(maxP_.y, p.y);
	maxP_.z = std::max(maxP_.z, p.z);

}

void
AABB::UpdateBox(const Sphere& sp) {
	minP_.x = std::min(minP_.x, sp.center_.x - sp.radius_);
	minP_.y = std::min(minP_.y, sp.center_.y - sp.radius_);
	minP_.z = std::min(minP_.z, sp.center_.z - sp.radius_);


	maxP_.x = std::max(maxP_.x, sp.center_.x + sp.radius_);
	maxP_.y = std::max(maxP_.y, sp.center_.y + sp.radius_);
	maxP_.z = std::max(maxP_.z, sp.center_.z + sp.radius_);
}

void
AABB::Rotate(const XMMATRIX& rotMat) {

	//AABBの各頂点
	std::array<Vector3, 8> points;
	points[0] = minP_;

	points[1].x = minP_.x;
	points[1].y = maxP_.y;
	points[1].z = maxP_.z;
	points[2].x = maxP_.x;
	points[2].y = minP_.y;
	points[2].z = maxP_.z;
	points[3].x = maxP_.x;
	points[3].y = maxP_.y;
	points[3].z = minP_.z;

	points[4].x = minP_.x;
	points[4].y = minP_.y;
	points[4].z = maxP_.z;
	points[5].x = minP_.x;
	points[5].y = maxP_.y;
	points[5].z = minP_.z;
	points[6].x = maxP_.x;
	points[6].y = minP_.y;
	points[6].z = minP_.z;

	points[7] = maxP_;

	//min,maxのリセット
	minP_ = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	maxP_ = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (auto& p : points) {
		auto newp = XMVector3Transform(XMLoadFloat3(&p), rotMat);
		UpdateBox(newp);
	}

}

void
AABB::Scale(const XMMATRIX& scaMat) {

	//AABBの各頂点
	std::array<Vector3, 8> points;
	points[0] = minP_;

	points[1].x = minP_.x;
	points[1].y = maxP_.y;
	points[1].z = maxP_.z;
	points[2].x = maxP_.x;
	points[2].y = minP_.y;
	points[2].z = maxP_.z;
	points[3].x = maxP_.x;
	points[3].y = maxP_.y;
	points[3].z = minP_.z;

	points[4].x = minP_.x;
	points[4].y = minP_.y;
	points[4].z = maxP_.z;
	points[5].x = minP_.x;
	points[5].y = maxP_.y;
	points[5].z = minP_.z;
	points[6].x = maxP_.x;
	points[6].y = minP_.y;
	points[6].z = minP_.z;

	points[7] = maxP_;

	//min,maxのリセット
	minP_ = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	maxP_ = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (auto& p : points) {
		auto newp = XMVector3Transform(XMLoadFloat3(&p), scaMat);
		UpdateBox(newp);
	}

}

void
AABB::SetCenter(const Vector3& center) {
	auto c = (minP_ + maxP_) / 2;
	auto moveVec = center - c;
	Move(moveVec);
}

void
AABB::Move(const Vector3& moveVec) {
	minP_ += moveVec;
	maxP_ += moveVec;
}

bool
AABB::Contain(const Vector3& p)const {
	if (
		minP_.x < p.x &&
		minP_.y < p.y &&
		minP_.z < p.z
		) {
		if (
			maxP_.x > p.x &&
			maxP_.y > p.y &&
			maxP_.z > p.z
			) {
			return true;
		}
	}

	return false;
}

void
AABB::Reset() {
	//min,maxのリセット
	minP_ = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	maxP_ = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void
AABB::GetModel(std::vector<VertexData>& vds, std::vector<Index>& ids)const {
	auto mm = maxP_ - minP_;
	vds.resize(8);
	//下面
	vds[0].pos = minP_;
	vds[4].pos = minP_ + Vector3(0, 0, mm.z);
	vds[3].pos = minP_ + Vector3(mm.x, 0, 0);
	vds[7].pos = minP_ + Vector3(mm.x, 0, mm.z);
	vds[0].uv = Vector2(0, 1.0f);
	vds[4].uv = Vector2(0, 0);
	vds[3].uv = Vector2(1.0f, 1.0f);
	vds[7].uv = Vector2(1.0f, 0);

	//上面
	vds[1].pos = minP_ + Vector3(0, mm.y, 0);
	vds[5].pos = minP_ + Vector3(0, mm.y, mm.z);
	vds[2].pos = minP_ + Vector3(mm.x, mm.y, 0);
	vds[6].pos = maxP_;
	vds[1].uv = Vector2(0, 1.0f);
	vds[5].uv = Vector2(0, 0);
	vds[2].uv = Vector2(1.0f, 1.0f);
	vds[6].uv = Vector2(1.0f, 0);
	//上面とした面にuvマッピングする

	ids.resize(12);
	ids[0] = Index(0, 1, 2);
	ids[1] = Index(2, 3, 0);
	ids[2] = Index(4, 5, 1);
	ids[3] = Index(1, 0, 4);
	ids[4] = Index(7, 6, 5);
	ids[5] = Index(5, 4, 7);
	ids[6] = Index(3, 2, 6);
	ids[7] = Index(6, 7, 3);
	ids[8] = Index(1, 5, 6);
	ids[9] = Index(6, 2, 1);
	ids[10] = Index(4, 0, 3);
	ids[11] = Index(3, 7, 4);

	CalcVertNormal(vds, ids);
}

void
AABB::GetPlaneModel(std::vector<VertexData>& vds, std::vector<Index>& ids)const {
	auto mm = maxP_ - minP_;
	vds.resize(4);
	vds[0].pos = minP_ + Vector3(0, mm.y, 0);
	vds[1].pos = minP_ + Vector3(0, mm.y, mm.z);
	vds[2].pos = minP_ + Vector3(mm.x, mm.y, 0);
	vds[3].pos = maxP_;
	vds[1].uv = Vector2(0, 1.0f);
	vds[0].uv = Vector2(0, 0);
	vds[3].uv = Vector2(1.0f, 1.0f);
	vds[2].uv = Vector2(1.0f, 0);

	ids.resize(2);
	ids[0] = Index(0, 1, 2);
	ids[1] = Index(2, 1, 3);

	CalcVertNormal(vds, ids);
}

float
AABB::MinDistPointSqr(const AABB& aabb, const Vector3& p, Vector3& closestP) {
	if (aabb.minP_.x > p.x) {
		closestP.x = aabb.minP_.x;
	}
	else if (aabb.maxP_.x < p.x) {
		closestP.x = aabb.maxP_.x;
	}
	else {
		closestP.x = p.x;
	}

	if (aabb.minP_.y > p.y) {
		closestP.y = aabb.minP_.y;
	}
	else if (aabb.maxP_.y < p.y) {
		closestP.y = aabb.maxP_.y;
	}
	else {
		closestP.y = p.y;
	}

	if (aabb.minP_.z > p.z) {
		closestP.z = aabb.minP_.z;
	}
	else if (aabb.maxP_.z < p.z) {
		closestP.z = aabb.maxP_.z;
	}
	else {
		closestP.z = p.z;
	}
	return (closestP - p).MagnitudeSqr();
}


float
AABB::MinDistPoint(const AABB& aabb, const Vector3& p, Vector3& closestP) {
	return sqrt(MinDistPointSqr(aabb, p, closestP));
}


//OBB
bool OBB::Contain(const Vector3& p)const {
	//対角の点のそれぞれにおいて各辺ベクトルと点へのベクトルの
	//角度を調べ、全て直角以内なら含有
	//対角の頂点を調べる
	Vector3 vert1 = center_;
	Vector3 vert2 = center_;
	for (int i = 0; i < 3; i++) {
		vert1 += sizedAxis_[i];
		vert2 += sizedAxis_[i];
	}
	//辺ベクトルを作る
	Vector3 edge[6];
	edge[0] = sizedAxis_[0] * 2;
	edge[1] = sizedAxis_[1] * 2;
	edge[2] = sizedAxis_[2] * 2;

	edge[3] = -edge[0];
	edge[4] = -edge[1];
	edge[5] = -edge[2];

	//vert1
	auto v1p = p - vert1;
	if (
		Dot(v1p, edge[0]) < 0 ||
		Dot(v1p, edge[1]) < 0 ||
		Dot(v1p, edge[2]) < 0
		) {
		return false;
	}
	//vert2
	auto v2p = p - vert2;
	if (
		Dot(v2p, edge[3]) < 0 ||
		Dot(v2p, edge[4]) < 0 ||
		Dot(v2p, edge[5]) < 0
		) {
		return false;
	}

	return true;
}

void
OBB::Rotate(const XMMATRIX& rotMat) {
	for (auto& ax : sizedAxis_) {
		ax = XMVector3Transform(XMLoadFloat3(&ax), rotMat);
	}
}

void
OBB::Scale(const XMMATRIX& scaMat) {
	for (auto& ax : sizedAxis_) {
		ax = XMVector3Transform(XMLoadFloat3(&ax), scaMat);
	}
	center_ = XMVector3Transform(XMLoadFloat3(&center_), scaMat);
}

//Capsule
bool
Capsule::Contain(const Vector3& p)const {
	Vector3 h;
	float t;
	auto dist = Line::MinDistPointSqr(p, line_, h, t);
	return dist <= radius_* radius_;
}

//ViewFrustum
ViewFrustum::ViewFrustum(const XMMATRIX& proj
	, const Vector3& eyepos, const Vector3& eyerot) {
	//視点回転行列
	auto rotmat = XMMatrixRotationRollPitchYaw(
		eyerot.x, eyerot.y, eyerot.z);
	//プロジェクション行列から平面の作成
	//https://edom18.hateblo.jp/entry/2017/10/29/112908
	int s = 1;
	//左右上下
	for (int i = 0; i < 4; i++) {
		int k = i / 2;
		float a = proj.r[0].m128_f32[3] +
			s * proj.r[0].m128_f32[k];
		float b = proj.r[1].m128_f32[3] +
			s * proj.r[1].m128_f32[k];
		float c = proj.r[2].m128_f32[3] +
			s * proj.r[2].m128_f32[k];
		float d = 0;
		//dはどうせ零

		auto normal = Vector3(a, b, c);

		normal = Vector3(XMVector3Transform(
			XMLoadFloat3(&normal), rotmat
		));
		normal.Normalize();


		planes_[i] = Plane(normal, eyepos);

		s *= -1;
	}
}


//以下交差判定
namespace Collide {


	//分離軸に落ちた影の長さ
	float LenShadowOnSeparateAxis(const Vector3& axis, const Vector3& x, const Vector3& y,
		const Vector3& z) {
		auto r1 = std::abs(Dot(axis, x));
		auto r2 = std::abs(Dot(axis, y));
		auto r3 = std::abs(Dot(axis, z));
		return r1 + r2 + r3;
	}

	bool Intersect(const Capsule& a, const Capsule& b, Vector3& outnorm) {

		Vector3 p1, p2;
		float t1, t2;
		auto dist = Line::MinDistLineSegment(a.line_, b.line_, p1, p2, t1, t2);
		outnorm = p1 - p2;
		outnorm.Normalize();
		auto sumrad = a.radius_ + b.radius_;
		outnorm *= (sumrad - dist);
		return dist <= sumrad;
	}

	bool Intersect(const Capsule& a, const Line& b, Vector3& outnorm) {
		Vector3 p1, p2;
		float t1, t2;
		auto dist = Line::MinDistLineSegment(a.line_, b, p1, p2, t1, t2);
		outnorm = p1 - p2;
		outnorm.Normalize();
		outnorm *= (a.radius_ - dist);
		return dist <= a.radius_;
	}

	bool Intersect(const Capsule& a, const AABB& b, Vector3& outnorm) {

		//TODO:これの実装を考える。とりあえず球とOBBの複合体にする
		Sphere capsp[2];
		capsp[0].center_ = a.line_.start_;
		capsp[0].radius_ = capsp[1].radius_ = a.radius_;
		capsp[1].center_ = a.line_.end_;
		OBB capobb;
		capobb.center_ = (a.line_.start_ + a.line_.end_) / 2;
		capobb.sizedAxis_[1] = a.line_.Vec() / 2;
		//適当に垂直なベクトル
		auto xaxis = Cross(a.line_.Vec(), Vector3(1, 0, 0));
		xaxis = xaxis * a.radius_ / xaxis.Magnitude();
		auto zaxis = Cross(a.line_.Vec(), xaxis);
		zaxis = zaxis * a.radius_ / zaxis.Magnitude();

		capobb.sizedAxis_[0] = xaxis;
		capobb.sizedAxis_[2] = zaxis;

		Vector3 spnorm[2];
		auto toSp1 = Intersect(capsp[0], b, spnorm[0]);
		auto toSp2 = Intersect(capsp[1], b, spnorm[1]);
		Vector3 obbnorm;
		auto toObb = Intersect(capobb, b, obbnorm);
		if (toObb) {
			outnorm = obbnorm;
		}
		if (toSp1) {
			outnorm = spnorm[0];
		}
		if (toSp2) {
			outnorm = spnorm[1];
		}
		if (toObb || toSp1 || toSp2) {
			return true;
		}
		return false;
	}

	bool Intersect(const Capsule& a, const OBB& b, Vector3& outnorm) {

		//TODO:これの実装を考える。とりあえず球とOBBの複合体にする
		Sphere capsp[2];
		capsp[0].center_ = a.line_.start_;
		capsp[0].radius_ = capsp[1].radius_ = a.radius_;
		capsp[1].center_ = a.line_.end_;
		OBB capobb;
		capobb.center_ = (a.line_.start_ + a.line_.end_) / 2;
		capobb.sizedAxis_[1] = a.line_.Vec() / 2;
		//適当に垂直なベクトル
		auto xaxis = Cross(a.line_.Vec(), Vector3(1, 0, 0));
		xaxis = xaxis * a.radius_ / xaxis.Magnitude();
		auto zaxis = Cross(a.line_.Vec(), xaxis);
		zaxis = zaxis * a.radius_ / zaxis.Magnitude();

		capobb.sizedAxis_[0] = xaxis;
		capobb.sizedAxis_[2] = zaxis;

		Vector3 spnorm[2];
		auto toSp1 = Intersect(capsp[0], b, spnorm[0]);
		auto toSp2 = Intersect(capsp[1], b, spnorm[1]);
		Vector3 obbnorm;
		auto toObb = Intersect(capobb, b, obbnorm);
		if (toObb) {
			outnorm = obbnorm;
		}
		if (toSp1) {
			outnorm = spnorm[0];
		}
		if (toSp2) {
			outnorm = spnorm[1];
		}
		if (toObb || toSp1 || toSp2) {
			return true;
		}
		return false;
	}

	bool Intersect(const Sphere& a, const AABB& b, Vector3& outnorm) {
		Vector3 point;
		auto dist = AABB::MinDistPoint(b, a.center_, point);
		outnorm = a.center_ - point;
		outnorm.Normalize();
		outnorm *= a.radius_ - dist;

		return dist <= a.radius_;
	}


	bool Intersect(const AABB& a, const AABB& b, Vector3& outnorm) {
		//どこかでminよりmaxが小さいなら交差しない
		float dx1 = b.maxP_.x - a.minP_.x;//正方向に脱出する
		float dx2 = b.minP_.x - a.maxP_.x;//負方向に脱出する
		float dy1 = b.maxP_.y - a.minP_.y;
		float dy2 = b.minP_.y - a.maxP_.y;
		float dz1 = b.maxP_.z - a.minP_.z;
		float dz2 = b.minP_.z - a.maxP_.z;

		if (dx1 < 0 || dy1 < 0 || dz1 < 0 ||
			dx2 > 0 || dy2 >0 || dz2 > 0) {
			return false;
		}
		//どこに脱出するのが一番最短か
		auto dx = (std::abs(dx1) < std::abs(dx2)) ?
			dx1 : dx2;
		auto dy = (std::abs(dy1) < std::abs(dy2)) ?
			dy1 : dy2;
		auto dz = (std::abs(dz1) < std::abs(dz2)) ?
			dz1 : dz2;
		if (std::abs(dx) < std::abs(dy) && std::abs(dx) < std::abs(dz)) {
			outnorm.x = dx;
			outnorm.y = 0;
			outnorm.z = 0;
		}
		if (std::abs(dy) < std::abs(dx) && std::abs(dy) < std::abs(dz)) {
			outnorm.x = 0;
			outnorm.y = dy;
			outnorm.z = 0;
		}
		if (std::abs(dz) < std::abs(dx) && std::abs(dz) < std::abs(dy)) {
			outnorm.x = 0;
			outnorm.y = 0;
			outnorm.z = dz;
		}
		return true;
	}

	bool Intersect(const AABB& a, const Line& b, Vector3& outnorm) {
		//それぞれの軸にそった断面の通過時刻を求め、
		//一番遅い入り時間が一番早い出時間より早かったら交差してる
		auto x1 = (a.minP_.x - b.start_.x) / b.Vec().x;
		auto y1 = (a.minP_.y - b.start_.y) / b.Vec().y;
		auto z1 = (a.minP_.z - b.start_.z) / b.Vec().z;
		auto x2 = (a.maxP_.x - b.start_.x) / b.Vec().x;
		auto y2 = (a.maxP_.y - b.start_.y) / b.Vec().y;
		auto z2 = (a.maxP_.z - b.start_.z) / b.Vec().z;

		auto tminX = std::min(x1, x2);
		auto tminY = std::min(y1, y2);
		auto tminZ = std::min(z1, z2);

		auto tmaxX = std::max(x1, x2);
		auto tmaxY = std::max(y1, y2);
		auto tmaxZ = std::max(z1, z2);

		auto tmin = std::max(std::max(tminX, tminY), tminZ);
		auto tmax = std::min(std::min(tmaxX, tmaxY), tmaxZ);

		if ((tmax - tmin) >= 0) {
			//衝突点が線分外
			if (tmin < 0 || tmin>1) {
				return false;
			}
			outnorm = b.Vec() * tmin;
			return true;
		}
		return false;
	}


	bool Intersect(const OBB& a, const AABB& b, Vector3& outnorm) {
		//分離軸を使う
		auto bsize = (b.maxP_ - b.minP_);
		auto Nbx1 = Vector3(1, 0, 0); auto bx1 = Nbx1 * bsize.x;
		auto Nbx2 = Vector3(0, 1, 0); auto bx2 = Nbx2 * bsize.y;
		auto Nbx3 = Vector3(0, 0, 1); auto bx3 = Nbx3 * bsize.z;

		OBB Bobb;
		Bobb.center_ = (b.maxP_ + b.minP_) / 2;
		Bobb.sizedAxis_[0] = bx1;
		Bobb.sizedAxis_[1] = bx2;
		Bobb.sizedAxis_[2] = bx3;
		return Intersect(a, Bobb, outnorm);
	}

	bool Intersect(const OBB& a, const OBB& b, Vector3& outnorm) {
		//分離軸を使う
		auto Nax1 = a.sizedAxis_[0]; Nax1.Normalize();
		auto ax1 = a.sizedAxis_[0];
		auto Nax2 = a.sizedAxis_[1]; Nax2.Normalize();
		auto ax2 = a.sizedAxis_[1];
		auto Nax3 = a.sizedAxis_[2]; Nax3.Normalize();
		auto ax3 = a.sizedAxis_[2];

		auto Nbx1 = b.sizedAxis_[0]; Nbx1.Normalize();
		auto bx1 = b.sizedAxis_[0];
		auto Nbx2 = b.sizedAxis_[1]; Nbx2.Normalize();
		auto bx2 = b.sizedAxis_[1];
		auto Nbx3 = b.sizedAxis_[2]; Nbx3.Normalize();
		auto bx3 = b.sizedAxis_[2];

		//中心間ベクトル。aが遠ざかる向き
		auto abC = (a.center_ - b.center_) * 2;
		//脱出ベクトルの候補
		std::vector<Vector3> outVecs;

		//ax1
		float La = ax1.Magnitude();
		float Lb = LenShadowOnSeparateAxis(Nax1, bx1, bx2, bx3);
		float L = std::abs(Dot(abC, Nax1));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nax1) > 0) ? Nax1 : -Nax1;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}
		//ax2
		La = ax2.Magnitude();
		Lb = LenShadowOnSeparateAxis(Nax2, bx1, bx2, bx3);
		L = std::abs(Dot(abC, Nax2));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nax2) > 0) ? Nax2 : -Nax2;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}
		//ax3
		La = ax3.Magnitude();
		Lb = LenShadowOnSeparateAxis(Nax3, bx1, bx2, bx3);
		L = std::abs(Dot(abC, Nax3));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nax3) > 0) ? Nax3 : -Nax3;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}


		//bx1
		Lb = bx1.Magnitude();
		La = LenShadowOnSeparateAxis(Nbx1, ax1, ax2, ax3);
		L = std::abs(Dot(abC, Nbx1));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nbx1) > 0) ? Nbx1 : -Nbx1;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}
		//bx2
		Lb = bx2.Magnitude();
		La = LenShadowOnSeparateAxis(Nbx2, ax1, ax2, ax3);
		L = std::abs(Dot(abC, Nbx2));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nbx2) > 0) ? Nbx2 : -Nbx2;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}
		//bx3
		Lb = bx3.Magnitude();
		La = LenShadowOnSeparateAxis(Nbx3, ax1, ax2, ax3);
		L = std::abs(Dot(abC, Nbx3));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, Nbx3) > 0) ? Nbx3 : -Nbx3;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//外積の軸についても分離軸を探す
		//C11
		Vector3 C = Cross(Nax1, Nbx1);
		if (C.ApproxZero()) {
			C = Cross(Nax1, Vector3(Nbx1.z, -Nbx1.x, Nbx1.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax2, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx2, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C12
		C = Cross(Nax1, Nbx2);
		if (C.ApproxZero()) {
			C = Cross(Nax1, Vector3(Nbx2.z, -Nbx2.x, Nbx2.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax2, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C13
		C = Cross(Nax1, Nbx3);
		if (C.ApproxZero()) {
			C = Cross(Nax1, Vector3(Nbx3.z, -Nbx3.x, Nbx3.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax2, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx2, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C21
		C = Cross(Nax2, Nbx1);
		if (C.ApproxZero()) {
			C = Cross(Nax2, Vector3(Nbx1.z, -Nbx1.x, Nbx1.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx2, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C22
		C = Cross(Nax2, Nbx2);
		if (C.ApproxZero()) {
			C = Cross(Nax2, Vector3(Nbx2.z, -Nbx2.x, Nbx2.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C23
		C = Cross(Nax2, Nbx3);
		if (C.ApproxZero()) {
			C = Cross(Nax2, Vector3(Nbx3.z, -Nbx3.x, Nbx3.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax3, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx2, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C31
		C = Cross(Nax3, Nbx1);
		if (C.ApproxZero()) {
			C = Cross(Nax3, Vector3(Nbx1.z, -Nbx1.x, Nbx1.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax2, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx2, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C32
		C = Cross(Nax3, Nbx2);
		if (C.ApproxZero()) {
			C = Cross(Nax3, Vector3(Nbx2.z, -Nbx2.x, Nbx2.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax2, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx3, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		//C33
		C = Cross(Nax3, Nbx3);
		if (C.ApproxZero()) {
			C = Cross(Nax3, Vector3(Nbx3.z, -Nbx3.x, Nbx3.y));
		}
		C.Normalize();
		La = LenShadowOnSeparateAxis(C, ax1, ax2, Vector3::Zero());
		Lb = LenShadowOnSeparateAxis(C, bx1, bx2, Vector3::Zero());
		L = std::abs(Dot(abC, C));
		if (L > La + Lb) {
			//分離軸が存在
			return false;
		}
		else {
			auto outVec = (Dot(abC, C) > 0) ? C : -C;
			auto outlength = La + Lb - L;
			//脱出方向候補を追加
			outVecs.emplace_back(outVec * outlength);
		}

		float minMag = FLT_MAX;
		//交差してるので脱出方向を検討
		for (auto ov : outVecs) {
			if (minMag > ov.MagnitudeSqr()) {
				minMag = ov.MagnitudeSqr();
				outnorm = ov / 2.0f;
			}
		}
		return true;
	}

	bool Intersect(const Line& a, const OBB& b, Vector3& outnorm) {
		Vector3 normAxis[3];
		for (int i = 0; i < 3; i++) {
			normAxis[i] = b.sizedAxis_[i];
			normAxis[i].Normalize();
		}
		//OBBを回転させてAABBにする
		XMMATRIX invmat = XMMatrixIdentity();
		//通常軸をobb軸に変換する行列
		invmat.r[0] = XMLoadFloat3(&normAxis[0]);
		invmat.r[1] = XMLoadFloat3(&normAxis[1]);
		invmat.r[2] = XMLoadFloat3(&normAxis[2]);

		XMVECTOR det;
		//obb軸を通常軸に変換する行列
		auto rotmat = XMMatrixInverse(&det, invmat);

		std::array<Vector3, 3>axis;
		for (int i = 0; i < 3; i++) {
			axis[i] = Vector3(XMVector3Transform(
				XMLoadFloat3(&b.sizedAxis_[i]), rotmat
			));
		}
		Vector3 newC = Vector3(XMVector3Transform(
			XMLoadFloat3(&b.center_), rotmat
		));

		Line newA;
		newA.start_ = Vector3(XMVector3Transform(
			XMLoadFloat3(&a.start_), rotmat
		));
		newA.end_ = Vector3(XMVector3Transform(
			XMLoadFloat3(&a.end_), rotmat
		));

		AABB Baabb;
		Baabb.minP_ =
			(newC * 2 - axis[0] - axis[1] - axis[2]) / 2;
		Baabb.maxP_ =
			Baabb.minP_ + axis[0] + axis[1] + axis[2];

		auto result = Intersect(Baabb, newA, outnorm);
		if (result) {
			outnorm *= -1;
			//元の角度に戻す
			outnorm = Vector3(XMVector3Transform(
				XMLoadFloat3(&outnorm), invmat
			));
		}

		return result;
	}


	bool Intersect(const Sphere& a, const OBB& b, Vector3& outnorm) {
		//OBBと点の最短距離をだしてそれと比べる
		//点からOBBに向かう最短の距離ベクトル
		Vector3 len = Vector3::Zero();
		for (int i = 0; i < 3; i++) {
			float l = b.sizedAxis_[i].Magnitude() / 2;
			if (l <= 0)continue;
			auto norm = b.sizedAxis_[i]; norm.Normalize();
			//辺の半分にたいしてそこに落とした中心間ベクトルの影の長さの比
			float d = Dot((a.center_ - b.center_), norm) / l;
			d = std::abs(d);
			if (d > 1) {
				//辺からはみ出した部分のベクトルを出す
				len += norm * (1 - d) * l;
			}
		}

		float length = len.Magnitude();
		if (length <= a.radius_) {
			//交差している
			len /= -length;
			len *= (a.radius_ - length);
			outnorm = len;
			return true;
		}
		else {
			return false;
		}
	}

	//視錘台とAABBの判定
	bool Intersect(const ViewFrustum& vf, const AABB& aabb) {
		//平面ごとに判定する
		for (int i = 0; i < 4; i++) {
			auto& pl = vf.planes_[i];
			//平面の法線方向軸に投影して一番値が大きいものをPositivePoint(pp),
//最も小さいものをNegativePoint(np)とする
			auto pp = aabb.minP_;
			auto np = aabb.minP_;
			auto size = aabb.maxP_ - aabb.minP_;
			if (pl.normal_.x > 0) {
				pp.x += size.x;
			}
			else {
				np.x += size.x;
			}
			if (pl.normal_.y > 0) {
				pp.y += size.y;
			}
			else {
				np.y += size.y;
			}
			if (pl.normal_.z > 0) {
				pp.z += size.z;
			}
			else {
				np.z += size.z;
			}

			//それぞれの点と平面の距離を求める
			float ppd = pl.SignedMinDist(pp);
			float npd = pl.SignedMinDist(np);

			if (ppd < 0) {
				//AABBは完全に平面の裏側
				return false;
			}

		}
		//交差している
		return true;

	}

}





