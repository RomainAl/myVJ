#version 150

uniform float uStretchX;
uniform float uStretchY;

in vec4 vColor;
out vec4 outputColor;

void main() {
    vec2 uv = gl_PointCoord - vec2(0.5);
    uv.x /= uStretchX;
    uv.y /= uStretchY;
    float dist = length(uv);
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    outputColor = vec4(vColor.rgb, vColor.a * alpha);
}