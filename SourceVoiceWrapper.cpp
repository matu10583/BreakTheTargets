#include "SourceVoiceWrapper.h"
#include "AudioSystem.h"
#include "WaveFile.h"
#include "Object.h"
#include <cassert>
#include <xaudio2fx.h>
#include "AudioComponent.h"
#ifdef _DEBUG
#include "Dx12Wrapper.h"
#endif // _DEBUG


#pragma comment(lib,"xaudio2.lib")


SourceVoiceWrapper::SourceVoiceWrapper():
wavFile_(nullptr),
isPlaying_(false),
isLoop_(false),
preEmitterPos_(0, 0, 0)
{
	emitterCone_ = X3DAudioDefault_DirectionalCone;
	emitter_.pCone = nullptr;
	emitter_.ChannelCount = 1;//�V���O���G�~�b�^�[
	emitter_.CurveDistanceScaler = 60;//�����̒P�ʂ����킹�邽�߂̌W���炵��
}

SourceVoiceWrapper::~SourceVoiceWrapper() {
	sourceVoice_->DestroyVoice();
}

bool 
SourceVoiceWrapper::Init(
	IXAudio2* xaudio2, const WaveFile* file, unsigned int classFlag) {
	audioClassFlag_ = classFlag;
	wavFile_ = file;
	for (int i = 0; i < 2; i++) {
		//��b���̃T�C�Y��p��
		buffers_[i].resize(wavFile_->Info().fmt.Format.nAvgBytesPerSec * 1);
	}


	auto audio = AudioSystem::Instance();
	UINT32 flags = 0;
	if (classFlag & 
		(AUDIO_FLAG_USEFILTER | AUDIO_FLAG_SE)) {
		flags |= XAUDIO2_VOICE_USEFILTER;
	}

	auto result = xaudio2->CreateSourceVoice(
		&sourceVoice_, &wavFile_->Info().fmt.Format,
		flags);

	if (FAILED(result)) {
		return false;
	}
	AddNextBuffer();

	return true;
}

HRESULT 
SourceVoiceWrapper::Start(UINT32 flags) {
	auto result = sourceVoice_->Start(flags);
	isPlaying_ = true;//�Đ������Ƃ�
	doPlay_ = true;
	return result;
}
HRESULT
SourceVoiceWrapper::Stop(bool pause) {
	//�|�[�Y�Ȃ�t���O��ς���.
	UINT32 flags = 0;
	if (pause) {
		flags |= XAUDIO2_PLAY_TAILS;
	}
	else {//�X�g�b�v�Ȃ�ŏ��̈ʒu�Ɏ����Ă�
		cursor_ = 0;
		//�o�b�t�@�[�̃��Z�b�g
		sourceVoice_->FlushSourceBuffers();
	}
	auto result = sourceVoice_->Stop(flags);
	doPlay_ = false;
	isPlaying_ = false;//�~�߂�Ƃ�
	return result;
}

void 
SourceVoiceWrapper::SetVolume(float vol) {
	sourceVoice_->SetVolume(vol);
}

const WaveInfo&
SourceVoiceWrapper::GetWAVInfo()const {
	return wavFile_->Info();
}

//�Z�J���_���ɑ����̃f�[�^���������݁A�\�[�X�{�C�X�ɒǉ�
void
SourceVoiceWrapper::AddNextBuffer() {
	//���ۂɓǂ񂾗�
	DWORD readBytes = 0;
	//�ړI�̃f�[�^��ǂݍ���
	auto& buff = buffers_[secondary];
	if (!(wavFile_->GetWAVData(
		buff.data(), buff.size(), cursor_, &readBytes))) {
		assert(0);
		//�J�[�\���ʒu���o�O���Ă�̂łƂ肠�����ŏ��ɖ߂�
		cursor_ = 0;
	}
	//�J�[�\����i�߂�
	cursor_ += readBytes;

	//source voice�Ƀo�b�t�@�𑗐M
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = readBytes;
	buffer.pAudioData = buff.data();

	if (wavFile_->GetSize() <= cursor_) {//�Ō�܂œǂݏI�����
		buffer.Flags = XAUDIO2_END_OF_STREAM;
	}
	//���M
	auto result = sourceVoice_->SubmitSourceBuffer(&buffer);
	if (FAILED(result)) {
		assert(0);
	}

	//primary��secondary�����ւ���
	std::swap(primary, secondary);
}

void
SourceVoiceWrapper::UpdateSourceVoice() {
	//��Ԃ��擾
	XAUDIO2_VOICE_STATE state;
	sourceVoice_->GetState(&state);

	if (doPlay_) {
		if (state.BuffersQueued < 2) {
			if (cursor_ >= wavFile_->GetSize()) {
				if (isLoop_) {//���[�v���ǂ���
					cursor_ = 0;//�ŏ��ɖ߂�
				}
				else {
					//�Đ�����߂�
					doPlay_ = false;
					cursor_ = 0;
				}
			}
			else {
				AddNextBuffer();
				state.BuffersQueued += 1;//���ǉ�����
			}
		}
	}
	if (state.BuffersQueued == 0) {
		sourceVoice_->GetState(&state);
		isPlaying_ = false;//�Đ�������Ă�
	}
}

void
SourceVoiceWrapper::UpdateEmitter(float deltaTime, Object* obj) {
	preEmitterPos_ = emitter_.Position;

	emitter_.OrientFront = obj->GetForward();
	emitter_.OrientTop = obj->GetUp();
	emitter_.Position = obj->GetWorldPos();
	emitter_.Velocity = (Vector3(emitter_.Position) - preEmitterPos_) / deltaTime;


}

void
SourceVoiceWrapper::ApplyDSPSetting(const X3DAUDIO_DSP_SETTINGS& dsp) {
	if (audioClassFlag_ & AUDIO_FLAG_USEREVERB) {
		auto inoutCh = dsp.SrcChannelCount * dsp.DstChannelCount;
		//���o�[�u���g���Ȃ��ڂ̐ݒ��
		std::vector<FLOAT32> wetMatCoefficent(
			inoutCh);//���o�[�u�̉���
		//dry�̃{�����[��������ɍ��킹�ĕς���
		auto dryVol = 1.0f - dsp.ReverbLevel;
		std::copy(&dsp.pMatrixCoefficients[0], &dsp.pMatrixCoefficients[inoutCh],
			wetMatCoefficent.data());
		for (unsigned int i = 0; i < dsp.DstChannelCount * dsp.SrcChannelCount; i++) {
			dsp.pMatrixCoefficients[i] *= dryVol;
			wetMatCoefficent[i] *= dsp.ReverbLevel;
		}

		sourceVoice_->SetOutputMatrix(
			sendList_.subDst[0], dsp.SrcChannelCount,
			dsp.DstChannelCount, &wetMatCoefficent[0]);
	}
	//�Ƃ肠������ڂ̃|�C���^��dry�̂͂�
	sourceVoice_->SetOutputMatrix(
		sendList_.mainDst, dsp.SrcChannelCount,
		dsp.DstChannelCount, dsp.pMatrixCoefficients);
	sourceVoice_->SetFrequencyRatio(dsp.DopplerFactor);
	
	XAUDIO2_FILTER_PARAMETERS FilterParameters = { LowPassFilter, 
		2.0f * sinf(X3DAUDIO_PI / 6.0f * dsp.LPFReverbCoefficient), 1.0f };
	sourceVoice_->SetFilterParameters(&FilterParameters);
}

//�����������֏o�͂��邩��ۑ�����
void 
SourceVoiceWrapper::SetOutputVoice(XAUDIO2_VOICE_SENDS* pSendsList) {
	sendList_.mainDst = pSendsList->pSends->pOutputVoice;
	for (unsigned int i = 1; i < pSendsList->SendCount; i++) {
		sendList_.subDst.push_back(
			pSendsList->pSends[i].pOutputVoice);
	}
	sourceVoice_->SetOutputVoices(pSendsList);
}

bool 
SourceVoiceWrapper::IsPlaying()const{

	XAUDIO2_VOICE_STATE state;
	sourceVoice_->GetState(&state);
	return isPlaying_;
}