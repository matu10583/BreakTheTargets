#pragma once
#include "Component.h"
#include "Dx12Wrapper.h"
#include "Model.h"
#include "Collider.h"
#include <memory>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d12.h>

using namespace DirectX;

class Object;


//�A�j���[�V�����J�ڗp�\����
//���𑝂₹��悤�ɍ\���̂ɂ��Ă���
struct AnimationNode
{
    std::shared_ptr<AnimationData> anime;
    bool isLoop;//���[�v���邩�ǂ����B���Ȃ��Ȃ�ŏI�t���[���̃|�[���̂܂�
    int transitFrameNum;//�O�̃A�j���[�V�����Ƃ̕�Ԃɉ��t���[�������邩
};
//�J�ڐ�
struct TransitLine
{
    const unsigned int end;//�J�ڐ�̃A�j���[�V����
    const bool* condition;//�J�ڏ����t���O�̃A�h���X.nullptr�̏ꍇ��ɑJ�ڂ���
    const bool value;//condition = value�őJ�ڂ���
    const bool isInterrupted;//�A�j���[�V�����̓r���őJ�ڂ��邩
    TransitLine(const unsigned int endid, const bool* cond, bool val, bool isInter=true):
        end(endid),condition(cond), value(val), isInterrupted(isInter){}
};

struct WorldMatrix {//���[���h�s��
    XMMATRIX position;
    XMMATRIX rotate;
};

class MeshComponent :
    public Component
{
    //ComPtr�ŊǗ�����
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    MeshComponent(Object* owner, const char* fileName, int updateOrder=100);
    MeshComponent(
        Object* owner, const std::vector<VertexData>& vertices, //���_�z��
        const std::vector<Index>& indecies, //�C���f�b�N�X�z��
        const std::vector<Material>& materials,//�}�e���A��
        const char* name = nullptr,//���ʖ��i�C�Ӂj
        int updateOrder = 100);
    ~MeshComponent();
    //isShadow:�ԓ��}�b�v�̕`��p��
    void DrawPMD(bool isShadow);
    void DrawVert(bool isShadow);
    void Update(float deltaTime) override;

    //�A�j���m�[�h�̒ǉ��A�폜.
    unsigned int AddAnimationNode(const char* filename, 
        bool isLoop=true, unsigned int transitFrameNum=5);
    void DeleteAnimationNode(const unsigned int anim);
    //�J�ڐ��̍쐻
    //�����Ȃ��ŏ�ɑJ�ڂ���Ȃ������nullptr
    void MakeAnimationTransit(
        const unsigned int startID, const unsigned int endID, 
        const bool* condition=nullptr, bool value = true, bool isInterrupted = true);
    //�ŏ��̃A�j���[�V���������߂�
    void SetEntryAnimationNode(const unsigned int entryID);

    void SetIsLeftHanded(bool isLH);

    //���f�����͂�AABB��Ԃ�
    const AABB& GetLocalAABB()const { return localAABB_; }
    const AABB& GetWorldAABB()const { return worldAABB_; }
private:

    class Dx12Wrapper* dx12_;

    std::shared_ptr<Model> model_;
    XMMATRIX* mappedPosMatrix_;

    //�ړ��s��i�{�[���̐������j
    std::vector<XMMATRIX> boneMatrices_;

    //�ʒu�����p�萔�o�b�t�@
    ComPtr<ID3D12Resource> transformBuff_;
    ComPtr<ID3D12DescriptorHeap> transformHeap_;

    HRESULT CreateTransformResourceAndView();

    //�A�j���[�V�����̃m�[�h
    std::unordered_map<unsigned int, AnimationNode> animeNodes_;
    //���̃A�j���[�V�������琶���Ă�J�ڐ�
    std::unordered_map<unsigned int,
        std::vector<TransitLine>>
        transitMap_;
    //���݂���A�j���[�V�����m�[�hID
    int nowAnimeNodeID_;
    //�ЂƂO�̃A�j���[�V�����̏��
    std::unordered_map<std::string,
    KeyFrame> forMotionData_;
    //�A�j���[�V���������b�ڂ�
    float elapsedTime_;
    //�A�j���[�V�����m�[�h��ID
    unsigned int aniID_ = 0;

    //���[�V�����̎��Ԃ�i�߂�
    void MotionUpdate(float deltaTime);
    //�q�m�[�h�ɐe�m�[�h�̍s���������
    void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);
    //IK�̉���
    void IKSolve(unsigned int frameNo);
    //IK�����w���p�[�֐�
    void SolveLookAt(const PMDIK& ik);
    void SolveCosine(const PMDIK& ik);
    void SolveCCDIK(const PMDIK& ik);

    //AABB�̍X�V
    void UpdateAABB();

    //���f��������n�ō���Ă��邩
    //�����false�ɂ���Ɣ��]���ĕ\��������
    bool isLeftHanded;

    //�`�悷�邩�ǂ���
    bool isDraw;

    const int WORLD_MATRIX_COUNT = 2;//���[���h�s��̐�
    const int TRANSIT_FRAME = 5;//���̃A�j���[�V�����ɑJ�ڂ���܂ł̃t���[����
    bool isTransiting;//�A�j���[�V�����̑J�ڂ��イ��
    bool isFirstFrame;//�A�j���[�V�����J�ڌ�̍ŏ��̃t���[����

    AABB localAABB_;//�����������J�����O�̂��߂̔���
    AABB worldAABB_;
};

