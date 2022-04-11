#include "BoardShaderHeader.hlsli"

float4 ps0(Output input) : SV_TARGET
{
    float4 col = tex.Sample(smp, input.uv);
    //�V�O�}�C�h�Ȑ��Ɋ�Â��ăR���g���X�g�̋���
    col.rgb = 1.0f / (1.0f + exp(-contrast * (col.rgb - 0.5)));
    
    //�r�l�b�g���ʂ�����
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