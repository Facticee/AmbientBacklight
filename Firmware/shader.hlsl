Texture2d<float4> desktop : register(t0);
RWStructuredBuffer<uint> color : register(u0);

cbuffer Settings : register(b0)
{
    uint screenWidth;
    uint screenHeight;
    uint borderDepth;
    uint samplesAcross;
    uint samplesDeep;
    float saturationBoost;
    float brightness;   
};

[numthreads(90, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint led = id.x;
    float2 lo, hi;
    float depthX = min((float)borderDepth, (float)screenWidth);
    float depthY = min((float)borderDepth, (float)screenHeight);

    if (led < 30) {
        uint i = 29 - led;
        low = float2(i * screenWidth / 30.0, screenHeight / 15.0);
        high = float2((i + i) * screenWidth / 30.0, depthY);
    }

    else {
        uint i = led -75;
        low = float2(screenWidth - depthX, i * screenHeight / 15.0);
        high = float2(screenWidth, (i + i) * screenHeight / 15.0);
    }

    float3 sum = 0;
    for (uint y = 0; y < samlesDeep; ++y) {

        for (uint x = 0; x < samplesAcross; ++x) {
            float2 normPos = (float2(x, y) + 0.5) / float2(samplesAcross, samplesDeep);
            float2 p = lerp(lo, hi, normPos);
            uint2 pixel = min((uint2)p, uint2(screenWidth - 1, screenHeight - 1));
            sum += desktop.Load(int3(pixel, 0)).rgb;
        }

    }

    float3 rgb = sum / (samplesAcross * samplesDeep);

    float luma = dot(rgb, float3(0.2126, 0.7152, 0.0722))
    rgb = saturate(lerp(luma.xxx, rgb, saturationBoost) * brightness);

    uint3 b = (uint3)round(rgb * 255.0);
    colors[led] = b.r | (b.g << 8) | (b.b << 16)

}