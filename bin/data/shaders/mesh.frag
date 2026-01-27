#version 150

uniform float uStretchX;
uniform float uStretchY;
uniform float uAlphaMult;

in vec4 vColor;
out vec4 outputColor;

void main() {
    if(vColor.a < 0.01) discard;

    bool isPoint = (gl_PointCoord.x > 0.0 || gl_PointCoord.y > 0.0);

    if(isPoint) {
        vec2 uv = gl_PointCoord - vec2(0.5);
        uv.x /= uStretchX;
        uv.y /= uStretchY;
        if(length(uv) > 0.5) discard;
    }

    outputColor = vec4(vColor.rgb, vColor.a * uAlphaMult);
}