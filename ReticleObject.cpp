#include "ReticleObject.h"
#include "SpriteComponent.h"
#include "Game.h"

ReticleObject::ReticleObject(Object* parent):
Object(parent)
{
	reticleSprite_ = new SpriteComponent(this, "img/reticle.png");
	auto size = reticleSprite_->GetImgSize();
	reticleSprite_->SetOffset(size / 2);
}

void 
ReticleObject::UpdateObject(float deltatime) {
	reticleSprite_->SetScaleRatioToWindowSizeX(0.05);
	reticleSprite_->SetPosRatioToWindowSize(Vector3(
		0.5f, 0.5f, 0.1f));
}