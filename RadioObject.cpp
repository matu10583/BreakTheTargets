#include "RadioObject.h"
#include "AudioComponent.h"
#include "MeshComponent.h"
#include "OBBColliderComponent.h"
#include "Collider.h"

RadioObject::RadioObject(Object* parent):
Object(parent)
{
	//BGM
	unsigned int flags = AUDIO_FLAG_SE | AUDIO_FLAG_USEREVERB;
	bgm_ = new AudioComponent(this, "sound/魔王魂 ループ BGM ネオロック44.wav", flags);
	bgm_->SetVolume(0.25f);
	bgm_->Play();
	bgm_->SetLoop(true);

	//model
	model_ = new MeshComponent(this, "Model/モノラルラジカセ.pmd");
	//判定を作製
	AABB aabb = model_->GetLocalAABB();
	
	//obb
	OBB obb;
	auto x = aabb.maxP_ - aabb.minP_;
	obb.center_ = (aabb.maxP_ + aabb.minP_) / 2;
	obb.sizedAxis_[0] = Vector3(x.x, 0, 0);
	obb.sizedAxis_[1] = Vector3(0, x.y, 0);
	obb.sizedAxis_[2] = Vector3(0, 0, x.z);

	obb_ = new OBBColliderComponent(this, obb);
	obb_->SetType(TYPE_TARGET | TYPE_GROUND);
}