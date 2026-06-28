#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube uEnvironment;
layout(rgba16f, binding = 1) uniform writeonly imageCube uPrefiltered;

uniform float uRoughness;
uniform int uFaceSize;
uniform uint uSampleCount;

const float PI = 3.14159265359;

float radical_inverse_vdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), radical_inverse_vdc(i));
}

vec3 importance_sample_ggx(vec2 xi, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float distribution_ggx(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

vec3 face_dir(int face, vec2 st)
{
    switch (face)
    {
        case 0: return normalize(vec3( 1.0, -st.y, -st.x));
        case 1: return normalize(vec3(-1.0, -st.y,  st.x));
        case 2: return normalize(vec3( st.x,  1.0,  st.y));
        case 3: return normalize(vec3( st.x, -1.0, -st.y));
        case 4: return normalize(vec3( st.x, -st.y,  1.0));
        case 5: return normalize(vec3(-st.x, -st.y, -1.0));
    }
    return vec3(0.0);
}

mat3 make_tbn(vec3 n)
{
    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, n));
    vec3 fwd = cross(n, right);
    return mat3(right, fwd, n);
}

void main()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    int face = int(gl_GlobalInvocationID.z);

    if (any(greaterThanEqual(texel, ivec2(uFaceSize)))) return;

    vec2 st = (vec2(texel) + 0.5) / float(uFaceSize) * 2.0 - 1.0;
    vec3 n = face_dir(face, st);
    mat3 tbn = make_tbn(n);

    vec3 R = n;
    vec3 V = n;

    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < uSampleCount; ++i)
    {
        vec2 xi = hammersley(i, uSampleCount);

        vec3 localH = importance_sample_ggx(xi, uRoughness);
        vec3 H = normalize(tbn * localH);

        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(n, L), 0.0);
        if (NdotL <= 0.0) continue;

        float NdotH = max(dot(n, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        float D = distribution_ggx(NdotH, uRoughness);
        float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;

        float saTexel = 4.0 * PI / (6.0 * float(uFaceSize * uFaceSize));
        float saSample = 1.0 / (float(uSampleCount) * pdf + 0.0001);
        float mipBias = uRoughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

        prefilteredColor += textureLod(uEnvironment, L, mipBias).rgb * NdotL;
        totalWeight += NdotL;
    }

    prefilteredColor /= totalWeight;

    imageStore(uPrefiltered, ivec3(texel, face), vec4(prefilteredColor, 1.0));
}
