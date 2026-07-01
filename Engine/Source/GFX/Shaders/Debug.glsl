#ifndef DEBUG_GLSL
#define DEBUG_GLSL

bool apply_debug_channels(out vec4 fragColor, vec4 albedo,
    vec3 N, float roughness, float metallic, float ao, vec3 emissive)
{
#if defined(DEBUG_ALBEDO)
    fragColor = vec4(albedo.rgb, 1.0);
    return true;
#elif defined(DEBUG_NORMALS)
    fragColor = vec4(N * 0.5 + 0.5, 1.0);
    return true;
#elif defined(DEBUG_ROUGHNESS)
    fragColor = vec4(vec3(roughness), 1.0);
    return true;
#elif defined(DEBUG_METALLIC)
    fragColor = vec4(vec3(metallic), 1.0);
    return true;
#elif defined(DEBUG_AO)
    fragColor = vec4(vec3(ao), 1.0);
    return true;
#elif defined(DEBUG_EMISSIVE)
    fragColor = vec4(emissive, 1.0);
    return true;
#else
    return false;
#endif
}

#endif // DEBUG_GLSL