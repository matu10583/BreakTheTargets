#pragma once
#include "SceneObject.h"
class ResultSceneObject :
    public SceneObject
{
public:
    ResultSceneObject(class Game* game, float score, 
        int stageMode, const char* rankDat);
private:
    SceneObject* UpdateScene(float deltaTime)override;
    SceneObject* SceneInput(const struct InputState&)override;
    void Init(const char* rankDat, int stageMode);

    float score_;
    std::vector<float> ranks_;
    class TextSpriteComponent* thx_;
    TextSpriteComponent* scoreTex_;
    TextSpriteComponent* rankTex_;
    TextSpriteComponent* return_;

    class AudioComponent* applause_;

    bool transitFlag_;
};

