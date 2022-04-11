#include "BasicShaderHeader.hlsli"



PixelOutput BasicPS(Output input):SV_Target
{
    PixelOutput output;
    float4 col;//出力カラー
    //頂点座標を正規化
    float3 posFromL = input.posFromLight.xyz / input.posFromLight.w;
    float2 depUV = (posFromL + float2(1, -1)) * float2(0.5, -0.5);

    float shadW = 1.0f;
    //光源からのデプス地
    if(depUV.x>=0&&depUV.x<=1 &&
        depUV.y >= 0 && depUV.y <= 1 &&
        posFromL.z>=0 && posFromL.z<=1)
    {
        float depthFromL = lightDepth.SampleCmpLevelZero(
        smpBlur, depUV, posFromL.z - 0.005f);
        shadW = lerp(0.5f, 1.0f, depthFromL);//0.5~1.0
    }
    float3 light = normalize(lightDir.xyz); //光の向き
    
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    
    float specularB = pow(saturate(dot(refLight, -input.ray)), abs(specular.a));
    float diffuseB = saturate(dot(-light, input.normal.xyz));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1 - diffuseB));
    float2 sphereMapUV = input.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
    float4 texColor = tex.Sample(smp, input.uv);
    float4 spaCol = spa.Sample(smp, sphereMapUV);
    float4 sphCol = sph.Sample(smp, sphereMapUV);
    
    float limB = saturate(dot(light, input.normal.xyz));
    float p1 = 1.0f - limB;//リムライト係数1光と垂直か
    float p2 = 1.0f - max(0.0f, input.vnormal.z * -1.0f);//リムライト係数2視線と垂直か
    float limP = p1 * p2;
    limP = pow(limP, 1.3f);
    
    col = max(
    (toonDif * diffuse * texColor * sphCol * shadW
    + spaCol
    //+ float4(limP * lightCol, 1)
    + float4(specularB * specular.rgb, 1))
    , float4(texColor * ambient, 1)
    );
    
    output.col = saturate(col);
    float4 norm = input.normal;
    norm.xyz = (norm.xyz + float3(1, 1, 1)) / 2.0f;
    output.normal = norm;
    return output;
}

PixelOutput VertPS(Output input) : SV_TARGET
{
    PixelOutput output;
    float4 col; //出力カラー
    
    //頂点座標を正規化
    float3 posFromL = input.posFromLight.xyz / input.posFromLight.w;
    float2 depUV = (posFromL + float2(1.0f, -1.0f)) * float2(0.5f, -0.5f);

    float shadW = 1.0f;
    //光源からのデプス値
    if (depUV.x >= 0 && depUV.x <= 1 &&
        depUV.y >= 0 && depUV.y <= 1 &&
        posFromL.z >= 0 && posFromL.z <= 1)
    {
        float depthFromL = lightDepth.SampleCmpLevelZero(
        smpBlur, depUV, posFromL.z - 0.005f);
        shadW = lerp(0.5f, 1.0f, depthFromL); //0.5~1.0
    }

    float3 light = normalize(lightDir.xyz); //光の向き
    
    float3 normMap = normalTex.Sample(smp, input.uv).xyz;
    normMap = 2 * normMap - 1;
    normMap = normalize(mul(rotMat, normMap));
    //軸を合わせる
    normMap = float3(normMap.y, normMap.z, normMap.x);
    input.normal = mul(rotMat, normMap);
    input.vnormal = mul(view, input.normal);
    
    //アンビエントオクルージョンマップ
    float aoPower = ao.Sample(smp, input.uv).x;
    
    float3 refLight = normalize(reflect(light, input.normal));//反射光の向き
    
    float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
    float diffuseB = saturate(dot(-light, input.normal));
    float4 texColor = tex.Sample(smp, input.uv);
    
    col = max(
    (diffuse * texColor * shadW * aoPower * diffuseB
    + float4(specularB * specular.rgb, 1))
    , float4(texColor.xyz * ambient * aoPower, 1)
    );
    
    output.col  = saturate(col);
    float4 norm = input.normal;
    norm.xyz = (norm.xyz + float3(1, 1, 1)) / 2.0f;
    output.normal = norm;
    return output;
}

