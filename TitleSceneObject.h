#pragma once
#include "SceneObject.h"
#include <array>
class TitleSceneObject :
    public SceneObject
{
    enum SelectedScene
    {
        GAME_SCOREATTACK = 0,
        GAME_STAGE1 = 1,
        EXIT = 2
    };

public:
    TitleSceneObject(class Game* game);
private:
    SceneObject* UpdateScene(float deltaTime)override;
    SceneObject* SceneInput(const struct InputState&)override;
    void Init();

    class TextSpriteComponent* titleTex_;
    TextSpriteComponent* scAttackTex_;
    std::array<TextSpriteComponent*, 1> stageTex_;
    TextSpriteComponent* exitTex_;

    class AudioComponent* bgm_;

    int selected_;

    bool transitFlag_ = false;//BGMがフェードアウトするまでは遷移しない
};

