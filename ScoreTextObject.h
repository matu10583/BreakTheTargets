#pragma once
#include "Object.h"
class ScoreTextObject :
    public Object
{
public:
    ScoreTextObject(class Object* parent);
    void AddScore(int sc);
    int GetScore() { return score_; }
private:
    class TextSpriteComponent* text_;

    void UpdateObject(float deltaTime)override;
    int score_;
    int dispScore_;//表示用スコア

    class AudioComponent* hitSound_;
};

