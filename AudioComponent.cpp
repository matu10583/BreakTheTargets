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
	//���݂̉��ʂɍ��킹��
	fdDst_ = audioEvent_.GetVolume();
}

void
AudioComponent::Play(float time) {
	if (!audioEvent_.IsPlaying()) {//�~�܂��Ă���
		stopFlag_ = false;//��~�����͏I���
		//��x�O�ɂ��Ă���ŏ��̃{�����[���܂Ńt�F�[�h�C��������
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
	//�t�F�[�h�A�E�g������
	if (!stopFlag_) {
		SetFade(0, time);
	}
	stopFlag_ = true;
	if (((fdDst_ - vol) * fdVel_) < FLT_EPSILON) {//�t�F�[�h���I�������
		audioEvent_.Stop();
	}
}

void
AudioComponent::SetFade(float dst, float time) {
	if (stopFlag_) {
		return;//���������~�܂�̂Ƀt�F�[�h��ς����
	}
	fdDst_ = dst;
	//�f�V�x���ɕϊ�
	float voldb = audioEvent_.GetVolume();
	voldb = GetDBFromAmpRatio(voldb);
	float dstdb = GetDBFromAmpRatio(fdDst_);
	//�ǂꂭ�炢����Ă邩
	auto delta = dstdb - voldb;
	fdVel_ = delta / time;
}

void 
AudioComponent::Update(float deltaTime) {
	//�t�F�[�h����
	float vol = volume_;
	float voldb = GetDBFromAmpRatio(vol);
	if (((fdDst_ - vol) * fdVel_) > 0) {//�������Ȃ�܂����ǂ蒅���ĂȂ�
		voldb += fdVel_ * deltaTime;
		vol = GetAmpRatioFromDB(voldb);
	}
	if(((fdDst_ - vol) * fdVel_) <= 0) {//���ǂ蒅�����̂�fade�I��
		vol = fdDst_;
		fdVel_ = 0;
	}
	audioEvent_.SetVolume(vol);
	volume_ = vol;

	//emitter�̍X�V
	audioEvent_.UpdateEmitter(deltaTime, owner_);
}