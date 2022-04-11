#include "AudioComponent.h"
#include "MathFunc.h"


AudioComponent::AudioComponent(Object* owner, const char* fileName,
	const unsigned int aeFlag, int updateOrder) :
	Component(owner, updateOrder),
	audioEvent_(fileName, aeFlag),
	fdVel_(0),
	stopFlag_(true),
	canPlay_(true),
	volume_(1.0f),
	startVol_(1.0f)
{
	//現在の音量に合わせる
	fdDst_ = audioEvent_.GetVolume();
}

void
AudioComponent::Play(float time) {
	if (!audioEvent_.IsPlaying()) {//止まってたら
		stopFlag_ = false;//停止処理は終わる
		//一度０にしてから最初のボリュームまでフェードインさせる
		audioEvent_.SetVolume(0);
		SetFade(startVol_, time);
		volume_ = 0;
	}
	if (canPlay_) {
		audioEvent_.Play();
	}
}

void
AudioComponent::Stop(float time) {
	float vol = audioEvent_.GetVolume();
	//フェードアウトさせる
	if (!stopFlag_) {
		SetFade(0, time);
	}
	stopFlag_ = true;
	if (((fdDst_ - vol) * fdVel_) < FLT_EPSILON) {//フェードが終わったら
		audioEvent_.Stop();
	}
}

void
AudioComponent::SetFade(float dst, float time) {
	if (stopFlag_) {
		return;//もうすぐ止まるのにフェードを変えるな
	}
	fdDst_ = dst;
	//デシベルに変換
	float voldb = audioEvent_.GetVolume();
	voldb = GetDBFromAmpRatio(voldb);
	float dstdb = GetDBFromAmpRatio(fdDst_);
	//どれくらい離れてるか
	auto delta = dstdb - voldb;
	fdVel_ = delta / time;
}

void 
AudioComponent::Update(float deltaTime) {
	//フェード処理
	float vol = volume_;
	float voldb = GetDBFromAmpRatio(vol);
	if (((fdDst_ - vol) * fdVel_) > 0) {//同符号ならまだたどり着いてない
		voldb += fdVel_ * deltaTime;
		vol = GetAmpRatioFromDB(voldb);
	}
	if(((fdDst_ - vol) * fdVel_) <= 0) {//たどり着いたのでfade終了
		vol = fdDst_;
		fdVel_ = 0;
	}
	audioEvent_.SetVolume(vol);
	volume_ = vol;

	//emitterの更新
	audioEvent_.UpdateEmitter(deltaTime, owner_);
}