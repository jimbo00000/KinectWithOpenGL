// simpletex.frag

varying vec2 vfTex;

uniform sampler2D tex;

void main()
{
    gl_FragColor = texture2D(tex, vfTex);
}
