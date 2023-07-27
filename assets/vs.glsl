uniform vec2 offset;

in vec2 coords;
in vec3 instData;
in vec3 color0;
in vec3 color1;

out vec2 uv;
out vec4 color0x;
out vec4 color1x;

void main() {
    gl_Position = vec4((coords + instData.xy + offset) * 0.1, 0.0, 1.0);
    uv = coords;
    color0x = vec4(color0, 1);
    color1x = vec4(color1, 1);
}
