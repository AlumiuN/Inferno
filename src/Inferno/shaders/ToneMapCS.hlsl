//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
// 
// HDR input with Typed UAV loads (32 bit -> 24 bit assignment) to a SDR output

#include "utility.hlsli"

#define RS \
    "RootConstants(b0, num32BitConstants = 4), " \
    "DescriptorTable(UAV(u0))," \
    "DescriptorTable(UAV(u1))," \
    "DescriptorTable(SRV(t0))," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

Texture2D<float3> Bloom : register(t0);

RWTexture2D<float3> ColorRW : register(u0);
RWTexture2D<float> OutLuma : register(u1);

SamplerState LinearSampler : register(s0);

cbuffer CB0 : register(b0) {
    float2 g_RcpBufferDim;
    float g_BloomStrength;
    float g_Exposure;
};

// The Reinhard tone operator.  Typically, the value of k is 1.0, but you can adjust exposure by 1/k.
// I.e. TM_Reinhard(x, 0.5) == TM_Reinhard(x * 2.0, 1.0)
float3 TM_Reinhard(float3 hdr, float k = 1.0) {
    return hdr / (hdr + k);
}

// The inverse of Reinhard
float3 ITM_Reinhard(float3 sdr, float k = 1.0) {
    return k * sdr / (k - sdr);
}


// This is the new tone operator.  It resembles ACES in many ways, but it is simpler to evaluate with ALU.  One
// advantage it has over Reinhard-Squared is that the shoulder goes to white more quickly and gives more overall
// brightness and contrast to the image.

float3 TM_Stanard(float3 hdr) {
    return TM_Reinhard(hdr * sqrt(hdr), sqrt(4.0 / 27.0));
}

float3 ITM_Stanard(float3 sdr) {
    return pow(ITM_Reinhard(sdr, sqrt(4.0 / 27.0)), 2.0 / 3.0);
}

float Luminance(float3 v) {
    return dot(v, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ChangeLuma(float3 color, float lumaOut) {
    float luma = Luminance(color);
    return color * (lumaOut / luma);
}

float3 reinhard_extended_luminance(float3 v, float max_white_l) {
    float luma = Luminance(v);
    float numerator = luma * (1.0f + (luma / (max_white_l * max_white_l)));
    float destLuma = numerator / (1.0f + luma);
    return ChangeLuma(v, destLuma);
}

float3 GammaRamp(float3 color, float gamma) {
    return pow(color, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));
}

float3 Uncharted2ToneMapping(float3 color, float gamma) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposure = 2.0;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    return GammaRamp(color, gamma);
}

float3 lumaBasedReinhardToneMapping(float3 color, float gamma = 1) {
    float luma = Luminance(color);
    float toneMappedLuma = luma / (1. + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, float3(1. / gamma, 1. / gamma, 1. / gamma));
    return color;
}

[RootSignature(RS)]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    // Load HDR and bloom
    float3 hdrColor = ColorRW[DTid.xy];
    //float l = Luminance(hdrColor);
    //if (l > 1.00)
    //    hdrColor += (l - 1) / 3;
    hdrColor += g_BloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);
    hdrColor *= g_Exposure;

    // Tone map to SDR
    hdrColor = reinhard_extended_luminance(hdrColor, 4.0);
    ColorRW[DTid.xy] = hdrColor;
    //ColorRW[DTid.xy] = Uncharted2ToneMapping(hdrColor, 1.1);
    //ColorRW[DTid.xy] = hdrColor;

}
