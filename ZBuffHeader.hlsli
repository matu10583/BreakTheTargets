Texture2D<float4> tex : register(t0);
Texture2D<float4> normal : register(t1);
Texture2D<float> depth : register(t2);

SamplerState smp : register(s0);

cbuffer SceneData : register(b0)
{
    matrix invview;
    matrix view;
    matrix invproj;
    matrix proj;
    float3 dlightCol;
    float4 lightDir;
    float4 eye;
    matrix lightView;
};

cbuffer LightData : register(b1)
{
    float4 lightPos;
    float4 lightCol;
    float radius;
    float range;
    float lightScale;
}

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};