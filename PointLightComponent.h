#pragma once
#include "Component.h"
#include "XMFLOAT_Helper.h"
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>



//点光源の設定
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

    //光の情報をバッファーにセットし、それを4晩テーブルに配置
    void SetLightBuffer();

private:
    struct LightData
    {
        XMFLOAT4 lightPos;//光源の場所
        XMFLOAT4 lightCol;//光の色
        float radius;//光の半径
        float range;//影響範囲
        float lightScale;//光自体の大きさ（半径）
    };
    LightData* mappedLightData_ = nullptr;

    Vector3 color_;//光の色
    float scale_;//光源の大きさ
    float radius_;//光の半径。この中では影響度が100%
    float range_;//影響範囲

        //光の情報用定数バッファ
    ComPtr<ID3D12Resource> lightBuff_;
    ComPtr<ID3D12DescriptorHeap> lightHeap_;

    class Dx12Wrapper* dx12_;

    HRESULT CreateLightResourceAndView();
};

