#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D textAtlas;

void main()
{
    float alpha = texture(textAtlas, TexCoord).r;
    FragColor = vec4(1.0, 1.0, 1.0, alpha);
}