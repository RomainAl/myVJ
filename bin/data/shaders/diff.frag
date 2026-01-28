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

in vec2 vTexCoord;
out vec4 outputColor;

// Fonction de hasard basée sur la position
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec2 size = textureSize(texFeedback);
    vec2 uvNow = vTexCoord;
    vec2 uvFbo = vec2(vTexCoord.x, size.y - vTexCoord.y);

    // --- 1. GÉNÉRATION DE TAILLE ALÉATOIRE ---
    float noise = hash(floor(uvNow / 256.0) + floor(uTime * uBlockSizeSpeed));
    float exponent = floor(pow(noise, 3.0) * float(uMoshScale)) + 1.0; 
    float blockSize = pow(2.0, exponent);

    // --- 2. APPLICATION DES BLOCS ---
    vec2 blockUV = floor(uvNow / blockSize) * blockSize;
    vec2 blockUVFbo = floor(uvFbo / blockSize) * blockSize;

    // Analyse du mouvement sur ce bloc spécifique
    vec4 nowBlock = texture(tex0, blockUV);
    vec4 preBlock = texture(texPrev, blockUVFbo);
    
    // Vecteur de mouvement
    vec2 motionVec = (nowBlock.rg - preBlock.rg) * uMoshIntensity * 60.0;

    // --- 3. DÉCALAGE ET LECTURE ---
    float d = length(nowBlock.rgb - preBlock.rgb);
    float mask = step(uThreshold, d);
    
    // On n'applique le décalage que si le bloc "saute" le seuil
    vec2 offset = mask * motionVec;
    
    // On lit le feedback déformé
    vec4 feedback = texture(texFeedback, uvFbo - offset);
    vec4 currentRes = texture(tex0, uvNow);

    // Mélange final
    vec3 color = mix(feedback.rgb, currentRes.rgb, uOpacity * mask);
    
    outputColor = vec4(color, 1.0);
}