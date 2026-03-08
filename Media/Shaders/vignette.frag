

uniform sampler2D texture;
uniform vec2 resolution;   // window size in pixels (x,y)
uniform float strength;    // 0..1 (0 = off, 1 = strong)

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;

    // Distance from center (0.0 center -> ~0.707 corners)
    float d = distance(uv, vec2(0.5, 0.5));

    // Smooth vignette curve: start darkening near edges
    float v = smoothstep(0.35, 0.80, d);

    vec4 col = texture2D(texture, gl_TexCoord[0].xy);

    // Darken edges by strength
    col.rgb *= (1.0 - v * strength);

    gl_FragColor = col;
}
