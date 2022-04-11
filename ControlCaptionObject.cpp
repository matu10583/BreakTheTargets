#include "ControlCaptionObject.h"
#include "TextSpriteComponent.h"
#include "InputSystem.h"

ControlCaptionObject::ControlCaptionObject(Object* parent) :
	Object(parent) {
	auto posrat = Vector3(0.95f, 0, 0);

	caps1_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	caps1_->SetPosRatioToWindowSize(posrat);
	caps1_->SetScaleRatioToWindowSizeX(0.015f);
	caps1_->SetStr(
		"Move: WASD\n"
		"EyeMove: Mouse\n"
		"HipShoot: HoldRButton\n"
	);
	caps1_->SetAllign(ALLIGN_RIGHT);

	posrat.y += 0.14f;
	caps2_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	caps2_->SetPosRatioToWindowSize(posrat);
	caps2_->SetScaleRatioToWindowSizeX(0.015f);
	caps2_->SetStr(
		"FPS: PressRButton\n"
		"Shoot: LButton\n"
		"Jump: Space\n"
	);
	caps2_->SetAllign(ALLIGN_RIGHT);

	posrat.y += 0.14f;
	caps3_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	caps3_->SetPosRatioToWindowSize(posrat);
	caps3_->SetScaleRatioToWindowSizeX(0.015f);
	caps3_->SetStr(
		"Pause: Esc\n"
		"DisplayCaption: Shift"
	);
	caps3_->SetAllign(ALLIGN_RIGHT);
}
