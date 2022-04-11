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

    //���[���h�ɑ��݂���I�u�W�F�N�g
    //�K�{�̃I�u�W�F�N�g
    class PlayerObject* player_;
    //UI
    class RemainNumObject* rNumText_;//�c��^�[�Q�b�gor�e���̕\��
    class ScoreTextObject* score_;
    class TimeObject* time_;
    class ControlCaptionObject* cco_;
    class PauseMenuObject* pause_;

    //�f�o�b�O�p
    class FreeCameraObject* freeCamera_;

    //�ϒ�
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

    //�w��̃I�u�W�F�N�g�𑝂₷
    Object* AddObject(const std::string id, const std::string name);
    void DeleteObject(const std::string id, const std::string name);

    bool editMode_;//�X�e�[�W�ҏW���[�h���ǂ���
    void SaveStageData(const char* fileName);

    //�����������ʃI�u�W�F�N�g���}�b�v����폜����
    void RemoveDeadObjectFromMap();

    //imgui�p�����ϐ�
    std::string objId_;
    std::string objName_;
    std::string modelName_;

    //�X�e�[�W�t�@�C����
    std::string stageFile_;
    //�����L���O�t�@�C����
    std::string rankFile_;

    //�ŏ��Ɏ����Ă�e�̐�
    int bulletNum_;
    //��������
    float timeLimit_;
    //�X�e�[�W�̃��[�h
    int stageMode_;
};

