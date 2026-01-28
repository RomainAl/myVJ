#version 150

uniform sampler2DRect tex0;          // Image Syphon actuelle
uniform sampler2DRect texPrev;       // Image Syphon frame T-1
uniform sampler2DRect texFeedback;   // Résultat du FBO frame T-1 (la mémoire)

uniform float uThreshold;
uniform float uOpacity;
uniform float uBlockSizeSpeed;
uniform float uMoshIntensity;
uniform float uTime;
uniform int uMoshScale;

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
    
    // Inversion Y pour les FBOs (texPrev et texFeedback)
    vec2 uvInvert = vec2(vTexCoord.x, size.y - vTexCoord.y);

    // --- 1. TAILLE DE BLOC ALÉATOIRE ---
    float noise = hash(floor(uvNow / 1024.0) + floor(uTime * uBlockSizeSpeed));
    float exponent = floor(pow(noise, 2.0) * float(uMoshScale)) + 1.0; 
    float blockSize = pow(2.0, exponent);

    vec2 blockUV = floor(uvNow / blockSize) * blockSize;
    vec2 blockUVInvert = floor(uvInvert / blockSize) * blockSize;

    // --- 2. ANALYSE DU MOUVEMENT ---
    vec4 nowBlock = texture(tex0, blockUV);
    vec4 preBlock = texture(texPrev, blockUVInvert);
    
    // Calcul de la différence (Vecteur de poussée)
    // On utilise RG comme direction X et Y. 
    // Rappel : ces valeurs peuvent être négatives !
    vec2 push = (nowBlock.rg - preBlock.rg) * uMoshIntensity * 20;

    // Intensité brute du mouvement pour le masque
    float d = length(nowBlock.rgb - preBlock.rgb);
    float mask = step(uThreshold, d);

    // --- 3. LECTURE DU FEEDBACK DÉCALÉ (LE MOSHING) ---
    // On va chercher dans la mémoire du FBO à une position décalée par 'push'
    vec2 uvMosh = uvInvert - (push * mask);

    // Sécurité pour ne pas sortir de la texture
    uvMosh = clamp(uvMosh, vec2(0.01), size - 1.0);
    
    vec4 feedback = texture(texFeedback, uvMosh);
    vec4 currentRes = texture(tex0, uvNow);

    // --- 4. MIX FINAL AVEC NETTOYAGE DES GRIS ---
    vec3 color;
    if(mask > 0.5) {
        color = feedback.rgb + (currentRes.rgb * uOpacity);
    } else {
        // On applique l'atténuation
        color = feedback.rgb * 0.98; // Un peu plus rapide que 0.99
        
        // --- LE FIX : Si la couleur est trop sombre, on force le noir ---
        // On calcule la luminosité perçue
        float brightness = dot(color, vec3(0.299, 0.587, 0.114));
        if(brightness < 0.01) { // Ajuste ce seuil (0.01 à 0.05) si besoin
            color = vec3(0.0);
        }
    }

    outputColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}