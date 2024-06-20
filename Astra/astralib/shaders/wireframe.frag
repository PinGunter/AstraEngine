#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "wavefront.glsl"

layout(push_constant) uniform _PushConstantWireframe
{
  PushConstantWireframe pc;
};

void main()
{
  o_color = vec4(pc.wireColor, 1.0f);
}
