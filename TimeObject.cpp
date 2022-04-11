#include "TimeObject.h"
#include "GameSceneObject.h"
#include "Game.h"

TimeObject::TimeObject(Object* owner, int stageMode):
Object(owner),
stageMode_(stageMode)
{
	SetWorldPos(0, 0, 0);

	nowTimeImg_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	nowTimeImg_->SetStr(" ");
	nowTimeImg_->SetScale(Vector2(0.5f, 0.5f));
}

void
TimeObject::UpdateObject(float deltaTime) {
	if (stageMode_ == GameSceneObject::TIMEATTACK) {
		nowTime_ += deltaTime;
	}else if (stageMode_ == GameSceneObject::SCOREATTACK) {
		nowTime_ -= deltaTime;
	}
	nowTimeImg_->SetStr(std::to_string(nowTime_).c_str());
}