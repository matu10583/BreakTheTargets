#pragma once
#include "Component.h"
#include "AudioEvent.h"

enum AUDIO_CLASSIFIC_FLAG//オーディオの種類設定
{
    AUDIO_FLAG_NONE = 0,
    AUDIO_FLAG_SE = 1,//SEか(3Dサウンドが適用される)
    AUDIO_FLAG_USEFILTER = 2,//フィルターを使うか
    AUDIO_FLAG_USEREVERB = 4,//リバーブを使うか  
    AUDIO_FLAG_BGM = 8
};

class AudioComponent :
    public Component
{
public:
    AudioComponent(class Object* owner, const char* fileName,
        const unsigned int aeFlag = 0, int updateOrder=100);
    //フェードインの時間を指定して再生
    void Play(float time = 0.05f);
    //フェードアウトの時間を指定して停止.止まるまでずっと呼んでほしいです
    void Stop(float time = 0.05f);
    void Pause() { audioEvent_.Pause(); }
    void SetFade(float dst, float time);
    void SetVolume(float vol) {
        startVol_ = vol;
        volume_ = vol;
        fdDst_ = vol;
        fdVel_ = 0;
        audioEvent_.SetVolume(vol);
    }
    void SetLoop(bool loop) {
        audioEvent_.SetLoop(loop);
    }
    void SetCanPlay(bool flag) { canPlay_ = flag; }

    bool IsPlaying() { return audioEvent_.IsPlaying(); }

private:
    void Update(float deltaTime)override;

    AudioEvent audioEvent_;

    //フェード設定
    float fdDst_;//フェード先音量(倍率)
    float fdVel_;//フェードの速度(デシベル)

    bool stopFlag_;//停止処理フラグ
    bool canPlay_;//再生できるかを制御するフラグ

    float volume_;//現在のボリューム
    float startVol_;//再生スタートした時の鳴らす音量
};

