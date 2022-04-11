#include "ZBuffHeader.hlsli"

float4 ZBuffPS(Output input) : SV_Target
{
    float4 col; //�o�̓J���[
    float dp = depth.Sample(smp, input.uv);
    //�ʒu���̕���
    float4 pos =
    mul(invproj, float4(
    input.uv * float2(2, -2) + float2(-1, 1), dp, 1
));
    pos /= pos.w;
    pos = mul(invview, pos);

    float3 lightVec = pos.xyz - lightPos.xyz;
    float D = length(lightVec); //�����Ƃ̋���
    float A = 1.0f;
    if (D >= radius)
    {
        A = 1.0f - (D - radius) / range; //�e���x
        A = clamp(A, 0.0f, 1.0f);
        A = pow(A, 2); //���̕ω��ɂ���
    }
    
    float3 light = lightVec / D; //���̌���
    float4 ligCol = lightCol * A; //���̋���
    float4 nor = normal.Sample(smp, input.uv);
    nor.xyz = (nor.xyz * 2 - float3(1, 1, 1));
    
    float3 refLight = normalize(reflect(light, nor.xyz));
    
    float diffuseB = saturate(dot(-light, nor.xyz));
    float4 texColor = tex.Sample(smp, input.uv);
    
    float p1 = 1.0f - diffuseB; //�������C�g�W��1���Ɛ�����
    float p2 = 1.0f - max(0.0f, nor.z * -1.0f); //�������C�g�W��2�����Ɛ�����
    float limP = p1 * p2;
    limP = pow(limP, 1.4f);
    
    col = diffuseB * ligCol
    //* (1 + limP)
    ;
    
    return saturate(col);
}