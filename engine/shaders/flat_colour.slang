struct PushConstants {
	float4 tint;
	float a;
	float b;
	float tx;
	float c;
	float d;
	float ty;
};

struct VertexInput {
	float2 position : POSITION;
	float4 colour : COLOR;
};

struct VertexOutput {
	float4 svPosition : SV_Position;
	float4 colour : COLOR;
};

[shader("vertex")] VertexOutput vertexMain(VertexInput input, uniform PushConstants pushConstants) {
	VertexOutput output;

	float2 worldPosition = float2(pushConstants.a * input.position.x + pushConstants.b * input.position.y + pushConstants.tx,
	                              pushConstants.c * input.position.x + pushConstants.d * input.position.y + pushConstants.ty);

	output.svPosition = float4(worldPosition, 0.0, 1.0);
	output.colour = input.colour;

	return output;
}
[shader("fragment")] float4 fragmentMain(VertexOutput input, uniform PushConstants pushConstants) : SV_Target {
	return input.colour * pushConstants.tint;
}
