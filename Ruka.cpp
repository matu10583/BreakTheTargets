#include "Ruka.h"
#include "MeshComponent.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "CapsuleColliderComponent.h"
#include "Collider.h"
#include "AudioComponent.h"
//デバッグ用モデル。Releaseでは動かさない
Ruka::Ruka(class Object* parent) :
	Object(parent)
{
	auto model = new MeshComponent(this, "Model/巡音ルカ.pmd");
	model->SetIsLeftHanded(false);

	Capsule cp;
	cp.line_.start_ = Vector3(0, 3, 0);
	cp.line_.end_ = Vector3(0, 17, 0);
	cp.radius_ = 3;
	//auto ccol = new CapsuleColliderComponent(this, cp);

	OBB obb;
	obb.center_ = Vector3(0, 10, 0);
	obb.sizedAxis_[0] = Vector3(4, 0, 0);
	obb.sizedAxis_[1] = Vector3(0, 20, 0);
	obb.sizedAxis_[2] = Vector3(0, 0, 4);
	auto ccol = new OBBColliderComponent(this, obb);

	ccol->SetType(TYPE_TARGET);
	//モデルをもらってくる
	std::vector<VertexData> vd;
	std::vector<Index> id;
	std::vector<Material> materials;
	ccol->GetLocalAABB().GetModel(vd, id);
	materials.emplace_back(Material(id.size()*3, Vector3(0, 0, 0), 0.6f));
	auto aabbmodel = new MeshComponent(this, vd, id, materials);

	//音（テスト用）
	unsigned int aeFlag = (AUDIO_FLAG_USEREVERB | AUDIO_FLAG_USEFILTER |
		AUDIO_FLAG_SE);
	ac_ = new AudioComponent(this, "sound/MusicSurround.wav", aeFlag);
	//ac_->Play();
	ac_->SetLoop(true);
}

void
Ruka::UpdateObject(float deltaTime) {
	count_ += deltaTime;
	if (count_ > 3) {
		//ac_->Stop();
	}
	if (count_ > 3) {
		//ac_->Play();
		count_ = 0;
	}
}