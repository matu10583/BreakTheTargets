#pragma once
#include "Component.h"
#include "AudioEvent.h"

enum AUDIO_CLASSIFIC_FLAG//�I�[�f�B�I�̎�ސݒ�
{
    AUDIO_FLAG_NONE = 0,
    AUDIO_FLAG_SE = 1,//SE��(3D�T�E���h���K�p�����)
    AUDIO_FLAG_USEFILTER = 2,//�t�B���^�[���g����
    AUDIO_FLAG_USEREVERB = 4,//���o�[�u���g����  
    AUDIO_FLAG_BGM = 8
};

class AudioComponent :
    public Component
{
public:
    AudioComponent(class Object* owner, const char* fileName,
        const unsigned int aeFlag = 0, int updateOrder=100);
    //�t�F�[�h�C���̎��Ԃ��w�肵�čĐ�
    void Play(float time = 0.05f);
    //�t�F�[�h�A�E�g�̎��Ԃ��w�肵�Ē�~.�~�܂�܂ł����ƌĂ�łق����ł�
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

    //�t�F�[�h�ݒ�
    float fdDst_;//�t�F�[�h�批��(�{��)
    float fdVel_;//�t�F�[�h�̑��x(�f�V�x��)

    bool stopFlag_;//��~�����t���O
    bool canPlay_;//�Đ��ł��邩�𐧌䂷��t���O

    float volume_;//���݂̃{�����[��
    float startVol_;//�Đ��X�^�[�g�������̖炷����
};

