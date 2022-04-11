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
    class AudioComponent* stepSound_;//足音
    AudioComponent* landSound_;//着地音
    AudioComponent* readyShootSound_;//射撃準備音   
    std::array<AudioComponent*, 5> shootSound_;//射撃音（同時に5発まで）
    AudioComponent* walkSound_;

    //残り弾数
    int bNum_;
    
    //アニメーションノード
    AnimID breath_;
    AnimID run_;
    AnimID jump_;
    AnimID fall1_;
    AnimID fall2_;
    AnimID gun_;
    AnimID gunkeep_;
    AnimID gunshoot_;
    //アニメーションフラグ
    bool doNeutral_;
    bool doRun_;
    bool doJump_;
    bool doFall_;
    bool doGunReady_;
    bool doGunShoot_;

    //前の判定で地面にいたか
    bool forOnGround_;
    bool onGround_;

    //子オブジェクト
    class CameraObject* camera_;
    class ReticleObject* reticle_;

};

