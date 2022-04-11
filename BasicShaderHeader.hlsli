#define EPSILON 1.0e-4

Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);
Texture2D<float4> normalTex : register(t4);
Texture2D<float4> ao : register(t5);
Texture2D<float> lightDepth : register(t6);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);
SamplerComparisonState smpBlur : register(s2);

cbuffer SceneData : register(b0)
{
    matrix invview;
    matrix view;
    matrix invproj;
    matrix proj;
    float3 lightCol;
    float4 lightDir;
    float4 eye;
    matrix lightView;
};

cbuffer Transform : register(b1)
{
    matrix posMat;//移動行列
    matrix rotMat;//回転行列
    //最大のボーン数
    matrix bones[256];
}

cbuffer Material : register(b2)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL0;
    float4 vnormal : NORMAL1;
    float2 uv : TEXCOORD;
    float3 ray : VECTOR;
    float4 posFromLight : TPOS;
    uint instNo : SV_InstanceID;
};

struct PixelOutput
{
    float4 col : SV_Target0; //カラー値
    float4 normal : SV_Target1; //法線
};

//魚眼に変換する射影処理を行う
float4 ConvertToFishPerspective(float4 vpos, float near, float far)
{
    float4 ret = vpos;
    float xy2 = vpos.x * vpos.x + vpos.y * vpos.y;
    float d = sqrt(xy2 + (vpos.z * vpos.z));
    ret.x = (vpos.x / xy2) * (d - vpos.z);
    ret.y = (vpos.y / xy2) * (d - vpos.z);
    ret.z = ((d - near) / (far - near));
    ret.w = 1;
    return ret;
}
