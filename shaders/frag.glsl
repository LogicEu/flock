out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;

void main() 
{
	vec2 uv = gl_FragCoord.xy / u_resolution.y;
	vec3 color = vec3(uv.x, uv.y, (cos(u_time) + 1.0) * 0.5);
	FragColor = vec4(color, sin(u_time * 0.2) * 0.4 + 0.5);
}
