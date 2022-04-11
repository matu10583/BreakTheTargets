#include "SpriteShaderHeader.hlsli"

PixelOutput PS(Output input)
{
    PixelOutput po;
    po.col = tex.Sample(smp, input.uv);
    return po;
}