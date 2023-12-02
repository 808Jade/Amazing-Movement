//#version 330 core
//layout(location = 0) in vec3 vPos;
//layout(location = 1) in vec3 vColor;
//
//out vec3 outColor;
//
//uniform mat4 transform;
//uniform mat4 projection;
//uniform mat4 view;
//
//void main()
//{
//
//    gl_Position = projection * view * transform * vec4(vPos, 1.0);
//    outColor = vColor;
//
//}

#version 330 core

layout (location = 0) in vec3 in_Position; 
layout (location = 1) in vec3 in_Color;
layout (location = 2) in vec3 in_Normal;

out vec3 out_Color; 
out vec3 FragPos;
out vec3 Normal;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

void main(void)
{
	gl_Position = projection * view * transform * vec4(in_Position, 1.0f);
	FragPos = vec3(transform * vec4(in_Position,1.0));
	Normal = vec3(transform * vec4(in_Normal,0.0));
	out_Color = in_Color;
}