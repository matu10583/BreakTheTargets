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
	void StopAll(bool pause);//音を一回全部止める

	//リスナーを登録(位置のオフセットも登録可能.回転は……いいや)
	void SetListenerObject(class Object* listner, 
		const Vector3 offset = Vector3::Zero()) {
		listenerObj_ = listner;
		listenerOfst_ = offset;
	}
	//万一リスナーが破棄されたときはアクセスしないようにこれを呼ぶ
	void ResetListenerObject(Object* listener);

	void SetSEVolume(float vol) { subMixVoiceSE_->SetVolume(vol); }
	void SetBGMVolume(float vol) { subMixVoiceBGM_->SetVolume(vol); }
	void SetVolume(float vol) { masterVoice_->SetVolume(vol); }

	//最終出力にフィルターをかける
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
	struct FilterState//フィルターの状態
	{
		bool isFading = false;;//フェード中か
		float oneOverQVel;//フェードの速度
		float freqVel;
	};
	//フィルターの適用。フェードさせる
	void SetFilterParametersToFinalRender(Filter_Parameters* params,
		float deltaTime, float time = 0.2f);
	//フィルター群のリセット
	void ResetFilterParametersToFinalRender();
	//周波数からfrequencyを計算
	float CalcFrequency(float hz);
private:
	static AudioSystem* instance_;

	AudioSystem();
	~AudioSystem();

	const class WaveFile* GetWavFile(const char* fileName);

	ComPtr<IXAudio2> xAudio2_ = nullptr;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
	IXAudio2SubmixVoice* subMixVoiceSE_ = nullptr;//SE処理用のサブミックスボイス
	IXAudio2SubmixVoice* subMixVoiceBGM_ = nullptr;//BGM処理用のサブミックスボイス
	IXAudio2SubmixVoice* subMixVoiceReverb_[2];//センドリバーブ用(0がdry,1がwet)
	IXAudio2SubmixVoice* subMixVoiceFinal_ = nullptr;//マスターボイスの前の最終出力先
	X3DAUDIO_HANDLE x3DInstance_;//X3DAudioのハンドラ

	//eventIDとソースボイスのマップ
	std::unordered_map<unsigned int, 
		std::unique_ptr<class SourceVoiceWrapper>>
		sourceVoiceMap_;
	//次に返すEventID
	unsigned int nextEID_;

	//フィルターの状態
	std::unordered_map<FilterType, FilterState>
		filterStateMap_;

	//ファイル名とwavデータのマップ
	std::unordered_map < std::string,
		std::unique_ptr<WaveFile>> waveFileMap_;

	//3Dオーディオのリスナー
	X3DAUDIO_LISTENER listener_;
	X3DAUDIO_CONE listenerCone_;
	Vector3 preListenerPos_;
	//リスナーのオブジェクト
	Object* listenerObj_ = nullptr;
	Vector3 listenerOfst_;//位置のオフセット

	//リスナーの更新
	void UpdateListner(float deltaTime);

	//ソースボイスを作りidを返す
	unsigned int CreateSourceVoice(const char* fileName,
		const UINT32 aeFlag);
	//ソースボイスを消す
	void DestroySourceVoice(const class AudioEvent* event);
	//ボイスの再生
	void Play(const class AudioEvent* event);
	//ボイスの停止
	void Stop(const class AudioEvent* event, bool pause);
	//ボリューム設定
	void SetVolume(const AudioEvent* event);
	//ループ設定
	void SetLoop(const AudioEvent* event, bool loop);
	//エミッターの更新
	void UpdateEmitter(const AudioEvent* event, float deltaTime, class Object* obj);
	//再生中か
	bool IsPlaying(const AudioEvent* event);
};

