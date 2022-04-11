#include "ScoreTextObject.h"
#include "TextSpriteComponent.h"
#include <sstream>
#include "AudioComponent.h"

ScoreTextObject::ScoreTextObject(Object* parent):
Object(parent),
score_(0),
dispScore_(0)
{
	text_ = new TextSpriteComponent(this, "img/k_gosicfont.png");

	//‰¹
	unsigned int flags = 0;
	flags |= AUDIO_FLAG_BGM;
	hitSound_ = new AudioComponent(this, "sound/ƒVƒ‡ƒbƒg–½’†.wav", flags);
	hitSound_->SetVolume(1.5f);
}

void
ScoreTextObject::UpdateObject(float deltaTime) {
	text_->SetScaleRatioToWindowSizeX(0.02f);
	text_->SetPosRatioToWindowSize(Vector3(
		0, 0.9f, 0));
	if (dispScore_ < score_) {
		dispScore_ += 100;
	}
	else {
		dispScore_ = score_;
	}


	std::stringstream ss;
	ss << "Score: " << dispScore_;
	text_->SetStr(ss.str().c_str());
}

void
ScoreTextObject::AddScore(int sc) {
	score_ += sc;
	hitSound_->Play();
}