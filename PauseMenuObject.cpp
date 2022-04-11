#include "PauseMenuObject.h"
#include "InputSystem.h"
#include "TextSpriteComponent.h"
#include <sstream>


PauseMenuObject::PauseMenuObject(Object* parent) :
	Object(parent) {

//テキストの作成
	menu_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	menu_->SetPosRatioToWindowSize(
		Vector3(0.4f, 0.4f, 0)
	);
	menu_->SetScaleRatioToWindowSizeX(0.015f);

	pause_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	pause_->SetPosRatioToWindowSize(
		Vector3(0.4f, 0.2f, 0)
	);
	pause_->SetScaleRatioToWindowSizeX(0.03f);
	pause_->SetStr("PAUSE");

	menuName_[0] = "         ";
	menuName_[1] = "Resume";
	menuName_[2] = "ReStart";
	menuName_[3] = "Title";
	menuName_[4] = "GiveUp";
}

void
PauseMenuObject::UpdateObject(float deltaTime) {

	std::stringstream ss;
	const int windowWidth = menuName_[0].size();
	for (int i = 0; i < 4; i++) {//カーソル表示とメニューの指定
		if (i == selected_) {
			ss << " >";
		}
		else {
			ss << "  ";
		}
		ss << menuName_[i + 1];
		auto padSize = windowWidth -
			menuName_[i + 1].size() - 2;//足りない部分を空白で埋める
		for (unsigned int i = 0; i < padSize; i++) {
			ss << ' ';
		}
		ss << '\n';
	}

	menu_->SetStr(ss.str().c_str());
}

void
PauseMenuObject::ActorInput(const InputState& inputState) {
	int selectNum = 4;
	if (inputState.kb.GetButtonState('S') == ButtonState::PRESSED) {
		selected_ = (selected_ + 1) % selectNum;
	}
	else if (inputState.kb.GetButtonState('W') == ButtonState::PRESSED) {
		selected_ = (selected_ - 1 + selectNum) % selectNum;
	}
}