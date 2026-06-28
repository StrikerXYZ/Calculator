struct VertexInput
{
	float2 position		: POSITION;		//location 0
	float2 uv			: TEXCOORD0;	//location 1
	float3 color		: COLOR;		//location 2
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 uv		: TEXCOORD0;
	float3 color	: COLOR;
};

[shader("vertex")]
VertexOutput pass_vertex(VertexInput vertex_input)
{
	VertexOutput output;
	output.position = float4(vertex_input.position, 0.0, 1.0);
	output.uv = vertex_input.uv;
	output.color = vertex_input.color;
	return output;
}

[[vk::binding(0,0)]]
Sampler2D texture_sampler;

[shader("fragment")]
float4 pass_fragment_texture(VertexOutput vertex_output) : SV_TARGET
{
	//Sample all 3 channels of MSDF atlas
	float4 texture = texture_sampler.Sample(vertex_output.uv);
	float alpha = texture.a;

	float3 color = texture.rgb * vertex_output.color;

	return float4(color, alpha);
}

[shader("fragment")]
float4 pass_fragment_font(VertexOutput vertex_output) : SV_TARGET
{
	//Sample all 3 channels of MSDF atlas
	float3 msd = texture_sampler.Sample(vertex_output.uv).rgb;

	// Median of three: gives a smooth, direction-aware edge reconstruction
	// This is what correctly handles sharp corners that single-channel SDF rounds off
	float sd = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
	
	// fwidth gives the rate of screen-space change — automatically sets 
	// anti-aliasing width to exactly 1 pixel regardless of zoom level
	float alpha = clamp((sd - 0.5) / fwidth(sd) + 0.5, 0.0, 1.0);

	float3 color = vertex_output.color.rgb;

	return float4(color, alpha);
}