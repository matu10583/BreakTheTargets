#pragma once
#include <wrl.h>
#include <xaudio2.h>
#include <x3daudio.h>
#include <memory>
#include <unordered_map>
#include <utility>
#include "AudioComponent.h"
#include "XMFLOAT_Helper.h"


class AudioSystem
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	friend class AudioEvent;

public:
	static void Create();
	static void Destroy();
	static AudioSystem* Instance();

	bool Init();
	void Update(float deltaTime);
	void StopAll(bool pause);//�������S���~�߂�

	//���X�i�[��o�^(�ʒu�̃I�t�Z�b�g���o�^�\.��]�́c�c������)
	void SetListenerObject(class Object* listner, 
		const Vector3 offset = Vector3::Zero()) {
		listenerObj_ = listner;
		listenerOfst_ = offset;
	}
	//���ꃊ�X�i�[���j�����ꂽ�Ƃ��̓A�N�Z�X���Ȃ��悤�ɂ�����Ă�
	void ResetListenerObject(Object* listener);

	void SetSEVolume(float vol) { subMixVoiceSE_->SetVolume(vol); }
	void SetBGMVolume(float vol) { subMixVoiceBGM_->SetVolume(vol); }
	void SetVolume(float vol) { masterVoice_->SetVolume(vol); }

	//�ŏI�o�͂Ƀt�B���^�[��������
	enum FilterType
	{
		AS_LowPassFilter,
		AS_BandPassFilter,
		AS_HighPassFilter,
		AS_NotchFilter,
		AS_LowPassOnePoleFilter,
		AS_HighPassOnePoleFilter
	};
	struct Filter_Parameters
	{
		FilterType type;
		float frequency;
		float oneOverQ;
	};
	struct FilterState//�t�B���^�[�̏��
	{
		bool isFading = false;;//�t�F�[�h����
		float oneOverQVel;//�t�F�[�h�̑��x
		float freqVel;
	};
	//�t�B���^�[�̓K�p�B�t�F�[�h������
	void SetFilterParametersToFinalRender(Filter_Parameters* params,
		float deltaTime, float time = 0.2f);
	//�t�B���^�[�Q�̃��Z�b�g
	void ResetFilterParametersToFinalRender();
	//���g������frequency���v�Z
	float CalcFrequency(float hz);
private:
	static AudioSystem* instance_;

	AudioSystem();
	~AudioSystem();

	const class WaveFile* GetWavFile(const char* fileName);

	ComPtr<IXAudio2> xAudio2_ = nullptr;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
	IXAudio2SubmixVoice* subMixVoiceSE_ = nullptr;//SE�����p�̃T�u�~�b�N�X�{�C�X
	IXAudio2SubmixVoice* subMixVoiceBGM_ = nullptr;//BGM�����p�̃T�u�~�b�N�X�{�C�X
	IXAudio2SubmixVoice* subMixVoiceReverb_[2];//�Z���h���o�[�u�p(0��dry,1��wet)
	IXAudio2SubmixVoice* subMixVoiceFinal_ = nullptr;//�}�X�^�[�{�C�X�̑O�̍ŏI�o�͐�
	X3DAUDIO_HANDLE x3DInstance_;//X3DAudio�̃n���h��

	//eventID�ƃ\�[�X�{�C�X�̃}�b�v
	std::unordered_map<unsigned int, 
		std::unique_ptr<class SourceVoiceWrapper>>
		sourceVoiceMap_;
	//���ɕԂ�EventID
	unsigned int nextEID_;

	//�t�B���^�[�̏��
	std::unordered_map<FilterType, FilterState>
		filterStateMap_;

	//�t�@�C������wav�f�[�^�̃}�b�v
	std::unordered_map < std::string,
		std::unique_ptr<WaveFile>> waveFileMap_;

	//3D�I�[�f�B�I�̃��X�i�[
	X3DAUDIO_LISTENER listener_;
	X3DAUDIO_CONE listenerCone_;
	Vector3 preListenerPos_;
	//���X�i�[�̃I�u�W�F�N�g
	Object* listenerObj_ = nullptr;
	Vector3 listenerOfst_;//�ʒu�̃I�t�Z�b�g

	//���X�i�[�̍X�V
	void UpdateListner(float deltaTime);

	//�\�[�X�{�C�X�����id��Ԃ�
	unsigned int CreateSourceVoice(const char* fileName,
		const UINT32 aeFlag);
	//�\�[�X�{�C�X������
	void DestroySourceVoice(const class AudioEvent* event);
	//�{�C�X�̍Đ�
	void Play(const class AudioEvent* event);
	//�{�C�X�̒�~
	void Stop(const class AudioEvent* event, bool pause);
	//�{�����[���ݒ�
	void SetVolume(const AudioEvent* event);
	//���[�v�ݒ�
	void SetLoop(const AudioEvent* event, bool loop);
	//�G�~�b�^�[�̍X�V
	void UpdateEmitter(const AudioEvent* event, float deltaTime, class Object* obj);
	//�Đ�����
	bool IsPlaying(const AudioEvent* event);
};

