#include "RemainNumObject.h"
#include "TextSpriteComponent.h"
#include "PlayerObject.h"
#include "Game.h"
#include <sstream>

RemainNumObject::RemainNumObject(Object* parent, int mode) :
	Object(parent),
	numMode_(mode)
{
	text_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	text_->SetAllign(ALLIGN_RIGHT);

}

void
RemainNumObject::UpdateObject(float deltaTime) {
	text_->SetScaleRatioToWindowSizeX(0.02f);
	text_->SetPosRatioToWindowSize(Vector3(
		0.95f, 0.9f, 0));

	std::stringstream ss;
	if (numMode_ == MODE_BULLETNUM) {
		ss << "Bullets: ";//残り弾数
	}
	else if (numMode_ == MODE_TARGETNUM) {
		ss << "Targets: ";//残りターゲット
	}
	ss << remainNum_;
	text_->SetStr(ss.str().c_str());
}