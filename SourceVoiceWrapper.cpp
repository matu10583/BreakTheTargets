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
	emitter_.ChannelCount = 1;//シングルエミッター
	emitter_.CurveDistanceScaler = 60;//距離の単位を合わせるための係数らしい
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
		//一秒分のサイズを用意
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
	isPlaying_ = true;//再生したとき
	doPlay_ = true;
	return result;
}
HRESULT
SourceVoiceWrapper::Stop(bool pause) {
	//ポーズならフラグを変える.
	UINT32 flags = 0;
	if (pause) {
		flags |= XAUDIO2_PLAY_TAILS;
	}
	else {//ストップなら最初の位置に持ってく
		cursor_ = 0;
		//バッファーのリセット
		sourceVoice_->FlushSourceBuffers();
	}
	auto result = sourceVoice_->Stop(flags);
	doPlay_ = false;
	isPlaying_ = false;//止めるとき
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

//セカンダリに続きのデータを書き込み、ソースボイスに追加
void
SourceVoiceWrapper::AddNextBuffer() {
	//実際に読んだ量
	DWORD readBytes = 0;
	//目的のデータを読み込む
	auto& buff = buffers_[secondary];
	if (!(wavFile_->GetWAVData(
		buff.data(), buff.size(), cursor_, &readBytes))) {
		assert(0);
		//カーソル位置がバグってるのでとりあえず最初に戻す
		cursor_ = 0;
	}
	//カーソルを進める
	cursor_ += readBytes;

	//source voiceにバッファを送信
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = readBytes;
	buffer.pAudioData = buff.data();

	if (wavFile_->GetSize() <= cursor_) {//最後まで読み終わった
		buffer.Flags = XAUDIO2_END_OF_STREAM;
	}
	//送信
	auto result = sourceVoice_->SubmitSourceBuffer(&buffer);
	if (FAILED(result)) {
		assert(0);
	}

	//primaryとsecondaryを入れ替える
	std::swap(primary, secondary);
}

void
SourceVoiceWrapper::UpdateSourceVoice() {
	//状態を取得
	XAUDIO2_VOICE_STATE state;
	sourceVoice_->GetState(&state);

	if (doPlay_) {
		if (state.BuffersQueued < 2) {
			if (cursor_ >= wavFile_->GetSize()) {
				if (isLoop_) {//ループかどうか
					cursor_ = 0;//最初に戻す
				}
				else {
					//再生をやめる
					doPlay_ = false;
					cursor_ = 0;
				}
			}
			else {
				AddNextBuffer();
				state.BuffersQueued += 1;//今追加した
			}
		}
	}
	if (state.BuffersQueued == 0) {
		sourceVoice_->GetState(&state);
		isPlaying_ = false;//再生おわってる
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
		//リバーブを使うなら二個目の設定も
		std::vector<FLOAT32> wetMatCoefficent(
			inoutCh);//リバーブの音量
		//dryのボリュームもそれに合わせて変える
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
	//とりあえず一つ目のポインタはdryのはず
	sourceVoice_->SetOutputMatrix(
		sendList_.mainDst, dsp.SrcChannelCount,
		dsp.DstChannelCount, dsp.pMatrixCoefficients);
	sourceVoice_->SetFrequencyRatio(dsp.DopplerFactor);
	
	XAUDIO2_FILTER_PARAMETERS FilterParameters = { LowPassFilter, 
		2.0f * sinf(X3DAUDIO_PI / 6.0f * dsp.LPFReverbCoefficient), 1.0f };
	sourceVoice_->SetFilterParameters(&FilterParameters);
}

//自分がそこへ出力するかを保存する
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