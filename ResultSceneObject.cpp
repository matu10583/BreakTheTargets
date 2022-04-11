#include "ResultSceneObject.h"
#include "GameSceneObject.h"
#include "TitleSceneObject.h"
#include "TextSpriteComponent.h"
#include "InputSystem.h"
#include "AudioComponent.h"
#include "AudioSystem.h"
#include <sstream>
#include <fstream>
#include <algorithm>


ResultSceneObject::ResultSceneObject(Game* game, float score,
	int stageMode, const char* rankDat) :
	SceneObject(game),
	score_(score),
	transitFlag_(false)
{
	//基準の座標の設定
	SetWorldPos(0, 0, 0);
	SetWorldRot(0, 0, 0);

	//ゲームオブジェクトの配置
	Init(rankDat, stageMode);


}


SceneObject*
ResultSceneObject::UpdateScene(float deltaTime) {
	//デフォルトはこのシーン
	SceneObject* nextScene = this;
	if (transitFlag_) {
		if (applause_->IsPlaying()) {
			applause_->Stop();
		}
		else {
			nextScene = new TitleSceneObject(game_);
		}
	}

	return nextScene;
}

SceneObject*
ResultSceneObject::SceneInput(const struct InputState& inputState) {
	//デフォルトはこのシーン
	SceneObject* nextScene = this;
	if (inputState.kb.GetButtonState(VK_RETURN) == ButtonState::PRESSED) {
		transitFlag_ = true;
		applause_->Stop();
	}

	return nextScene;
}

void 
ResultSceneObject::Init(const char* rankDat, int stageMode) {
	//コンポーネント
	thx_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	thx_->SetPosRatioToWindowSize(Vector3(
		0.2f, 0.2f, 0
	));
	thx_->SetScaleRatioToWindowSizeX(0.03f);
	scoreTex_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	scoreTex_->SetPosRatioToWindowSize(Vector3(
		0.2f, 0.3f, 0
	));
	scoreTex_->SetScaleRatioToWindowSizeX(0.03f);

	rankTex_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	rankTex_->SetPosRatioToWindowSize(Vector3(
		0.2f, 0.4f, 0
	));
	rankTex_->SetScaleRatioToWindowSizeX(0.015f);

	//ファイルの読み込み
	FILE* rankF;
	fopen_s(&rankF, rankDat, "r+b");
	if (rankF == nullptr) {
		assert(0);
		fclose(rankF);
		return;
	}
	ranks_.resize(5);
	std::fill(ranks_.begin(), ranks_.end(), 0);

	fread(ranks_.data(), sizeof(float), 5, rankF);

	//ランキングの変動

	if (stageMode == GameSceneObject::TIMEATTACK) {
		for (auto& rk : ranks_) {//0はありえないので無効な値に変えておく
			rk = (rk < FLT_EPSILON) ? 9999.99 : rk;
		}
		if (score_ > 0) {
			ranks_.emplace_back(score_);
			std::sort(ranks_.begin(), ranks_.end(),
				std::less<float>());
			ranks_.erase(ranks_.end() - 1);
		}
	}
	else if (stageMode == GameSceneObject::SCOREATTACK) {
		ranks_.emplace_back(score_);
		std::sort(ranks_.begin(), ranks_.end(),
			std::greater<float>());
		ranks_.erase(ranks_.end() - 1);
	}

	fseek(rankF, 0, SEEK_SET);
	fwrite(ranks_.data(), sizeof(float), 5, rankF);
	//ファイルを閉じる
	fclose(rankF);

	return_ = new TextSpriteComponent(this, "img/k_gosicfont.png");
	return_->SetPosRatioToWindowSize(Vector3(
		0.1f, 0.8f, 0
	));
	return_->SetScaleRatioToWindowSizeX(0.03f);

	//SetStr
	std::stringstream sstream;
	thx_->SetStr("Finish!!");

	if (stageMode == GameSceneObject::TIMEATTACK) {
		if (score_ > 0) {
			sstream << "RemainTime: " << score_;
		}
		else {
			sstream << "RemainTime: " << "None";
		}
	}
	else if (stageMode == GameSceneObject::SCOREATTACK) {
		sstream << "Score: " << score_;
	}

	scoreTex_->SetStr(sstream.str().c_str());

	sstream.str("");
	sstream.clear();
	for (int i = 0; i < 5; i++) {
		sstream << i + 1 << " :"  << 
			std::fixed << std::setprecision(2) << ranks_[i] << '\n';
	}
	rankTex_->SetStr(sstream.str().c_str());

	return_->SetStr("Press ENTER To Return Title");

	//bgm
	applause_ = new AudioComponent(this, "sound/スタジアムの拍手.wav",
		AUDIO_FLAG_BGM);
	AudioSystem::Instance()->SetBGMVolume(0.3f);
	applause_->SetLoop(false);
	applause_->Play();
}