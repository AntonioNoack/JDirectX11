
in vec2 uv;
in vec4 color0x;
in vec4 color1x;

out vec4 result;

uniform sampler2D mytexture;

void main() {
    float f = texture(mytexture, uv).x;
    result = mix(color0x, color1x, f);
}