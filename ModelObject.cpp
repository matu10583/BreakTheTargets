#include "ModelObject.h"
#include "MeshComponent.h"
#include "OBBColliderComponent.h"
#include "Collider.h"

ModelObject::ModelObject(Object* parent) :
	Object(parent),
model_(nullptr),
col_(nullptr),
fileName_("") {

}

void
ModelObject::CreateModel(const char* fileName) {
	if (model_ && col_) {
		delete model_;
		delete col_;
	}
	fileName_ = fileName;
	model_ = new MeshComponent(this, fileName);
	AABB aabb = model_->GetLocalAABB();
	OBB obb;
	auto x = aabb.maxP_ - aabb.minP_;
	obb.center_ = (aabb.maxP_ + aabb.minP_) / 2.0f;
	obb.sizedAxis_[0] = Vector3(x.x, 0, 0);
	obb.sizedAxis_[1] = Vector3(0, x.y, 0);
	obb.sizedAxis_[2] = Vector3(0, 0, x.z);
	col_ = new OBBColliderComponent(this, obb);
	col_->SetType(TYPE_GROUND);
}

void
ModelObject::SetAdditionalDataFromJson(const nlohmann::json& additional) {
	auto fileName = additional["FileName"].get<std::string>();
	CreateModel(fileName.c_str());
}
void
ModelObject::GetAdditionalJsonFromObj(nlohmann::json& jsdat) {
	auto fileName = GetFileName();
	jsdat["Additional"] = {
		{"FileName", fileName}
	};
}