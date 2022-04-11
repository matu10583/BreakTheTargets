#pragma once

#include <xaudio2.h>
#include <x3daudio.h>
#include <vector>
#include "XMFLOAT_Helper.h"

class SourceVoiceWrapper
{
	struct DstVoiceInfo
	{
		IXAudio2Voice* mainDst = nullptr;//���C���̏o�͐�(nullptr�̓}�X�^�[)
		std::vector<IXAudio2Voice*> subDst;//�T�u�̏o�͐�
	};
public:
	SourceVoiceWrapper();
	~SourceVoiceWrapper();
	bool Init(
		IXAudio2* xaudio2, const class WaveFile* file, 
		unsigned int classFlag);
	//�\�[�X�{�C�X�̃X�g���[�~���O�X�V���s��
	void UpdateSourceVoice();
	//�G�~�b�^�[�̏�ԍX�V���s��
	void UpdateEmitter(float deltaTime, class Object* obj);
	//3D�I�[�f�B�I��K�p����
	void ApplyDSPSetting(const X3DAUDIO_DSP_SETTINGS& dsp);


	//SourceVoice�̊֐�
	HRESULT Start(UINT32 flags=0);
	HRESULT Stop(bool pause);
	void SetVolume(float vol);
	void SetLoop(bool loop) { isLoop_ = loop; }
	//�������ǂ̎�ނ��̃t���O��ۑ�����
	void SetOutputVoice(XAUDIO2_VOICE_SENDS* pSendsList);
	unsigned int GetClassFlag()const { return audioClassFlag_; }
	const X3DAUDIO_EMITTER& GetEmitter()const { return emitter_; }

	const struct WaveInfo& GetWAVInfo()const;
	//isplaying�ϐ���萸�x�悭��Ԃ�Ԃ�
	bool IsPlaying()const;


private:
	void AddNextBuffer();

	IXAudio2SourceVoice* sourceVoice_;
	const WaveFile* wavFile_;

	std::vector<BYTE> buffers_[2];//�v���C�}���o�b�t�@�ƃZ�J���_���o�b�t�@
	int primary = 0;
	int secondary = 1;

	DWORD cursor_ = 0;//���ݓǂݍ���ł���ꏊ

	bool isPlaying_;
	bool doPlay_;//�Đ��������s�����ǂ���
	bool isLoop_;//���[�v�Đ����ǂ���

	unsigned int audioClassFlag_ = 0;//�I�[�f�B�I�̎�ނ̃t���O
									//(enum AUDIO_CLASSIFIC_FLAG in AudioComponent.h)
	DstVoiceInfo sendList_;//�������ǂ̃T�u�~�b�N�X�{�C�X�ɑ����Ă邩���L�^����

	X3DAUDIO_EMITTER emitter_ = { 0 };//3d�I�[�f�B�I�̂��߂̃G�~�b�^�[
	X3DAUDIO_CONE emitterCone_;
	Vector3 preEmitterPos_;



};

