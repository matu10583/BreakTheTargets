#pragma once
#include "SpriteComponent.h"
#include <string>

enum TEXTSPRITE_ALLIGN
{
    //ALLIGN_LEFT = 0,
    ALLIGN_RIGHT = 1,
    //ALLIGN_TOP = 0,
    ALLIGN_BUTTOM=2
    //�t���O�Ƃ��Ďg��.
    //��)flag=0b10...ALLIGN_LEFT_BUTTOM
};


class TextSpriteComponent :
    public SpriteComponent
{
public:
    TextSpriteComponent(class Object* owner, const char* fontFile, int updateOrder = 100);
    void Draw()override;
    void Update(float deltaTime)override;
    void SetStr(const char* str) {
        str_ = str;
    }

    void SetScaleRatioToWindowSize(const Vector2& ratio);
    void SetScaleRatioToWindowSizeX(float ratio);
    void SetScaleRatioToWindowSizeY(float ratio);

    void SetAllign(unsigned int flag) { allign_ = flag; }

private:
    //�t�H���g�摜�̏c���̕�����
    const int X_CHAR_NUM = 16;
    const int Y_CHAR_NUM = 6;
    std::string str_;//�\�����镶
    Vector2 charSize_;//�ꕶ��������̃T�C�Y
    unsigned int allign_ = 0;//�A���C���t���O
};

