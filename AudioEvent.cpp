#include "AudioEvent.h"
#include "AudioSystem.h"

AudioEvent::AudioEvent(const char* fileName,
	const unsigned int aeFlag):
volume_(1)
{
	audio_ = AudioSystem::Instance();
	eventID_ = audio_->CreateSourceVoice(fileName, aeFlag);
}

AudioEvent::~AudioEvent() {
	//�\�[�X�{�C�X�̍폜
	audio_->DestroySourceVoice(this);
}

void
AudioEvent::Play() {
	audio_->Play(this);
}

void
AudioEvent::Stop() {
	audio_->Stop(this, false);
}

void
AudioEvent::Pause() {
	audio_->Stop(this, true);
}

void
AudioEvent::SetVolume(float vol) {
	volume_ = vol;//getvolume�̂��߂ɋL�����Ă���
	audio_->SetVolume(this);
}

void
AudioEvent::SetLoop(bool loop) {
	audio_->SetLoop(this, loop);
}

bool
AudioEvent::IsPlaying()const {
	return audio_->IsPlaying(this);
}


void 
AudioEvent::UpdateEmitter(float deltaTime, class Object* obj){
	audio_->UpdateEmitter(this, deltaTime, obj);
}