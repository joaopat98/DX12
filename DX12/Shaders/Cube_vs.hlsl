struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
    float4x4 Model  : MODEL;
    uint InstanceId : SV_InstanceID;
};

struct ViewProjection
{
    matrix VP;
};

ConstantBuffer<ViewProjection> ViewProjectionCB : register(b0);

struct VertexShaderOutput
{
    float4 Color    : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ViewProjectionCB.VP, mul(IN.Model, float4(IN.Position, 1.0f)));
    OUT.Color = float4(IN.Color, 1.0f);

    return OUT;
}
