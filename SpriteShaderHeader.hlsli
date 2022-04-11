Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

cbuffer SceneData : register(b0)
{
    matrix proj;
};

cbuffer Transform : register(b1)
{
    matrix world;
    float2 pivot;
    float2 clipSize;
    float2 spSize; //スプライトの大きさ
    float2 offset;
    float4 clipLeftTop[25];
    float4 instancePos[25];
}

struct Output
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
    uint instNo : SV_InstanceID;
};

struct PixelOutput
{
    float4 col : SV_Target; //カラー値
};

