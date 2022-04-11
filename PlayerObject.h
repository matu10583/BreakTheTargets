#pragma once
#include "Object.h"
#include <Windows.h>
#include <array>

class PlayerObject :
    public Object
{
    using AnimID = unsigned int;
public:
    PlayerObject(class Object*);
    ~PlayerObject();

    int GetBulletNum() { return bNum_; }
    void SetBulletNum(int b) { bNum_ = b; }

private:
    void OnCollision(const struct CollisionInfo& ci)override;
    void UpdateObject(float deltaTime) override;
    void ActorInput(const struct InputState& inputState)override;

    class PlayerMoveComponent* playComp_;
    class AudioComponent* stepSound_;//����
    AudioComponent* landSound_;//���n��
    AudioComponent* readyShootSound_;//�ˌ�������   
    std::array<AudioComponent*, 5> shootSound_;//�ˌ����i������5���܂Łj
    AudioComponent* walkSound_;

    //�c��e��
    int bNum_;
    
    //�A�j���[�V�����m�[�h
    AnimID breath_;
    AnimID run_;
    AnimID jump_;
    AnimID fall1_;
    AnimID fall2_;
    AnimID gun_;
    AnimID gunkeep_;
    AnimID gunshoot_;
    //�A�j���[�V�����t���O
    bool doNeutral_;
    bool doRun_;
    bool doJump_;
    bool doFall_;
    bool doGunReady_;
    bool doGunShoot_;

    //�O�̔���Œn�ʂɂ�����
    bool forOnGround_;
    bool onGround_;

    //�q�I�u�W�F�N�g
    class CameraObject* camera_;
    class ReticleObject* reticle_;

};

