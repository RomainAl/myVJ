#version 150
uniform sampler2DRect tex0;
uniform sampler2DRect texPrev;
uniform float uThreshold;
uniform float uOpacity;
in vec2 vTexCoord;
out vec4 outputColor;

void main() {
    // 1. Coordonnées (avec inversion Y pour texPrev si nécessaire comme vu avant)
    vec2 uvNow = vTexCoord;
    vec2 size = textureSize(texPrev);
    vec2 uvPre = vec2(vTexCoord.x, size.y - vTexCoord.y);

    vec4 now = texture(tex0, uvNow);
    vec4 pre = texture(texPrev, uvPre);
    
    // 2. Calcul du masque de mouvement
    float d = length(now.rgb - pre.rgb);
    float mask = smoothstep(uThreshold, uThreshold + 0.3, d);
    
    // 3. LA LOGIQUE MAGIQUE :
    // Si uThreshold est à 0, finalMask sera proche de 1.0 partout.
    // On utilise uThreshold pour doser l'influence du masque.
    // On peut aussi utiliser un simple if ou un mix :
    
    float influenceMouvement = clamp(uThreshold * 5.0, 0.0, 1.0); // Monte vite à 1.0
    float finalMask = mix(1.0, mask, influenceMouvement);

    outputColor = vec4(now.rgb * finalMask, finalMask * uOpacity);
}