#include "AudioSystem.h"
#include "AudioEvent.h"
#include <cassert>
#include <xaudio2fx.h>
#include "WaveFile.h"
#include "SourceVoiceWrapper.h"
#include "Object.h"

#pragma comment(lib,"xaudio2.lib")
AudioSystem* AudioSystem::instance_;

void
AudioSystem::Create() {
	assert(!instance_ && "�V���O���g���ł���I");
	if (!instance_) {
		instance_ = new AudioSystem();
	}
}

void 
AudioSystem::Destroy() {
	delete instance_;
}

AudioSystem*
AudioSystem::Instance() {
	return instance_;
}

AudioSystem::AudioSystem():
nextEID_(0)
{
}

AudioSystem::~AudioSystem() {
	//�}�X�^�[�{�C�X�̔j��
	subMixVoiceReverb_[0]->DestroyVoice();
	subMixVoiceReverb_[1]->DestroyVoice();
	subMixVoiceSE_->DestroyVoice();
	subMixVoiceBGM_->DestroyVoice();
	subMixVoiceFinal_->DestroyVoice();
	masterVoice_->DestroyVoice();
}

bool
AudioSystem::Init() {
	//XAudio2�̍쐻
	UINT32 flags = 0;
	auto result = XAudio2Create(xAudio2_.ReleaseAndGetAddressOf(), flags);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debug = { 0 };
	debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	debug.BreakMask = XAUDIO2_LOG_ERRORS;
	xAudio2_->SetDebugConfiguration(&debug, 0);
#endif // _DEBUG

	//�}�X�^�[�{�C�X�̍쐻
	result = xAudio2_->CreateMasteringVoice(
		&masterVoice_, 2, 0, flags);//2ch�T���E���h
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	XAUDIO2_VOICE_DETAILS details;
	masterVoice_->GetVoiceDetails(&details);

	//X3daudio�̏�����
	DWORD channelMask;
	masterVoice_->GetChannelMask(&channelMask);
	result = X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND,
		x3DInstance_);

	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//���X�i�[�̏�����
	listenerCone_ = X3DAudioDefault_DirectionalCone;
	listener_.pCone = nullptr;

	//�G�t�F�N�g�ނ̍쐻
	//���o�[�u
	IUnknown* pReverb;
	XAudio2CreateReverb(&pReverb);

	//�G�t�F�N�g�f�X�N
	XAUDIO2_EFFECT_DESCRIPTOR revDesc;
	revDesc.InitialState = true;//�ŏ�����L��
	revDesc.OutputChannels = details.InputChannels;
	revDesc.pEffect = pReverb;
	
	//�T�u�~�b�N�X�{�C�X�̍쐻
		//�ŏI�o�͗p
	flags = 0;
	flags |= XAUDIO2_VOICE_USEFILTER;
	result = xAudio2_->CreateSubmixVoice(
		&subMixVoiceFinal_, details.InputChannels, details.InputSampleRate, flags, 2);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	XAUDIO2_SEND_DESCRIPTOR desc = { 0 };
	desc.pOutputVoice = subMixVoiceFinal_;
	desc.Flags = XAUDIO2_SEND_USEFILTER;
	XAUDIO2_VOICE_SENDS sendlist = { 0 };
	sendlist.SendCount = 1;
	sendlist.pSends = &desc;
	
	//���ʉ��p
	result = xAudio2_->CreateSubmixVoice(
		&subMixVoiceSE_, details.InputChannels, details.InputSampleRate,
		0, 1,&sendlist);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//BGM�p
	result = xAudio2_->CreateSubmixVoice(
		&subMixVoiceBGM_, details.InputChannels, details.InputSampleRate,
		0, 1, &sendlist);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//�Z���h���o�[�u�p
		//���o�[�u��������SE�ɑ���
	desc.pOutputVoice = subMixVoiceSE_;

	for (int i = 0; i < 2; i++) {
		result = xAudio2_->CreateSubmixVoice(
			&subMixVoiceReverb_[i], details.InputChannels, details.InputSampleRate,
			0, 0, &sendlist);
		if (FAILED(result)) {
			assert(0);
			return false;
		}
	}

	//�G�t�F�N�g�`�F�C���̍쐬
	XAUDIO2_EFFECT_CHAIN chain3d;
	chain3d.EffectCount = 1;
	chain3d.pEffectDescriptors = &revDesc;
	subMixVoiceReverb_[1]->SetEffectChain(&chain3d);//1��wet
	//���o�[�u�p�����[�^��ݒ�
	XAUDIO2FX_REVERB_I3DL2_PARAMETERS i3dl2revParam =
		XAUDIO2FX_I3DL2_PRESET_STONEROOM;
	XAUDIO2FX_REVERB_PARAMETERS revParam;
	ReverbConvertI3DL2ToNative(&i3dl2revParam, &revParam);
	subMixVoiceReverb_[1]->SetEffectParameters(0, &revParam, sizeof(revParam));

	//�G�t�F�N�g�̉��
	pReverb->Release();

	return true;
}

void
AudioSystem::Update(float deltaTime) {
	//���X�i�[�̍X�V
	UpdateListner(deltaTime);
	for (auto& sv : sourceVoiceMap_) {
		//�\�[�X�{�C�X�̃X�g���[�~���O
		sv.second->UpdateSourceVoice();
		if (sv.second->GetClassFlag() & AUDIO_FLAG_SE &&
			sv.second->IsPlaying()) {
			//3DAudio�̌v�Z
			X3DAUDIO_DSP_SETTINGS dsp = { 0 };

			XAUDIO2_VOICE_DETAILS dstDetail;
			subMixVoiceSE_->GetVoiceDetails(&dstDetail);//�`�����l�����͑S���2
			auto inputChCount = sv.second->GetWAVInfo().fmt.Format.nChannels;
			std::vector<FLOAT32> matrix(
				dstDetail.InputChannels * inputChCount);

			dsp.SrcChannelCount = inputChCount;
			dsp.DstChannelCount = dstDetail.InputChannels;
			dsp.pMatrixCoefficients = matrix.data();

			X3DAudioCalculate(x3DInstance_, &listener_,
				&sv.second->GetEmitter(),
				X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER |
				X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB |
				X3DAUDIO_CALCULATE_REVERB,
				&dsp);
			//���m�����ȊO�̓��͂��󂯎��ƂȂ�����ڂ̓��͈ȊO��������̂�
			//�����K�p������
			
			if (sv.second->GetWAVInfo().fmt.dwChannelMask == 0) {
				for (int i = dstDetail.InputChannels - 1; i >= 0; i--) {
					auto k = i * inputChCount;
					for (int j = 0; j < inputChCount; j++) {
						dsp.pMatrixCoefficients[i + j * dstDetail.InputChannels] =
							dsp.pMatrixCoefficients[k];
					}
				}
			}

				

			sv.second->ApplyDSPSetting(dsp);
		}
	}
}

unsigned int 
AudioSystem::CreateSourceVoice(const char* fileName,
	const UINT32 aeFlag) {
	//�Ԃ�EventID���L�^
	unsigned int retID = nextEID_;
	nextEID_++;

	//�t�@�C��������wav�̏����擾
	const auto wavFile = GetWavFile(fileName);
	const auto& wavInfo = wavFile->Info();
	
	//�\�[�X���H�C�X�̍쐻
	sourceVoiceMap_[retID].reset(new SourceVoiceWrapper());
	auto result = sourceVoiceMap_[retID]->Init(xAudio2_.Get(), wavFile,
		aeFlag);
	if (!result) {
		assert(0);
		sourceVoiceMap_.erase(retID);
		return -1;
	}

	//�K�X�T�u�~�b�N�X�{�C�X�ɒǉ�
	XAUDIO2_VOICE_SENDS sendList = { 0 };
	std::vector<XAUDIO2_SEND_DESCRIPTOR> pSends;

	if (aeFlag & AUDIO_FLAG_USEREVERB) {//3D�ݒ���g����
		XAUDIO2_SEND_DESCRIPTOR desc1 = { 0 };
		desc1.pOutputVoice = subMixVoiceReverb_[0];
		pSends.push_back(desc1);
		XAUDIO2_SEND_DESCRIPTOR desc2 = { 0 };
		desc2.pOutputVoice = subMixVoiceReverb_[1];
		pSends.push_back(desc2);
	}
	else if (aeFlag & AUDIO_FLAG_SE) {
		XAUDIO2_SEND_DESCRIPTOR desc = { 0 };
		desc.pOutputVoice = subMixVoiceSE_;
		pSends.push_back(desc);
	}
	else if (aeFlag & AUDIO_FLAG_BGM) {
		XAUDIO2_SEND_DESCRIPTOR desc = { 0 };
		desc.pOutputVoice = subMixVoiceBGM_;
		pSends.push_back(desc);
	}

	if (pSends.size() > 0) {
		sendList.SendCount = pSends.size();
		sendList.pSends = &pSends[0];
		sourceVoiceMap_[retID]->SetOutputVoice(&sendList);
	}

	return retID;
}

const WaveFile*
AudioSystem::GetWavFile(const char* fileName) {
	//��x�ǂ݂��񂾃t�@�C���͘A�z�z�񂩂�T��
	auto it = waveFileMap_.find(fileName);
	if (it != waveFileMap_.end()) {
		return it->second.get();
	}
	//�t�@�C����ǂݍ���
	waveFileMap_[fileName].reset(new WaveFile());
	auto result = waveFileMap_[fileName]->ReadFile(fileName);
	
	if (!result) {
		assert(0);
	}
	return waveFileMap_[fileName].get();
}

void
AudioSystem::DestroySourceVoice(const class AudioEvent* event) {
	auto it = sourceVoiceMap_.find(event->GetID());
	if (it != sourceVoiceMap_.end()) {
		sourceVoiceMap_.erase(it);
	}
	else {
		assert(0 && "�\�[�X�{�C�X��������܂���");
	}
}

void
AudioSystem::Play(const AudioEvent* event) {
	const unsigned int eid = event->GetID();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return;
	}
	auto sVoive = it->second.get();
	//�Ƃ肠�����ŏ�����Đ�
	auto result = sVoive->Start();
}

void
AudioSystem::Stop(const AudioEvent* event, bool pause) {
	const unsigned int eid = event->GetID();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return;
	}
	auto sVoive = it->second.get();
	auto result = sVoive->Stop(pause);
}

void
AudioSystem::SetVolume(const AudioEvent* event) {
	const unsigned int eid = event->GetID();
	const float vol = event->GetVolume();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return;
	}
	it->second->SetVolume(vol);
}

void
AudioSystem::SetLoop(const AudioEvent* event, bool loop) {
	const unsigned int eid = event->GetID();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return;
	}
	it->second->SetLoop(loop);
}

void
AudioSystem::UpdateListner(float deltaTime) {
	if (listenerObj_ == nullptr) {//���X�i�[�������Ă���X�V���Ȃ�
		listener_.Velocity = Vector3::Zero();
		return;
	}
	listener_.OrientFront = listenerObj_->GetForward();
	listener_.OrientTop = listenerObj_->GetUp();
	preListenerPos_ = listener_.Position;
	listener_.Position = listenerObj_->GetWorldPos() + listenerOfst_;
	auto vel = (Vector3(listener_.Position) - preListenerPos_) / deltaTime;
	listener_.Velocity = vel;

}

void 
AudioSystem::ResetListenerObject(Object* listener) {
	if (listenerObj_ == listener) {
		listenerObj_ = nullptr;
		listener_.OrientFront =
			listener_.OrientTop =
			listener_.Position =
			listener_.Velocity =
			listenerOfst_ = Vector3::Zero();
	}

}


void 
AudioSystem::UpdateEmitter(const AudioEvent* event, 
	float deltaTime, class Object* obj) {
	const unsigned int eid = event->GetID();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return;
	}
	it->second->UpdateEmitter(deltaTime, obj);
}

bool
AudioSystem::IsPlaying(const AudioEvent* event) {
	const unsigned int eid = event->GetID();
	auto it = sourceVoiceMap_.find(eid);
	if (it == sourceVoiceMap_.end()) {
		assert(0 && "�\�[�X�{�C�X��������܂���");
		return false;
	}
	return it->second->IsPlaying();
}


void 
AudioSystem::SetFilterParametersToFinalRender(Filter_Parameters* params,
	float deltaTime, float time) {

	XAUDIO2_FILTER_PARAMETERS xparams;
	subMixVoiceFinal_->GetFilterParameters(&xparams);
	params->frequency = (params->frequency > XAUDIO2_MAX_FILTER_FREQUENCY) ?
		XAUDIO2_MAX_FILTER_FREQUENCY : params->frequency;
	params->oneOverQ = (params->oneOverQ > XAUDIO2_MAX_FILTER_ONEOVERQ) ?
		XAUDIO2_MAX_FILTER_ONEOVERQ : params->oneOverQ;
	switch (params->type)
	{
	case LowPassFilter:
		xparams.Type = LowPassFilter;
		break;
	case AS_BandPassFilter:
		xparams.Type = BandPassFilter;
		break;
	case AS_HighPassFilter:
		xparams.Type = HighPassFilter;
		break;
	case AS_NotchFilter:
		xparams.Type = NotchFilter;
		break;
	case AS_LowPassOnePoleFilter:
		xparams.Type = LowPassOnePoleFilter;
		break;
	case AS_HighPassOnePoleFilter:
		xparams.Type = HighPassOnePoleFilter;
		break;
	default:
		assert(0);
		return;
		break;
	}
	//�t�F�[�h����
	auto& state = filterStateMap_[params->type];
	if (state.isFading) {
		//�t�F�[�h��
		xparams.Frequency += state.freqVel * deltaTime;
		xparams.OneOverQ += state.oneOverQVel * deltaTime;
		if ((params->frequency - xparams.Frequency) * state.freqVel < 0 ||
			(params->oneOverQ - xparams.OneOverQ) * state.oneOverQVel < 0) {
			//�w��l��ʂ肷���Ă�.�t�F�[�h�I��
			xparams.Frequency = params->frequency;
			xparams.OneOverQ = params->oneOverQ;
			state.isFading = false;
		}
	}
	else {
		if (fabs(params->frequency - xparams.Frequency) < FLT_EPSILON &&
			fabs(params->oneOverQ - xparams.OneOverQ) < FLT_EPSILON) {
			//�w��l�Ɠ����Ȃ̂łȂɂ����Ȃ�
			state.isFading = false;
			return;
		}
		else {
			//�t�F�[�h�̊J�n
			state.isFading = true;
			state.freqVel = (params->frequency - xparams.Frequency) / time;
			state.oneOverQVel = (params->oneOverQ - xparams.OneOverQ) / time;
		}
	}

	subMixVoiceFinal_->SetFilterParameters(&xparams);
}

void
AudioSystem::ResetFilterParametersToFinalRender() {

	XAUDIO2_FILTER_PARAMETERS params;
	params.Type = LowPassFilter;
	params.Frequency = 1.0f;
	params.OneOverQ = 1.0f;
	subMixVoiceFinal_->SetFilterParameters(&params);

}

void
AudioSystem::StopAll(bool pause) {
	for (auto& sv : sourceVoiceMap_) {
		sv.second->Stop(pause);
	}
}