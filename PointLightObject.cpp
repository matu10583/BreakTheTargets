#include "PointLightObject.h"
#include "PointLightComponent.h"
#include "Collider.h"
#include "MeshComponent.h"
#include "json.hpp"

PointLightObject::PointLightObject(
	Object* parent, const Vector3& col, float range, float rad):
Object(parent)
{
	plcomp_ = new PointLightComponent(this);
	plcomp_->SetColor(col);
	plcomp_->SetRange(range);
	plcomp_->SetRadius(rad);
	auto sc = GetLocalScale();
	plcomp_->SetScale(sc.x);
	//点光源のモデルを取得
	Sphere sp;
	sp.radius_ = sc.x;
	std::vector<VertexData> vd;
	std::vector<Index> id;
	std::vector<Material> mats;
	sp.GetModel(vd, id);
	auto idSize = id.size() * 3;
	mats.emplace_back(
		idSize, col, 1.0f
	);
	//オンにすると光の形がわかるよ
	//model_ = new MeshComponent(this, vd, id, mats);
}

void 
PointLightObject::UpdateObject(float deltaTime) {
}

void
PointLightObject::SetLightParam(const Vector3& col, float range, float rad) {
	plcomp_->SetColor(col);
	plcomp_->SetRange(range);
	plcomp_->SetRadius(rad);
}

const Vector3& 
PointLightObject::GetColor()const {
	return plcomp_->GetColor();
}
float 
PointLightObject::GetRadius()const {
	return plcomp_->GetRadius();
}

float 
PointLightObject::GetRange()const {
	return plcomp_->GetRange();
}

void 
PointLightObject::SetAdditionalDataFromJson(const nlohmann::json& additional) {
	Vector3 col = Vector3::Zero();
	float range = 0;
	float rad = 0;
	col = Vector3(
		additional["Color"][0].get<float>(),
		additional["Color"][1].get<float>(),
		additional["Color"][2].get<float>()
	);
	range = additional["Range"].get<float>();
	rad = additional["Radius"].get<float>();

	SetLightParam(col, range, rad);
}
void 
PointLightObject::GetAdditionalJsonFromObj(nlohmann::json& jsdat) {
	auto col = GetColor();
	jsdat["Additional"] = {
		{"Color", {col.x, col.y, col.z}},
		{"Range", GetRange()},
		{"Radius", GetRadius()}
	};
}