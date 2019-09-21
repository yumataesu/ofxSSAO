#version 450 core

const int kernelSize = 64;
const float bias = .1;

uniform sampler2D		gPosition;
uniform sampler2D		gNormal;
uniform sampler2D		noiseTexture;
uniform mat4 projection;
uniform mat4 view;

uniform float u_radius = 64.;
uniform vec3 samples[kernelSize];

in vec2 v_texcoord;

out vec4 FragColor;

void main() {
    ivec2 size = textureSize(gPosition, 0);
    vec2 noiseScale = vec2(float(size.x), float(size.y));
    ivec2 uv = ivec2(gl_FragCoord.xy);

	vec4 position = texelFetch(gPosition, uv, 0);
    vec3 normal = texelFetch(gNormal, uv, 0).xyz;

    vec3 randomVec = texture(noiseTexture, v_texcoord * noiseScale * 0.25).xyz;
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    vec3 sample_ = vec3(0.0);

	for(int i = 0; i < kernelSize; ++i) {
        // get sample position
        sample_ = TBN * samples[i]; // from tangent to view-space
        sample_ = position.xyz + sample_ * u_radius;
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(sample_, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= sample_.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelSize);

	FragColor = vec4(occlusion, occlusion, occlusion, 1.0);
}