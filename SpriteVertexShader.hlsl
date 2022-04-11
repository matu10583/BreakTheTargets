#include "SpriteShaderHeader.hlsli"

Output VS(
	float4 pos : POSITION,
	float2 uv:TEXCOORD,
    uint instNo:SV_InstanceID)
{
    int idx = (instNo % 2) * 2;
    float4 newPos = pos + float4(instancePos[instNo / 2][idx],
    instancePos[instNo / 2][idx+1],
    0, 0);
	//頂点座標の中心をずらして座標変換
    Output output;
    //ピボットの位置調整
    float2 newPivot = pivot -
    float2(instancePos[instNo / 2][idx], instancePos[instNo / 2][idx + 1]);
    
    newPos = newPos - float4(pivot, 0, 0);
    newPos = newPos - float4(offset, 0, 0);
    newPos = mul(world, newPos);
    newPos = newPos + float4(pivot, 0, 0);
    
    output.pos = newPos;
    output.svpos = mul(proj, newPos);
    output.instNo = instNo;
    
    //uvを変換する
    uv = float2(uv.x * clipSize.x, uv.y * clipSize.y);
    uv.x += clipLeftTop[instNo / 2][idx];
    uv.y += clipLeftTop[instNo / 2][idx + 1];
    output.uv = uv;
	return output;
}