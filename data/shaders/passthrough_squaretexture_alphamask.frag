#version 130

uniform sampler2D texUnit0; // Base texture

const float tolerance = 0.8;

smooth in vec2 vertOutTexCoord;

out vec4 fragmentColor;

void main()
{
  vec4 diffuse = texture2D(texUnit0, vertOutTexCoord);
  if (diffuse.a < tolerance) discard;

  // TODO: Test with alpha = 1.0
  fragmentColor = vec4(diffuse.rgb, diffuse.a);
}

