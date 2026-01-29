#version 150

uniform sampler2DRect tex0;
uniform sampler2DRect texPrev;
uniform sampler2DRect texFeedback;

uniform float uThreshold;
uniform float uOpacity;
uniform float uBlockSizeSpeed;
uniform float uMoshIntensity;
uniform float uTime;
uniform int uMoshScale;

// Nouveaux outils de look
uniform float uSaturation;
uniform float uContrast;
uniform float uBrightness;

in vec2 vTexCoord;
out vec4 outputColor;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec2 size = textureSize(tex0);
    vec2 uvNow = vTexCoord;
    vec2 uvInvert = vec2(uvNow.x, size.y - uvNow.y);

    float noise = hash(floor(uvNow / 1024.0) + floor(uTime * uBlockSizeSpeed));
    float exponent = floor(pow(noise, 2.0) * float(uMoshScale)) + 1.0; 
    float blockSize = pow(2.0, exponent);

    vec2 blockUV = floor(uvNow / blockSize) * blockSize;
    vec2 blockUVInvert = floor(uvInvert / blockSize) * blockSize;

    vec4 nowBlock = texture(tex0, blockUV);
    vec4 preBlock = texture(texPrev, blockUVInvert);
    vec2 push = (nowBlock.rg - preBlock.rg) * uMoshIntensity * 100.0;

    float d = length(nowBlock.rgb - preBlock.rgb);
    float mask = step(uThreshold, d);
    vec2 uvMosh = uvInvert - (push * mask);

    uvMosh = clamp(uvMosh, vec2(0.01), size - 1.0);
    
    vec4 feedback = texture(texFeedback, uvMosh);
    vec4 currentRes = texture(tex0, uvNow);

    vec3 color;
    if(mask > 0.5) {
        color = feedback.rgb + (currentRes.rgb * uOpacity);
    } else {
        color = feedback.rgb * 0.98;
        float br = dot(color, vec3(0.299, 0.587, 0.114));
        if(br < 0.01) color = vec3(0.0);
    }

    // --- TRAITEMENT D'IMAGE (Look) ---
    
    // 1. Brightness
    color *= uBrightness;

    // 2. Saturation
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, uSaturation);

    // 3. Contraste
    // On décale autour du gris moyen (0.5) pour étirer les tons
    color = (color - 0.5) * uContrast + 0.5;

    outputColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}