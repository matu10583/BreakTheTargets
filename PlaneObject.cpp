#include "PlaneObject.h"
#include "MeshComponent.h"
#include "AABBColliderComponent.h"
#include "Collider.h"
#include "XMFLOAT_Helper.h"
PlaneObject::PlaneObject(class Object* parent,
	const Vector3& minP, const Vector3& maxP) :
	Object(parent, false),
	aabbModel_(nullptr)
{
	AABB aabb;
	aabb.minP_ = minP;
	aabb.maxP_ = maxP;
	aabbCol_ = new AABBColliderComponent(this, aabb);
	aabbCol_->SetType(TYPE_GROUND);
}

void
PlaneObject::UpdateObject(float deltaTime) {

}

void
PlaneObject::SetType(ColliderType t) {
	aabbCol_->SetType(t);
}

void
PlaneObject::AddMaterialInfo(const char* texPath,
	const char* normalPath,
	const char* aoPath) {
	Material mat(2 * 3, Vector3(1, 1, 1), 1.0f);
	mat.additional.texPath = texPath;
	mat.additional.normalPath = normalPath;
	mat.additional.aoPath = aoPath;
	material_.emplace_back(mat);
}

void
PlaneObject::CreateModel() {
	//モデルをもらう
	std::vector<VertexData> vd;
	std::vector<Index> id;
	aabbCol_->GetLocalAABB().GetPlaneModel(vd, id);
	//テクスチャを繰り返すのでuvを大きくとる
	for (auto& v : vd) {
		auto r = static_cast<int>(repeatNum_);
		v.uv *= r;
	}
	if (aabbCol_->GetType() == TYPE_WALL) {
		aabbModel_ = new MeshComponent(this, vd, id, material_, "Wall");
	}
	else if (aabbCol_->GetType() == TYPE_GROUND) {
		aabbModel_ = new MeshComponent(this, vd, id, material_, "Ground");
	}

}