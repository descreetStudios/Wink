#ifndef TONEMAPPING_GLSL
#define TONEMAPPING_GLSL

/* --- Helpers --- */
vec3 apply_exposure(vec3 color, float exposure)
{
    return color * exp2(exposure);
}

vec3 clamp_0_1(vec3 color)
{
    return clamp(color, 0.0, 1.0);
}

/* --- Just Exposure --- */
vec3 tonemap_none(vec3 color, float exposure)
{
    return clamp_0_1(apply_exposure(color, exposure));
}

/* --- Just Gamma --- */
vec3 apply_gamma(vec3 color, float gamma)
{
    return clamp_0_1(pow(color, vec3(1.0 / gamma)));
}

/* --- Reinhard --- */
vec3 tonemap_reinhard(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);
    return x / (1.0 + x);
}

/* --- Luminance-Preserving Reinhard --- */
vec3 tonemep_reinhard_luminance(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);
    const vec3 LUMA = vec3(0.2126, 0.7152, 0.0722);
    float L = dot(x, LUMA);
    float Lm = L / (1.0 + L);
    float scale = (L > 0.0) ? (Lm / L) : 0.0;
    return x * scale;
}

/* --- Hejl-Burgess-Dawson --- */
vec3 tonemap_hejl(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);
    vec3 t = max(vec3(0.0), x - 0.004);
    return clamp_0_1((t * (6.2 * t + 0.5)) / 
        (t * (6.2 * t + 1.7) + 0.06));
}

/* --- Hable filmic / Uncharted 2 --- */
vec3 tonemap_hable_curve(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / 
        (x * (A * x + B) + D * F)) - E / F;
}

vec3 tonemap_hable(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);

    const float W = 11.2;
    float whiteScale = tonemap_hable_curve(vec3(W)).x;

    vec3 mapped = tonemap_hable_curve(x) / whiteScale;
    return clamp_0_1(mapped);
}

/* --- ACES (Narkowicz approximation) --- */
vec3 tonemap_aces(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);

    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;

    vec3 mapped = (x * (A * x + B)) / (x * (C * x + D) + E);
    return clamp_0_1(mapped);
}

/* --- Lottes Filmic --- */
vec3 tonemap_lottes(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);

    const float A = 1.6;
    const float D = 0.977;
    const float HDR_MAX = 8.0;
    const float MID = 0.18;

    float b = -pow(MID, A) / log(D);

    vec3 num = D * (1.0 - exp(-pow(x, vec3(A)) / b));
    float den = 1.0 - exp(-pow(HDR_MAX, A) / b);
    vec3 mapped = num / den;

    return clamp_0_1(mapped);
}

/* --- Uchimura --- */
vec3 tonemap_uchimura(vec3 color, float exposure)
{
    vec3 x = apply_exposure(color, exposure);

    const float P = 1.0;
    const float a = 1.0;
    const float m = 0.22;
    const float l = 0.4;
    const float c = 1.33;
    const float b = 0.0;

    vec3 xp = max(x - b, 0.0);

    vec3 lm = vec3(m * P);
    vec3 l0 = lm + pow(xp - lm, vec3(a));
    vec3 l1 = lm + (xp - lm) * (P - lm) / (l + 1e-6);
    vec3 w = smoothstep(0.0, 1.0, (xp - lm) / (l + 1e-6));

    vec3 tone = mix(l0, l1, w);
    tone = tone / (tone + c);

    return tone;
}

/* --- Simple Gamma Curve --- */
vec3 tonemap_gamma(vec3 color, float exposure, float gamma)
{
    vec3 x = apply_exposure(color, exposure);
    return clamp_0_1(pow(x, vec3(1.0 / gamma)));
}

#endif // TONEMAPPING_GLSL