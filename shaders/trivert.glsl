layout (location = 0) in vec2 vertCoord;

uniform vec2 u_resolution;
uniform float u_scale;

void main()
{
    vec2 pos = vertCoord * u_scale * 2.0;
    //pos.x *= (u_resolution.y / u_resolution.x);
    gl_Position = vec4(pos, 0.0, 1.0);
}
