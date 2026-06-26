static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vertMain(uint vertexId : SV_VertexID) {
    VertexOutput output;
    output.sv_position = float4(positions[vertexId], 0.0, 1.0);
    output.color = colors[vertexId];
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVertex) : SV_Target {
    float3 color = inVertex.color;
    return float4(color, 1.0);
}