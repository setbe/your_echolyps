#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D textAtlas;

void main()
{
    float value = texture(textAtlas, TexCoord).r;
    FragColor = vec4(1.0, 0.0, 0.0, value);
}