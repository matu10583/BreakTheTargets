#pragma once

#include <xaudio2.h>
#include <x3daudio.h>
#include <vector>
#include "XMFLOAT_Helper.h"

class SourceVoiceWrapper
{
	struct DstVoiceInfo
	{
		IXAudio2Voice* mainDst = nullptr;//メインの出力先(nullptrはマスター)
		std::vector<IXAudio2Voice*> subDst;//サブの出力先
	};
public:
	SourceVoiceWrapper();
	~SourceVoiceWrapper();
	bool Init(
		IXAudio2* xaudio2, const class WaveFile* file, 
		unsigned int classFlag);
	//ソースボイスのストリーミング更新を行う
	void UpdateSourceVoice();
	//エミッターの状態更新を行う
	void UpdateEmitter(float deltaTime, class Object* obj);
	//3Dオーディオを適用する
	void ApplyDSPSetting(const X3DAUDIO_DSP_SETTINGS& dsp);


	//SourceVoiceの関数
	HRESULT Start(UINT32 flags=0);
	HRESULT Stop(bool pause);
	void SetVolume(float vol);
	void SetLoop(bool loop) { isLoop_ = loop; }
	//自分がどの種類かのフラグを保存する
	void SetOutputVoice(XAUDIO2_VOICE_SENDS* pSendsList);
	unsigned int GetClassFlag()const { return audioClassFlag_; }
	const X3DAUDIO_EMITTER& GetEmitter()const { return emitter_; }

	const struct WaveInfo& GetWAVInfo()const;
	//isplaying変数より精度よく状態を返す
	bool IsPlaying()const;


private:
	void AddNextBuffer();

	IXAudio2SourceVoice* sourceVoice_;
	const WaveFile* wavFile_;

	std::vector<BYTE> buffers_[2];//プライマリバッファとセカンダリバッファ
	int primary = 0;
	int secondary = 1;

	DWORD cursor_ = 0;//現在読み込んでいる場所

	bool isPlaying_;
	bool doPlay_;//再生処理を行うかどうか
	bool isLoop_;//ループ再生かどうか

	unsigned int audioClassFlag_ = 0;//オーディオの種類のフラグ
									//(enum AUDIO_CLASSIFIC_FLAG in AudioComponent.h)
	DstVoiceInfo sendList_;//自分がどのサブミックスボイスに送ってるかを記録する

	X3DAUDIO_EMITTER emitter_ = { 0 };//3dオーディオのためのエミッター
	X3DAUDIO_CONE emitterCone_;
	Vector3 preEmitterPos_;



};

