#version 130

uniform vec4 colour;

out vec4 fragmentColor;

void main()
{
  fragmentColor = colour;
}

