#pragma once
#include "SceneObject.h"
#include <string>
#include <array>
#include <unordered_map>
#include "json.hpp"

using json = nlohmann::json;

class GameSceneObject :
    public SceneObject
{

public:
    enum StageMode
    {
        TIMEATTACK = 0,
        SCOREATTACK = 1
    };


    GameSceneObject(class Game* game, const char* stgeFile);
    ~GameSceneObject();

private:
    struct ObjData {
        std::string objID;
        std::string name;
        std::array<float, 3> pos;
        std::array<float, 3> rot;
        std::array<float, 3> scale;
    };


    SceneObject* UpdateScene(float deltaTime)override;
    SceneObject* SceneInput(const struct InputState&)override;
    bool Init(const char* fileName);

    //ワールドに存在するオブジェクト
    //必須のオブジェクト
    class PlayerObject* player_;
    //UI
    class RemainNumObject* rNumText_;//残りターゲットor弾数の表示
    class ScoreTextObject* score_;
    class TimeObject* time_;
    class ControlCaptionObject* cco_;
    class PauseMenuObject* pause_;

    //デバッグ用
    class FreeCameraObject* freeCamera_;

    //可変長
    std::unordered_map<std::string, 
        class PlaneObject*> 
        ground_;
    std::unordered_map<std::string, 
        class PlaneObject*> 
        wall_;
    std::unordered_map<std::string, 
        class TargetFactoryObject*> 
        targetFac_;
    std::unordered_map<std::string, 
        class TargetObject*>
        targets_;
    std::unordered_map<std::string,
        class PointLightObject*>
        lights_;
    std::unordered_map<std::string,
        class RadioObject*>
        radio_;
    std::unordered_map<std::string,
        class ModelObject*>
        models_;


    void MakeImguiWindow();

    //指定のオブジェクトを増やす
    Object* AddObject(const std::string id, const std::string name);
    void DeleteObject(const std::string id, const std::string name);

    bool editMode_;//ステージ編集モードかどうか
    void SaveStageData(const char* fileName);

    //もうすぐ死ぬオブジェクトをマップから削除する
    void RemoveDeadObjectFromMap();

    //imgui用文字変数
    std::string objId_;
    std::string objName_;
    std::string modelName_;

    //ステージファイル名
    std::string stageFile_;
    //ランキングファイル名
    std::string rankFile_;

    //最初に持ってる弾の数
    int bulletNum_;
    //制限時間
    float timeLimit_;
    //ステージのモード
    int stageMode_;
};

