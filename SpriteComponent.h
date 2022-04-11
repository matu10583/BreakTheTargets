#pragma once
#include "Component.h"
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include<DirectXMath.h>
#include "XMFLOAT_Helper.h"


using namespace DirectX;
using namespace Microsoft::WRL;

class SpriteComponent :
    public Component
{
public:
    SpriteComponent(Object* owner, const char* fileName, int updateOrder = 100);
    ~SpriteComponent();
    virtual void Update(float deltaTime) override;
    virtual void Draw();

    void SetScale(const Vector2& sc) { scale_ = sc; }
    //��ʂɑ΂���䗦�ŃX�P�[�������߂�
    void SetScaleRatioToWindowSize(const Vector2& ratio);
    void SetScaleRatioToWindowSizeX(float ratio);
    void SetScaleRatioToWindowSizeY(float ratio);
    void SetPivot(const Vector2& p) { pivot_ = p; }
    void SetOffset(const Vector2& o) { offset_ = o; }
    void SetPos(const Vector3& p) { pos_ = p; }
    void SetPosRatioToWindowSize(const Vector3& p);
    void SetRotate(float r) { rot_ = r; }
    void SetClipSize(const Vector2& cs) { clipSize_ = cs; }
    void SetClipLeftTop(const Vector2& lefttop) { clipLeftTop_ = lefttop; }

    const Vector2 GetImgSize()const;

    struct TransformData
    {
        XMMATRIX world;//���[���h�s��
        XMFLOAT2 pivot;//�s�{�b�g
        XMFLOAT2 clipSize;//�N���b�v�T�C�Y
        XMFLOAT2 spSize;
        XMFLOAT2 offset;
        XMFLOAT2 clipLeftTop[50];
        XMFLOAT2 instancePos[50];//�C���X�^���X�̏ꏊ.�قƂ�Ǖ����p
        //�N���b�v�̍�����W(�����o�͂̂��߂ɕ����̃N���b�v�ɑΉ�����)
//������`��p�Ɉ떇������̃X�N���[�����W�T�C�Y���i�[����
    };
    
protected:
    class Dx12Wrapper* dx12_;
    //�傫���̔{��
    Vector2 scale_;
    //�s�{�b�g�i�X�N���[�����W�j
    Vector2 pivot_;
    //��]�i���W�A���A�E�܂�肪���j
    float rot_;
    //�X�N���[�����W�ł̈ʒu(z�����͉�������unorm)
    Vector3 pos_;
    //�؂���@�\�iuv���W)
    Vector2 clipSize_;
    Vector2 clipLeftTop_;
    //���S�I�t�Z�b�g
    Vector2 offset_;

    //�`�悷�邩�ǂ���
    bool isDraw;

    std::shared_ptr<class Sprite> sprite_;

    TransformData* mappedTransMatrix_;
    ComPtr<ID3D12Resource> transformBuff_;
    ComPtr<ID3D12DescriptorHeap> transformHeap_;

    HRESULT CreateTransformResourceAndView();
    const int MAX_CLIP_SIZE = 100;//�N���b�v���W��x���ő吔

};

