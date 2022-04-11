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
    //画面に対する比率でスケールを決める
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
        XMMATRIX world;//ワールド行列
        XMFLOAT2 pivot;//ピボット
        XMFLOAT2 clipSize;//クリップサイズ
        XMFLOAT2 spSize;
        XMFLOAT2 offset;
        XMFLOAT2 clipLeftTop[50];
        XMFLOAT2 instancePos[50];//インスタンスの場所.ほとんど文字用
        //クリップの左上座標(文字出力のために複数のクリップに対応する)
//文字列描画用に壱枚当たりのスクリーン座標サイズを格納する
    };
    
protected:
    class Dx12Wrapper* dx12_;
    //大きさの倍率
    Vector2 scale_;
    //ピボット（スクリーン座標）
    Vector2 pivot_;
    //回転（ラジアン、右まわりが正）
    float rot_;
    //スクリーン座標での位置(z成分は奥向きのunorm)
    Vector3 pos_;
    //切り取り機能（uv座標)
    Vector2 clipSize_;
    Vector2 clipLeftTop_;
    //中心オフセット
    Vector2 offset_;

    //描画するかどうか
    bool isDraw;

    std::shared_ptr<class Sprite> sprite_;

    TransformData* mappedTransMatrix_;
    ComPtr<ID3D12Resource> transformBuff_;
    ComPtr<ID3D12DescriptorHeap> transformHeap_;

    HRESULT CreateTransformResourceAndView();
    const int MAX_CLIP_SIZE = 100;//クリップ座標を遅れる最大数

};

