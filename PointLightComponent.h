#pragma once
#include "Component.h"
#include "XMFLOAT_Helper.h"
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>



//�_�����̐ݒ�
class PointLightComponent :
    public Component
{
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
    PointLightComponent(class Object* owner, int updateOrder=100);
    ~PointLightComponent();
    void SetColor(const Vector3& col) {
        color_ = col;
    }
    void SetRange(const float ran) {
        range_ = ran;
    }
    void SetRadius(const float rad) {
        radius_ = rad;
    }
    void SetScale(const float scale) {
        scale_ = scale;
    }
    const Vector3& GetColor()const { return color_; }
    float GetRange()const { return range_; }
    float GetRadius()const { return radius_; }

    //���̏����o�b�t�@�[�ɃZ�b�g���A�����4�Ӄe�[�u���ɔz�u
    void SetLightBuffer();

private:
    struct LightData
    {
        XMFLOAT4 lightPos;//�����̏ꏊ
        XMFLOAT4 lightCol;//���̐F
        float radius;//���̔��a
        float range;//�e���͈�
        float lightScale;//�����̂̑傫���i���a�j
    };
    LightData* mappedLightData_ = nullptr;

    Vector3 color_;//���̐F
    float scale_;//�����̑傫��
    float radius_;//���̔��a�B���̒��ł͉e���x��100%
    float range_;//�e���͈�

        //���̏��p�萔�o�b�t�@
    ComPtr<ID3D12Resource> lightBuff_;
    ComPtr<ID3D12DescriptorHeap> lightHeap_;

    class Dx12Wrapper* dx12_;

    HRESULT CreateLightResourceAndView();
};

