#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D uEquirectMap;

layout(rgba16f, binding = 0) uniform writeonly imageCube uCubemap;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;
const float HALF_PI = 1.57079632679489661923;

vec3 face_direction(uint face, vec2 uv)
{
    // uv is in [-1, 1] x [-1, 1]
    switch (face)
    {
        case 0u: return normalize(vec3( 1.0,  uv.y, -uv.x)); // +X
        case 1u: return normalize(vec3(-1.0,  uv.y,  uv.x)); // -X
        case 2u: return normalize(vec3( uv.x,  1.0, -uv.y)); // +Y
        case 3u: return normalize(vec3( uv.x, -1.0,  uv.y)); // -Y
        case 4u: return normalize(vec3( uv.x,  uv.y,  1.0)); // +Z
        case 5u: return normalize(vec3(-uv.x,  uv.y, -1.0)); // -Z
        default: return vec3(0.0);
    }
}

vec2 dir_to_equirect(vec3 dir)
{
    // atan returns [-PI, PI]; remap to [0, 1]
    float u = atan(dir.z, dir.x) / TWO_PI + 0.5;
    // asin returns [-PI/2, PI/2]; remap to [0, 1]
    float v = asin(clamp(dir.y, -1.0, 1.0)) / PI + 0.5;
    return vec2(u, v);
}

void main()
{
    uint face = gl_GlobalInvocationID.z; // 0..5

    // Size of one face derived from the cubemap image
    ivec2 faceSize = imageSize(uCubemap).xy;

    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x >= faceSize.x || texel.y >= faceSize.y)
        return; // guard for non-power-of-two dispatch

    // Map texel to [-1, 1] NDC, centred on pixel
    vec2 uv = (vec2(texel) + 0.5) / vec2(faceSize) * 2.0 - 1.0;

    vec3 dir = face_direction(face, uv);
    vec2 eqRect = dir_to_equirect(dir);

    vec3 colour = texture(uEquirectMap, eqRect).rgb;

    imageStore(uCubemap, ivec3(texel, face), vec4(colour, 1.0));
}
