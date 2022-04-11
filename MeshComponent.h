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


//アニメーション遷移用構造体
//情報を増やせるように構造体にしておく
struct AnimationNode
{
    std::shared_ptr<AnimationData> anime;
    bool isLoop;//ループするかどうか。しないなら最終フレームのポーずのまま
    int transitFrameNum;//前のアニメーションとの補間に何フレームかけるか
};
//遷移線
struct TransitLine
{
    const unsigned int end;//遷移先のアニメーション
    const bool* condition;//遷移条件フラグのアドレス.nullptrの場合常に遷移する
    const bool value;//condition = valueで遷移する
    const bool isInterrupted;//アニメーションの途中で遷移するか
    TransitLine(const unsigned int endid, const bool* cond, bool val, bool isInter=true):
        end(endid),condition(cond), value(val), isInterrupted(isInter){}
};

struct WorldMatrix {//ワールド行列
    XMMATRIX position;
    XMMATRIX rotate;
};

class MeshComponent :
    public Component
{
    //ComPtrで管理する
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    MeshComponent(Object* owner, const char* fileName, int updateOrder=100);
    MeshComponent(
        Object* owner, const std::vector<VertexData>& vertices, //頂点配列
        const std::vector<Index>& indecies, //インデックス配列
        const std::vector<Material>& materials,//マテリアル
        const char* name = nullptr,//識別名（任意）
        int updateOrder = 100);
    ~MeshComponent();
    //isShadow:車道マップの描画用か
    void DrawPMD(bool isShadow);
    void DrawVert(bool isShadow);
    void Update(float deltaTime) override;

    //アニメノードの追加、削除.
    unsigned int AddAnimationNode(const char* filename, 
        bool isLoop=true, unsigned int transitFrameNum=5);
    void DeleteAnimationNode(const unsigned int anim);
    //遷移線の作製
    //条件なしで常に遷移するなら条件はnullptr
    void MakeAnimationTransit(
        const unsigned int startID, const unsigned int endID, 
        const bool* condition=nullptr, bool value = true, bool isInterrupted = true);
    //最初のアニメーションを決める
    void SetEntryAnimationNode(const unsigned int entryID);

    void SetIsLeftHanded(bool isLH);

    //モデルを囲むAABBを返す
    const AABB& GetLocalAABB()const { return localAABB_; }
    const AABB& GetWorldAABB()const { return worldAABB_; }
private:

    class Dx12Wrapper* dx12_;

    std::shared_ptr<Model> model_;
    XMMATRIX* mappedPosMatrix_;

    //移動行列（ボーンの数だけ）
    std::vector<XMMATRIX> boneMatrices_;

    //位置調整用定数バッファ
    ComPtr<ID3D12Resource> transformBuff_;
    ComPtr<ID3D12DescriptorHeap> transformHeap_;

    HRESULT CreateTransformResourceAndView();

    //アニメーションのノード
    std::unordered_map<unsigned int, AnimationNode> animeNodes_;
    //そのアニメーションから生えてる遷移線
    std::unordered_map<unsigned int,
        std::vector<TransitLine>>
        transitMap_;
    //現在いるアニメーションノードID
    int nowAnimeNodeID_;
    //ひとつ前のアニメーションの状態
    std::unordered_map<std::string,
    KeyFrame> forMotionData_;
    //アニメーションが何秒目か
    float elapsedTime_;
    //アニメーションノードのID
    unsigned int aniID_ = 0;

    //もーションの時間を進める
    void MotionUpdate(float deltaTime);
    //子ノードに親ノードの行列をかける
    void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);
    //IKの解決
    void IKSolve(unsigned int frameNo);
    //IK解決ヘルパー関数
    void SolveLookAt(const PMDIK& ik);
    void SolveCosine(const PMDIK& ik);
    void SolveCCDIK(const PMDIK& ik);

    //AABBの更新
    void UpdateAABB();

    //モデルが左手系で作られているか
    //これをfalseにすると反転して表示させる
    bool isLeftHanded;

    //描画するかどうか
    bool isDraw;

    const int WORLD_MATRIX_COUNT = 2;//ワールド行列の数
    const int TRANSIT_FRAME = 5;//次のアニメーションに遷移するまでのフレーム数
    bool isTransiting;//アニメーションの遷移ちゅうか
    bool isFirstFrame;//アニメーション遷移後の最初のフレームか

    AABB localAABB_;//しすいだいカリングのための判定
    AABB worldAABB_;
};

