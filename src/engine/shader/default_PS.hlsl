#include "common.hlsli"

//Texture2D g_texture[MAX_TEXTURE_COUNT] : register(t0, space1);
SamplerState g_sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    //return float4(input.uv.xy, 0, 1);
    //return g_texture.Sample(g_sampler, float2(0, 0));
    float2 uv = float2(0, 0);
    //uv.x = cos(input.color.x);
    //uv.y = sin(input.color.x);
    //return g_texture.Sample(g_sampler, uv);
    //
    // switch (input.temp)
    // {
    //     case 0:
    //         //return float4(input.uv.r, input.uv.g, 0, 1);
    //         return g_texture[0].Sample(g_sampler, input.uv);
    //     case 1:
    //         return g_texture[1].Sample(g_sampler, input.uv);
    //     case 2:
    //         return g_texture[2].Sample(g_sampler, input.uv);
    //     case 3:
    //         return g_texture[3].Sample(g_sampler, input.uv);
    //     case 4:
    //         return g_texture[4].Sample(g_sampler, input.uv);
    //     case 5:
    //         return g_texture[5].Sample(g_sampler, input.uv);
    //     case 6:
    //         return g_texture[6].Sample(g_sampler, input.uv);
    //     case 7:
    //         return g_texture[7].Sample(g_sampler, input.uv);
    //     default:
    //         break;
    // }
    //
    return float4(1, input.temp, 1, 1);
    //return g_texture[input.temp].Sample(g_sampler, input.uv);
    //return input.color;

}
