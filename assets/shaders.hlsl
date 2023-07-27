cbuffer constants : register(b0) {
    float2 offset;
};

struct VS_Input {
    float2 coords : coords;
    uint InstanceID : SV_InstanceID;
    uint VertexID : SV_VertexID;
};

struct VS_Instanced {
    float3 instData : instData;
    float3 color0 : color0x;
    float3 color1 : color1x;
};

struct VS_Output {
    float4 pos : SV_POSITION;
    float2 uv : out_uv;
    float4 color0x : out_color0;
    float4 color1x : out_color1;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

VS_Output vs_main(VS_Input input, VS_Instanced instInput) {
    VS_Output output;
    output.pos = float4((input.coords + instInput.instData.xy + offset) * 0.1, 0.0f, 1.0f);
    output.uv = input.coords;
    output.color0x = float4(instInput.color0,1);
    output.color1x = float4(instInput.color1,1);
    return output;
}

float4 ps_main(VS_Output input) : SV_Target {
    float f = mytexture.Sample(mysampler, input.uv).x;
    return lerp(input.color0x, input.color1x, f);
}