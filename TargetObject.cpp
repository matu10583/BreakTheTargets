#include "TargetObject.h"
#include "OBBColliderComponent.h"
#include "Collider.h"
#include "MeshComponent.h"
#include "ScoreTextObject.h"
#include "MoveComponent.h"
#include <random>


TargetObject::TargetObject(Object* parent,
	ScoreTextObject* score) :
	Object(parent),
age_(0),
lifespan_(10),
mvType_(MOVE_NONE)
{
	score_ = score;

	OBB obb;
	AABB aabb;
	aabb.minP_ = Vector3(-5, -1, -5);
	aabb.maxP_ = Vector3(5, 0, 5);
	obb.center_ = (aabb.maxP_ + aabb.minP_) / 2;
	auto size = aabb.maxP_ - aabb.minP_;
	obb.sizedAxis_[0] = Vector3(size.x, 0, 0);
	obb.sizedAxis_[1] = Vector3(0, size.y, 0);
	obb.sizedAxis_[2] = Vector3(0, 0, size.z);
	col1_ = new OBBColliderComponent(this, obb, true);
	col1_->SetType(TYPE_TARGET);

	obb.sizedAxis_[0] *= 2.0f/3.0f;
	obb.sizedAxis_[2] *= 2.0f/3.0f;
	col2_ = new OBBColliderComponent(this, obb, true);
	col2_->SetType(TYPE_TARGET);

	obb.sizedAxis_[0] *= 1.0f/2.0f;
	obb.sizedAxis_[2] *= 1.0f/2.0f;
	col3_ = new OBBColliderComponent(this, obb, true);
	col3_->SetType(TYPE_TARGET);

	std::vector<VertexData> vd;
	std::vector<Index> id;
	std::vector<Material> mats;
	aabb.GetModel(vd, id);
	Material mat(id.size() * 3, Vector3(0.5, 0.5, 0.5), 1.0f);
	mat.additional.texPath = "target.jpg";
	mats.emplace_back(mat);
	model_ = new MeshComponent(this, vd, id, mats, "Target");

	move_ = new MoveComponent(this);
	std::random_device rnd;
	int r = static_cast<int>(rnd());
	r = std::abs(r);
	int x = (r % 20) - 10;
	r = static_cast<int>(rnd());
	r = std::abs(r);
	int y = (r % 20) - 10;
	r = static_cast<int>(rnd());
	r = std::abs(r);
	int z = (r % 20) - 10;
	move_->SetVelocity(x, y, z);

}

void 
TargetObject::Reset() {
	age_ = 0;

	std::random_device rnd;
	int r = static_cast<int>(rnd());
	r = std::abs(r);
	int x = (r % 20) - 10;
	r = static_cast<int>(rnd());
	r = std::abs(r);
	int y = (r % 20) - 10;
	r = static_cast<int>(rnd());
	r = std::abs(r);
	int z = (r % 20) - 10;
	move_->SetVelocity(x, y, z);
}

void
TargetObject::UpdateObject(float deltaTime) {
	age_ += dir_ * deltaTime;


	auto pos = Vector3::Zero();
	switch (mvType_)
	{
	case TargetObject::MOVE_NONE:
		move_->SetVelocity(0, 0, 0);//動かない
		return;
	case TargetObject::MOVE_RANDOM:
		if (age_ > lifespan_ || age_ < 0) {
			SetState(InActive);
		}
		return;//何もしない
		break;
	case TargetObject::MOVE_BEZIERLINE:
		if (age_ > lifespan_ || age_ < 0) {
			dir_ *= -1;//往復させる
			age_ = (age_ < 0) ? 0 : lifespan_;
		}
		move_->SetVelocity(0, 0, 0);//速さで管理しない
		pos = GetPosFromBecier(age_);
		break;
	case TargetObject::MOVE_CIRCLE:
		move_->SetVelocity(0, 0, 0);
		pos = GetPosFromCircle(age_);
		break;
	default:
		break;
	}

	SetWorldPos(pos.x, pos.y, pos.z);
}

void 
TargetObject::OnCollision(const CollisionInfo& ci) {
	auto& self = ci.collider[0];
	auto& other = ci.collider[1];

	if (other->GetType() & TYPE_BULLET) {
		//銃弾に当たった
		//スコアを上げる
		score_->AddScore(1000);
	}
}


const Vector3 
TargetObject::GetPosFromBecier(float count) {
	float t = count / lifespan_;
	float invt = 1 - t;
	auto st = bzInfo_.start;
	auto pos = st * invt * invt +
		bzInfo_.control * 2 * invt * t +
		bzInfo_.end * t * t;
	return pos;
}
const Vector3 
TargetObject::GetPosFromCircle(float count) {
	float t = count / lifespan_;
	float ang = XM_2PI * t;
	auto rad = cInfo_.Rad();
	auto cen = cInfo_.center;

	//中心0の時の座標を求める
	auto y = rad.y * std::sinf(ang);
	auto x = rad.x * std::cosf(ang);//仮のx座標
	auto z = x * std::sinf(cInfo_.pitch);
	x = x * std::cosf(cInfo_.pitch);

	auto pos = Vector3(x, y, z) + cen;
	return pos;
}

void
TargetObject::SetAdditionalDataFromJson(const nlohmann::json& additional) {
	SetSpan(additional["Span"].get<float>());
	if (additional.count("BezierInfo") != 0) {//ベジェ曲線設定
		auto& bzi = additional["BezierInfo"];
		TargetObject::BezierInfo bz;
		bz.start = Vector3(
			bzi["Start"][0].get<float>(),
			bzi["Start"][1].get<float>(),
			bzi["Start"][2].get<float>()
		);
		bz.control = Vector3(
			bzi["Control"][0].get<float>(),
			bzi["Control"][1].get<float>(),
			bzi["Control"][2].get<float>()
		);
		bz.end = Vector3(
			bzi["End"][0].get<float>(),
			bzi["End"][1].get<float>(),
			bzi["End"][2].get<float>()
		);
		SetMoveType(TargetObject::MOVE_BEZIERLINE);
		SetBezierInfo(bz);
	}
	else if (additional.count("CircleInfo") != 0) {//サークル情報設定
		auto& ci = additional["CircleInfo"];
		TargetObject::CircleInfo c;
		c.center = Vector3(
			ci["Center"][0].get<float>(),
			ci["Center"][1].get<float>(),
			ci["Center"][2].get<float>()
		);
		c.SetRadius(Vector2(
			ci["Radius"][0].get<float>(),
			ci["Radius"][1].get<float>())
		);
		c.pitch = ci["Pitch"].get<float>();
		SetMoveType(TargetObject::MOVE_CIRCLE);
		SetCircleInfo(c);
	}
}
void
TargetObject::GetAdditionalJsonFromObj(nlohmann::json& jsdat) {
	auto bz = GetBezierInfo();
	auto ci = GetCircleInfo();

	jsdat["Additional"]["Span"] = GetSpan();
	switch (GetMoveType())
	{
	case TargetObject::MOVE_BEZIERLINE:
		jsdat["Additional"]["BezierInfo"] = nlohmann::json({
			{ "Start", {bz.start.x, bz.start.y, bz.start.z} },
			{ "Control", {bz.control.x, bz.control.y, bz.control.z} },
			{ "End", {bz.end.x, bz.end.y, bz.end.z} }
			}
		);
		break;
	case TargetObject::MOVE_CIRCLE:
		jsdat["Additional"]["CircleInfo"] = nlohmann::json({
				{"Center", {ci.center.x, ci.center.y, ci.center.z}},
				{"Radius", {ci.Rad().x, ci.Rad().y}},
				{"Pitch", ci.pitch}
			}
		);
		break;
	default:
		break;
	}
}