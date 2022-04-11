#include "BasicShaderHeader.hlsli"


Output BasicVS(
	float4 pos : POSITION,
    float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
    uint instNo:SV_InstanceID)
{
    Output output;
    //ボーンの処理
    float w = weight / 100.0f;
    matrix bm = bones[boneno[0]] * w
    + bones[boneno[1]] * (1 - w);
    matrix world = mul(posMat, rotMat);
    pos = mul(bm, pos);
    pos = mul(world, pos);
    output.svpos = mul(mul(proj, view), pos);
    output.pos = pos;
    output.posFromLight = mul(lightView, pos);
    normal.w = 0;
    output.normal = mul(rotMat, normal);
    output.uv = uv;
    output.vnormal = mul(view, output.normal);
    output.ray = normalize(pos.xyz -  eye);
    output.instNo = instNo;
    return output;
}

Output VertVS(float4 pos : POSITION,
    float4 normal : NORMAL,
float2 uv : TEXCOORD,
uint instNo : SV_InstanceID)
{
    Output output;
    matrix world = mul(posMat, rotMat);
    pos = mul(world, pos);
    output.svpos = mul(mul(proj, view), pos);
    output.pos = pos;
    output.posFromLight = mul(lightView, pos);
    output.uv = uv;
    output.ray = normalize(pos.xyz - eye);
    output.instNo = instNo;
    
    return output;
}

float4 DepthBasicVS(float4 pos : POSITION,
    float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT):SV_POSITION
{
        //ボーンの処理
    float w = weight / 100.0f;
    matrix bm = bones[boneno[0]] * w
    + bones[boneno[1]] * (1 - w);
    matrix world = mul(posMat, rotMat);
    pos = mul(bm, pos);
    pos = mul(world, pos);
    float4 lpos = mul(lightView, pos);
    return lpos;
}

float4 DepthVertVS(float4 pos : POSITION,
    float4 normal : NORMAL,
	float2 uv : TEXCOORD) : SV_POSITION
{
    matrix world = mul(posMat, rotMat);
    pos = mul(world, pos);
    float4 lpos = mul(lightView, pos);
    return lpos;
}