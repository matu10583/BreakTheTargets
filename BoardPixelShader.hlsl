#include "BoardShaderHeader.hlsli"

float4 ps0(Output input) : SV_TARGET
{
    float4 col = tex.Sample(smp, input.uv);
    //シグマイド曲線に基づいてコントラストの強調
    col.rgb = 1.0f / (1.0f + exp(-contrast * (col.rgb - 0.5)));
    
    //ビネット効果をつける
    if (isVignette != 0)
    {
        float vi = length(float2(0.5, 0.5) - input.uv);
        vi = clamp(vi - 0.2, 0, 1);
        col.rgb -= vi;
    }
    return col;
}

float4 ps1(Output input) : SV_TARGET
{
    return tex.Sample(smp, input.uv);
}