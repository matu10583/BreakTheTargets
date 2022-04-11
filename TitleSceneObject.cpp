#include "TitleSceneObject.h"
#include "GameSceneObject.h"
#include "TextSpriteComponent.h"
#include "InputSystem.h"
#include "AudioComponent.h"
#include "AudioSystem.h"
#include <sstream>
#include <fstream>
#include <algorithm>


TitleSceneObject::TitleSceneObject(Game* game) :
	SceneObject(game)
{
	//基準の座標の設定
	SetWorldPos(0, 0, 0);
	SetWorldRot(0, 0, 0);

	//ゲームオブジェクトの配置
	Init();
	selected_ = GAME_SCOREATTACK;
	
}


SceneObject*
TitleSceneObject::UpdateScene(float deltaTime) {
	//デフォルトはこのシーン
	SceneObject* nextScene = this;

	std::stringstream ss;
	if (selected_ == GAME_SCOREATTACK) {
		ss << ">";
		scAttackTex_->SetScaleRatioToWindowSizeX(0.03f);
	}
	else {
		scAttackTex_->SetScaleRatioToWindowSizeX(0.015f);
	}
	ss << "ScoreAttack";
	scAttackTex_->SetStr(ss.str().c_str());

	int stageEnum = GAME_STAGE1;
	int stageNum = 1;
	for (auto& st : stageTex_) {
		ss.str("");
		ss.clear();
		if (selected_ == stageEnum) {
			ss << ">";
			st->SetScaleRatioToWindowSizeX(0.03f);
		}
		else {
			st->SetScaleRatioToWindowSizeX(0.015f);
		}
		ss << "Stage" << stageNum;
		st->SetStr(ss.str().c_str());
		stageEnum++;
		stageNum++;
	}

	ss.str("");
	ss.clear();
	if (selected_ == EXIT) {
		ss << ">";
		exitTex_->SetScaleRatioToWindowSizeX(0.03f);
	}
	else {
		exitTex_->SetScaleRatioToWindowSizeX(0.015f);
	}
	ss << "Exit";
	exitTex_->SetStr(ss.str().c_str());

	//遷移
	if (transitFlag_) {//bgmがとまったら遷移
		if (!bgm_->IsPlaying()) {
			switch (selected_)
			{
			case TitleSceneObject::GAME_SCOREATTACK:
				nextScene = new GameSceneObject(game_, "stage/stageSA.json");
				break;
			case TitleSceneObject::GAME_STAGE1:
				nextScene = new GameSceneObject(game_, "stage/stage1.json");
				break;
			case TitleSceneObject::EXIT:
				nextScene = nullptr;
				break;
			default:
				assert(0);
				break;
			}
		}
		else {
			bgm_->Stop();
		}
	}

	return nextScene;
}

SceneObject*
TitleSceneObject::SceneInput(const struct InputState& inputState) {
	//デフォルトはこのシーン
	const int selectNum = 3;
	SceneObject* nextScene = this;
	if (inputState.kb.GetButtonState(VK_RETURN) == ButtonState::PRESSED &&
		!transitFlag_) {
		transitFlag_ = true;
		bgm_->Stop();
	}
	else if (inputState.kb.GetButtonState('S') == ButtonState::PRESSED &&
		!transitFlag_) {
		selected_ = (selected_ + 1) % selectNum;
	}
	else if (inputState.kb.GetButtonState('W') == ButtonState::PRESSED &&
		!transitFlag_) {
		selected_ = (selected_ - 1 + selectNum) % selectNum;
	}

	return nextScene;
}

void
TitleSceneObject::Init() {
	//コンポーネント
	titleTex_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	titleTex_->SetPosRatioToWindowSize(Vector3(
		0.2f, 0.2f, 0
	));
	titleTex_->SetScaleRatioToWindowSizeX(0.03f);
	titleTex_->SetStr("BreakTheTargets!!");

	//テキストのy座標位置の画面比
	float yRat = 0.6f;

	scAttackTex_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	scAttackTex_->SetPosRatioToWindowSize(Vector3(
		0.2f, yRat, 0
	));
	scAttackTex_->SetScaleRatioToWindowSizeX(0.015f);

	for (auto& st : stageTex_) {
		yRat += 0.1f;
		st = new TextSpriteComponent(this, "img/k_gosicfont.png");
		st->SetPosRatioToWindowSize(Vector3(
			0.2f, yRat, 0
		));
		st->SetScaleRatioToWindowSizeX(0.015f);
	}

	yRat += 0.1f;
	exitTex_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	exitTex_->SetPosRatioToWindowSize(Vector3(
		0.2f, yRat, 0
	));
	exitTex_->SetScaleRatioToWindowSizeX(0.015f);

	bgm_ = new AudioComponent(this, "sound/魔王魂 ループ アコースティック10.wav",
		AUDIO_FLAG_BGM);
	bgm_->SetLoop(true);
	bgm_->Play();
	AudioSystem::Instance()->SetBGMVolume(0.25f);
}