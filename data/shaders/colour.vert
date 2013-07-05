#version 330

uniform mat4 matModelViewProjection;

#define POSITION 0

layout(location = POSITION) in vec2 position;

void main()
{
  gl_Position = matModelViewProjection * vec4(position, 0.0, 1.0);
}
