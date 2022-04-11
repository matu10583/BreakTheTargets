#pragma once
#include "Object.h"
#include <string>



class PauseMenuObject :
    public Object
{

public:
    enum SelectedScene
    {
        THISSCENE = 0,
        RESTARTSCENE=1,
        TITLESCENE = 2,
        RESULTSCENE = 3
    };
    PauseMenuObject(class Object* parent);
    SelectedScene GetSelectedScene() {
        return 
            (SelectedScene)selected_;
    }
private:
    void UpdateObject(float deltaTime)override;
    void ActorInput(const struct InputState&)override;

    class TextSpriteComponent* menu_;//ポーズメニュー用
    class TextSpriteComponent* pause_;//ポーズの文字
    
    int selected_;

    std::string menuName_[5];//メニューに出す文字
};

